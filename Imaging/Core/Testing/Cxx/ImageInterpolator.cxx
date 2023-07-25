// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageImport.h"
#include "vtkImageInterpolator.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkNew.h"
#include "vtkPointData.h"

// Test functions
namespace
{

// Print "<text>: (a, b, c) != (x, y, z)"
template <class T>
void PrintError(const char* text, const T* x, const T* y, int size)
{
  std::cout << text << " ";

  const T* v = x;
  for (int j = 0; j < 2; ++j)
  {
    if (j != 0)
    {
      std::cout << " != ";
    }

    const char* delim = "";
    std::cout << "(";
    for (int i = 0; i < size; ++i)
    {
      std::cout << delim << v[i];
      delim = ", ";
    }
    std::cout << ")";
    v = y;
  }

  std::cout << std::endl;
}

// Compare vectors with a small tolerance
template <class T>
bool CompareVectorFuzzy(const char* text, const T* x, const T* y, int size, T tol)
{
  for (int i = 0; i < size; ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(x[i], y[i], tol))
    {
      PrintError(text, x, y, size);
      return false;
    }
  }

  return true;
}

bool TestImageNoDirection()
{
  std::cout << "Testing with no image direction:" << std::endl;

  // basic image information
  const double origin[3] = { 5.1234165, -12.09375, 0.857643 };
  const double spacing[3] = { 5.0, 2.0, 3.0 };
  int extent[6] = { 0, 19, 0, 49, 0, 29 };

  // the input image
  vtkNew<vtkImageData> input;
  input->SetSpacing(spacing);
  input->SetOrigin(origin);
  input->SetExtent(extent);
  input->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  // set the data values to index times spacing for testing
  vtkDataArray* pixels = input->GetPointData()->GetScalars();
  vtkIdType idx = 0;
  for (int k = extent[4]; k <= extent[5]; ++k)
  {
    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      for (int i = extent[0]; i <= extent[1]; ++i)
      {
        pixels->SetTuple3(idx++, i * spacing[0], j * spacing[1], k * spacing[2]);
      }
    }
  }

  vtkNew<vtkImageInterpolator> interpolator;
  interpolator->Initialize(input);
  interpolator->SetInterpolationModeToLinear();
  interpolator->SetOutValue(255);

  bool success = true;

  double points[20][3] = {
    // random points within image bounds
    { 80.6616917, 16.2843800, 58.7851199 },
    { 62.5605082, 47.4324900, 51.7575806 },
    { 90.4047476, -5.3094740, 9.7738484 },
    { 87.2008769, 14.7361779, 45.2379583 },
    { 44.8079918, 33.3200863, 87.2547544 },
    { 11.4262999, 53.5768517, 87.0814281 },
    { 64.8173899, 2.8272480, 45.1864982 },
    { 90.5444488, 22.7720092, 70.7924360 },
    { 97.1644100, 66.8605872, 54.7898647 },
    { 44.9673259, 60.1069581, 44.9178906 },
    // some out-of-bounds points
    { 88.9490496, 55.7590724, 89.1766919 },
    { 103.9643647, 50.7450499, 79.7674468 },
    { 97.7313546, 34.6269689, 88.6995403 },
    { 101.9604439, 46.7481909, 67.1910364 },
    { 77.4775538, 84.4625972, -1.9124923 },
    { 103.5592146, 82.3749221, 10.9448482 },
    { 102.0220226, 3.3229726, 38.7948792 },
    { 49.7995352, 72.2975327, 0.6357556 },
    { 40.6762731, 57.7901959, 88.4700388 },
    { 0.8091914, -5.9155755, 39.3323177 },
  };

  for (int i = 0; i < 20; ++i)
  {
    // expected value if point is out of bounds
    double expectedValue[3] = { 255.0, 255.0, 255.0 };
    if (i < 10)
    {
      // expected interpolated value if point is within bounds
      expectedValue[0] = points[i][0] - origin[0];
      expectedValue[1] = points[i][1] - origin[1];
      expectedValue[2] = points[i][2] - origin[2];
    };

    double value[3];
    bool inBounds = interpolator->Interpolate(points[i], value);
    success &= CompareVectorFuzzy("Value:", value, expectedValue, 3, 1e-4);
    if (!inBounds && i < 10)
    {
      std::cout << "Point " << i << " incorrectly marked out-of-bounds\n";
      success = false;
    }
    else if (inBounds && i >= 10)
    {
      std::cout << "Point " << i << " incorrectly marked in-bounds\n";
      success = false;
    }
  }

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

bool TestImageWithDirection()
{
  std::cout << "Testing with image direction:" << std::endl;

  // basic image information
  const double origin[3] = { 5.1234165, -12.09375, 0.857643 };
  const double direction[9] = {
    // 3x3 matrix
    -0.5618556200580342, -0.34610201570703625, 0.7513532171573689, // 1st row
    0.826672975724893, -0.2014274000446417, 0.525393941454797,     // 2nd row
    -0.030496777130583796, 0.9163189385987733, 0.39928629997767906 // 3rd row
  };
  const double spacing[3] = { 5.0, 2.0, 3.0 };
  int extent[6] = { 0, 19, 0, 49, 0, 29 };

  // the direction matrix
  vtkNew<vtkMatrix3x3> matrix;
  matrix->DeepCopy(direction);
  vtkNew<vtkMatrix3x3> matrixInverse;
  vtkMatrix3x3::Invert(matrix, matrixInverse);

  // the input image
  vtkNew<vtkImageData> input;
  input->SetSpacing(spacing);
  input->SetDirectionMatrix(matrix);
  input->SetOrigin(origin);
  input->SetExtent(extent);
  input->AllocateScalars(VTK_FLOAT, 3);
  // set the data values to index times spacing for testing
  vtkDataArray* pixels = input->GetPointData()->GetScalars();
  vtkIdType idx = 0;
  for (int k = extent[4]; k <= extent[5]; ++k)
  {
    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      for (int i = extent[0]; i <= extent[1]; ++i)
      {
        pixels->SetTuple3(idx++, i * spacing[0], j * spacing[1], k * spacing[2]);
      }
    }
  }

  vtkNew<vtkImageInterpolator> interpolator;
  interpolator->Initialize(input);
  interpolator->SetInterpolationModeToLinear();

  bool success = true;

  double points[20][3] = {
    // random points within image bounds
    { 2.8462919, -8.5164647, 12.7463881 },
    { 9.9924921, 72.6033040, 61.1140928 },
    { -31.8801448, 59.3476671, 43.7503173 },
    { 39.0528309, 15.6922992, 109.4925365 },
    { -60.8563933, 50.6743971, 92.7543887 },
    { -39.5351161, 64.8472584, 89.8646631 },
    { -26.7609181, 33.8420170, 31.1569328 },
    { -18.8116334, 76.8430662, 79.6248037 },
    { 17.6741224, 57.6621854, 94.5859167 },
    { -11.5610305, 90.3415364, 61.0285089 },
    // some out-of-bounds points
    { 34.5208733, 66.5281775, 62.0378498 },
    { 39.0771521, 12.3222314, 61.3746313 },
    { 60.0838912, 46.9133954, 44.1492156 },
    { -22.9413822, 72.5683548, 116.1745177 },
    { -0.5906894, 91.1229086, 94.1050301 },
    { -22.6683535, 37.8556764, 110.2892534 },
    { -8.0201237, 93.3770987, 17.0129858 },
    { 35.5092073, 50.1582868, 80.5899676 },
    { 29.7921164, 30.2326700, 17.7492048 },
    { -21.4728238, 90.5506128, 54.2677831 },
  };

  for (int i = 0; i < 20; ++i)
  {
    // expected value if point is out of bounds
    double expectedValue[3] = { 0.0, 0.0, 0.0 };
    if (i < 10)
    {
      // expected interpolated value if point is in bounds,
      // pixel values are equal to image coordinate with no
      // orientation (direction) or offset (origin)
      expectedValue[0] = points[i][0] - origin[0];
      expectedValue[1] = points[i][1] - origin[1];
      expectedValue[2] = points[i][2] - origin[2];
      matrixInverse->MultiplyPoint(expectedValue, expectedValue);
    };

    double value[3];
    bool inBounds = interpolator->Interpolate(points[i], value);
    success &= CompareVectorFuzzy("Value:", value, expectedValue, 3, 1e-4);
    if (!inBounds && i < 10)
    {
      std::cout << "Point " << i << " incorrectly marked out-of-bounds\n";
      success = false;
    }
    else if (inBounds && i >= 10)
    {
      std::cout << "Point " << i << " incorrectly marked in-bounds\n";
      success = false;
    }
  }

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

}

// Driver Function
int ImageInterpolator(int, char*[])
{
  bool success = true;

  success &= TestImageNoDirection();
  success &= TestImageWithDirection();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
