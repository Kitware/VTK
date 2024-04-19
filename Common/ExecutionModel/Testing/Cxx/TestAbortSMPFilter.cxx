// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPointDataToCellData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSMPContourGrid.h"
#include "vtkUnstructuredGrid.h"

int TestAbortSMPFilter(int, char*[])
{
  int EXTENT = 30;
  vtkNew<vtkRTAnalyticSource> imageSource;

  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);

  vtkNew<vtkElevationFilter> ev;
  ev->SetInputConnection(imageSource->GetOutputPort());
  ev->SetLowPoint(-EXTENT, -EXTENT, -EXTENT);
  ev->SetHighPoint(EXTENT, EXTENT, EXTENT);

  vtkNew<vtkDataSetTriangleFilter> tetraFilter;
  tetraFilter->SetInputConnection(ev->GetOutputPort());

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputConnection(tetraFilter->GetOutputPort());
  p2c->Update();

  tetraFilter->GetOutput()->GetCellData()->ShallowCopy(p2c->GetOutput()->GetCellData());

  vtkNew<vtkSMPContourGrid> cg;
  cg->SetInputData(tetraFilter->GetOutput());
  cg->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg->SetValue(0, 200);
  cg->SetValue(1, 220);
  cg->SetAbortExecuteAndUpdateTime();
  cg->Update();

  if (!cg->GetAbortExecute())
  {
    vtkLog(ERROR, "vtkSMPContourGrid AbortExecute flag is not set.");
    return 1;
  }

  if (!cg->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkSMPContourGrid ABORTED flag is not set.");
    return 1;
  }

  if (cg->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return 1;
  }

  cg->SetAbortExecute(0);
  cg->Update();

  if (cg->GetAbortExecute())
  {
    vtkLog(ERROR, "vtkSMPContourGrid AbortExecute flag is set.");
    return 1;
  }

  if (cg->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkSMPContourGrid ABORTED flag is set.");
    return 1;
  }

  if (!cg->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "No output data.");
    return 1;
  }

  return 0;
}
