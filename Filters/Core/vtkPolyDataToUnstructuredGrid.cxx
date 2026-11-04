// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataToUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>

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
enum ArrayKind
{
  VERTS = 0,
  LINES,
  POLYS,
  STRIPS
};

// Cell type of a cell with the given number of points that comes from the given cell array.
int GetCellType(ArrayKind kind, vtkIdType size)
{
  switch (kind)
  {
    case VERTS:
      return size == 1 ? VTK_VERTEX : VTK_POLY_VERTEX;
    case LINES:
      return size == 2 ? VTK_LINE : VTK_POLY_LINE;
    case POLYS:
      if (size == 3)
      {
        return VTK_TRIANGLE;
      }
      return size == 4 ? VTK_QUAD : VTK_POLYGON;
    default:
      return VTK_TRIANGLE_STRIP;
  }
}

struct CopyConnectivityImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* conn, vtkIdType* out)
  {
    vtkSMPTools::For(0, conn->GetNumberOfValues(),
      [&](vtkIdType begin, vtkIdType end)
      {
        auto inConnRange = GetRange(conn).GetSubRange(begin, end);
        std::copy(inConnRange.begin(), inConnRange.end(), out + begin);
      });
  }
};

struct CopyOffsetsImpl : public vtkCellArray::DispatchUtilities
{
  // Writes the offsets of a segment with cells of different sizes, shifted by `shift`.
  template <class OffsetsT, class ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType* out, vtkIdType shift)
  {
    const vtkIdType numCells = offsets->GetNumberOfValues() - 1;
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto inOffRange = GetRange(offsets).GetSubRange(begin, end);
        std::transform(inOffRange.begin(), inOffRange.end(), out + begin,
          [shift](auto val) -> vtkIdType { return static_cast<vtkIdType>(val) + shift; });
      });
  }
};

struct BuildCellTypesImpl : public vtkCellArray::DispatchUtilities
{
  // Writes the type of every cell of a segment with cells of different sizes
  template <typename OffsetsT, typename ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), unsigned char* types, ArrayKind kind)
  {
    const vtkIdType numCells = offsets->GetNumberOfValues() - 1;
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          types[cellId] =
            static_cast<unsigned char>(GetCellType(kind, GetCellSize(offsets, cellId)));
        }
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

  if (!input)
  {
    vtkDebugMacro("No input data.");
    return 1;
  }

  // the field data is shared, even without points
  output->GetFieldData()->ShallowCopy(input->GetFieldData());
  if (input->GetNumberOfPoints() == 0)
  {
    vtkDebugMacro("No input points.");
    return 1;
  }

  // points, point data and cell data are shared
  output->SetPoints(input->GetPoints());
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->ShallowCopy(input->GetCellData());

  // the cell arrays of the polydata in the order of its cells
  const std::array<vtkCellArray*, 4> cellArraysPerType = { input->GetVerts(), input->GetLines(),
    input->GetPolys(), input->GetStrips() };
  std::array<vtkIdType, 4> numCellsPerType = {};
  // number of points of every cell of an array if they all have the same size, otherwise -1
  std::array<vtkIdType, 4> cellSizesPerType = {};
  // first cell and first connectivity entry of an array in the output
  std::array<vtkIdType, 4> cellOffsetsPerType = {};
  std::array<vtkIdType, 4> connectivityOffsetsPerType = {};
  vtkIdType numCells = 0;
  vtkIdType numConnectivity = 0;
  int numArrays = 0;
  int lastArray = 0;
  for (int i = 0; i < 4; ++i)
  {
    numCellsPerType[i] = cellArraysPerType[i]->GetNumberOfCells();
    if (numCellsPerType[i] == 0)
    {
      continue;
    }
    cellSizesPerType[i] = cellArraysPerType[i]->IsHomogeneous(); // O(1) for fixed size storage
    cellOffsetsPerType[i] = numCells;
    connectivityOffsetsPerType[i] = numConnectivity;
    numCells += numCellsPerType[i];
    numConnectivity += cellArraysPerType[i]->GetNumberOfConnectivityIds();
    ++numArrays;
    lastArray = i;
  }
  if (numCells == 0)
  {
    return 1;
  }

  this->UpdateProgress(0.05);
  if (this->CheckAbort())
  {
    return 1;
  }

  // all the cells of an array have the same type, so no type per cell is needed
  auto hasConstantType = [&](int i) { return i == STRIPS || cellSizesPerType[i] > 0; };
  auto constantType = [&](int i) {
    return static_cast<unsigned char>(GetCellType(static_cast<ArrayKind>(i), cellSizesPerType[i]));
  };

  // The cells are shared when there is only one cell array
  vtkSmartPointer<vtkCellArray> outCells;
  if (numArrays == 1)
  {
    outCells = cellArraysPerType[lastArray];
  }
  else
  {
    // if all the cells have the same size the offsets are implicit (fixed size cell array)
    bool sameSize = true;
    vtkIdType firstSize = -1;
    for (int i = 0; i < 4; ++i)
    {
      if (numCellsPerType[i] == 0)
      {
        continue;
      }
      firstSize = firstSize < 0 ? cellSizesPerType[i] : firstSize;
      sameSize = sameSize && cellSizesPerType[i] > 0 && cellSizesPerType[i] == firstSize;
    }

    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(numConnectivity);
    for (int i = 0; i < 4; ++i)
    {
      if (numCellsPerType[i] > 0)
      {
        cellArraysPerType[i]->Dispatch(
          CopyConnectivityImpl{}, connectivity->GetPointer(connectivityOffsetsPerType[i]));
      }
    }

    outCells = vtkSmartPointer<vtkCellArray>::New();
    if (sameSize)
    {
      outCells->SetData(firstSize, connectivity);
    }
    else
    {
      vtkNew<vtkIdTypeArray> offsets;
      offsets->SetNumberOfValues(numCells + 1);
      offsets->SetValue(numCells, numConnectivity);
      for (int i = 0; i < 4; ++i)
      {
        if (numCellsPerType[i] == 0)
        {
          continue;
        }
        vtkIdType* out = offsets->GetPointer(cellOffsetsPerType[i]);
        if (cellSizesPerType[i] > 0)
        {
          // the offsets are an arithmetic sequence
          const vtkIdType size = cellSizesPerType[i];
          const vtkIdType shift = connectivityOffsetsPerType[i];
          vtkSMPTools::For(0, numCellsPerType[i],
            [&](vtkIdType begin, vtkIdType end)
            {
              for (vtkIdType c = begin; c < end; ++c)
              {
                out[c] = shift + c * size;
              }
            });
        }
        else
        {
          cellArraysPerType[i]->Dispatch(CopyOffsetsImpl{}, out, connectivityOffsetsPerType[i]);
        }
      }
      outCells->SetData(offsets, connectivity);
    }
  }
  this->UpdateProgress(0.5);
  if (this->CheckAbort())
  {
    return 1;
  }

  // The cell types are implicit when all the cells have the same type. They are set directly
  // since there are no polyhedra in a polydata.
  vtkSmartPointer<vtkDataArray> cellTypes;
  if (numArrays == 1 && hasConstantType(lastArray))
  {
    vtkNew<vtkConstantArray<unsigned char>> types;
    types->ConstructBackend(constantType(lastArray));
    types->SetNumberOfValues(numCells);
    cellTypes = types;
  }
  else
  {
    vtkNew<vtkUnsignedCharArray> types;
    types->SetNumberOfValues(numCells);
    for (int i = 0; i < 4; ++i)
    {
      if (numCellsPerType[i] == 0)
      {
        continue;
      }
      unsigned char* out = types->GetPointer(cellOffsetsPerType[i]);
      if (hasConstantType(i))
      {
        const unsigned char type = constantType(i);
        vtkSMPTools::For(0, numCellsPerType[i],
          [&](vtkIdType begin, vtkIdType end) { std::fill(out + begin, out + end, type); });
      }
      else
      {
        cellArraysPerType[i]->Dispatch(BuildCellTypesImpl{}, out, static_cast<ArrayKind>(i));
      }
    }
    cellTypes = types;
  }
  output->SetPolyhedralCells(cellTypes, outCells, nullptr, nullptr);
  this->UpdateProgress(1.0);

  return 1;
}
VTK_ABI_NAMESPACE_END
