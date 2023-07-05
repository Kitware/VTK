// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPiecewiseFunction.h"

#include "vtkNew.h"

#include <cmath>
#include <vector>

#define TEST_ASSERT_FUZZY_EQUAL(expect, actual)                                                    \
  do                                                                                               \
  {                                                                                                \
    if (std::fabs((expect) - (actual)) >= 1e-12)                                                   \
    {                                                                                              \
      std::cerr << "Error at line " << __LINE__ << ": Expected value " << (expect) << ", got "     \
                << (actual) << std::endl;                                                          \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int BasicTestPiecewiseFunction()
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

//----------------------------------------------------------------------------
// Test the interpolation search method
int TestInterpolationSearchMethod()
{
  // we will create two piecewise functions,
  // one with a linear distributed sampling
  // one with a non linear sampling
  // number of points in the piecewise function
  const int Ndata = 15000;

  // number of interpolations asked to the piecewise function
  const int N = 5 * Ndata;

  std::vector<double> linearInterlacedData(2 * Ndata, 0);
  std::vector<double> nonLinearInterlacedData(2 * Ndata, 0);

  vtkNew<vtkPiecewiseFunction> lin;
  vtkNew<vtkPiecewiseFunction> nonLin;

  for (int i = 0; i < Ndata; i++)
  {
    double time = 1.0 + static_cast<double>(2 * i) / static_cast<double>(Ndata) * (3.0 - 1.0);
    double value = std::exp(time);

    // linearly distributed samples
    linearInterlacedData[2 * i] = time;
    linearInterlacedData[2 * i + 1] = value;

    // non linearly distributed samples
    nonLinearInterlacedData[2 * i] = 0.54 * time * time + 1.89 * time;
    nonLinearInterlacedData[2 * i + 1] = value;
  }

  lin->FillFromDataPointer(Ndata, linearInterlacedData.data());
  nonLin->FillFromDataPointer(Ndata, nonLinearInterlacedData.data());

  // Test first if the update search method is pertinent
  lin->UpdateSearchMethod();
  nonLin->UpdateSearchMethod();

  int method = lin->GetAutomaticSearchMethod();
  if (method != static_cast<int>(vtkPiecewiseFunction::INTERPOLATION_SEARCH))
  {
    std::cerr << "Error at line " << __LINE__ << ": interpolation search method expected"
              << std::endl;
    return EXIT_FAILURE;
  }

  method = nonLin->GetAutomaticSearchMethod();
  if (method != static_cast<int>(vtkPiecewiseFunction::BINARY_SEARCH))
  {
    std::cerr << "Error at line " << __LINE__ << ": binary search method expected" << std::endl;
    return EXIT_FAILURE;
  }

  // Then compare the two methods with linear and non linear samples
  lin->SetUseCustomSearchMethod(true);
  lin->SetCustomSearchMethod(vtkPiecewiseFunction::BINARY_SEARCH);
  nonLin->SetUseCustomSearchMethod(true);
  nonLin->SetCustomSearchMethod(vtkPiecewiseFunction::BINARY_SEARCH);

  std::vector<double> linearTableBinary(N, 0);
  std::vector<double> nonLinearTableBinary(N, 0);
  std::vector<double> linearTableInterpolation(N, 0);
  std::vector<double> nonLinearTableInterpolation(N, 0);

  lin->GetTable(0, 8, N, linearTableBinary.data());
  nonLin->GetTable(0, 8, N, nonLinearTableBinary.data());

  lin->SetCustomSearchMethod(vtkPiecewiseFunction::INTERPOLATION_SEARCH);
  nonLin->SetCustomSearchMethod(vtkPiecewiseFunction::INTERPOLATION_SEARCH);

  lin->GetTable(0, 8, N, linearTableInterpolation.data());
  nonLin->GetTable(0, 8, N, nonLinearTableInterpolation.data());

  for (int k = 0; k < N; ++k)
  {
    TEST_ASSERT_FUZZY_EQUAL(linearTableBinary[k], linearTableInterpolation[k]);
    TEST_ASSERT_FUZZY_EQUAL(nonLinearTableBinary[k], nonLinearTableInterpolation[k]);
  }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
// Test if the interpolation is correct on a few data points
int TestGetTable()
{
  // number of points in the piecewise function
  const int Ndata = 6;
  // number of interpolations asked to the piecewise function
  const int N = 9;

  // expected result
  const double res[9] = { 2.718281828459, 2.718281828459, 3.684359911713, 5.133477036594,
    7.803374275898, 10.923088402692, 16.42055751499, 23.654739499744, 34.362347229412 };

  std::vector<double> interlacedData(2 * Ndata, 0);
  std::vector<double> table(N, 0);
  vtkNew<vtkPiecewiseFunction> f1;

  for (int i = 0; i < Ndata; i++)
  {
    double time = 1.0 + static_cast<double>(2 * i) / Ndata * (3 - 1);
    double value = std::exp(time);
    interlacedData[2 * i] = time;
    interlacedData[2 * i + 1] = value;
  }

  f1->FillFromDataPointer(Ndata, interlacedData.data());
  f1->GetTable(0.5, 3.5, N, table.data());

  for (int k = 0; k < N; ++k)
  {
    TEST_ASSERT_FUZZY_EQUAL(res[k], table[k]);
  }

  return EXIT_SUCCESS;
}

int TestPiecewiseFunction(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  retVal += BasicTestPiecewiseFunction();
  retVal += TestInterpolationSearchMethod();
  retVal += TestGetTable();

  return retVal;
}
