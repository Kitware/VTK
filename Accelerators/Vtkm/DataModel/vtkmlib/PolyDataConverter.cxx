//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Field.h>

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
namespace tovtkm
{

//------------------------------------------------------------------------------
// convert an polydata type
vtkm::cont::DataSet Convert(vtkPolyData* input, FieldsFlag fields)
{
  // the poly data is an interesting issue with the fact that the
  // vtk datastructure can contain multiple types.
  // we should look at querying the cell types, so we can use single cell
  // set where possible
  vtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  vtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);

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
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_TRIANGLE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else if (homoSize == 4)
    {
      // We are all quads
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_QUAD, numPoints);
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
      types->SetNumberOfTuples(static_cast<vtkIdType>(cells->GetNumberOfCells()));

      cells->Visit(build_type_array{}, types.GetPointer());

      vtkm::cont::DynamicCellSet dcells = Convert(types, cells, numPoints);
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
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_LINE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      vtkErrorWithObjectMacro(input,
        "VTK-m does not currently support "
        "PolyLine cells.");
    }
  }
  else if (onlyVerts)
  {
    vtkCellArray* cells = input->GetVerts();
    const vtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 1)
    {
      // We are all single vertex
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_VERTEX, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      vtkErrorWithObjectMacro(input,
        "VTK-m does not currently support "
        "PolyVertex cells.");
    }
  }
  else
  {
    vtkErrorWithObjectMacro(input,
      "VTK-m does not currently support mixed "
      "cell types or triangle strips in "
      "vtkPolyData.");
  }

  if (!filled)
  {
    // todo: we need to convert a mixed type which
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

} // namespace tovtkm

namespace fromvtkm
{

//------------------------------------------------------------------------------
bool Convert(const vtkm::cont::DataSet& voutput, vtkPolyData* output, vtkDataSet* input)
{
  vtkPoints* points = fromvtkm::Convert(voutput.GetCoordinateSystem());
  output->SetPoints(points);
  points->FastDelete();

  // this should be fairly easy as the cells are all a single cell type
  // so we just need to determine what cell type it is and copy the results
  // into a new array
  vtkm::cont::DynamicCellSet const& outCells = voutput.GetCellSet();
  vtkNew<vtkCellArray> cells;
  const bool cellsConverted = fromvtkm::Convert(outCells, cells.GetPointer());
  if (!cellsConverted)
  {
    return false;
  }

  output->SetPolys(cells.GetPointer());

  // next we need to convert any extra fields from vtkm over to vtk
  bool arraysConverted = ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

} // namespace fromvtkm
