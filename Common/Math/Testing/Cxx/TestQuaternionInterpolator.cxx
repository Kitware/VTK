// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMathUtilities.h"
#include "vtkQuaternion.h"
#include "vtkQuaternionInterpolator.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"

// Pre-declarations of the test functions
static int TestBinarySearchMethod();

//----------------------------------------------------------------------------
int TestQuaternionInterpolator(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  retVal += TestBinarySearchMethod();

  return retVal;
}

//----------------------------------------------------------------------------
static int TestBinarySearchMethod()
{
  int retVal = 0;

  double epsilon = 1e-12;

  // Number of points / nodes
  int nPoints = 2500;

  // Number of request
  int nRequest = 5 * nPoints;

  vtkSmartPointer<vtkQuaternionInterpolator> interp =
    vtkSmartPointer<vtkQuaternionInterpolator>::New();
  interp->SetInterpolationTypeToLinear();

  // fill the interpolator
  for (int k = 0; k < nPoints; ++k)
  {
    double time = 1.0 + static_cast<double>(k) / static_cast<double>(nPoints) * (6.0 - 1.0);
    double axis[3];
    axis[0] = static_cast<double>(k) / static_cast<double>(nPoints);
    axis[1] = 1.0 - static_cast<double>(k) / static_cast<double>(nPoints);
    axis[2] = 0.5 + static_cast<double>(k) / static_cast<double>(2.0 * nPoints);
    double angle = static_cast<double>(k) / static_cast<double>(nPoints) * 360.0;

    vtkQuaterniond quat;
    quat.SetRotationAngleAndAxis(angle, axis);
    interp->AddQuaternion(time, quat);
  }

  // test the BinarySearch using the LinearSearch
  for (int k = 0; k < nRequest; ++k)
  {
    // Test the binary method in Linear interpolation type
    // and in Spline interpolation type
    if (k % 2 == 0)
    {
      interp->SetInterpolationTypeToLinear();
    }
    else
    {
      interp->SetInterpolationTypeToSpline();
    }

    // requested time
    double time = 0.5 + static_cast<double>(k) / static_cast<double>(nRequest) * (6.5 - 0.5);

    double q1[4];
    double q2[4];

    // Search method : Binary
    interp->SetSearchMethod(0);
    interp->InterpolateQuaternion(time, q1);

    // Search method : Linear
    interp->SetSearchMethod(1);
    interp->InterpolateQuaternion(time, q2);

    for (int i = 0; i < 4; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(q1[i], q2[i], epsilon))
      {
        retVal = 1;
      }
    }

    if (retVal >= 1)
    {
      break;
    }
  }

  return retVal;
}
