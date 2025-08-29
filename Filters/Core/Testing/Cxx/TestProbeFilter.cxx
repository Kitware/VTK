// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayCalculator.h"
#include "vtkCellLocator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkProbeFilter.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphereSource.h"

#include <iostream>

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

  vtkNew<vtkProbeFilter> probe;
  probe->SetInputConnection(line1->GetOutputPort());
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

// Test probing one image into an oriented one
bool CompareArrayWithGT(vtkDataArray* array, const double valuesGT[])
{
  auto valuesArray = vtkDoubleArray::SafeDownCast(array);
  bool allTestPassed = true;
  for (vtkIdType i = 0; i < valuesArray->GetNumberOfValues(); ++i)
  {
    if (std::abs(valuesArray->GetValue(i) - valuesGT[i]) > 1e-7)
    {
      allTestPassed = false;
      std::cerr << "Probed value is " << valuesArray->GetValue(i) << " but expected " << valuesGT[i]
                << std::endl;
    }
  }
  return allTestPassed;
}

bool TestProbeFilterWithPolyDataSource()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(0., 0., 0.);
  sphere->SetRadius(0.5);
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(7);

  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(sphere->GetOutputPort());
  calc->AddCoordinateScalarVariable("coordsY", 1);
  calc->AddCoordinateScalarVariable("coordsZ", 2);
  calc->SetFunction("coordsY - coordsZ");
  calc->SetResultArrayName("value");

  vtkNew<vtkLineSource> line;
  line->SetPoint1(0.4714045, 0.2357023, 0.45);
  line->SetPoint2(0.2357023, 0.4714045, -0.45);
  line->SetResolution(6);

  vtkNew<vtkProbeFilter> probe;
  probe->SetInputConnection(line->GetOutputPort());
  probe->SetSourceConnection(calc->GetOutputPort());
  probe->ComputeToleranceOff();
  probe->SetTolerance(1e-6);
  probe->SnapToCellWithClosestPointOn();
  probe->SetCellLocatorPrototype(vtkNew<vtkCellLocator>());
  probe->Update();

  constexpr double valuesGT[7] = { -0.12365478, -0.01496942, 0.15683462, 0.35355338, 0.47838341,
    0.58565986, 0.61193030 };
  bool allTestPassed =
    CompareArrayWithGT(probe->GetOutput()->GetPointData()->GetArray("value"), valuesGT);

  probe->SetSnappingRadius(0.1);
  probe->Update();

  constexpr double valuesWithRadiusGT[7] = { 0, 0, 0.15683462, 0.35355338, 0.47838341, 0, 0 };
  allTestPassed &=
    CompareArrayWithGT(probe->GetOutput()->GetPointData()->GetArray("value"), valuesWithRadiusGT);

  return allTestPassed;
}

// Currently mostly tests the ComputeThreshold and Threshold.
// Only TestProbeFilterWithPolyDataSource tests the probe output at some points.
// Other tests should be added.
int TestProbeFilter(int, char*[])
{
  bool status = true;
  status &= TestProbeFilterThreshold();
  status &= TestProbeFilterWithImages();
  status &= TestProbeFilterWithOrientedImages();
  status &= TestProbeFilterWithPolyDataSource();
  return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
