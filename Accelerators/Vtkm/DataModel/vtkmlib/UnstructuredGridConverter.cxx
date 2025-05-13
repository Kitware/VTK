// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "UnstructuredGridConverter.h"

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
#include <viskores/cont/Field.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// convert an unstructured grid type
viskores::cont::DataSet Convert(vtkUnstructuredGrid* input, FieldsFlag fields)
{
  // This will need to use the custom storage and portals so that
  // we can efficiently map between VTK and Viskores
  viskores::cont::DataSet dataset;

  // first step convert the points over to an array handle
  viskores::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);
  // last

  // Use our custom explicit cell set to do the conversion
  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (input->IsHomogeneous())
  {
    int cellType = input->GetCellType(0); // get the celltype
    auto cells = ConvertSingleType(input->GetCells(), cellType, numPoints);
    dataset.SetCellSet(cells);
  }
  else
  {
    auto cells = Convert(input->GetCellTypesArray(), input->GetCells(), numPoints);
    dataset.SetCellSet(cells);
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
bool Convert(const viskores::cont::DataSet& voutput, vtkUnstructuredGrid* output, vtkDataSet* input)
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
  // viskores to vtk
  vtkNew<vtkCellArray> cells;
  vtkNew<vtkUnsignedCharArray> types;
  auto const& outCells = voutput.GetCellSet();

  const bool cellsConverted = fromvtkm::Convert(outCells, cells.GetPointer(), types.GetPointer());

  if (!cellsConverted)
  {
    return false;
  }

  output->SetCells(types.GetPointer(), cells.GetPointer());

  // now have to set this info back to the unstructured grid

  // Next we need to convert any extra fields from viskores over to vtk
  const bool arraysConverted = fromvtkm::ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

VTK_ABI_NAMESPACE_END
} // namespace fromvtkm
