// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTestConditionals_txx
#define vtkTestConditionals_txx

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN

/**\brief Conditionals for use in unit tests.
 *
 * These templated functions all return boolean values: true
 * when the test passes and false when the test fails.
 *
 * Each prints out a message string you pass to it followed
 * either by "ok" or "failed".
 * If the string is empty, no message is printed.
 */
//@{
/**\brief Return true when \a a and \a b are equal; false otherwise.
 *
 * This tests for exact equality, so it is generally unwise to
 * use this for floating point number comparison.
 */
template <typename T>
inline bool testEqual(const T& a, const T& b, const std::string& msg)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  if (a == b)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: " << a << " != " << b << "\n";
  return false;
}

template <typename T>
inline bool testNotEqual(const T& a, const T& b, const std::string& msg)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  if (a != b)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: " << a << " == " << b << "\n";
  return false;
}

template <typename T>
inline bool testNotNull(const T& a, const std::string& msg)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  if (a != nullptr)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: NULL pointer\n";
  return false;
}

/**\brief Test two values for equality to within a tolerance.
 *
 * This test is useful for floating-point values.
 * It also has specializations for vtkVector2d and vtkVector3d that
 * make it useful for comparing points or vectors.
 *
 * It can be used for any type where a subtraction operator
 * exists whose return value is implicitly castable to a
 * double-precision number.
 *
 * The threshold (whose default is 1e-8) is an absolute number;
 * no normalization relative to the input values \a a and \a b
 * is done.
 */
template <typename T>
inline bool testNearlyEqual(const T& a, const T& b, const std::string& msg, double thresh = 1e-8)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  double diff = a - b;
  if (std::abs(diff) < thresh)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: abs(" << a << " - " << b << " = " << diff << ") > " << thresh << "\n";
  return false;
}

template <>
inline bool testNearlyEqual(
  const vtkVector2d& a, const vtkVector2d& b, const std::string& msg, double thresh)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  double diff = (a - b).Norm();
  if (std::abs(diff) < thresh)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: abs(" << a << " - " << b << " = " << diff << ") > " << thresh << "\n";
  return false;
}

template <>
inline bool testNearlyEqual(
  const vtkVector3d& a, const vtkVector3d& b, const std::string& msg, double thresh)
{
  bool print = !msg.empty();
  if (print)
  {
    std::cout << msg << ": ";
  }
  double diff = (a - b).Norm();
  if (std::abs(diff) < thresh)
  {
    if (print)
    {
      std::cout << "ok\n";
    }
    return true;
  }
  if (print)
  {
    std::cout << "failed\n";
  }
  std::cerr << "  ERROR: abs(" << a << " - " << b << " = " << diff << ") > " << thresh << "\n";
  return false;
}

VTK_ABI_NAMESPACE_END
} // namespace vtk

#endif // vtkTestConditionals_txx
