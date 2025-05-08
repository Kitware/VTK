//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/NewtonsMethod.h>

#include <viskores/testing/Testing.h>

namespace
{

// We will test Newton's method with the following three functions:
//
// f1(x,y,z) = x^2 + y^2 + z^2
// f2(x,y,z) = 2x - y + z
// f3(x,y,z) = x + y - z
//
// If we want the result of all three equations to be 1, then there are two
// valid solutions: (2/3, -1/3, -2/3) and (2/3, 2/3, 1/3).
template <typename T>
struct EvaluateFunctions
{
  using Vector3 = viskores::Vec<T, 3>;

  VISKORES_EXEC_CONT
  Vector3 operator()(Vector3 x) const
  {
    Vector3 fx;
    fx[0] = x[0] * x[0] + x[1] * x[1] + x[2] * x[2];
    fx[1] = 2 * x[0] - x[1] + x[2];
    fx[2] = x[0] + x[1] - x[2];
    return fx;
  }
};
template <typename T>
struct EvaluateJacobian
{
  using Vector3 = viskores::Vec<T, 3>;
  using Matrix3x3 = viskores::Matrix<T, 3, 3>;

  VISKORES_EXEC_CONT
  Matrix3x3 operator()(Vector3 x) const
  {
    Matrix3x3 jacobian;
    jacobian(0, 0) = 2 * x[0];
    jacobian(0, 1) = 2 * x[1];
    jacobian(0, 2) = 2 * x[2];
    jacobian(1, 0) = 2;
    jacobian(1, 1) = -1;
    jacobian(1, 2) = 1;
    jacobian(2, 0) = 1;
    jacobian(2, 1) = 1;
    jacobian(2, 2) = -1;
    return jacobian;
  }
};

template <typename T>
void TestNewtonsMethodTemplate()
{
  std::cout << "Testing Newton's Method." << std::endl;

  using Vector3 = viskores::Vec<T, 3>;

  Vector3 desiredOutput(1, 1, 1);
  Vector3 expected1(2.0f / 3.0f, -1.0f / 3.0f, -2.0f / 3.0f);
  Vector3 expected2(2.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f);

  Vector3 initialGuess;
  for (initialGuess[0] = 0.25f; initialGuess[0] <= 1; initialGuess[0] += 0.25f)
  {
    for (initialGuess[1] = 0.25f; initialGuess[1] <= 1; initialGuess[1] += 0.25f)
    {
      for (initialGuess[2] = 0.25f; initialGuess[2] <= 1; initialGuess[2] += 0.25f)
      {
        std::cout << "   " << initialGuess << std::endl;

        auto result = viskores::NewtonsMethod(
          EvaluateJacobian<T>(), EvaluateFunctions<T>(), desiredOutput, initialGuess, T(1e-6));

        VISKORES_TEST_ASSERT(test_equal(result.Solution, expected1) ||
                               test_equal(result.Solution, expected2),
                             "Newton's method did not converge to expected result.");
      }
    }
  }
}

void TestNewtonsMethod()
{
  std::cout << "*** Float32 *************************" << std::endl;
  TestNewtonsMethodTemplate<viskores::Float32>();
  std::cout << "*** Float64 *************************" << std::endl;
  TestNewtonsMethodTemplate<viskores::Float64>();
}

} // anonymous namespace

int UnitTestNewtonsMethod(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestNewtonsMethod, argc, argv);
}
