// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestHyperTreeGrid2DGeometryFillMaterial(int argc, char* argv[])
{
  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;

  // Test data
  char* fileNameC =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/donut_XZ_shift_2d.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;

  // Geometry filter
  vtkNew<vtkHyperTreeGridGeometry> geometryFilter;
  geometryFilter->SetInputConnection(reader->GetOutputPort());
  geometryFilter->Update();

  vtkPolyData* geometry = geometryFilter->GetPolyDataOutput();
  if (!geometry)
  {
    vtkLog(ERROR, "Unable to retrieve htg geometry.");
    return EXIT_FAILURE;
  }

  if (geometry->GetNumberOfPoints() != 456 || geometry->GetNumberOfCells() != 114)
  {
    vtkLog(ERROR, "Incorrect number of points or cells.");
    return EXIT_FAILURE;
  }

  geometryFilter->FillMaterialOff();
  geometryFilter->Update();

  vtkPolyData* interfaceLines = geometryFilter->GetPolyDataOutput();
  if (!interfaceLines)
  {
    vtkLog(ERROR, "Unable to retrieve htg geometry.");
    return EXIT_FAILURE;
  }

  if (interfaceLines->GetNumberOfPoints() != 282 || interfaceLines->GetNumberOfCells() != 100)
  {
    vtkLog(ERROR, "Incorrect number of points or cells.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
