/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPiecewiseFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPiecewiseFunction.h"

#include "vtkNew.h"

#include <cmath>

#define TEST_ASSERT_FUZZY_EQUAL(expect, actual)                                                    \
  do                                                                                               \
  {                                                                                                \
    if (std::fabs((expect) - (actual)) >= 1e-5)                                                    \
    {                                                                                              \
      std::cerr << "Error at line " << __LINE__ << ": Expected value " << (expect) << ", got "     \
                << (actual) << std::endl;                                                          \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestPiecewiseFunction(int, char*[])
{
  vtkNew<vtkPiecewiseFunction> func;
  func->AllowDuplicateScalarsOn();

  // Add some points that will give easily predictable interpolations.
  func->AddPoint(0., -2.);
  func->AddPoint(50., 0.);
  int ret = func->AddPoint(50., 2.);
  if (ret != 2)
  {
    std::cerr << "Error adding duplicated point" << std::endl;
    return EXIT_FAILURE;
  }

  func->AddPoint(100., 5.);

  // Check that the interpolations are correct.
  TEST_ASSERT_FUZZY_EQUAL(-1., func->GetValue(25.));
  TEST_ASSERT_FUZZY_EQUAL(3.5, func->GetValue(75.));

  // Check that point removing is working
  func->RemovePoint(50., 2.);
  TEST_ASSERT_FUZZY_EQUAL(2.5, func->GetValue(75.));
  func->AddPoint(50., 2.);

  func->RemovePoint(50.);
  TEST_ASSERT_FUZZY_EQUAL(0., func->GetValue(25.));

  return EXIT_SUCCESS;
}
