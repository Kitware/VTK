// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkClipDataSet.h"
#include "vtkCommand.h"
#include "vtkContourGrid.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkPlane.h"
#include "vtkPointDataToCellData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

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

class vtkCustomRTAnalyticSource : public vtkRTAnalyticSource
{
public:
  int AbortEventCounts = 0;
  int CleanupEventCounts = 0;

  static vtkCustomRTAnalyticSource* New();
  vtkTypeMacro(vtkCustomRTAnalyticSource, vtkRTAnalyticSource);

  void AbortCallback()
  {
    if (this->GetAbortExecute())
    {
      // Count abort event while aborted
      this->AbortEventCounts++;
    }
  }

  void CleanupCallback() { this->CleanupEventCounts++; }

  // Overridden to avoid races
  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) override
  {
    gAllowAbort = true;
    this->Superclass::ExecuteDataWithInformation(data, outInfo);

    // Ensure there is no race by checking abort at the end
    while (!gAllowEnd)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    this->CheckAbort();
  }
};
vtkStandardNewMacro(vtkCustomRTAnalyticSource);

class vtkCustomShrinkFilter : public vtkShrinkFilter
{
public:
  static vtkCustomShrinkFilter* New();
  vtkTypeMacro(vtkCustomShrinkFilter, vtkShrinkFilter);

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
vtkStandardNewMacro(vtkCustomShrinkFilter);

class vtkCustomCellCenters : public vtkCellCenters
{
public:
  int AbortEventCounts = 0;
  int CleanupEventCounts = 0;

  static vtkCustomCellCenters* New();
  vtkTypeMacro(vtkCustomCellCenters, vtkCellCenters);

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
vtkStandardNewMacro(vtkCustomCellCenters);

bool AbortExecutePipeline()
{
  vtkNew<::vtkCustomRTAnalyticSource> wavelet;
  vtkNew<::vtkCustomShrinkFilter> shrink;
  vtkNew<vtkContourGrid> contour;
  vtkNew<vtkClipDataSet> clip;

  wavelet->SetWholeExtent(0, 50, 0, 50, 0, 50);
  wavelet->AddObserver(
    vtkCommand::AbortCheckEvent, wavelet.Get(), &::vtkCustomRTAnalyticSource::AbortCallback);
  wavelet->AddObserver(vtkCommand::CleanupAbortCheckEvent, wavelet.Get(),
    &::vtkCustomRTAnalyticSource::CleanupCallback);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(1, 10, 10);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  ::UpdateAbort(clip, wavelet);

  if (wavelet->AbortEventCounts != 1)
  {
    vtkLog(
      ERROR, << "Wavelet did not invoke expected abort check event: " << wavelet->AbortEventCounts);
    return false;
  }
  if (wavelet->CleanupEventCounts != 1)
  {
    vtkLog(ERROR, "Wavelet did not invoke expected cleanup abort check event");
    return false;
  }
  if (!wavelet->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet AbortExecute flag is not set.");
    return false;
  }

  if (shrink->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink, Contour, or Clip AbortExecute flag is set.");
    return false;
  }

  if (!wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set.");
    return false;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return false;
  }
  wavelet->SetAbortExecute(0);

  ::UpdateAbort(clip, shrink);
  if (!shrink->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink AbortExecute flag is not set.");
    return false;
  }

  if (wavelet->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet, Contour, or Clip AbortExecute flag is set.");
    return false;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet ABORTED flag is set.");
    return false;
  }

  if (!shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set.");
    return false;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return false;
  }

  shrink->SetAbortExecute(0);
  clip->Update();

  if (wavelet->GetAbortExecute() || shrink->GetAbortExecute() || contour->GetAbortExecute() ||
    clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip AbortExecute flag is set.");
    return false;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is set.");
    return false;
  }

  if (!clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "No output data.");
    return false;
  }

  return true;
}

bool AbortExecuteSMP()
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

  vtkNew<::vtkCustomCellCenters> cc;
  cc->SetInputData(tetraFilter->GetOutput());
  ::UpdateAbort(cc, cc);

  if (!cc->GetAbortExecute())
  {
    vtkLog(ERROR, "vtkCellCenters AbortExecute flag is not set.");
    return false;
  }

  if (!cc->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkCellCenters ABORTED flag is not set.");
    return false;
  }

  if (cc->GetOutput()->GetNumberOfPoints() > 0)
  {
    vtkLog(ERROR, "Found output data.");
    return false;
  }

  cc->SetAbortExecute(0);
  cc->Update();

  if (cc->GetAbortExecute())
  {
    vtkLog(ERROR, "vtkCellCenters AbortExecute flag is set.");
    return false;
  }

  if (cc->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkCellCenters ABORTED flag is set.");
    return false;
  }

  if (cc->GetOutput()->GetActualMemorySize() == 0)
  {
    vtkLog(ERROR, "No output data.");
    return false;
  }

  return true;
}

// A custom algorithm to check if RD has been called
class vtkTestAlgorithm : public vtkPassInputTypeAlgorithm
{
public:
  static vtkTestAlgorithm* New();
  vtkTestAlgorithm(const vtkTestAlgorithm&) = delete;
  void operator=(const vtkTestAlgorithm&) = delete;

  vtkTypeMacro(vtkTestAlgorithm, vtkAlgorithm);

  vtkGetMacro(RequestDataCount, int);

protected:
  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* vtkNotUsed(outputVector)) override
  {
    this->RequestDataCount++;

    if (this->GetAbortExecute())
    {
      vtkErrorMacro("Filter should not execute while aborted");
      return 0;
    }

    if (this->CheckAbort())
    {
      // We dont consider this a failure despite being aborted
      return 1;
    }
    return 1;
  }

private:
  vtkTestAlgorithm() = default;
  int RequestDataCount = 0;
};
vtkStandardNewMacro(vtkTestAlgorithm);

bool AbortExecuteNoThread()
{
  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkTestAlgorithm> test;
  test->SetInputConnection(sphere->GetOutputPort());

  // Update once and check RD was called
  test->Update();
  if (test->GetRequestDataCount() != 1)
  {
    vtkLog(ERROR, "Unexpected RequestData count after a normal update.");
    return false;
  }

  // Abort and update, then check RD was NOT called
  test->AbortExecuteOn();
  test->Update();
  if (test->GetRequestDataCount() != 1)
  {
    vtkLog(ERROR, "Unexpected RequestData count after an update while aborted.");
    return false;
  }

  // Stop aborting and update, then check RD was called again
  test->AbortExecuteOff();
  test->Update();

  if (test->GetRequestDataCount() != 2)
  {
    vtkLog(ERROR, "Unexpected RequestData count after an update while aborted.");
    return false;
  }

  return true;
}

}

int TestAbortExecute(int, char*[])
{
  bool ret = ::AbortExecutePipeline();
  ret &= ::AbortExecuteSMP();
  ret &= ::AbortExecuteNoThread();
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
