// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataToUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyDataToUnstructuredGrid);

//------------------------------------------------------------------------------
vtkPolyDataToUnstructuredGrid::vtkPolyDataToUnstructuredGrid() = default;

//------------------------------------------------------------------------------
vtkPolyDataToUnstructuredGrid::~vtkPolyDataToUnstructuredGrid() = default;

//------------------------------------------------------------------------------
void vtkPolyDataToUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkPolyDataToUnstructuredGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
bool vtkPolyDataToUnstructuredGrid::CanBeProcessedFast(vtkPolyData* polyData)
{
  if (!polyData || polyData->GetNumberOfCells() == 0)
  {
    return false;
  }
  const vtkIdType numVerts = polyData->GetNumberOfVerts();
  const vtkIdType numLines = polyData->GetNumberOfLines();
  const vtkIdType numPolys = polyData->GetNumberOfPolys();
  const vtkIdType numStrips = polyData->GetNumberOfStrips();
  const bool hasVerts = (numVerts > 0);
  const bool hasLines = (numLines > 0);
  const bool hasPolys = (numPolys > 0);
  const bool hasStrips = (numStrips > 0);
  const bool hasOnlyVerts = (hasVerts && !hasLines && !hasPolys && !hasStrips);
  const bool hasOnlyLines = (!hasVerts && hasLines && !hasPolys && !hasStrips);
  const bool hasOnlyPolys = (!hasVerts && !hasLines && hasPolys && !hasStrips);
  const bool hasOnlyStrips = (!hasVerts && !hasLines && !hasPolys && hasStrips);
  return hasOnlyVerts || hasOnlyLines || hasOnlyPolys || hasOnlyStrips;
}

//------------------------------------------------------------------------------
namespace
{
struct BuildCellTypesImpl
{
  // Given a polyData cell array and a size to type functor, it creates the cell types
  template <typename CellStateT, typename SizeToTypeFunctor>
  void operator()(
    CellStateT& state, vtkUnsignedCharArray* cellTypes, SizeToTypeFunctor&& typer, vtkIdType offset)
  {
    const vtkIdType numCells = state.GetNumberOfCells();
    if (numCells == 0)
    {
      return;
    }

    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto types = cellTypes->GetPointer(offset);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          types[cellId] = static_cast<unsigned char>(typer(state.GetCellSize(cellId)));
        }
      });
  }
};

struct BuildConnectivityImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& state, vtkIdTypeArray* outOffSets, vtkIdTypeArray* outConnectivity,
    vtkIdType offset, vtkIdType connectivityOffset)
  {
    using IdType = typename CellStateT::ValueType;
    const auto inOffsets = state.GetOffsets();
    const auto inConnectivity = state.GetConnectivity();
    const vtkIdType connectivitySize = inConnectivity->GetNumberOfValues();
    const vtkIdType numCells = state.GetNumberOfCells();

    // copy connectivity values
    vtkSMPTools::For(0, connectivitySize,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto inConnPtr = inConnectivity->GetPointer(0);
        auto outConnPtr = outConnectivity->GetPointer(connectivityOffset);
        std::copy(inConnPtr + begin, inConnPtr + end, outConnPtr + begin);
      });
    // transform offset values
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto inOffPtr = inOffsets->GetPointer(0);
        auto outOffPtr = outOffSets->GetPointer(offset);
        std::transform(inOffPtr + begin, inOffPtr + end, outOffPtr + begin,
          [&connectivityOffset](IdType val) -> vtkIdType { return val + connectivityOffset; });
      });
  }
};
} // end anonymous namespace

//------------------------------------------------------------------------------
int vtkPolyDataToUnstructuredGrid::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPolyData::GetData(inputVector[0], 0);
  auto output = vtkUnstructuredGrid::GetData(outputVector, 0);

  if (!input || input->GetNumberOfPoints() == 0)
  {
    vtkDebugMacro("No input data.");
    return 1;
  }

  // copy points
  vtkNew<vtkPoints> points;
  points->ShallowCopy(input->GetPoints());
  output->SetPoints(points);

  // copy point data
  output->GetPointData()->ShallowCopy(input->GetPointData());

  const vtkIdType numVerts = input->GetNumberOfVerts();
  const vtkIdType numLines = input->GetNumberOfLines();
  const vtkIdType numPolys = input->GetNumberOfPolys();
  const vtkIdType numStrips = input->GetNumberOfStrips();
  const vtkIdType numCells = numVerts + numLines + numPolys + numStrips;

  if (numCells == 0)
  {
    return 1;
  }

  this->UpdateProgress(0.05);
  if (this->CheckAbort())
  {
    return 1;
  }

  const bool hasVerts = (numVerts > 0);
  const bool hasLines = (numLines > 0);
  const bool hasPolys = (numPolys > 0);
  const bool hasStrips = (numStrips > 0);

  // construct cell types array
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfValues(numCells);
  vtkIdType offset = 0;
  if (hasVerts)
  {
    input->GetVerts()->Visit(
      BuildCellTypesImpl{}, cellTypes,
      [](vtkIdType size) -> VTKCellType { return size == 1 ? VTK_VERTEX : VTK_POLY_VERTEX; },
      offset);
  }
  if (hasLines)
  {
    offset += numVerts;
    input->GetLines()->Visit(
      BuildCellTypesImpl{}, cellTypes,
      [](vtkIdType size) -> VTKCellType { return size == 2 ? VTK_LINE : VTK_POLY_LINE; }, offset);
  }
  if (hasPolys)
  {
    offset += numLines;
    input->GetPolys()->Visit(
      BuildCellTypesImpl{}, cellTypes,
      [](vtkIdType size) -> VTKCellType
      {
        switch (size)
        {
          case 3:
            return VTK_TRIANGLE;
          case 4:
            return VTK_QUAD;
          default:
            return VTK_POLYGON;
        }
      },
      offset);
  }
  if (hasStrips)
  {
    offset += numPolys;
    input->GetStrips()->Visit(
      BuildCellTypesImpl{}, cellTypes,
      [](vtkIdType vtkNotUsed(size)) -> VTKCellType { return VTK_TRIANGLE_STRIP; }, offset);
  }
  this->UpdateProgress(0.5);
  if (this->CheckAbort())
  {
    return 1;
  }

  // check if we can shallow copy only one cell array
  const bool hasOnlyVerts = (hasVerts && !hasLines && !hasPolys && !hasStrips);
  const bool hasOnlyLines = (!hasVerts && hasLines && !hasPolys && !hasStrips);
  const bool hasOnlyPolys = (!hasVerts && !hasLines && hasPolys && !hasStrips);
  const bool hasOnlyStrips = (!hasVerts && !hasLines && !hasPolys && hasStrips);

  if (hasOnlyVerts)
  {
    output->SetPolyhedralCells(cellTypes, input->GetVerts(), nullptr, nullptr);
  }
  else if (hasOnlyLines)
  {
    output->SetPolyhedralCells(cellTypes, input->GetLines(), nullptr, nullptr);
  }
  else if (hasOnlyPolys)
  {
    output->SetPolyhedralCells(cellTypes, input->GetPolys(), nullptr, nullptr);
  }
  else if (hasOnlyStrips)
  {
    output->SetPolyhedralCells(cellTypes, input->GetStrips(), nullptr, nullptr);
  }
  else
  {
    // create offset array
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(numCells + 1);

    // create connectivity array
    vtkNew<vtkIdTypeArray> connectivity;
    const vtkIdType numConnectivity = input->GetVerts()->GetNumberOfConnectivityIds() +
      input->GetLines()->GetNumberOfConnectivityIds() +
      input->GetPolys()->GetNumberOfConnectivityIds() +
      input->GetStrips()->GetNumberOfConnectivityIds();
    connectivity->SetNumberOfValues(numConnectivity);

    offset = 0;
    vtkIdType connectivityOffset = 0;
    if (hasVerts)
    {
      input->GetVerts()->Visit(
        BuildConnectivityImpl{}, offsets, connectivity, offset, connectivityOffset);
    }
    if (hasLines)
    {
      offset += numVerts;
      connectivityOffset += input->GetVerts()->GetNumberOfConnectivityIds();
      input->GetLines()->Visit(
        BuildConnectivityImpl{}, offsets, connectivity, offset, connectivityOffset);
    }
    if (hasPolys)
    {
      offset += numLines;
      connectivityOffset += input->GetLines()->GetNumberOfConnectivityIds();
      input->GetPolys()->Visit(
        BuildConnectivityImpl{}, offsets, connectivity, offset, connectivityOffset);
    }
    if (hasStrips)
    {
      offset += numPolys;
      connectivityOffset += input->GetPolys()->GetNumberOfConnectivityIds();
      input->GetStrips()->Visit(
        BuildConnectivityImpl{}, offsets, connectivity, offset, connectivityOffset);
    }
    // set last offset
    offsets->SetValue(numCells, numConnectivity);

    // create cell array
    vtkNew<vtkCellArray> cellArray;
    cellArray->SetData(offsets, connectivity);
    // set cells
    output->SetPolyhedralCells(cellTypes, cellArray, nullptr, nullptr);
  }

  this->UpdateProgress(0.95);

  // copy cell data
  output->GetCellData()->ShallowCopy(input->GetCellData());
  this->UpdateProgress(1.0);

  return 1;
}
VTK_ABI_NAMESPACE_END
