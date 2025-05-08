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
#ifndef viskores_NewtonsMethod_h
#define viskores_NewtonsMethod_h

#include <viskores/Math.h>
#include <viskores/Matrix.h>

namespace viskores
{

/// An object returned from `NewtonsMethod()` that contains the result and
/// other information about the final state.
template <typename ScalarType, viskores::IdComponent Size>
struct NewtonsMethodResult
{
  /// True if Newton's method ran into a singularity.
  bool Valid;
  /// True if Newton's method converted to below the convergence value.
  bool Converged;
  /// The solution found by Newton's method. If `Converged` is false,
  /// then this value is likely inaccurate. If `Valid` is false, then
  /// this value is undefined.
  viskores::Vec<ScalarType, Size> Solution;
};

/// Uses Newton's method (a.k.a. Newton-Raphson method) to solve a nonlinear
/// system of equations. This function assumes that the number of variables
/// equals the number of equations. Newton's method operates on an iterative
/// evaluate and search. Evaluations are performed using the functors passed
/// into the NewtonsMethod. The first functor returns the NxN matrix of the
/// Jacobian at a given input point. The second functor returns the N tuple
/// that is the function evaluation at the given input point. The input point
/// that evaluates to the desired output, or the closest point found, is
/// returned.
///
/// @param[in] jacobianEvaluator A functor whose operation takes a `viskores::Vec`
///            and returns a `viskores::Matrix` containing the math function's
///            Jacobian vector at that point.
/// @param[in] functionEvaluator A functor whose operation takes a `viskores::Vec`
///            and returns the evaluation of the math function at that point as
///            another `viskores::Vec`.
/// @param[in] desiredFunctionOutput The desired output of the function.
/// @param[in] initialGuess The initial guess to search from. If not specified,
///            the origin is used.
/// @param[in] convergeDifference The convergence distance. If the iterative method
///            changes all values less than this amount. Once all values change less,
///            it considers the solution found. If not specified, set to 0.001.
/// @param[in] maxIterations The maximum amount of iterations to run before giving up and
///            returning the best solution found. If not specified, set to 10.
///
/// @returns A `viskores::NewtonsMethodResult` containing the best found result and state
/// about its validity.
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename ScalarType,
          viskores::IdComponent Size,
          typename JacobianFunctor,
          typename FunctionFunctor>
VISKORES_EXEC_CONT NewtonsMethodResult<ScalarType, Size> NewtonsMethod(
  JacobianFunctor jacobianEvaluator,
  FunctionFunctor functionEvaluator,
  viskores::Vec<ScalarType, Size> desiredFunctionOutput,
  viskores::Vec<ScalarType, Size> initialGuess = viskores::Vec<ScalarType, Size>(ScalarType(0)),
  ScalarType convergeDifference = ScalarType(1e-3),
  viskores::IdComponent maxIterations = 10)
{
  using VectorType = viskores::Vec<ScalarType, Size>;
  using MatrixType = viskores::Matrix<ScalarType, Size, Size>;

  VectorType x = initialGuess;

  bool valid = false;
  bool converged = false;
  for (viskores::IdComponent iteration = 0; !converged && (iteration < maxIterations); iteration++)
  {
    // For Newton's method, we solve the linear system
    //
    // Jacobian x deltaX = currentFunctionOutput - desiredFunctionOutput
    //
    // The subtraction on the right side simply makes the target of the solve
    // at zero, which is what Newton's method solves for. The deltaX tells us
    // where to move to to solve for a linear system, which we assume will be
    // closer for our nonlinear system.

    MatrixType jacobian = jacobianEvaluator(x);
    VectorType currentFunctionOutput = functionEvaluator(x);

    VectorType deltaX =
      viskores::SolveLinearSystem(jacobian, currentFunctionOutput - desiredFunctionOutput, valid);
    if (!valid)
    {
      break;
    }

    x = x - deltaX;

    converged = true;
    for (viskores::IdComponent index = 0; index < Size; index++)
    {
      converged &= (viskores::Abs(deltaX[index]) < convergeDifference);
    }
  }

  // Not checking whether converged.
  return { valid, converged, x };
}

} // namespace viskores

#endif //viskores_NewtonsMethod_h
