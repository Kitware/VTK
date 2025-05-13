// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "PolyDataConverter.h"

#include "ArrayConverters.h"
#include "CellSetConverters.h"
#include "DataSetConverters.h"

// datasets we support
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/Field.h>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
struct build_type_array
{
  template <typename CellStateT>
  void operator()(CellStateT& state, vtkUnsignedCharArray* types) const
  {
    const vtkIdType size = state.GetNumberOfCells();
    for (vtkIdType i = 0; i < size; ++i)
    {
      auto cellSize = state.GetCellSize(i);
      unsigned char cellType;
      switch (cellSize)
      {
        case 3:
          cellType = VTK_TRIANGLE;
          break;
        case 4:
          cellType = VTK_QUAD;
          break;
        default:
          cellType = VTK_POLYGON;
          break;
      }
      types->SetValue(i, cellType);
    }
  }
};
}
VTK_ABI_NAMESPACE_END
namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// convert an polydata type
viskores::cont::DataSet Convert(vtkPolyData* input, FieldsFlag fields)
{
  // the poly data is an interesting issue with the fact that the
  // vtk datastructure can contain multiple types.
  // we should look at querying the cell types, so we can use single cell
  // set where possible
  viskores::cont::DataSet dataset;

  // Only set coordinates if they exists in the vtkPolyData
  if (input->GetPoints())
  {
    // first step convert the points over to an array handle
    viskores::cont::CoordinateSystem coords = Convert(input->GetPoints());
    dataset.AddCoordinateSystem(coords);
  }

  // first check if we only have polys/lines/verts
  bool filled = false;
  const bool onlyPolys = (input->GetNumberOfCells() == input->GetNumberOfPolys());
  const bool onlyLines = (input->GetNumberOfCells() == input->GetNumberOfLines());
  const bool onlyVerts = (input->GetNumberOfCells() == input->GetNumberOfVerts());

  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (onlyPolys)
  {
    vtkCellArray* cells = input->GetPolys();
    const vtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 3)
    {
      // We are all triangles
      auto dcells = ConvertSingleType(cells, VTK_TRIANGLE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else if (homoSize == 4)
    {
      // We are all quads
      auto dcells = ConvertSingleType(cells, VTK_QUAD, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      // need to construct a vtkUnsignedCharArray* types mapping for our zoo data
      // we can do this by mapping number of points per cell to the type
      // 3 == tri, 4 == quad, else polygon
      vtkNew<vtkUnsignedCharArray> types;
      types->SetNumberOfComponents(1);
      types->SetNumberOfTuples(cells->GetNumberOfCells());

      cells->Visit(build_type_array{}, types.GetPointer());

      auto dcells = Convert(types, cells, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
  }
  else if (onlyLines)
  {
    vtkCellArray* cells = input->GetLines();
    const vtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 2)
    {
      // We are all lines
      auto dcells = ConvertSingleType(cells, VTK_LINE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      throw viskores::cont::ErrorBadType("Viskores does not currently support PolyLine cells.");
    }
  }
  else if (onlyVerts)
  {
    vtkCellArray* cells = input->GetVerts();
    const vtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 1)
    {
      // We are all single vertex
      auto dcells = ConvertSingleType(cells, VTK_VERTEX, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      throw viskores::cont::ErrorBadType("Viskores does not currently support PolyVertex cells.");
    }
  }
  else
  {
    throw viskores::cont::ErrorBadType(
      "Viskores does not currently support mixed cell types or triangle strips in vtkPolyData.");
  }

  if (!filled)
  {
    // todo: we need to convert a mixed type which
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

VTK_ABI_NAMESPACE_END
} // namespace tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
bool Convert(const viskores::cont::DataSet& voutput, vtkPolyData* output, vtkDataSet* input)
{
  vtkPoints* points = fromvtkm::Convert(voutput.GetCoordinateSystem());
  output->SetPoints(points);
  points->FastDelete();

  // this should be fairly easy as the cells are all a single cell type
  // so we just need to determine what cell type it is and copy the results
  // into a new array
  auto const& outCells = voutput.GetCellSet();
  vtkNew<vtkCellArray> cells;
  const bool cellsConverted = fromvtkm::Convert(outCells, cells.GetPointer());
  if (!cellsConverted)
  {
    return false;
  }

  output->SetPolys(cells.GetPointer());

  // next we need to convert any extra fields from viskores over to vtk
  bool arraysConverted = ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

VTK_ABI_NAMESPACE_END
} // namespace fromvtkm
