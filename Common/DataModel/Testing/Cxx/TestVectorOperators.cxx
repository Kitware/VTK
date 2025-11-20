// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSetGet.h"
#include "vtkVector.h"

#include <iostream>

//------------------------------------------------------------------------------
int TestVectorOperators(int, char*[])
{
  vtkVector3i vec3i(0, 6, 9);

  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  // Test out vtkVector3i and ensure the ostream operator is working.
  std::cout << "vec3i -> " << vec3i << std::endl;

  // Test the equality operator.
  vtkVector3i vec3ia(0, 6, 9);
  vtkVector3i vec3ib(0, 6, 8);
  vtkVector<int, 3> vector3f(vec3i.GetData());
  vector3f[0] = vector3f[2] = 6;
  if (!(vec3i == vec3ia))
  {
    std::cerr << "vec3i == vec3ia failed (are equal, but reported not)." << std::endl
              << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << std::endl;
    ++retVal;
  }
  if (vec3ia == vec3ib)
  {
    std::cerr << "vec3ia == vec3ib failed (are not equal, but reported equal)." << std::endl
              << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << std::endl;
    ++retVal;
  }
  if (vector3f == vec3i)
  {
    std::cerr << "vector3f == vec3ib failed (are not equal, but reported equal)." << std::endl
              << "vec3i = " << vector3f << ", vec3ia = " << vec3ia << std::endl;
    ++retVal;
  }

  // Test the inequality operator.
  if (vec3i != vec3ia)
  {
    std::cerr << "vec3i != vec3ia (reported as not equal, but are equal)." << std::endl
              << "vec3i = " << vec3i << ", vec3ia = " << vec3ia << std::endl;
    ++retVal;
  }

  // Does the + operator work as expected???
  vtkVector3i result = vec3ia + vec3ib;
  if (result != vtkVector3i(0, 12, 17))
  {
    std::cerr << "Vector addition operator failed." << std::endl;
    std::cerr << vec3ia << " + " << vec3ib << " = " << result << std::endl;
    ++retVal;
  }

  // Test the - operator.
  result = vec3ia - vec3ib;
  if (result != vtkVector3i(0, 0, 1))
  {
    std::cerr << "Vector subtraction operator failed." << std::endl;
    std::cerr << vec3ia << " - " << vec3ib << " = " << result << std::endl;
    ++retVal;
  }

  // Test the * operator.
  result = vec3ia * vec3ib;
  if (result != vtkVector3i(0, 36, 72))
  {
    std::cerr << "Vector multiplication operator failed." << std::endl;
    std::cerr << vec3ia << " * " << vec3ib << " = " << result << std::endl;
    ++retVal;
  }

  // Test the / operator.
  vec3i.SetX(1);
  result = vec3ia / vec3i;
  if (result != vtkVector3i(0, 1, 1))
  {
    std::cerr << "Vector division operator failed." << std::endl;
    std::cerr << vec3ia << " / " << vec3i << " = " << result << std::endl;
    ++retVal;
  }

  // Test the * operator with a scalar.
  result = vec3ia * 2;
  if (result != vtkVector3i(0, 12, 18))
  {
    std::cerr << "Vector multiplication by scalar operator failed." << std::endl;
    std::cerr << vec3ia << " * 2 = " << result << std::endl;
    ++retVal;
  }
  result = 2 * vec3ia;
  if (result != vtkVector3i(0, 12, 18))
  {
    std::cerr << "Vector multiplication by scalar operator failed." << std::endl;
    std::cerr << "2 * " << vec3ia << " = " << result << std::endl;
    ++retVal;
  }

  // Test the += operator
  result = vec3ia;
  result += vec3ib;
  if (result != vtkVector3i(0, 12, 17))
  {
    std::cerr << "Vector += operator failed." << std::endl;
    std::cerr << vec3ia << " + " << vec3ib << " = " << result << std::endl;
    ++retVal;
  }

  // Test the -= operator
  result = vec3ia;
  result -= vec3ib;
  if (result != vtkVector3i(0, 0, 1))
  {
    std::cerr << "Vector -= operator failed." << std::endl;
    std::cerr << vec3ia << " - " << vec3ib << " = " << result << std::endl;
    ++retVal;
  }

  return retVal;
}
