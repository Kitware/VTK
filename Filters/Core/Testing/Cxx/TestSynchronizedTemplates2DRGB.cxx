// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkRTAnalyticSource.h"
#include "vtkScalarsToColors.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

int TestSynchronizedTemplates2DRGB(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);

  vtkNew<vtkRTAnalyticSource> waveletSource;
  waveletSource->SetWholeExtent(-10, 10, -10, 10, 0, 0);
  waveletSource->SetCenter(0.0, 0.0, 0.0);
  waveletSource->Update();

  vtkNew<vtkImageCast> cast;
  cast->SetOutputScalarTypeToUnsignedChar();
  cast->SetInputData(waveletSource->GetOutput());
  cast->Update();

  vtkImageData* wavelet = cast->GetOutput();

  vtkNew<vtkScalarsToColors> colorTable;
  colorTable->SetRange(0, 255);

  vtkNew<vtkImageMapToColors> colors;
  colors->SetLookupTable(colorTable);
  colors->SetOutputFormatToRGB();
  colors->SetInputData(wavelet);
  colors->Update();

  vtkImageData* waveletRGB = colors->GetOutput();

  vtkNew<vtkSynchronizedTemplates2D> stFilter;
  stFilter->SetInputData(wavelet);
  stFilter->GenerateValues(3, 100, 250);
  stFilter->ComputeScalarsOff();
  stFilter->Update();

  vtkPolyData* outputData = stFilter->GetOutput();

  vtkNew<vtkSynchronizedTemplates2D> stFilterRGB;
  stFilterRGB->SetInputData(waveletRGB);
  stFilterRGB->GenerateValues(3, 100, 250);
  stFilterRGB->ComputeScalarsOff();
  stFilterRGB->Update();

  vtkPolyData* outputDataRGB = stFilterRGB->GetOutput();

  // output should be the same when data is RGB
  vtkTestUtilities::CompareDataObjects(outputData, outputDataRGB);

  return EXIT_SUCCESS;
}
