// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkUnstructuredGrid.h"

#include <vtkAppendFilter.h>
#include <vtkGenerateGlobalIds.h>
#include <vtkGeometryFilter.h>
#include <vtkLogger.h>
#include <vtkRedistributeDataSetFilter.h>
#include <vtkSpatioTemporalHarmonicsSource.h>

int TestAppendFilterDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  vtkNew<vtkSpatioTemporalHarmonicsSource> source;
  source->Update();

  vtkNew<vtkGenerateGlobalIds> globalIds;
  globalIds->SetInputData(source->GetOutput());
  globalIds->SetTolerance(0);
  globalIds->Update();

  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputData(globalIds->GetOutput());
  redistribute->Update();

  // This filter is only here to test cell's faces and points validity, it will crash if invalid.
  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(redistribute->GetOutput());
  geometry->Update();

  vtkSmartPointer<vtkUnstructuredGrid> grid =
    vtkUnstructuredGrid::SafeDownCast(redistribute->GetOutput());

  if (grid->GetNumberOfCells() != 8000)
  {
    std::cerr << "Incorrect number of cells. Expected 8000 got " << grid->GetNumberOfCells()
              << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  if (grid->GetNumberOfPoints() != 4851)
  {
    std::cerr << "Incorrect number of points. Expected 4851 got " << grid->GetNumberOfPoints()
              << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
