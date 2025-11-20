// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSetGet.h"

#include "vtkMathUtilities.h"
#include "vtkVector.h"

#include <iostream>

//------------------------------------------------------------------------------
int TestVector(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  // Test out the general vector data types, give nice API and great memory use
  vtkVector2i vec2i;
  int arr2i[2] = { 0, 0 };
  vec2i.Set(arr2i[0], arr2i[1]);

  if (sizeof(vec2i) != sizeof(arr2i))
  {
    // The two should be the same size and memory layout - error out if not
    std::cerr << "vtkVector2i should be the same size as int[2]." << std::endl
              << "sizeof(vec2i) = " << sizeof(vec2i) << std::endl
              << "sizeof(int[2]) = " << sizeof(arr2i) << std::endl;
    ++retVal;
  }

  vtkVector<float, 3> vector3f;
  if (vector3f.GetSize() != 3)
  {
    std::cerr << "Incorrect size of vector3f, should be 3, but is " << vector3f.GetSize()
              << std::endl;
    ++retVal;
  }

  // Test out vtkVector3i and ensure the various access methods are the same
  vtkVector3i vec3i(0, 6, 9);
  if (vec3i.GetX() != vec3i[0] || vec3i.GetX() != 0)
  {
    std::cerr << "vec3i.GetX() should equal vec3i.GetData()[0] which should equal 0."
              << "\nvec3i.GetX() = " << vec3i.GetX() << std::endl
              << "vec3i[0] = " << vec3i[0] << std::endl;
    ++retVal;
  }
  if (vec3i.GetY() != vec3i[1] || vec3i.GetY() != 6)
  {
    std::cerr << "vec3i.GetY() should equal vec3i.GetData()[1] which should equal 6."
              << "\nvec3i.GetY() = " << vec3i.GetY() << std::endl
              << "vec3i[1] = " << vec3i[1] << std::endl;
    ++retVal;
  }
  if (vec3i.GetZ() != vec3i[2] || vec3i.GetZ() != 9)
  {
    std::cerr << "vec3i.GetZ() should equal vec3i.GetData()[2] which should equal 9."
              << "\nvec3i.GetZ() = " << vec3i.GetZ() << std::endl
              << "vec3i[2] = " << vec3i[2] << std::endl;
    ++retVal;
  }

  // Assign the data to an int array and ensure the two ways of referencing are
  // the same.
  int* intPtr = vec3i.GetData();
  for (int i = 0; i < 3; ++i)
  {
    if (vec3i[i] != intPtr[i] || vec3i(i) != vec3i[i])
    {
      std::cerr << "Error: vec3i[i] != intPtr[i]" << std::endl
                << "vec3i[i] = " << vec3i[i] << std::endl
                << "intPtr[i] = " << intPtr[i] << std::endl;
      ++retVal;
    }
  }

  // Test out casting...
  vtkVector<float, 3> castVec = vec3i.Cast<float>();
  vtkVector3d castVecd(castVec.Cast<double>().GetData());
  if (castVecd[0] < -0.0000001 || castVecd[0] > 0.0000001)
  {
    // Then the number did not make it through within reasonable precision.
    std::cerr << "Error: castVecd value incorrect. Should be ~0.0 for component 1."
              << "\ncastVecd[0] = " << castVecd[0] << std::endl;
    ++retVal;
  }
  std::cout << "castVecd[0] = " << castVecd[0] << std::endl;

  // Test the normalize and normalized functions.
  vtkVector3d normy(1, 2, 3);
  vtkVector3d normed = normy.Normalized();
  double dotted = normy.Dot(normed);
  if (!vtkMathUtilities::FuzzyCompare(dotted, 3.74166, 0.0001))
  {
    std::cerr << "The dot product of " << normy << " and " << normed << " was " << dotted
              << ", expected 3.74166." << std::endl;
    ++retVal;
  }
  if (!normed.Compare(vtkVector3d(0.267261, 0.534522, 0.801784), 0.0001))
  {
    std::cerr << "Error vtkVector3d::Normalized() failed: " << normed << std::endl;
    ++retVal;
  }
  normy.Normalize();
  if (!normy.Compare(normed, 0.0001))
  {
    std::cerr << "Error vtkVector3d::Normalize() failed: " << normy << std::endl;
  }
  if (!vtkMathUtilities::FuzzyCompare(normy.Norm(), 1.0, 0.0001))
  {
    std::cerr << "Normalized length should always be ~= 1.0, value is " << normy.Norm()
              << std::endl;
    ++retVal;
  }
  if (!vtkMathUtilities::FuzzyCompare(dotted, 3.74166, 0.0001))
  {
    std::cerr << "The dot product of  " << normy.Norm() << std::endl;
    ++retVal;
  }
  if (!vtkMathUtilities::FuzzyCompare(normy.Dot(normed), 1.0, 0.0001))
  {
    std::cerr << "The dot product of " << normy << " and " << normed << " was " << normy.Dot(normed)
              << ", expected 1.0." << std::endl;
    ++retVal;
  }
  // Some cross product stuff now...
  if (!normy.Cross(normed).Compare(vtkVector3d(0, 0, 0), 0.0001))
  {
    std::cerr << normy << " cross " << normed << " expected to be 0, got " << normy.Cross(normed)
              << std::endl;
    ++retVal;
  }
  if (!normy.Cross(vtkVector3d(0, 1, 0)).Compare(vtkVector3d(-0.801784, 0, 0.267261), 0.0001))
  {
    std::cerr << normy << " cross (0, 1, 0) expected to be (-0.801784, 0, 0.267261), got "
              << normy.Cross(normed) << std::endl;
    ++retVal;
  }

  return retVal;
}
