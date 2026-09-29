// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test abort function for AMR Filters that call vtkAMRUtilities::BlankCells

#include "vtkAMRCutPlane.h"
#include "vtkAMRGaussianPulseSource.h"
#include "vtkGenerateIds.h"
#include "vtkGradientFilter.h"
#include "vtkImageToAMR.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTestUtilities.h"

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

class vtkCustomAMRGaussianPulseSource : public vtkAMRGaussianPulseSource
{
public:
  static vtkCustomAMRGaussianPulseSource* New();
  vtkTypeMacro(vtkCustomAMRGaussianPulseSource, vtkAMRGaussianPulseSource);

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
vtkStandardNewMacro(vtkCustomAMRGaussianPulseSource);

class vtkCustomAMRCutPlane : public vtkAMRCutPlane
{
public:
  static vtkCustomAMRCutPlane* New();
  vtkTypeMacro(vtkCustomAMRCutPlane, vtkAMRCutPlane);

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
vtkStandardNewMacro(vtkCustomAMRCutPlane);

class vtkCustomImageToAMR : public vtkImageToAMR
{
public:
  static vtkCustomImageToAMR* New();
  vtkTypeMacro(vtkCustomImageToAMR, vtkImageToAMR);

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
vtkStandardNewMacro(vtkCustomImageToAMR);

bool PulseSourceTest()
{
  vtkNew<::vtkCustomAMRGaussianPulseSource> src;
  ::UpdateAbort(src, src);

  if (!src->GetAbortExecute() || !src->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRGaussianPulseSource did not abort properly.");
    return false;
  }

  src->SetAbortExecute(0);
  src->Update();

  if (src->GetAbortExecute() || src->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRGaussianPulseSource did not run properly.");
    return false;
  }
  return true;
}

bool CutPlaneTest()
{
  vtkNew<vtkAMRGaussianPulseSource> src;

  vtkNew<::vtkCustomAMRCutPlane> cut;
  cut->SetInputConnection(src->GetOutputPort());
  ::UpdateAbort(cut, cut);

  if (!cut->GetAbortExecute() || !cut->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRCutPlane did not abort properly.");
    return false;
  }

  cut->SetAbortExecute(0);
  cut->Update();

  if (cut->GetAbortExecute() || cut->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRCutPlane did not run properly.");
    return false;
  }
  return true;
}

bool ImageToAMRTest()
{
  vtkNew<vtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(0, 0, -128, 128, -128, 128);

  vtkNew<vtkGenerateIds> idFilter;
  idFilter->SetInputConnection(imageSource->GetOutputPort());

  vtkNew<::vtkCustomImageToAMR> amrConverter;
  amrConverter->SetInputConnection(idFilter->GetOutputPort());
  amrConverter->SetNumberOfLevels(4);
  amrConverter->SetMaximumNumberOfBlocks(10);
  ::UpdateAbort(amrConverter, amrConverter);

  if (!amrConverter->GetAbortExecute() ||
    !amrConverter->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRamrConverterFilter did not abort properly.");
    return false;
  }

  amrConverter->SetAbortExecute(0);
  amrConverter->Update();

  if (amrConverter->GetAbortExecute() ||
    amrConverter->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRamrConverterFilter did not run properly.");
    return false;
  }
  return true;
}
}

int TestAMRAbortExecute(int, char*[])
{
  bool ret = ::PulseSourceTest();
  ret &= ::CutPlaneTest();
  ret &= ::ImageToAMRTest();

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
