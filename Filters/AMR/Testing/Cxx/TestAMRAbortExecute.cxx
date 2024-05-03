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
#include "vtkOverlappingAMR.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTestUtilities.h"

static int returnValue = 0;

void PulseSourceTest()
{
  vtkNew<vtkAMRGaussianPulseSource> src;
  src->SetAbortExecuteAndUpdateTime();
  src->Update();

  if (!src->GetAbortExecute() || !src->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRGaussianPulseSource did not abort properly.");
    returnValue = 1;
  }

  src->SetAbortExecute(0);
  src->Update();

  if (src->GetAbortExecute() || src->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRGaussianPulseSource did not run properly.");
    returnValue = 1;
  }
}

void CutPlaneTest()
{

  vtkNew<vtkAMRGaussianPulseSource> src;

  vtkNew<vtkAMRCutPlane> cut;
  cut->SetInputConnection(src->GetOutputPort());
  cut->SetAbortExecuteAndUpdateTime();
  cut->Update();

  if (!cut->GetAbortExecute() || !cut->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRCutPlane did not abort properly.");
    returnValue = 1;
  }

  cut->SetAbortExecute(0);
  cut->Update();

  if (cut->GetAbortExecute() || cut->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRCutPlane did not run properly.");
    returnValue = 1;
  }
}

void ImageToAMRTest()
{
  vtkNew<vtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(0, 0, -128, 128, -128, 128);

  vtkNew<vtkGenerateIds> idFilter;
  idFilter->SetInputConnection(imageSource->GetOutputPort());

  vtkNew<vtkImageToAMR> amrConverter;
  amrConverter->SetInputConnection(idFilter->GetOutputPort());
  amrConverter->SetNumberOfLevels(4);
  amrConverter->SetMaximumNumberOfBlocks(10);

  amrConverter->SetAbortExecuteAndUpdateTime();
  amrConverter->Update();

  if (!amrConverter->GetAbortExecute() ||
    !amrConverter->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRamrConverterFilter did not abort properly.");
    returnValue = 1;
  }

  amrConverter->SetAbortExecute(0);
  amrConverter->Update();

  if (amrConverter->GetAbortExecute() ||
    amrConverter->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "vtkAMRamrConverterFilter did not run properly.");
    returnValue = 1;
  }
}

int TestAMRAbortExecute(int, char*[])
{
  PulseSourceTest();
  CutPlaneTest();
  ImageToAMRTest();

  return returnValue;
}
