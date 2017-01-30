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

namespace tovtkm {

//------------------------------------------------------------------------------
// convert an polydata type
vtkm::cont::DataSet Convert(vtkPolyData* input)
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
  const bool onlyPolys =
      (input->GetNumberOfCells() == input->GetNumberOfPolys());
  const bool onlyLines =
      (input->GetNumberOfCells() == input->GetNumberOfLines());
  const bool onlyVerts =
      (input->GetNumberOfCells() == input->GetNumberOfVerts());

  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (onlyPolys)
  {
    vtkCellArray* cells = input->GetPolys();
    const int maxCellSize = cells->GetMaxCellSize();
    const vtkIdType numCells = cells->GetNumberOfCells();
    // deduce if we only have triangles or quads. We use maxCellSize+1 so
    // that we handle the length entry in the cell array for each cell
    cells->Squeeze();
    const bool allSameType =
        ((numCells * (maxCellSize + 1)) == cells->GetSize());
    if (allSameType && maxCellSize == 3)
    {
      // We are all triangles
      vtkm::cont::DynamicCellSet dcells =
          ConvertSingleType(cells, VTK_TRIANGLE, numPoints);
      dataset.AddCellSet(dcells);
      filled = true;
    }
    else if (allSameType && maxCellSize == 4)
    {
      // We are all quads
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_QUAD, numPoints);
      dataset.AddCellSet(dcells);
      filled = true;
    }

    // we have mixture of polygins/quads/triangles, we don't support that
    // currently
  }
  else if (onlyLines)
  {
    vtkCellArray* cells = input->GetLines();
    const int maxCellSize = cells->GetMaxCellSize();
    if (maxCellSize == 2)
    {
      // We are all lines
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_LINE, numPoints);
      dataset.AddCellSet(dcells);
      filled = true;
    }
    // we have a mixture of lines and poly lines, we don't support that
    // currently
  }
  else if (onlyVerts)
  {
    vtkCellArray* cells = input->GetVerts();
    const int maxCellSize = cells->GetMaxCellSize();
    if (maxCellSize == 1)
    {
      // We are all single vertex
      vtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, VTK_VERTEX, numPoints);
      dataset.AddCellSet(dcells);
      filled = true;
    }
  }

  if (!filled)
  {
    //todo: we need to convert a mixed type which
  }
  return dataset;
}

} // namespace tovtkm

namespace fromvtkm {

//------------------------------------------------------------------------------
bool Convert(const vtkm::cont::DataSet& voutput, vtkPolyData* output,
             vtkDataSet* input)
{
  vtkPoints* points = fromvtkm::Convert(voutput.GetCoordinateSystem());
  output->SetPoints(points);
  points->FastDelete();

  // this should be fairly easy as the cells are all a single cell type
  // so we just need to determine what cell type it is and copy the results
  // into a new array
  vtkm::cont::DynamicCellSet outCells = voutput.GetCellSet();
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
  for (int attributeType = 0;
       attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES; attributeType++)
  {
    vtkDataArray* attribute =
        input->GetPointData()->GetAttribute(attributeType);
    if (attribute == NULL)
    {
      continue;
    }
    output->GetPointData()->SetActiveAttribute(attribute->GetName(),
                                               attributeType);
  }

  return arraysConverted;
}

} // namespace fromvtkm
