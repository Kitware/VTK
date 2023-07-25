// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClipDataSet.h"
#include "vtkContourGrid.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"

#include <chrono>
#include <thread>

vtkNew<vtkContourGrid> contour;
bool returnFailure = false;
std::atomic<bool> runUpdate{ false };

void runPipeline()
{

  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkShrinkFilter> shrink;
  vtkNew<vtkClipDataSet> clip;

  wavelet->SetWholeExtent(0, 10, 0, 10, 0, 10);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(1, 10, 10);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  while (!runUpdate)
    ;

  clip->Update();

  if (!clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Clip ABORTED flag is not set.");
    returnFailure = true;
    return;
  }

  contour->SetAbortExecute(0);
  clip->Update();

  if (clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Clip ABORTED flag is set.");
    returnFailure = true;
    return;
  }
}

void toggleAbort()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  contour->SetAbortExecuteAndUpdateTime();
  runUpdate = true;
}

int TestAbortExecuteFromOtherThread(int, char*[])
{
  std::thread threadA(runPipeline);
  std::thread threadB(toggleAbort);

  threadA.join();
  threadB.join();

  return returnFailure;
}
