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

#include "UnstructuredGridConverter.h"

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
// convert an unstructured grid type
vtkm::cont::DataSet Convert(vtkUnstructuredGrid* input)
{
  // This will need to use the custom storage and portals so that
  // we can efficiently map between VTK and VTKm
  vtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  vtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);
  // last

  // Use our custom explicit cell set to do the conversion
  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (input->IsHomogeneous())
  {
    int cellType = input->GetCellType(0); // get the celltype
    vtkm::cont::DynamicCellSet cells =
        ConvertSingleType(input->GetCells(), cellType, numPoints);
    dataset.AddCellSet(cells);
  }
  else
  {
    vtkm::cont::DynamicCellSet cells =
        Convert(input->GetCellTypesArray(),
                input->GetCells(),
                input->GetCellLocationsArray(),
                numPoints);
    dataset.AddCellSet(cells);
  }

  return dataset;
}

} // namespace tovtkm

namespace fromvtkm {

//------------------------------------------------------------------------------
bool Convert(const vtkm::cont::DataSet& voutput, vtkUnstructuredGrid* output,
             vtkDataSet* input)
{
  vtkPoints* points = fromvtkm::Convert(voutput.GetCoordinateSystem());
  // If this fails, it's likely a missing entry in tovtkm::PointListOutVTK:
  if (!points)
  {
    return false;
  }
  output->SetPoints(points);
  points->FastDelete();

  // With unstructured grids we need to actually convert 3 arrays from
  // vtkm to vtk
  vtkNew<vtkCellArray> cells;
  vtkNew<vtkUnsignedCharArray> types;
  vtkNew<vtkIdTypeArray> locations;
  vtkm::cont::DynamicCellSet outCells = voutput.GetCellSet();

  const bool cellsConverted = fromvtkm::Convert(
      outCells, cells.GetPointer(), types.GetPointer(), locations.GetPointer());

  if (!cellsConverted)
  {
    return false;
  }

  output->SetCells(types.GetPointer(), locations.GetPointer(),
                   cells.GetPointer());

  // now have to set this info back to the unstructured grid

  // Next we need to convert any extra fields from vtkm over to vtk
  const bool arraysConverted = fromvtkm::ConvertArrays(voutput, output);

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
