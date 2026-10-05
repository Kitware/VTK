// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmClip.h"
#include "vtkmContour.h"

#include <viskores/cont/Initialize.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace
{

vtkAlgorithm* gToAbort = nullptr;
std::atomic<bool> gAllowAbort{ false };
std::atomic<bool> gAllowEnd{ false };

// A method to abort an algorithm while its running
void toggleAbort()
{
  while (!gAllowAbort)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // Then abort
  gToAbort->SetAbortExecuteAndUpdateTime();
  gAllowEnd = true;
}

// Update an algorithm then abort it from another thread
void UpdateAbort(vtkAlgorithm* toUpdate, vtkAlgorithm* toAbort)
{
  gAllowAbort = false;
  gAllowEnd = false;
  gToAbort = toAbort;
  std::thread abortThread(toggleAbort);
  toUpdate->Update();
  abortThread.join();
}

class vtkmCustomContour : public vtkmContour
{
public:
  static vtkmCustomContour* New();
  vtkTypeMacro(vtkmCustomContour, vtkmContour);

  // Overridden to avoid races
  int RequestData(
    vtkInformation* req, vtkInformationVector** in, vtkInformationVector* out) override
  {
    gAllowAbort = true;
    int ret = this->Superclass::RequestData(req, in, out);

    // Ensure there is no race by checking abort at the end
    while (!gAllowEnd)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    this->CheckAbort();
    return ret;
  }
};
vtkStandardNewMacro(vtkmCustomContour);
}

int TestVTKMAbort(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkShrinkFilter> shrink;
  vtkNew<::vtkmCustomContour> contour;
  contour->ForceVTKmOn();
  vtkNew<vtkmClip> clip;

  wavelet->SetWholeExtent(0, 10, 0, 10, 0, 10);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(5, -6, 250);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  //--------------------------------------------------------------------------
  std::cout << "Run 1 with abort on contour\n";

  ::UpdateAbort(clip, contour);

  if (!contour->GetAbortExecute())
  {
    vtkLog(ERROR, "Contour AbortExecute flag is not set.");
    return 1;
  }

  if (shrink->GetAbortExecute() || wavelet->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink, Wavelet, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (!contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Contour, or Clip ABORTED flag is not set.");
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return 1;
  }

  //--------------------------------------------------------------------------
  std::cout << "Run 2 with no aborts\n";
  contour->SetAbortExecute(0);
  clip->Update();

  if (wavelet->GetAbortExecute() || shrink->GetAbortExecute() || contour->GetAbortExecute() ||
    clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is set.");
    return 1;
  }

  if (!clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "No output data.");
    return 1;
  }

  std::cout << "Tests successful\n";

  return 0;
}
