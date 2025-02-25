// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayCalculator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkProbeFilter.h"
#include "vtkRTAnalyticSource.h"

// Gets the number of points the probe filter counted as valid.
// The parameter should be the output of the probe filter
vtkIdType GetNumberOfValidPoints(vtkDataSet* pd)
{
  vtkDataArray* data = pd->GetPointData()->GetScalars("vtkValidPointMask");
  vtkIdType numValid = 0;
  for (vtkIdType i = 0; i < data->GetNumberOfTuples(); ++i)
  {
    if (data->GetVariantValue(i).ToDouble() == 1)
    {
      ++numValid;
    }
  }
  return numValid;
}

bool TestProbeFilterWithProvidedData(
  vtkDataSet* input, vtkDataSet* source, vtkIdType expectedNValidPoints)
{
  vtkNew<vtkProbeFilter> probe;
  probe->SetInputData(input);
  probe->SetSourceData(source);
  probe->Update();

  vtkIdType nValidPoints = GetNumberOfValidPoints(probe->GetOutput());
  if (nValidPoints != expectedNValidPoints)
  {
    std::cerr << "Unexpected number of valid points, got " << nValidPoints << " instead of "
              << expectedNValidPoints << std::endl;
    return false;
  }

  return true;
}

// Tests the CompteThreshold and Threshold parameters on the vtkProbeFilter
bool TestProbeFilterThreshold()
{
  vtkNew<vtkLineSource> line1;
  line1->SetPoint1(-1, 0, 0);
  line1->SetPoint2(10, 0, 0);
  line1->SetResolution(11);

  vtkNew<vtkLineSource> line2;
  line2->SetPoint1(-0.499962, -0.00872654, 0);
  line2->SetPoint2(10.4996, 0.0872654, 0);
  line2->SetResolution(11);

  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(line1->GetOutputPort());
  calc->AddCoordinateScalarVariable("coordsX");
  calc->SetFunction("sin(coordsX)");

  vtkNew<vtkProbeFilter> probe;
  probe->SetInputConnection(calc->GetOutputPort());
  probe->SetSourceConnection(line2->GetOutputPort());
  probe->Update();

  int validDefault = GetNumberOfValidPoints(probe->GetOutput());
  if (validDefault != 2)
  {
    return false;
  }
  // turn off computing tolerance and set it to 11 times what is was.
  // 11 is magic number to get all the points within line1 selected.
  probe->SetComputeTolerance(false);
  probe->SetTolerance(11 * probe->GetTolerance());
  probe->Update();

  int validNext = GetNumberOfValidPoints(probe->GetOutput());

  if (validNext != 11)
  {
    return false;
  }
  // threshold is still set high, but we tell it to ignore it
  probe->SetComputeTolerance(true);
  probe->Update();

  int validIgnore = GetNumberOfValidPoints(probe->GetOutput());
  return (validIgnore == 2) ? true : false;
}

// Test probing one image into another
bool TestProbeFilterWithImages()
{
  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 16, 0, 16, 0, 16);
  wavelet->SetCenter(8, 8, 8);
  wavelet->Update();

  vtkNew<vtkImageData> img;
  img->SetExtent(1, 15, 1, 15, 1, 15);
  img->SetOrigin(1, 1, 1);

  return TestProbeFilterWithProvidedData(img, wavelet->GetOutput(), 3375);
}

// Test probing one image into an oriented one
bool TestProbeFilterWithOrientedImages()
{
  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 16, 0, 16, 0, 16);
  wavelet->SetCenter(8, 8, 8);
  wavelet->Update();

  vtkNew<vtkImageData> img;
  img->SetExtent(1, 15, 1, 15, 1, 15);
  img->SetOrigin(1, 1, 1);
  img->SetDirectionMatrix(0.7, -0.7, 0, 0.7, 0.7, 0, 0, 0, 1);

  return TestProbeFilterWithProvidedData(img, wavelet->GetOutput(), 1575);
}

// Currently only tests the ComputeThreshold and Threshold.  Other tests
// should be added
int TestProbeFilter(int, char*[])
{
  bool status = true;
  status &= TestProbeFilterThreshold();
  status &= TestProbeFilterWithImages();
  status &= TestProbeFilterWithOrientedImages();
  return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
