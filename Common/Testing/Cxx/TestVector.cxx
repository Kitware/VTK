/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSetGet.h"

#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkColor.h"
#include "vtkMathUtilities.h"

//----------------------------------------------------------------------------
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
    cerr << "vtkVector2i should be the same size as int[2]." << endl
        << "sizeof(vec2i) = " << sizeof(vec2i) << endl
        << "sizeof(int[2]) = " << sizeof(arr2i) << endl;
    ++retVal;
    }

  vtkVector<float, 3> vector3f;
  if (vector3f.GetSize() != 3)
    {
    cerr << "Incorrect size of vector3f, should be 3, but is "
        << vector3f.GetSize() << endl;
    ++retVal;
    }

  // Test out vtkVector3i and ensure the various access methods are the same
  vtkVector3i vec3i(0, 6, 9);
  if (vec3i.X() != vec3i[0] || vec3i.X() != 0)
    {
    cerr << "vec3i.X() should equal vec3i.GetData()[0] which should equal 0."
        << "\nvec3i.X() = " << vec3i.X() << endl
        << "vec3i[0] = " << vec3i[0] << endl;
    ++retVal;
    }
  if (vec3i.Y() != vec3i[1] || vec3i.Y() != 6)
    {
    cerr << "vec3i.Y() should equal vec3i.GetData()[1] which should equal 6."
        << "\nvec3i.Y() = " << vec3i.Y() << endl
        << "vec3i[1] = " << vec3i[1] << endl;
    ++retVal;
    }
  if (vec3i.Z() != vec3i[2] || vec3i.Z() != 9)
    {
    cerr << "vec3i.Z() should equal vec3i.GetData()[2] which should equal 9."
        << "\nvec3i.Z() = " << vec3i.Z() << endl
        << "vec3i[2] = " << vec3i[2] << endl;
    ++retVal;
    }

  // Assign the data to an int array and ensure the two ways of referencing are
  // the same.
  int *intPtr = vec3i.GetData();
  for (int i = 0; i < 3; ++i)
    {
    if (vec3i[i] != intPtr[i] || vec3i(i) != vec3i[i])
      {
      cerr << "Error: vec3i[i] != intPtr[i]" << endl
          << "vec3i[i] = " << vec3i[i] << endl
          << "intPtr[i] = " << intPtr[i] << endl;
      ++retVal;
      }
    }

  // Test out casting...
  vtkVector<float, 3> castVec = vec3i.Cast<float>();
  vtkVector3d castVecd(castVec.Cast<double>().GetData());
  if (castVecd[0] < -0.0000001 || castVecd[0] > 0.0000001)
  {
    // Then the number did not make it through within reasonable precision.
    cerr << "Error: castVecd value incorrect. Should be ~0.0 for component 1."
         << "\ncastVecd[0] = " << castVecd[0] << endl;
    ++retVal;
  }
  cout << "castVecd[0] = " << castVecd[0] << endl;

  // Now to test out one of the color classes and memory layouts of arrays
  // Note that the memory layout of a vtkColor3ub[5] is the same as an unsigned
  // char[15], and can be addressed as such.
  vtkColor3ub color[3] = { vtkColor3ub(0, 0, 0), vtkColor3ub(0, 0, 0),
        vtkColor3ub(0, 0, 0) };
  unsigned char* colorPtr = color->GetData();
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (color[i][j] != 0)
        {
        cerr << "Initializer problem in vtkColor3ub - should be zero, but = "
            << color[i][j] << endl;
        ++retVal;
        }
      if (color[i][j] != colorPtr[i*3+j])
        {
        cerr << "Error: color[i][j] != colorPtr[i*3+j]" << endl
            << "color[i][j] = " << color[i][j] << endl
            << "colorPtr[i*3+j] = " << colorPtr[i*3+j] << endl;
        ++retVal;
        }
      color[i][j] = static_cast<unsigned char>(i * 2 + i);
      }
    }

  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (color[i][j] != colorPtr[i*3+j])
        {
        cerr << "Error: color[i][j] != colorPtr[i*3+j]" << endl
            << "color[i][j] = " << color[i][j] << endl
            << "colorPtr[i*3+j] = " << colorPtr[i*3+j] << endl;
        ++retVal;
        }
      }
    }

  // Test the normalize and normalized functions.
  vtkVector3d normy(1, 2, 3);
  vtkVector3d normed = normy.Normalized();
  double dotted = normy.Dot(normed);
  if (!vtkMathUtilities::FuzzyCompare(dotted, 3.74166, 0.0001))
    {
    cerr << "The dot product of " << normy << " and " << normed << " was "
         << dotted << ", expected 3.74166." << endl;
    ++retVal;
    }
  if (!normed.Compare(vtkVector3d(0.267261, 0.534522, 0.801784), 0.0001))
    {
    cerr << "Error vtkVector3d::Normalized() failed: " << normed << endl;
    ++retVal;
    }
  normy.Normalize();
  if (!normy.Compare(normed, 0.0001))
    {
    cerr << "Error vtkVector3d::Normalize() failed: " << normy << endl;
    }
  if (!vtkMathUtilities::FuzzyCompare(normy.Norm(), 1.0, 0.0001))
    {
    cerr << "Normalized length should always be ~= 1.0, value is "
         << normy.Norm() << endl;
    ++retVal;
    }
  if (!vtkMathUtilities::FuzzyCompare(dotted, 3.74166, 0.0001))
    {
    cerr << "The dot product of  "
         << normy.Norm() << endl;
    ++retVal;
    }
  if (!vtkMathUtilities::FuzzyCompare(normy.Dot(normed), 1.0, 0.0001))
    {
    cerr << "The dot product of " << normy << " and " << normed << " was "
         << normy.Dot(normed) << ", expected 1.0." << endl;
    ++retVal;
    }
  // Some cross product stuff now...
  if (!normy.Cross(normed).Compare(vtkVector3d(0, 0, 0), 0.0001))
    {
    cerr << normy << " cross " << normed << " expected to be 0, got "
         << normy.Cross(normed) << endl;
    ++retVal;
    }
  if (!normy.Cross(vtkVector3d(0, 1, 0)).Compare(vtkVector3d(-0.801784, 0, 0.267261),
                                                 0.0001))
    {
    cerr << normy << " cross (0, 1, 0) expected to be (-0.801784, 0, 0.267261), got "
         << normy.Cross(normed) << endl;
    ++retVal;
    }

  return retVal;
}
