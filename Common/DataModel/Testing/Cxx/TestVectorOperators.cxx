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

//----------------------------------------------------------------------------
int TestVectorOperators(int, char*[])
{
  vtkVector3i vec3i(0, 6, 9);

  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  // Test out vtkVector3i and ensure the ostream operator is working.
  cout << "vec3i -> " << vec3i << endl;

  // Test the equality operator.
  vtkVector3i vec3ia(0, 6, 9);
  vtkVector3i vec3ib(0, 6, 8);
  vtkVector<int, 3> vector3f(vec3i.GetData());
  vector3f[0] = vector3f[2] = 6;
  if (!(vec3i == vec3ia))
    {
    cerr << "vec3i == vec3ia failed (are equal, but reported not)." << endl
         << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << endl;
    ++retVal;
    }
  if (vec3ia == vec3ib)
    {
    cerr << "vec3ia == vec3ib failed (are not equal, but reported equal)."
         << endl << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << endl;
    ++retVal;
    }
  if (vector3f == vec3i)
    {
    cerr << "vector3f == vec3ib failed (are not equal, but reported equal)."
         << endl << "vec3i = " << vector3f << ", vec3ia = " << vec3ia << endl;
    ++retVal;
    }

  // Test the inequality operator.
  if (vec3i != vec3ia)
    {
    cerr << "vec3i != vec3ia (reported as not equal, but are equal)." << endl
         << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << endl;
    ++retVal;
    }

  // Does the + operator work as expected???
  vtkVector3i result = vec3ia + vec3ib;
  if (result != vtkVector3i(0, 12, 17))
    {
    cerr << "Vector addition operator failed." << endl;
    cerr << vec3ia << " + " << vec3ib << " = " << result << endl;
    ++retVal;
    }

  // Test the - operator.
  result = vec3ia - vec3ib;
  if (result != vtkVector3i(0, 0, 1))
    {
    cerr << "Vector subtraction operator failed." << endl;
    cerr << vec3ia << " - " << vec3ib << " = " << result << endl;
    ++retVal;
    }

  // Test the * operator.
  result = vec3ia * vec3ib;
  if (result != vtkVector3i(0, 36, 72))
    {
    cerr << "Vector multiplication operator failed." << endl;
    cerr << vec3ia << " * " << vec3ib << " = " << result << endl;
    ++retVal;
    }

  // Test the / operator.
  vec3i.SetX(1);
  result = vec3ia / vec3i;
  if (result != vtkVector3i(0, 1, 1))
    {
    cerr << "Vector division operator failed." << endl;
    cerr << vec3ia << " / " << vec3i << " = " << result << endl;
    ++retVal;
    }

  // Test the * operator with a scalar.
  result = vec3ia * 2;
  if (result != vtkVector3i(0, 12, 18))
    {
    cerr << "Vector multiplication by scalar operator failed." << endl;
    cerr << vec3ia << " * 2 = " << result << endl;
    ++retVal;
    }
  result = 2 * vec3ia;
  if (result != vtkVector3i(0, 12, 18))
    {
    cerr << "Vector multiplication by scalar operator failed." << endl;
    cerr << "2 * " << vec3ia << " = " << result << endl;
    ++retVal;
    }

  return retVal;
}
