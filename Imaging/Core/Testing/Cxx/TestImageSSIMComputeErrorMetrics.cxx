// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageSSIM.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

namespace
{
bool CompareImages(
  const std::string& name0, const std::string& name1, double expectedLoose, double expectedTight)
{
  vtkNew<vtkPNGReader> reader0;
  reader0->SetFileName(name0.c_str());

  vtkNew<vtkPNGReader> reader1;
  reader1->SetFileName(name1.c_str());

  vtkNew<vtkImageSSIM> ssim;
  ssim->SetInputConnection(0, reader0->GetOutputPort());
  ssim->SetInputConnection(1, reader1->GetOutputPort());
  ssim->Update();

  double loose, tight;
  vtkImageSSIM::ComputeErrorMetrics(
    vtkDoubleArray::SafeDownCast(ssim->GetOutput()->GetPointData()->GetScalars()), tight, loose);

  if (!vtkMathUtilities::FuzzyCompare(loose, expectedLoose, 1e-7))
  {
    vtkLog(ERROR, << "Loose computation does not match for " << name0 << " and " << name1
                  << ". Expected: " << expectedLoose << ", got: " << loose);
    return false;
  }
  if (!vtkMathUtilities::FuzzyCompare(tight, expectedTight, 1e-7))
  {
    vtkLog(ERROR, << "Tight computation does not match for " << name0 << " and " << name1
                  << ". Expected: " << expectedTight << ", got: " << tight);
    return false;
  }
  return true;
}
}

int TestImageSSIMComputeErrorMetrics(int argc, char* argv[])
{
  bool ret = true;
  ret &= ::CompareImages(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rgb0.png"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rgb1.png"), 0.00391324, 0.0565826);
  ret &= ::CompareImages(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rgba0.png"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rgba1.png"), 0.00208575, 0.0444985);
  ret &= ::CompareImages(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/grayscale0.png"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/grayscale1.png"), 0.000462338,
    0.0172165);
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
