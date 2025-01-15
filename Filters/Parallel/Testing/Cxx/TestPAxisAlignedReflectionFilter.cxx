// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPAxisAlignedReflectionFilter.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

int TestPAxisAlignedReflectionFilter(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  vtkNew<vtkSphereSource> sphere;

  // Distribute it
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(false);
  redistribute->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPAxisAlignedReflectionFilter> axisAlignedReflectionFilter;
  axisAlignedReflectionFilter->SetInputConnection(redistribute->GetOutputPort());
  axisAlignedReflectionFilter->SetPlaneModeToXMin();
  axisAlignedReflectionFilter->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(axisAlignedReflectionFilter->GetOutput());

  assert(output->GetNumberOfPartitionedDataSets() == 2);

  double bounds[6];
  output->GetPartitionedDataSet(1)->GetBounds(bounds);

  if (controller->GetLocalProcessId() == 1)
  {
    assert(bounds[0] < -1.4);
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
