// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkMath
 * @brief   performs common math operations
 *
 * vtkMath provides methods to perform common math operations. These
 * include providing constants such as Pi; conversion from degrees to
 * radians; vector operations such as dot and cross products and vector
 * norm; matrix determinant for 2x2 and 3x3 matrices; univariate polynomial
 * solvers; and for random number generation (for backward compatibility only).
 * @sa
 * vtkMinimalStandardRandomSequence, vtkBoxMuellerRandomSequence,
 * vtkQuaternion
 */

#ifndef vtkMath_h
#define vtkMath_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkMathPrivate.hxx"    // For Matrix meta-class helpers
#include "vtkMatrixUtilities.h"  // For Matrix wrapping / mapping
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer.
#include "vtkTypeTraits.h"   // For type traits

#include "vtkMathConfigure.h" // For <cmath> and VTK_HAS_ISNAN etc.

#include <algorithm>   // for std::clamp
#include <cassert>     // assert() in inline implementations.
#include <type_traits> // for type_traits

#ifndef DBL_MIN
#define VTK_DBL_MIN 2.2250738585072014e-308
#else // DBL_MIN
#define VTK_DBL_MIN DBL_MIN
#endif // DBL_MIN

#ifndef DBL_EPSILON
#define VTK_DBL_EPSILON 2.2204460492503131e-16
#else // DBL_EPSILON
#define VTK_DBL_EPSILON DBL_EPSILON
#endif // DBL_EPSILON

#ifndef VTK_DBL_EPSILON
#ifndef DBL_EPSILON
#define VTK_DBL_EPSILON 2.2204460492503131e-16
#else // DBL_EPSILON
#define VTK_DBL_EPSILON DBL_EPSILON
#endif // DBL_EPSILON
#endif // VTK_DBL_EPSILON

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkPoints;
class vtkMathInternal;
class vtkMinimalStandardRandomSequence;
class vtkBoxMuellerRandomSequence;
VTK_ABI_NAMESPACE_END

namespace vtk_detail
{
VTK_ABI_NAMESPACE_BEGIN
// forward declaration
template <typename OutT>
void RoundDoubleToIntegralIfNecessary(double val, OutT* ret);
VTK_ABI_NAMESPACE_END
} // end namespace vtk_detail

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkMath : public vtkObject
{
public:
  static vtkMath* New();
  vtkTypeMacro(vtkMath, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

private:
  template <class VectorT, class = void>
  struct VectorImplementsSize : std::false_type
  {
  };

  template <class VectorT>
  struct VectorImplementsSize<VectorT, decltype((void)std::declval<VectorT>().size(), void())>
    : std::true_type
  {
  };

  /**
   * Convenient meta-class to enable function signatures if `VectorT::size()` exists;
   */
  template <class VectorT>
  using EnableIfVectorImplementsSize =
    typename std::enable_if<VectorImplementsSize<VectorT>::value>::type;

public:
  /**
   * When this value is passed to a select templated functions in `vtkMath`,
   * the computation can be performed on dynamic sized arrays as long as they implement the method
   * `size()`. One can also pass this constant to `vtkDataArrayTupleRange` to get dynamic sized
   * tuples.
   *
   * @sa vtkDataArrayRange
   */
  static constexpr int DYNAMIC_VECTOR_SIZE() { return 0; }

  /**
   * A mathematical constant. This version is atan(1.0) * 4.0
   */
  static constexpr double Pi() { return 3.141592653589793; }

  ///@{
  /**
   * Convert degrees into radians
   */
  static float RadiansFromDegrees(float degrees);
  static double RadiansFromDegrees(double degrees);
  ///@}

  ///@{
  /**
   * Convert radians into degrees
   */
  static float DegreesFromRadians(float radians);
  static double DegreesFromRadians(double radians);
  ///@}

  /**
   * Rounds a float to the nearest integer.
   */
#if 1
  static int Round(float f) { return static_cast<int>(f + (f >= 0.0 ? 0.5 : -0.5)); }
  static int Round(double f) { return static_cast<int>(f + (f >= 0.0 ? 0.5 : -0.5)); }
#endif

  /**
   * Round a double to type OutT if OutT is integral, otherwise simply clamp
   * the value to the output range.
   */
  template <typename OutT>
  static void RoundDoubleToIntegralIfNecessary(double val, OutT* ret)
  {
    // Can't specialize template methods in a template class, so we move the
    // implementations to a external namespace.
    vtk_detail::RoundDoubleToIntegralIfNecessary(val, ret);
  }

  /**
   * Rounds a double to the nearest integer not greater than itself.
   * This is faster than floor() but provides undefined output on
   * overflow.
   */
  static int Floor(double x);

  /**
   * Rounds a double to the nearest integer not less than itself.
   * This is faster than ceil() but provides undefined output on
   * overflow.
   */
  static int Ceil(double x);

  /**
   * Gives the exponent of the lowest power of two not less than x.
   * Or in mathspeak, return the smallest "i" for which 2^i >= x.
   * If x is zero, then the return value will be zero.
   */
  static int CeilLog2(vtkTypeUInt64 x);

  /**
   * Returns the minimum of the two arguments provided. If either
   * argument is NaN, the first argument will always be returned.
   */
  template <class T>
  static T Min(const T& a, const T& b);

  /**
   * Returns the maximum of the two arguments provided. If either
   * argument is NaN, the first argument will always be returned.
   */
  template <class T>
  static T Max(const T& a, const T& b);

  /**
   * Returns true if integer is a power of two.
   */
  static bool IsPowerOfTwo(vtkTypeUInt64 x);

  /**
   * Compute the nearest power of two that is not less than x.
   * The return value is 1 if x is less than or equal to zero,
   * and is VTK_INT_MIN if result is too large to fit in an int.
   */
  static int NearestPowerOfTwo(int x);

  /**
   * Compute N factorial, N! = N*(N-1) * (N-2)...*3*2*1.
   * 0! is taken to be 1.
   */
  static vtkTypeInt64 Factorial(int N);

  /**
   * The number of combinations of n objects from a pool of m objects (m>n).
   * This is commonly known as "m choose n" and sometimes denoted \f$_mC_n\f$
   * or \f$\left(\begin{array}{c}m \\ n\end{array}\right)\f$.
   */
  static vtkTypeInt64 Binomial(int m, int n);

  /**
   * Start iterating over "m choose n" objects.
   * This function returns an array of n integers, each from 0 to m-1.
   * These integers represent the n items chosen from the set [0,m[.

   * You are responsible for calling vtkMath::FreeCombination() once the iterator is no longer
   needed.

   * Warning: this gets large very quickly, especially when n nears m/2!
   * (Hint: think of Pascal's triangle.)
   */
  static int* BeginCombination(int m, int n);

  /**
   * Given \a m, \a n, and a valid \a combination of \a n integers in
   * the range [0,m[, this function alters the integers into the next
   * combination in a sequence of all combinations of \a n items from
   * a pool of \a m.

   * If the \a combination is the last item in the sequence on input,
   * then \a combination is unaltered and 0 is returned.
   * Otherwise, 1 is returned and \a combination is updated.
   */
  static int NextCombination(int m, int n, int* combination);

  /**
   * Free the "iterator" array created by vtkMath::BeginCombination.
   */
  static void FreeCombination(int* combination);

  /**
   * Initialize seed value. NOTE: Random() has the bad property that
   * the first random number returned after RandomSeed() is called
   * is proportional to the seed value! To help solve this, call
   * RandomSeed() a few times inside seed. This doesn't ruin the
   * repeatability of Random().

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static void RandomSeed(int s);

  /**
   * Return the current seed used by the random number generator.

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static int GetSeed();

  /**
   * Generate pseudo-random numbers distributed according to the uniform
   * distribution between 0.0 and 1.0.
   * This is used to provide portability across different systems.

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static double Random();

  /**
   * Generate pseudo-random numbers distributed according to the uniform
   * distribution between \a min and \a max.

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static double Random(double min, double max);

  /**
   * Generate pseudo-random numbers distributed according to the standard
   * normal distribution.

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static double Gaussian();

  /**
   * Generate pseudo-random numbers distributed according to the Gaussian
   * distribution with mean \a mean and standard deviation \a std.

   * DON'T USE Random(), RandomSeed(), GetSeed(), Gaussian()
   * THIS IS STATIC SO THIS IS PRONE TO ERRORS (SPECIALLY FOR REGRESSION TESTS)
   * THIS IS HERE FOR BACKWARD COMPATIBILITY ONLY.
   * Instead, for a sequence of random numbers with a uniform distribution
   * create a vtkMinimalStandardRandomSequence object.
   * For a sequence of random numbers with a gaussian/normal distribution
   * create a vtkBoxMuellerRandomSequence object.
   */
  static double Gaussian(double mean, double std);

  /**
   * Assign values to a 3-vector (templated version). Result is stored in b according to b = a.
   * Each parameter must implement operator[].
   */
  template <class VectorT1, class VectorT2>
  static void Assign(const VectorT1& a, VectorT2&& b)
  {
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
  }

  /**
   * Assign values to a 3-vector (double version). Result is stored in b according to b = a.
   */
  static void Assign(const double a[3], double b[3]) { vtkMath::Assign<>(a, b); }

  /**
   * Addition of two 3-vectors (float version). Result is stored in c according to c = a + b.
   */
  static void Add(const float a[3], const float b[3], float c[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] + b[i];
    }
  }

  /**
   * Addition of two 3-vectors (double version). Result is stored in c according to c = a + b.
   */
  static void Add(const double a[3], const double b[3], double c[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] + b[i];
    }
  }

  /**
   * Addition of two 3-vectors (double version). Result is stored in c according to c = a + b.
   *
   * Each parameter needs to implement `operator[]`
   */
  template <class VectorT1, class VectorT2, class VectorT3>
  static void Add(VectorT1&& a, VectorT2&& b, VectorT3& c)
  {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] + b[i];
    }
  }

  /**
   * Subtraction of two 3-vectors (float version). Result is stored in c according to c = a - b.
   */
  static void Subtract(const float a[3], const float b[3], float c[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] - b[i];
    }
  }

  /**
   * Subtraction of two 3-vectors (double version). Result is stored in c according to c = a - b.
   */
  static void Subtract(const double a[3], const double b[3], double c[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] - b[i];
    }
  }

  /**
   * Subtraction of two 3-vectors (templated version). Result is stored in c according to c = a - b.
   *
   * Each parameter needs to implement `operator[]`.
   */
  template <class VectorT1, class VectorT2, class VectorT3>
  static void Subtract(const VectorT1& a, const VectorT2& b, VectorT3&& c)
  {
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
  }

  /**
   * Multiplies a 3-vector by a scalar (float version).
   * This modifies the input 3-vector.
   */
  static void MultiplyScalar(float a[3], float s)
  {
    for (int i = 0; i < 3; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 2-vector by a scalar (float version).
   * This modifies the input 2-vector.
   */
  static void MultiplyScalar2D(float a[2], float s)
  {
    for (int i = 0; i < 2; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 3-vector by a scalar (double version).
   * This modifies the input 3-vector.
   */
  static void MultiplyScalar(double a[3], double s)
  {
    for (int i = 0; i < 3; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 2-vector by a scalar (double version).
   * This modifies the input 2-vector.
   */
  static void MultiplyScalar2D(double a[2], double s)
  {
    for (int i = 0; i < 2; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Dot product of two 3-vectors (float version).
   */
  static float Dot(const float a[3], const float b[3])
  {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * Dot product of two 3-vectors (double version).
   */
  static double Dot(const double a[3], const double b[3])
  {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * Compute dot product between two points p1 and p2.
   * This version allows for custom range and iterator types to be used. These
   * types must implement `operator[]`, and at least one of them must have
   * a `value_type` typedef.
   *
   * The first template parameter `ReturnTypeT` sets the return type of this method.
   * By default, it is set to `double`, but it can be overridden.
   *
   * The `EnableT` template parameter is used to make sure that this version
   * doesn't capture the `float*`, `float[]`, `double*`, `double[]` as
   * those should go to the other `Dot` functions.
   *
   * @warning This method assumes that both parameters have 3 components.
   */
  template <typename ReturnTypeT = double, typename TupleRangeT1, typename TupleRangeT2,
    typename EnableT = typename std::conditional<!std::is_pointer<TupleRangeT1>::value &&
        !std::is_array<TupleRangeT1>::value,
      TupleRangeT1, TupleRangeT2>::type::value_type>
  static ReturnTypeT Dot(const TupleRangeT1& a, const TupleRangeT2& b)
  {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * Outer product of two 3-vectors (float version).
   */
  static void Outer(const float a[3], const float b[3], float c[3][3])
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        c[i][j] = a[i] * b[j];
      }
    }
  }

  /**
   * Outer product of two 3-vectors (double version).
   */
  static void Outer(const double a[3], const double b[3], double c[3][3])
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        c[i][j] = a[i] * b[j];
      }
    }
  }

  /**
   * Cross product of two 3-vectors. Result (a x b) is stored in c.
   *
   * Input vectors need to implement operator[]
   */
  template <class VectorT1, class VectorT2, class VectorT3>
  static void Cross(VectorT1&& a, VectorT2&& b, VectorT3& c);

  /**
   * Cross product of two 3-vectors. Result (a x b) is stored in c.
   * (float version)
   */
  static void Cross(const float a[3], const float b[3], float c[3]);

  /**
   * Cross product of two 3-vectors. Result (a x b) is stored in c.
   * (double version)
   */
  static void Cross(const double a[3], const double b[3], double c[3]);

  ///@{
  /**
   * Compute the norm of n-vector. x is the vector, n is its length.
   */
  static float Norm(const float* x, int n);
  static double Norm(const double* x, int n);
  ///@}

  /**
   * Compute the norm of 3-vector (float version).
   */
  static float Norm(const float v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

  /**
   * Compute the norm of 3-vector (double version).
   */
  static double Norm(const double v[3])
  {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  }

  /**
   * Compute the squared norm of a 3-vector.
   *
   * The first template parameter `ReturnTypeT` sets the return type of this method.
   * By default, it is set to `double`, but it can be overridden.
   *
   * The parameter of this function must be a container, and array, or a range of 3
   * components implementing `operator[]`.
   */
  template <typename ReturnTypeT = double, typename TupleRangeT>
  static ReturnTypeT SquaredNorm(const TupleRangeT& v)
  {
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  }

  /**
   * Normalize (in place) a 3-vector. Returns norm of vector.
   * (float version)
   */
  static float Normalize(float v[3]);

  /**
   * Normalize (in place) a 3-vector. Returns norm of vector
   * (double version).
   */
  static double Normalize(double v[3]);

  ///@{
  /**
   * Given a unit vector v1, find two unit vectors v2 and v3 such that
   * v1 cross v2 = v3 (i.e. the vectors are perpendicular to each other).
   * There is an infinite number of such vectors, specify an angle theta
   * to choose one set.  If you want only one perpendicular vector,
   * specify nullptr for v3.
   */
  static void Perpendiculars(const double v1[3], double v2[3], double v3[3], double theta);
  static void Perpendiculars(const float v1[3], float v2[3], float v3[3], double theta);
  ///@}

  ///@{
  /**
   * Compute the projection of vector a on vector b and return it in projection[3].
   * If b is a zero vector, the function returns false and 'projection' is invalid.
   * Otherwise, it returns true.
   */
  static bool ProjectVector(const float a[3], const float b[3], float projection[3]);
  static bool ProjectVector(const double a[3], const double b[3], double projection[3]);
  ///@}

  ///@{
  /**
   * Compute the projection of 2D vector a on 2D vector b and returns the result
   * in projection[2].
   * If b is a zero vector, the function returns false and 'projection' is invalid.
   * Otherwise, it returns true.
   */
  static bool ProjectVector2D(const float a[2], const float b[2], float projection[2]);
  static bool ProjectVector2D(const double a[2], const double b[2], double projection[2]);
  ///@}

  /**
   * Compute distance squared between two points p1 and p2.
   * This version allows for custom range and iterator types to be used. These
   * types must implement `operator[]`, and at least one of them must have
   * a `value_type` typedef.
   *
   * The first template parameter `ReturnTypeT` sets the return type of this method.
   * By default, it is set to `double`, but it can be overridden.
   *
   * The `EnableT` template parameter is used to make sure that this version
   * doesn't capture the `float*`, `float[]`, `double*`, `double[]` as
   * those should go to the other `Distance2BetweenPoints` functions.
   *
   * @warning This method assumes that both parameters have 3 components.
   */
  template <typename ReturnTypeT = double, typename TupleRangeT1, typename TupleRangeT2,
    typename EnableT = typename std::conditional<!std::is_pointer<TupleRangeT1>::value &&
        !std::is_array<TupleRangeT1>::value,
      TupleRangeT1, TupleRangeT2>::type::value_type>
  static ReturnTypeT Distance2BetweenPoints(const TupleRangeT1& p1, const TupleRangeT2& p2);

  /**
   * Compute distance squared between two points p1 and p2.
   * (float version).
   */
  static float Distance2BetweenPoints(const float p1[3], const float p2[3]);

  /**
   * Compute distance squared between two points p1 and p2.
   * (double version).
   */
  static double Distance2BetweenPoints(const double p1[3], const double p2[3]);

  /**
   * Compute angle in radians between two vectors.
   */
  static double AngleBetweenVectors(const double v1[3], const double v2[3]);

  /**
   * Compute signed angle in radians between two vectors with regard to a third orthogonal vector
   */
  static double SignedAngleBetweenVectors(
    const double v1[3], const double v2[3], const double vn[3]);

  /**
   * Compute the amplitude of a Gaussian function with mean=0 and specified variance.
   * That is, 1./(std::sqrt(2 Pi * variance)) * exp(-distanceFromMean^2/(2.*variance)).
   */
  static double GaussianAmplitude(double variance, double distanceFromMean);

  /**
   * Compute the amplitude of a Gaussian function with specified mean and variance.
   * That is, 1./(std::sqrt(2 Pi * variance)) * exp(-(position - mean)^2/(2.*variance)).
   */
  static double GaussianAmplitude(double mean, double variance, double position);

  /**
   * Compute the amplitude of an unnormalized Gaussian function with mean=0 and specified variance.
   * That is, exp(-distanceFromMean^2/(2.*variance)). When distanceFromMean = 0, this function
   * returns 1.
   */
  static double GaussianWeight(double variance, double distanceFromMean);

  /**
   * Compute the amplitude of an unnormalized Gaussian function with specified mean and variance.
   * That is, exp(-(position - mean)^2/(2.*variance)). When the distance from 'position' to 'mean'
   * is 0, this function returns 1.
   */
  static double GaussianWeight(double mean, double variance, double position);

  /**
   * Dot product of two 2-vectors. (float version).
   */
  static float Dot2D(const float x[2], const float y[2]) { return x[0] * y[0] + x[1] * y[1]; }

  /**
   * Dot product of two 2-vectors. (double version).
   */
  static double Dot2D(const double x[2], const double y[2]) { return x[0] * y[0] + x[1] * y[1]; }

  /**
   * Outer product of two 2-vectors (float version).
   */
  static void Outer2D(const float x[2], const float y[2], float A[2][2])
  {
    for (int i = 0; i < 2; ++i)
    {
      for (int j = 0; j < 2; ++j)
      {
        A[i][j] = x[i] * y[j];
      }
    }
  }

  /**
   * Outer product of two 2-vectors (double version).
   */
  static void Outer2D(const double x[2], const double y[2], double A[2][2])
  {
    for (int i = 0; i < 2; ++i)
    {
      for (int j = 0; j < 2; ++j)
      {
        A[i][j] = x[i] * y[j];
      }
    }
  }

  /**
   * Compute the norm of a 2-vector.
   * (float version).
   */
  static float Norm2D(const float x[2]) { return std::sqrt(x[0] * x[0] + x[1] * x[1]); }

  /**
   * Compute the norm of a 2-vector.
   * (double version).
   */
  static double Norm2D(const double x[2]) { return std::sqrt(x[0] * x[0] + x[1] * x[1]); }

  /**
   * Normalize (in place) a 2-vector. Returns norm of vector.
   * (float version).
   */
  static float Normalize2D(float v[2]);

  /**
   * Normalize (in place) a 2-vector. Returns norm of vector.
   * (double version).
   */
  static double Normalize2D(double v[2]);

  /**
   * Compute determinant of 2x2 matrix. Two columns of matrix are input.
   */
  static float Determinant2x2(const float c1[2], const float c2[2])
  {
    return c1[0] * c2[1] - c2[0] * c1[1];
  }

  ///@{
  /**
   * Calculate the determinant of a 2x2 matrix: | a b | | c d |
   */
  static double Determinant2x2(double a, double b, double c, double d) { return a * d - b * c; }
  static double Determinant2x2(const double c1[2], const double c2[2])
  {
    return c1[0] * c2[1] - c2[0] * c1[1];
  }
  ///@}

  ///@{
  /**
   * LU Factorization of a 3x3 matrix.
   */
  static void LUFactor3x3(float A[3][3], int index[3]);
  static void LUFactor3x3(double A[3][3], int index[3]);
  ///@}

  ///@{
  /**
   * LU back substitution for a 3x3 matrix.
   */
  static void LUSolve3x3(const float A[3][3], const int index[3], float x[3]);
  static void LUSolve3x3(const double A[3][3], const int index[3], double x[3]);
  ///@}

  ///@{
  /**
   * Solve Ay = x for y and place the result in y.  The matrix A is
   * destroyed in the process.
   */
  static void LinearSolve3x3(const float A[3][3], const float x[3], float y[3]);
  static void LinearSolve3x3(const double A[3][3], const double x[3], double y[3]);
  ///@}

  ///@{
  /**
   * Multiply a vector by a 3x3 matrix.  The result is placed in out.
   */
  static void Multiply3x3(const float A[3][3], const float v[3], float u[3]);
  static void Multiply3x3(const double A[3][3], const double v[3], double u[3]);
  ///@}

  ///@{
  /**
   * Multiply one 3x3 matrix by another according to C = AB.
   */
  static void Multiply3x3(const float A[3][3], const float B[3][3], float C[3][3]);
  static void Multiply3x3(const double A[3][3], const double B[3][3], double C[3][3]);
  ///@}

  /**
   * Multiply matrices such that M3 = M1 x M2.
   * M3 must already be allocated.
   *
   * LayoutT1 (resp. LayoutT2) allow to perform basic matrix reindexing for M1 (resp. M2).
   * It should be set to a component of MatrixLayout.
   *
   * Matrices are assumed to be a 1D array implementing `operator[]`. The matrices
   * are indexed row by row. Let us develop the effect of LayoutT1 on M1 (the
   * same is true for LayoutT2 on M2).
   * If LayoutT1 == vtkMatrixUtilities::Layout::Identity (default), then M1 indexing is untouched.
   * If LayoutT1 == vtkMatrixUtilities::Layout::Transpose, then M1 is read column-wise.
   * If LayoutT1 == vtkMatrixUtilities::Layout::Diag, then M1 is considered to be composed
   * of non zero elements of a diagonal matrix.
   *
   * @note M3 components indexing can be entirely controlled by swapping M1, M2,
   * and turning on or off the appropriate transposition flag. Remember that
   * M3^T = (M1 x M2)^T = M2^T x M1^T
   *
   * @warning If both M1 and M2 are used with both layout being diagonal, then
   * M3 will be diagonal as well (i.e. there won't be allocated memory for
   * elements outsize of the diagonal).
   */
  template <int RowsT, int MidDimT, int ColsT,
    class LayoutT1 = vtkMatrixUtilities::Layout::Identity,
    class LayoutT2 = vtkMatrixUtilities::Layout::Identity, class MatrixT1, class MatrixT2,
    class MatrixT3>
  static void MultiplyMatrix(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    vtkMathPrivate::MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2>::Compute(
      std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2), std::forward<MatrixT3>(M3));
  }

  /**
   * Multiply matrix M with vector Y such that Y = M x X.
   * Y must be allocated.
   *
   * LayoutT allow to perform basic matrix reindexing. It should be set to a component
   * of MatrixLayout.
   *
   * Matrix M is assumed to be a 1D array implementing `operator[]`. The matrix
   * is indexed row by row. If the input array is indexed columns by columns.
   * If LayoutT == vtkMatrixUtilities::Layout::Identity (default), then M indexing is untouched.
   * If LayoutT == vtkMatrixUtilities::Layout::Transpose, then M is read column-wise.
   * If LayoutT == vtkMatrixUtilities::Layout::Diag, then M is considered to be composed
   * of non zero elements of a diagonal matrix.
   *
   * VectorT1 and VectorT2 are arrays of size RowsT, and must implement
   * `operator[]`.
   *
   * @warning In the particular case where M1 and M2 BOTH have layout
   * vtkMatrixUtilities::Layout::Diag, RowsT, MidDimT and ColsT MUST match.
   */
  template <int RowsT, int ColsT, class LayoutT = vtkMatrixUtilities::Layout::Identity,
    class MatrixT, class VectorT1, class VectorT2>
  static void MultiplyMatrixWithVector(MatrixT&& M, VectorT1&& X, VectorT2&& Y)
  {
    vtkMathPrivate::MultiplyMatrix<RowsT, ColsT, 1, LayoutT>::Compute(
      std::forward<MatrixT>(M), std::forward<VectorT1>(X), std::forward<VectorT2>(Y));
  }

  /**
   * Computes the dot product between 2 vectors x and y.
   * VectorT1 and VectorT2 are arrays of size SizeT, and must implement
   * `operator[]`.
   */
  template <class ScalarT, int SizeT, class VectorT1, class VectorT2,
    class = typename std::enable_if<SizeT != DYNAMIC_VECTOR_SIZE()>::type>
  static ScalarT Dot(VectorT1&& x, VectorT2&& y)
  {
    return vtkMathPrivate::ContractRowWithCol<ScalarT, 1, SizeT, 1, 0, 0,
      vtkMatrixUtilities::Layout::Identity,
      vtkMatrixUtilities::Layout::Transpose>::Compute(std::forward<VectorT1>(x),
      std::forward<VectorT2>(y));
  }

  /**
   * Computes the dot product between 2 vectors x and y.
   * This function is solely invoked when `SizeT == DYNAMIC_VECTOR_SIZE()`.
   * VectorT1 and VectorT2 are arrays of dynamic size, and must implement
   * `operator[]` and `size()`.
   */
  template <class ScalarT, int SizeT, class VectorT1, class VectorT2,
    class = typename std::enable_if<SizeT == DYNAMIC_VECTOR_SIZE()>::type,
    class = EnableIfVectorImplementsSize<VectorT1>>
  static ScalarT Dot(VectorT1&& x, VectorT2&& y)
  {
    ScalarT dot = 0.0;
    using SizeType = decltype(std::declval<VectorT1>().size());
    for (SizeType dim = 0; dim < x.size(); ++dim)
    {
      dot += x[dim] * y[dim];
    }
    return dot;
  }

  /**
   * Computes the dot product between 2 vectors x and y.
   * VectorT1 and VectorT2 are arrays of size SizeT, and must implement
   * `operator[]`.
   *
   * If `SizeT == DYNAMIC_VECTOR_SIZE()`, then x must implement the method `size()`.
   */
  template <int SizeT, class VectorT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT>::value_type SquaredNorm(
    VectorT&& x)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT>::value_type;
    return vtkMath::Dot<Scalar, SizeT>(std::forward<VectorT>(x), std::forward<VectorT>(x));
  }

  /**
   * Computes the determinant of input square SizeT x SizeT matrix M.
   * The return type is the same as the underlying scalar type of MatrixT.
   *
   * LayoutT allow to perform basic matrix reindexing. It should be set to a component
   * of MatrixLayout.
   *
   * Matrix M is assumed to be a 1D array implementing `operator[]`. The matrix
   * is indexed row by row. If the input array is indexed columns by columns.
   * If LayoutT == vtkMatrixUtilities::Layout::Identity (default), then M indexing is untouched.
   * If LayoutT == vtkMatrixUtilities::Layout::Transpose, then M is read column-wise.
   * If LayoutT == vtkMatrixUtilities::Layout::Diag, then M is considered to be composed
   * of non zero elements of a diagonal matrix.
   *
   * This method is currently implemented for SizeT lower or equal to 3.
   */
  template <int SizeT, class LayoutT = vtkMatrixUtilities::Layout::Identity, class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Determinant(
    MatrixT&& M)
  {
    return vtkMathPrivate::Determinant<SizeT, LayoutT>::Compute(std::forward<MatrixT>(M));
  }

  /**
   * Computes the inverse of input matrix M1 into M2.
   *
   * LayoutT allow to perform basic matrix reindexing of M1. It should be set to a component
   * of MatrixLayout.
   *
   * Matrix M is assumed to be a 1D array implementing `operator[]`. The matrix
   * is indexed row by row. If the input array is indexed columns by columns.
   * If LayoutT == vtkMatrixUtilities::Layout::Identity (default), then M indexing is untouched.
   * If LayoutT == vtkMatrixUtilities::Layout::Transpose, then M is read column-wise.
   * If LayoutT == vtkMatrixUtilities::Layout::Diag, then M is considered to be composed
   * of non zero elements of a diagonal matrix.
   *
   * This method is currently implemented for SizeT lower or equal to 3.
   */
  template <int SizeT, class LayoutT = vtkMatrixUtilities::Layout::Identity, class MatrixT1,
    class MatrixT2>
  static void InvertMatrix(MatrixT1&& M1, MatrixT2&& M2)
  {
    vtkMathPrivate::InvertMatrix<SizeT, LayoutT>::Compute(
      std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
  }

  /**
   * This method solves linear systems M * x = y.
   *
   * LayoutT allow to perform basic matrix reindexing of M1. It should be set to a component
   * of MatrixLayout.
   *
   * Matrix M is assumed to be a 1D array implementing `operator[]`. The matrix
   * is indexed row by row. If the input array is indexed columns by columns.
   * If LayoutT == vtkMatrixUtilities::Layout::Identity (default), then M indexing is untouched.
   * If LayoutT == vtkMatrixUtilities::Layout::Transpose, then M is read column-wise.
   * If LayoutT == vtkMatrixUtilities::Layout::Diag, then M is considered to be composed
   * of non zero elements of a diagonal matrix.
   */
  template <int RowsT, int ColsT, class LayoutT = vtkMatrixUtilities::Layout::Identity,
    class MatrixT, class VectorT1, class VectorT2>
  static void LinearSolve(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    vtkMathPrivate::LinearSolve<RowsT, ColsT, LayoutT>::Compute(
      std::forward<MatrixT>(M), std::forward<VectorT1>(x), std::forward<VectorT2>(y));
  }

  /**
   * Computes the dot product x^T M y, where x and y are vectors and M is a
   * metric matrix.
   *
   * VectorT1 and VectorT2 are arrays of size SizeT, and must implement
   * `operator[]`.
   *
   * Matrix M is assumed to be a 1D array implementing `operator[]`. The matrix
   * is indexed row by row. If the input array is indexed columns by columns.
   * If LayoutT == vtkMatrixUtilities::Layout::Identity (default), then M indexing is untouched.
   * If LayoutT == vtkMatrixUtilities::Layout::Transpose, then M is read column-wise.
   * If LayoutT == vtkMatrixUtilities::Layout::Diag, then M is considered to be composed
   * of non zero elements of a diagonal matrix.
   */
  template <class ScalarT, int SizeT, class LayoutT = vtkMatrixUtilities::Layout::Identity,
    class VectorT1, class MatrixT, class VectorT2,
    class = typename std::enable_if<SizeT != DYNAMIC_VECTOR_SIZE()>::type>
  static ScalarT Dot(VectorT1&& x, MatrixT&& M, VectorT2&& y)
  {
    ScalarT tmp[SizeT];
    vtkMathPrivate::MultiplyMatrix<SizeT, SizeT, 1, LayoutT>::Compute(
      std::forward<MatrixT>(M), std::forward<VectorT2>(y), tmp);
    return vtkMathPrivate::ContractRowWithCol<ScalarT, 1, SizeT, 1, 0, 0,
      vtkMatrixUtilities::Layout::Identity,
      vtkMatrixUtilities::Layout::Transpose>::Compute(std::forward<VectorT1>(x), tmp);
  }

  /**
   * General matrix multiplication.  You must allocate output storage.
   * colA == rowB
   * and matrix C is rowA x colB
   */
  static void MultiplyMatrix(const double* const* A, const double* const* B, unsigned int rowA,
    unsigned int colA, unsigned int rowB, unsigned int colB, double** C);

  ///@{
  /**
   * Transpose a 3x3 matrix. The input matrix is A. The output
   * is stored in AT.
   */
  static void Transpose3x3(const float A[3][3], float AT[3][3]);
  static void Transpose3x3(const double A[3][3], double AT[3][3]);
  ///@}

  ///@{
  /**
   * Invert a 3x3 matrix. The input matrix is A. The output is
   * stored in AI.
   */
  static void Invert3x3(const float A[3][3], float AI[3][3]);
  static void Invert3x3(const double A[3][3], double AI[3][3]);
  ///@}

  ///@{
  /**
   * Set A to the identity matrix.
   */
  static void Identity3x3(float A[3][3]);
  static void Identity3x3(double A[3][3]);
  ///@}

  ///@{
  /**
   * Return the determinant of a 3x3 matrix.
   */
  static double Determinant3x3(const float A[3][3]);
  static double Determinant3x3(const double A[3][3]);
  ///@}

  /**
   * Compute determinant of 3x3 matrix. Three columns of matrix are input.
   */
  static float Determinant3x3(const float c1[3], const float c2[3], const float c3[3]);

  /**
   * Compute determinant of 3x3 matrix. Three columns of matrix are input.
   */
  static double Determinant3x3(const double c1[3], const double c2[3], const double c3[3]);

  /**
   * Calculate the determinant of a 3x3 matrix in the form:
   * | a1,  b1,  c1 |
   * | a2,  b2,  c2 |
   * | a3,  b3,  c3 |
   */
  static double Determinant3x3(double a1, double a2, double a3, double b1, double b2, double b3,
    double c1, double c2, double c3);

  ///@{
  /**
   * Convert a quaternion to a 3x3 rotation matrix.  The quaternion
   * does not have to be normalized beforehand.
   * The quaternion must be in the form [w, x, y, z].
   * @sa Matrix3x3ToQuaternion() MultiplyQuaternion()
   * @sa vtkQuaternion
   */
  static void QuaternionToMatrix3x3(const float quat[4], float A[3][3]);
  static void QuaternionToMatrix3x3(const double quat[4], double A[3][3]);
  template <class QuaternionT, class MatrixT,
    class EnableT = typename std::enable_if<!vtkMatrixUtilities::MatrixIs2DArray<MatrixT>()>::type>
  static void QuaternionToMatrix3x3(QuaternionT&& q, MatrixT&& A);
  ///@}

  ///@{
  /**
   * Convert a 3x3 matrix into a quaternion.  This will provide the
   * best possible answer even if the matrix is not a pure rotation matrix.
   * The quaternion is in the form [w, x, y, z].
   * The method used is that of B.K.P. Horn.
   * See: https://people.csail.mit.edu/bkph/articles/Quaternions.pdf
   * @sa QuaternionToMatrix3x3() MultiplyQuaternion()
   * @sa vtkQuaternion
   */
  static void Matrix3x3ToQuaternion(const float A[3][3], float quat[4]);
  static void Matrix3x3ToQuaternion(const double A[3][3], double quat[4]);
  template <class MatrixT, class QuaternionT,
    class EnableT = typename std::enable_if<!vtkMatrixUtilities::MatrixIs2DArray<MatrixT>()>::type>
  static void Matrix3x3ToQuaternion(MatrixT&& A, QuaternionT&& q);
  ///@}

  ///@{
  /**
   * Multiply two quaternions. This is used to concatenate rotations.
   * Quaternions are in the form [w, x, y, z].
   * @sa Matrix3x3ToQuaternion() QuaternionToMatrix3x3()
   * @sa vtkQuaternion
   */
  static void MultiplyQuaternion(const float q1[4], const float q2[4], float q[4]);
  static void MultiplyQuaternion(const double q1[4], const double q2[4], double q[4]);
  ///@}

  ///@{
  /**
   * rotate a vector by a normalized quaternion
   * using // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
   */
  static void RotateVectorByNormalizedQuaternion(const float v[3], const float q[4], float r[3]);
  static void RotateVectorByNormalizedQuaternion(const double v[3], const double q[4], double r[3]);
  ///@}

  ///@{
  /**
   * rotate a vector by WXYZ
   * using // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
   */
  static void RotateVectorByWXYZ(const float v[3], const float q[4], float r[3]);
  static void RotateVectorByWXYZ(const double v[3], const double q[4], double r[3]);
  ///@}

  ///@{
  /**
   * Orthogonalize a 3x3 matrix and put the result in B.  If matrix A
   * has a negative determinant, then B will be a rotation plus a flip
   * i.e. it will have a determinant of -1.
   */
  static void Orthogonalize3x3(const float A[3][3], float B[3][3]);
  static void Orthogonalize3x3(const double A[3][3], double B[3][3]);
  ///@}

  ///@{
  /**
   * Diagonalize a symmetric 3x3 matrix and return the eigenvalues in
   * w and the eigenvectors in the columns of V.  The matrix V will
   * have a positive determinant, and the three eigenvectors will be
   * aligned as closely as possible with the x, y, and z axes.
   */
  static void Diagonalize3x3(const float A[3][3], float w[3], float V[3][3]);
  static void Diagonalize3x3(const double A[3][3], double w[3], double V[3][3]);
  ///@}

  ///@{
  /**
   * Perform singular value decomposition on a 3x3 matrix.  This is not
   * done using a conventional SVD algorithm, instead it is done using
   * Orthogonalize3x3 and Diagonalize3x3.  Both output matrices U and VT
   * will have positive determinants, and the w values will be arranged
   * such that the three rows of VT are aligned as closely as possible
   * with the x, y, and z axes respectively.  If the determinant of A is
   * negative, then the three w values will be negative.
   */
  static void SingularValueDecomposition3x3(
    const float A[3][3], float U[3][3], float w[3], float VT[3][3]);
  static void SingularValueDecomposition3x3(
    const double A[3][3], double U[3][3], double w[3], double VT[3][3]);
  ///@}

  /**
   * Solve linear equation Ax = b using Gaussian Elimination with Partial Pivoting
   * for a 2x2 system. If the matrix is found to be singular within a small numerical
   * tolerance close to machine precision then 0 is returned.
   * Note: Even if method succeeded the matrix A could be close to singular.
   *       The solution should be checked against relevant tolerance criteria.
   */
  static vtkTypeBool SolveLinearSystemGEPP2x2(
    double a00, double a01, double a10, double a11, double b0, double b1, double& x0, double& x1);

  /**
   * Solve linear equations Ax = b using Crout's method. Input is square
   * matrix A and load vector b. Solution x is written over load vector. The
   * dimension of the matrix is specified in size. If error is found, method
   * returns a 0.
   * Note: Even if method succeeded the matrix A could be close to singular.
   *       The solution should be checked against relevant tolerance criteria.
   */
  static vtkTypeBool SolveLinearSystem(double** A, double* x, int size);

  /**
   * Invert input square matrix A into matrix AI.
   * Note that A is modified during
   * the inversion. The size variable is the dimension of the matrix. Returns 0
   * if inverse not computed.
   */
  static vtkTypeBool InvertMatrix(double** A, double** AI, int size);

  /**
   * Thread safe version of InvertMatrix method.
   * Working memory arrays tmp1SIze and tmp2Size
   * of length size must be passed in.
   */
  static vtkTypeBool InvertMatrix(
    double** A, double** AI, int size, int* tmp1Size, double* tmp2Size);

  /**
   * Factor linear equations Ax = b using LU decomposition into the form
   * A = LU where L is a unit lower triangular matrix and U is upper triangular
   * matrix.
   * The input is a square matrix A, an integer array of pivot indices index[0->n-1],
   * and the size, n, of the square matrix.
   * The output is provided by overwriting the input A with a matrix of the same size as
   * A containing all of the information about L and U. If the output matrix is
   * \f$ A* = \left( \begin{array}{cc}
   * a & b \\ c & d \end{array} \right)\f$
   * then L and U can be obtained as:
   * \f$ L = \left( \begin{array}{cc}
   * 1 & 0 \\ c & 1 \end{array} \right)\f$
   * \f$ U = \left( \begin{array}{cc}
   * a & b \\ 0 & d \end{array} \right)\f$

   * That is, the diagonal of the resulting A* is the diagonal of U. The upper right
   * triangle of A is the upper right triangle of U. The lower left triangle of A is
   * the lower left triangle of L (and since L is unit lower triangular, the diagonal
   * of L is all 1's).
   * If an error is found, the function returns 0.
   */
  static vtkTypeBool LUFactorLinearSystem(double** A, int* index, int size);

  /**
   * Thread safe version of LUFactorLinearSystem method.
   * Working memory array tmpSize of length size
   * must be passed in.
   */
  static vtkTypeBool LUFactorLinearSystem(double** A, int* index, int size, double* tmpSize);

  /**
   * Solve linear equations Ax = b using LU decomposition A = LU where L is
   * lower triangular matrix and U is upper triangular matrix. Input is
   * factored matrix A=LU, integer array of pivot indices index[0->n-1],
   * load vector x[0->n-1], and size of square matrix n. Note that A=LU and
   * index[] are generated from method LUFactorLinearSystem). Also, solution
   * vector is written directly over input load vector.
   */
  static void LUSolveLinearSystem(double** A, int* index, double* x, int size);

  /**
   * Estimate the condition number of a LU factored matrix. Used to judge the
   * accuracy of the solution. The matrix A must have been previously factored
   * using the method LUFactorLinearSystem. The condition number is the ratio
   * of the infinity matrix norm (i.e., maximum value of matrix component)
   * divided by the minimum diagonal value. (This works for triangular matrices
   * only: see Conte and de Boor, Elementary Numerical Analysis.)
   */
  static double EstimateMatrixCondition(const double* const* A, int size);

  ///@{
  /**
   * Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
   * real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
   * and output eigenvectors in v arranged column-wise. Resulting
   * eigenvalues/vectors are sorted in decreasing order; the most positive
   * eigenvectors are selected for consistency; eigenvectors are normalized.
   * NOTE: the input matrix a is modified during the solution
   */
  static vtkTypeBool Jacobi(float** a, float* w, float** v);
  static vtkTypeBool Jacobi(double** a, double* w, double** v);
  ///@}

  ///@{
  /**
   * JacobiN iteration for the solution of eigenvectors/eigenvalues of a nxn
   * real symmetric matrix. Square nxn matrix a; size of matrix in n; output
   * eigenvalues in w; and output eigenvectors in v arranged column-wise.
   * Resulting eigenvalues/vectors are sorted in decreasing order; the most
   * positive eigenvectors are selected for consistency; and eigenvectors are
   * normalized. w and v need to be allocated previously.
   * NOTE: the input matrix a is modified during the solution
   */
  static vtkTypeBool JacobiN(float** a, int n, float* w, float** v);
  static vtkTypeBool JacobiN(double** a, int n, double* w, double** v);
  ///@}

  /**
   * Solves for the least squares best fit matrix for the homogeneous equation X'M' = 0'.
   * Uses the method described on pages 40-41 of Computer Vision by
   * Forsyth and Ponce, which is that the solution is the eigenvector
   * associated with the minimum eigenvalue of T(X)X, where T(X) is the
   * transpose of X.
   * The inputs and output are transposed matrices.
   * Dimensions: X' is numberOfSamples by xOrder,
   * M' dimension is xOrder by yOrder.
   * M' should be pre-allocated. All matrices are row major. The resultant
   * matrix M' should be pre-multiplied to X' to get 0', or transposed and
   * then post multiplied to X to get 0
   */
  static vtkTypeBool SolveHomogeneousLeastSquares(
    int numberOfSamples, double** xt, int xOrder, double** mt);

  /**
   * Solves for the least squares best fit matrix for the equation X'M' = Y'.
   * Uses pseudoinverse to get the ordinary least squares.
   * The inputs and output are transposed matrices.
   * Dimensions: X' is numberOfSamples by xOrder,
   * Y' is numberOfSamples by yOrder,
   * M' dimension is xOrder by yOrder.
   * M' should be pre-allocated. All matrices are row major. The resultant
   * matrix M' should be pre-multiplied to X' to get Y', or transposed and
   * then post multiplied to X to get Y
   * By default, this method checks for the homogeneous condition where Y==0, and
   * if so, invokes SolveHomogeneousLeastSquares. For better performance when
   * the system is known not to be homogeneous, invoke with checkHomogeneous=0.
   */
  static vtkTypeBool SolveLeastSquares(int numberOfSamples, double** xt, int xOrder, double** yt,
    int yOrder, double** mt, int checkHomogeneous = 1);

  ///@{
  /**
   * Convert color in RGB format (Red, Green, Blue) to HSV format
   * (Hue, Saturation, Value). The input color is not modified.
   * The input RGB must be float values in the range [0, 1].
   * The output ranges are hue [0, 1], saturation [0, 1], and
   * value [0, 1].
   */
  static void RGBToHSV(const float rgb[3], float hsv[3])
  {
    RGBToHSV(rgb[0], rgb[1], rgb[2], hsv, hsv + 1, hsv + 2);
  }
  static void RGBToHSV(float r, float g, float b, float* h, float* s, float* v);
  static void RGBToHSV(const double rgb[3], double hsv[3])
  {
    RGBToHSV(rgb[0], rgb[1], rgb[2], hsv, hsv + 1, hsv + 2);
  }
  static void RGBToHSV(double r, double g, double b, double* h, double* s, double* v);
  ///@}

  ///@{
  /**
   * Convert color in HSV format (Hue, Saturation, Value) to RGB
   * format (Red, Green, Blue). The input color is not modified.
   * The input 'hsv' must be float values in the range [0, 1].
   * The elements of each component of the output 'rgb' are in
   * the range [0, 1].
   */
  static void HSVToRGB(const float hsv[3], float rgb[3])
  {
    HSVToRGB(hsv[0], hsv[1], hsv[2], rgb, rgb + 1, rgb + 2);
  }
  static void HSVToRGB(float h, float s, float v, float* r, float* g, float* b);
  static void HSVToRGB(const double hsv[3], double rgb[3])
  {
    HSVToRGB(hsv[0], hsv[1], hsv[2], rgb, rgb + 1, rgb + 2);
  }
  static void HSVToRGB(double h, double s, double v, double* r, double* g, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the ProLAB system to CIE XYZ.
   * DOI : 10.1109/ACCESS.2021.3115425
   */
  static void ProLabToXYZ(const double prolab[3], double xyz[3])
  {
    ProLabToXYZ(prolab[0], prolab[1], prolab[2], xyz + 0, xyz + 1, xyz + 2);
  }
  static void ProLabToXYZ(double L, double a, double b, double* x, double* y, double* z);
  ///@}

  ///@{
  /**
   * Convert Color from the CIE XYZ system to ProLAB.
   * DOI : 10.1109/ACCESS.2021.3115425
   */
  static void XYZToProLab(const double xyz[3], double prolab[3])
  {
    XYZToProLab(xyz[0], xyz[1], xyz[2], prolab + 0, prolab + 1, prolab + 2);
  }
  static void XYZToProLab(double x, double y, double z, double* L, double* a, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the CIE-L*ab system to CIE XYZ.
   */
  static void LabToXYZ(const double lab[3], double xyz[3])
  {
    LabToXYZ(lab[0], lab[1], lab[2], xyz + 0, xyz + 1, xyz + 2);
  }
  static void LabToXYZ(double L, double a, double b, double* x, double* y, double* z);
  ///@}

  ///@{
  /**
   * Convert Color from the CIE XYZ system to CIE-L*ab.
   */
  static void XYZToLab(const double xyz[3], double lab[3])
  {
    XYZToLab(xyz[0], xyz[1], xyz[2], lab + 0, lab + 1, lab + 2);
  }
  static void XYZToLab(double x, double y, double z, double* L, double* a, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the CIE XYZ system to RGB.
   */
  static void XYZToRGB(const double xyz[3], double rgb[3])
  {
    XYZToRGB(xyz[0], xyz[1], xyz[2], rgb + 0, rgb + 1, rgb + 2);
  }
  static void XYZToRGB(double x, double y, double z, double* r, double* g, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the RGB system to CIE XYZ.
   */
  static void RGBToXYZ(const double rgb[3], double xyz[3])
  {
    RGBToXYZ(rgb[0], rgb[1], rgb[2], xyz + 0, xyz + 1, xyz + 2);
  }
  static void RGBToXYZ(double r, double g, double b, double* x, double* y, double* z);
  ///@}

  ///@{
  /**
   * Convert color from the RGB system to CIE-L*ab.
   * The input RGB must be values in the range [0, 1].
   * The output ranges of 'L' is [0, 100]. The output
   * range of 'a' and 'b' are approximately [-110, 110].
   */

  static void RGBToLab(const double rgb[3], double lab[3])
  {
    RGBToLab(rgb[0], rgb[1], rgb[2], lab + 0, lab + 1, lab + 2);
  }
  static void RGBToLab(double red, double green, double blue, double* L, double* a, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the ProLab system to RGB.
   */
  static void ProLabToRGB(const double prolab[3], double rgb[3])
  {
    ProLabToRGB(prolab[0], prolab[1], prolab[2], rgb + 0, rgb + 1, rgb + 2);
  }
  static void ProLabToRGB(double L, double a, double b, double* red, double* green, double* blue);
  ///@}

  ///@{
  /**
   * Convert color from the RGB system to Prolab
   * The input RGB must be values in the range [0, 1].
   * The output ranges of 'L' is [0, 100]. The output
   * range of 'a' and 'b' are approximately [-110, 110].
   *
   */
  static void RGBToProLab(const double rgb[3], double prolab[3])
  {
    RGBToProLab(rgb[0], rgb[1], rgb[2], prolab + 0, prolab + 1, prolab + 2);
  }
  static void RGBToProLab(double red, double green, double blue, double* L, double* a, double* b);
  ///@}

  ///@{
  /**
   * Convert color from the CIE-L*ab system to RGB.
   */
  static void LabToRGB(const double lab[3], double rgb[3])
  {
    LabToRGB(lab[0], lab[1], lab[2], rgb + 0, rgb + 1, rgb + 2);
  }
  static void LabToRGB(double L, double a, double b, double* red, double* green, double* blue);
  ///@}

  ///@{
  /**
   * Set the bounds to an uninitialized state
   */
  static void UninitializeBounds(double bounds[6])
  {
    bounds[0] = 1.0;
    bounds[1] = -1.0;
    bounds[2] = 1.0;
    bounds[3] = -1.0;
    bounds[4] = 1.0;
    bounds[5] = -1.0;
  }
  ///@}

  ///@{
  /**
   * Are the bounds initialized?
   */
  static vtkTypeBool AreBoundsInitialized(const double bounds[6])
  {
    if (bounds[1] - bounds[0] < 0.0)
    {
      return 0;
    }
    return 1;
  }
  ///@}

  /**
   * Clamp some value against a range, return the result.
   * min must be less than or equal to max. Semantics the same as std::clamp.
   */
  template <class T>
  static T ClampValue(const T& value, const T& min, const T& max);

  ///@{
  /**
   * Clamp some values against a range
   * The method without 'clamped_values' will perform in-place clamping.
   */
  static void ClampValue(double* value, const double range[2]);
  static void ClampValue(double value, const double range[2], double* clamped_value);
  static void ClampValues(double* values, int nb_values, const double range[2]);
  static void ClampValues(
    const double* values, int nb_values, const double range[2], double* clamped_values);
  ///@}

  /**
   * Clamp a value against a range and then normalize it between 0 and 1.
   * If range[0]==range[1], the result is 0.
   * \pre valid_range: range[0]<=range[1]
   * \post valid_result: result>=0.0 && result<=1.0
   */
  static double ClampAndNormalizeValue(double value, const double range[2]);

  /**
   * Convert a 6-Component symmetric tensor into a 9-Component tensor, no allocation performed.
   * Symmetric tensor is expected to have the following order : XX, YY, ZZ, XY, YZ, XZ
   */
  template <class T1, class T2>
  static void TensorFromSymmetricTensor(const T1 symmTensor[6], T2 tensor[9]);

  /**
   * Convert a 6-Component symmetric tensor into a 9-Component tensor, overwriting
   * the tensor input.
   * Symmetric tensor is expected to have the following order : XX, YY, ZZ, XY, YZ, XZ
   */
  template <class T>
  static void TensorFromSymmetricTensor(T tensor[9]);

  /**
   * Return the scalar type that is most likely to have enough precision
   * to store a given range of data once it has been scaled and shifted
   * (i.e. [range_min * scale + shift, range_max * scale + shift].
   * If any one of the parameters is not an integer number (decimal part != 0),
   * the search will default to float types only (float or double)
   * Return -1 on error or no scalar type found.
   */
  static int GetScalarTypeFittingRange(
    double range_min, double range_max, double scale = 1.0, double shift = 0.0);

  /**
   * Get a vtkDataArray's scalar range for a given component.
   * If the vtkDataArray's data type is unsigned char (VTK_UNSIGNED_CHAR)
   * the range is adjusted to the whole data type range [0, 255.0].
   * Same goes for unsigned short (VTK_UNSIGNED_SHORT) but the upper bound
   * is also adjusted down to 4095.0 if was between ]255, 4095.0].
   * Return 1 on success, 0 otherwise.
   */
  static vtkTypeBool GetAdjustedScalarRange(vtkDataArray* array, int comp, double range[2]);

  /**
   * Return true if first 3D extent is within second 3D extent
   * Extent is x-min, x-max, y-min, y-max, z-min, z-max
   */
  static vtkTypeBool ExtentIsWithinOtherExtent(const int extent1[6], const int extent2[6]);

  /**
   * Return true if first 3D bounds is within the second 3D bounds
   * Bounds is x-min, x-max, y-min, y-max, z-min, z-max
   * Delta is the error margin along each axis (usually a small number)
   */
  static vtkTypeBool BoundsIsWithinOtherBounds(
    const double bounds1[6], const double bounds2[6], const double delta[3]);

  /**
   * Return true if point is within the given 3D bounds
   * Bounds is x-min, x-max, y-min, y-max, z-min, z-max
   * Delta is the error margin along each axis (usually a small number)
   */
  static vtkTypeBool PointIsWithinBounds(
    const double point[3], const double bounds[6], const double delta[3]);

  /**
   * Implements Plane / Axis-Aligned Bounding-Box intersection as described in
   * Graphics Gems IV, Ned Greene; pp. 75-76. Variable names are based on the
   * description in the book. This function returns +1 if the box lies fully in
   * the positive side of the plane (by convention, the side to which the plane's
   * normal points to), -1 if the box fully lies in the negative side and 0 if
   * the plane intersects the box.  -2 is returned if any of the arguments is
   * invalid.
   */
  static int PlaneIntersectsAABB(
    const double bounds[6], const double normal[3], const double point[3]);

  /**
   * In Euclidean space, there is a unique circle passing through any given
   * three non-collinear points P1, P2, and P3. Using Cartesian coordinates
   * to represent these points as spatial vectors, it is possible to use the
   * dot product and cross product to calculate the radius and center of the
   * circle. See: http://en.wikipedia.org/wiki/Circumscribed_circle and more
   * specifically the section Barycentric coordinates from cross- and
   * dot-products
   */
  static double Solve3PointCircle(
    const double p1[3], const double p2[3], const double p3[3], double center[3]);

  /**
   * Special IEEE-754 number used to represent positive infinity.
   */
  static double Inf();

  /**
   * Special IEEE-754 number used to represent negative infinity.
   */
  static double NegInf();

  /**
   * Special IEEE-754 number used to represent Not-A-Number (Nan).
   */
  static double Nan();

  /**
   * Test if a number is equal to the special floating point value infinity.
   */
  static vtkTypeBool IsInf(double x);

  /**
   * Test if a number is equal to the special floating point value Not-A-Number (Nan).
   */
  static vtkTypeBool IsNan(double x);

  /**
   * Test if a number has finite value i.e. it is normal, subnormal or zero, but not infinite or
   * Nan.
   */
  static bool IsFinite(double x);

  /**
   * find roots of ax^2+bx+c=0  in the interval min,max.
   * place the roots in u[2] and return how many roots found
   */
  static int QuadraticRoot(double a, double b, double c, double min, double max, double* u);

  /**
   * Compute the greatest common divisor (GCD) of two positive integers m and n. If the
   * computed GCD==1, then the two integers are coprime to one another. This is a simple,
   * recursive implementation.
   */
  static vtkIdType ComputeGCD(vtkIdType m, vtkIdType n) { return (n ? ComputeGCD(n, m % n) : m); }

  /**
   * Support the convolution operations.
   */
  enum class ConvolutionMode
  {
    FULL,
    SAME,
    VALID
  };

  /**
   * Compute the convolution of a sampled 1D signal by a given kernel. There are 3 different modes
   * available:
   *
   * The "full" mode (default), returning the convolution at each point of overlap between
   * the sample and the kernel. The output size is equal to sampleSize + kernelSize + 1.
   *
   * The "same" mode, where the convolution is computed only if the center of the kernel
   * overlaps with the sample. The output size is equal to the sampleSize.
   *
   * The "valid" mode, where the convolution is computed only if the kernel overlaps completely
   * with the sample. The output size is equal to the sampleSize - kernelSize + 1.
   *
   * @note By convention, here the kernel refers to the smallest input signal of the two, but it
   * doesn't matter if it's passed as the first or the second parameter (the convolution is
   * commutative).
   *
   * @note The function does nothing if iteratorBegin >= iteratorEnd (for each couple of iterators)
   *
   * @note The output signal is padded with zeros if its size (endOut - beginOut) is bigger than
   * the number of generated values. If its size is smaller, the result is truncated from the end.
   */
  template <class Iter1, class Iter2, class Iter3>
  static void Convolve1D(Iter1 beginSample, Iter1 endSample, Iter2 beginKernel, Iter2 endKernel,
    Iter3 beginOut, Iter3 endOut, ConvolutionMode mode = ConvolutionMode::FULL)
  {
    int sampleSize = std::distance(beginSample, endSample);
    int kernelSize = std::distance(beginKernel, endKernel);
    int outSize = std::distance(beginOut, endOut);

    if (sampleSize <= 0 || kernelSize <= 0 || outSize <= 0)
    {
      return;
    }

    int begin = 0;
    int end = outSize;

    switch (mode)
    {
      case ConvolutionMode::SAME:
        begin = static_cast<int>(std::ceil((std::min)(sampleSize, kernelSize) / 2.0)) - 1;
        end = begin + (std::max)(sampleSize, kernelSize);
        break;
      case ConvolutionMode::VALID:
        begin = (std::min)(sampleSize, kernelSize) - 1;
        end = begin + std::abs(sampleSize - kernelSize) + 1;
        break;
      case ConvolutionMode::FULL:
      default:
        break;
    }

    for (int i = begin; i < end; i++)
    {
      Iter3 out = beginOut + i - begin;
      *out = 0;
      for (int j = (std::max)(i - sampleSize + 1, 0); j <= (std::min)(i, kernelSize - 1); j++)
      {
        *out += *(beginSample + (i - j)) * *(beginKernel + j);
      }
    }
  }

  /**
   * Get the coordinates of a point along a line defined by p1 and p2, at a
   * specified offset relative to p2.
   */
  static void GetPointAlongLine(double result[3], double p1[3], double p2[3], const double offset)
  {
    double directionVector[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    vtkMath::Normalize(directionVector);
    result[0] = p2[0] + (offset * directionVector[0]);
    result[1] = p2[1] + (offset * directionVector[1]);
    result[2] = p2[2] + (offset * directionVector[2]);
  }

protected:
  vtkMath() = default;
  ~vtkMath() override = default;

  static vtkSmartPointer<vtkMathInternal> Internal;

private:
  vtkMath(const vtkMath&) = delete;
  void operator=(const vtkMath&) = delete;
};

//----------------------------------------------------------------------------
inline float vtkMath::RadiansFromDegrees(float x)
{
  return x * 0.017453292f;
}

//----------------------------------------------------------------------------
inline double vtkMath::RadiansFromDegrees(double x)
{
  return x * 0.017453292519943295;
}

//----------------------------------------------------------------------------
inline float vtkMath::DegreesFromRadians(float x)
{
  return x * 57.2957795131f;
}

//----------------------------------------------------------------------------
inline double vtkMath::DegreesFromRadians(double x)
{
  return x * 57.29577951308232;
}

//----------------------------------------------------------------------------
inline bool vtkMath::IsPowerOfTwo(vtkTypeUInt64 x)
{
  return ((x != 0) & ((x & (x - 1)) == 0));
}

//----------------------------------------------------------------------------
// Credit goes to Peter Hart and William Lewis on comp.lang.python 1997
inline int vtkMath::NearestPowerOfTwo(int x)
{
  unsigned int z = static_cast<unsigned int>(((x > 0) ? x - 1 : 0));
  z |= z >> 1;
  z |= z >> 2;
  z |= z >> 4;
  z |= z >> 8;
  z |= z >> 16;
  return static_cast<int>(z + 1);
}

//----------------------------------------------------------------------------
// Modify the trunc() operation provided by static_cast<int>() to get floor(),
// Note that in C++ conditions evaluate to values of 1 or 0 (true or false).
inline int vtkMath::Floor(double x)
{
  int i = static_cast<int>(x);
  return i - (i > x);
}

//----------------------------------------------------------------------------
// Modify the trunc() operation provided by static_cast<int>() to get ceil(),
// Note that in C++ conditions evaluate to values of 1 or 0 (true or false).
inline int vtkMath::Ceil(double x)
{
  int i = static_cast<int>(x);
  return i + (i < x);
}

//----------------------------------------------------------------------------
template <class T>
inline T vtkMath::Min(const T& a, const T& b)
{
  return (b <= a ? b : a);
}

//----------------------------------------------------------------------------
template <class T>
inline T vtkMath::Max(const T& a, const T& b)
{
  return (b > a ? b : a);
}

//----------------------------------------------------------------------------
inline float vtkMath::Normalize(float v[3])
{
  float den = vtkMath::Norm(v);
  if (den != 0.0)
  {
    for (int i = 0; i < 3; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline double vtkMath::Normalize(double v[3])
{
  double den = vtkMath::Norm(v);
  if (den != 0.0)
  {
    for (int i = 0; i < 3; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline float vtkMath::Normalize2D(float v[2])
{
  float den = vtkMath::Norm2D(v);
  if (den != 0.0)
  {
    for (int i = 0; i < 2; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline double vtkMath::Normalize2D(double v[2])
{
  double den = vtkMath::Norm2D(v);
  if (den != 0.0)
  {
    for (int i = 0; i < 2; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline float vtkMath::Determinant3x3(const float c1[3], const float c2[3], const float c3[3])
{
  return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
    c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const double c1[3], const double c2[3], const double c3[3])
{
  return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
    c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(
  double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3)
{
  return (a1 * vtkMath::Determinant2x2(b2, b3, c2, c3) -
    b1 * vtkMath::Determinant2x2(a2, a3, c2, c3) + c1 * vtkMath::Determinant2x2(a2, a3, b2, b3));
}

//----------------------------------------------------------------------------
inline float vtkMath::Distance2BetweenPoints(const float p1[3], const float p2[3])
{
  return ((p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1]) +
    (p1[2] - p2[2]) * (p1[2] - p2[2]));
}

//----------------------------------------------------------------------------
inline double vtkMath::Distance2BetweenPoints(const double p1[3], const double p2[3])
{
  return ((p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1]) +
    (p1[2] - p2[2]) * (p1[2] - p2[2]));
}

//----------------------------------------------------------------------------
template <typename ReturnTypeT, typename TupleRangeT1, typename TupleRangeT2, typename EnableT>
inline ReturnTypeT vtkMath::Distance2BetweenPoints(const TupleRangeT1& p1, const TupleRangeT2& p2)
{
  return ((p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1]) +
    (p1[2] - p2[2]) * (p1[2] - p2[2]));
}

//----------------------------------------------------------------------------
template <class VectorT1, class VectorT2, class VectorT3>
void vtkMath::Cross(VectorT1&& a, VectorT2&& b, VectorT3& c)
{
  typedef typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT3>::value_type ValueType;
  ValueType Cx = a[1] * b[2] - a[2] * b[1];
  ValueType Cy = a[2] * b[0] - a[0] * b[2];
  ValueType Cz = a[0] * b[1] - a[1] * b[0];
  c[0] = Cx;
  c[1] = Cy;
  c[2] = Cz;
}

//----------------------------------------------------------------------------
// Cross product of two 3-vectors. Result (a x b) is stored in c[3].
inline void vtkMath::Cross(const float a[3], const float b[3], float c[3])
{
  float Cx = a[1] * b[2] - a[2] * b[1];
  float Cy = a[2] * b[0] - a[0] * b[2];
  float Cz = a[0] * b[1] - a[1] * b[0];
  c[0] = Cx;
  c[1] = Cy;
  c[2] = Cz;
}

//----------------------------------------------------------------------------
// Cross product of two 3-vectors. Result (a x b) is stored in c[3].
inline void vtkMath::Cross(const double a[3], const double b[3], double c[3])
{
  double Cx = a[1] * b[2] - a[2] * b[1];
  double Cy = a[2] * b[0] - a[0] * b[2];
  double Cz = a[0] * b[1] - a[1] * b[0];
  c[0] = Cx;
  c[1] = Cy;
  c[2] = Cz;
}

//----------------------------------------------------------------------------
template <class T>
inline double vtkDeterminant3x3(const T A[3][3])
{
  return A[0][0] * A[1][1] * A[2][2] + A[1][0] * A[2][1] * A[0][2] + A[2][0] * A[0][1] * A[1][2] -
    A[0][0] * A[2][1] * A[1][2] - A[1][0] * A[0][1] * A[2][2] - A[2][0] * A[1][1] * A[0][2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const float A[3][3])
{
  return vtkDeterminant3x3(A);
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const double A[3][3])
{
  return vtkDeterminant3x3(A);
}

//----------------------------------------------------------------------------
template <class T>
inline T vtkMath::ClampValue(const T& value, const T& min, const T& max)
{
  assert("pre: valid_range" && min <= max);

#if __cplusplus >= 201703L
  return std::clamp(value, min, max);
#else
  // compilers are good at optimizing the ternary operator,
  // use '<' since it is preferred by STL for custom types
  T v = (min < value ? value : min);
  return (v < max ? v : max);
#endif
}

//----------------------------------------------------------------------------
inline void vtkMath::ClampValue(double* value, const double range[2])
{
  if (value && range)
  {
    assert("pre: valid_range" && range[0] <= range[1]);

    *value = vtkMath::ClampValue(*value, range[0], range[1]);
  }
}

//----------------------------------------------------------------------------
inline void vtkMath::ClampValue(double value, const double range[2], double* clamped_value)
{
  if (range && clamped_value)
  {
    assert("pre: valid_range" && range[0] <= range[1]);

    *clamped_value = vtkMath::ClampValue(value, range[0], range[1]);
  }
}

// ---------------------------------------------------------------------------
inline double vtkMath::ClampAndNormalizeValue(double value, const double range[2])
{
  assert("pre: valid_range" && range[0] <= range[1]);

  double result;
  if (range[0] == range[1])
  {
    result = 0.0;
  }
  else
  {
    // clamp
    result = vtkMath::ClampValue(value, range[0], range[1]);

    // normalize
    result = (result - range[0]) / (range[1] - range[0]);
  }

  assert("post: valid_result" && result >= 0.0 && result <= 1.0);

  return result;
}

//-----------------------------------------------------------------------------
template <class T1, class T2>
inline void vtkMath::TensorFromSymmetricTensor(const T1 symmTensor[9], T2 tensor[9])
{
  for (int i = 0; i < 3; ++i)
  {
    tensor[4 * i] = symmTensor[i];
  }
  tensor[1] = tensor[3] = symmTensor[3];
  tensor[2] = tensor[6] = symmTensor[5];
  tensor[5] = tensor[7] = symmTensor[4];
}

//-----------------------------------------------------------------------------
template <class T>
inline void vtkMath::TensorFromSymmetricTensor(T tensor[9])
{
  tensor[6] = tensor[5]; // XZ
  tensor[7] = tensor[4]; // YZ
  tensor[8] = tensor[2]; // ZZ
  tensor[4] = tensor[1]; // YY
  tensor[5] = tensor[7]; // YZ
  tensor[2] = tensor[6]; // XZ
  tensor[1] = tensor[3]; // XY
}
VTK_ABI_NAMESPACE_END

namespace
{
template <class QuaternionT, class MatrixT>
inline void vtkQuaternionToMatrix3x3(QuaternionT&& quat, MatrixT&& A)
{
  typedef typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Scalar;

  Scalar ww = quat[0] * quat[0];
  Scalar wx = quat[0] * quat[1];
  Scalar wy = quat[0] * quat[2];
  Scalar wz = quat[0] * quat[3];

  Scalar xx = quat[1] * quat[1];
  Scalar yy = quat[2] * quat[2];
  Scalar zz = quat[3] * quat[3];

  Scalar xy = quat[1] * quat[2];
  Scalar xz = quat[1] * quat[3];
  Scalar yz = quat[2] * quat[3];

  Scalar rr = xx + yy + zz;
  // normalization factor, just in case quaternion was not normalized
  Scalar f = 1 / (ww + rr);
  Scalar s = (ww - rr) * f;
  f *= 2;

  typedef vtkMatrixUtilities::Wrapper<3, 3, MatrixT> Wrapper;

  Wrapper::template Get<0, 0>(std::forward<MatrixT>(A)) = xx * f + s;
  Wrapper::template Get<1, 0>(std::forward<MatrixT>(A)) = (xy + wz) * f;
  Wrapper::template Get<2, 0>(std::forward<MatrixT>(A)) = (xz - wy) * f;

  Wrapper::template Get<0, 1>(std::forward<MatrixT>(A)) = (xy - wz) * f;
  Wrapper::template Get<1, 1>(std::forward<MatrixT>(A)) = yy * f + s;
  Wrapper::template Get<2, 1>(std::forward<MatrixT>(A)) = (yz + wx) * f;

  Wrapper::template Get<0, 2>(std::forward<MatrixT>(A)) = (xz + wy) * f;
  Wrapper::template Get<1, 2>(std::forward<MatrixT>(A)) = (yz - wx) * f;
  Wrapper::template Get<2, 2>(std::forward<MatrixT>(A)) = zz * f + s;
}
} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
inline void vtkMath::QuaternionToMatrix3x3(const float quat[4], float A[3][3])
{
  vtkQuaternionToMatrix3x3(quat, A);
}

//------------------------------------------------------------------------------
inline void vtkMath::QuaternionToMatrix3x3(const double quat[4], double A[3][3])
{
  vtkQuaternionToMatrix3x3(quat, A);
}

//-----------------------------------------------------------------------------
template <class QuaternionT, class MatrixT, class EnableT>
inline void vtkMath::QuaternionToMatrix3x3(QuaternionT&& q, MatrixT&& A)
{
  vtkQuaternionToMatrix3x3(std::forward<QuaternionT>(q), std::forward<MatrixT>(A));
}
VTK_ABI_NAMESPACE_END

namespace
{
//------------------------------------------------------------------------------
//  The solution is based on
//  Berthold K. P. Horn (1987),
//  "Closed-form solution of absolute orientation using unit quaternions,"
//  Journal of the Optical Society of America A, 4:629-642
template <class MatrixT, class QuaternionT>
inline void vtkMatrix3x3ToQuaternion(MatrixT&& A, QuaternionT&& quat)
{
  typedef typename vtkMatrixUtilities::ScalarTypeExtractor<QuaternionT>::value_type Scalar;

  Scalar N[4][4];

  typedef vtkMatrixUtilities::Wrapper<3, 3, MatrixT> Wrapper;

  // on-diagonal elements
  N[0][0] = Wrapper::template Get<0, 0>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<1, 1>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<2, 2>(std::forward<MatrixT>(A));
  N[1][1] = Wrapper::template Get<0, 0>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<1, 1>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<2, 2>(std::forward<MatrixT>(A));
  N[2][2] = -Wrapper::template Get<0, 0>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<1, 1>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<2, 2>(std::forward<MatrixT>(A));
  N[3][3] = -Wrapper::template Get<0, 0>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<1, 1>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<2, 2>(std::forward<MatrixT>(A));

  // off-diagonal elements
  N[0][1] = N[1][0] = Wrapper::template Get<2, 1>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<1, 2>(std::forward<MatrixT>(A));
  N[0][2] = N[2][0] = Wrapper::template Get<0, 2>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<2, 0>(std::forward<MatrixT>(A));
  N[0][3] = N[3][0] = Wrapper::template Get<1, 0>(std::forward<MatrixT>(A)) -
    Wrapper::template Get<0, 1>(std::forward<MatrixT>(A));

  N[1][2] = N[2][1] = Wrapper::template Get<1, 0>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<0, 1>(std::forward<MatrixT>(A));
  N[1][3] = N[3][1] = Wrapper::template Get<0, 2>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<2, 0>(std::forward<MatrixT>(A));
  N[2][3] = N[3][2] = Wrapper::template Get<2, 1>(std::forward<MatrixT>(A)) +
    Wrapper::template Get<1, 2>(std::forward<MatrixT>(A));

  Scalar eigenvectors[4][4], eigenvalues[4];

  // convert into format that JacobiN can use,
  // then use Jacobi to find eigenvalues and eigenvectors
  Scalar *NTemp[4], *eigenvectorsTemp[4];
  for (int i = 0; i < 4; ++i)
  {
    NTemp[i] = N[i];
    eigenvectorsTemp[i] = eigenvectors[i];
  }
  vtkMath::JacobiN(NTemp, 4, eigenvalues, eigenvectorsTemp);

  // the first eigenvector is the one we want
  quat[0] = eigenvectors[0][0];
  quat[1] = eigenvectors[1][0];
  quat[2] = eigenvectors[2][0];
  quat[3] = eigenvectors[3][0];
}
} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
inline void vtkMath::Matrix3x3ToQuaternion(const float A[3][3], float quat[4])
{
  vtkMatrix3x3ToQuaternion(A, quat);
}

//------------------------------------------------------------------------------
inline void vtkMath::Matrix3x3ToQuaternion(const double A[3][3], double quat[4])
{
  vtkMatrix3x3ToQuaternion(A, quat);
}

//-----------------------------------------------------------------------------
template <class MatrixT, class QuaternionT, class EnableT>
inline void vtkMath::Matrix3x3ToQuaternion(MatrixT&& A, QuaternionT&& q)
{
  vtkMatrix3x3ToQuaternion(std::forward<MatrixT>(A), std::forward<QuaternionT>(q));
}
VTK_ABI_NAMESPACE_END

namespace vtk_detail
{
VTK_ABI_NAMESPACE_BEGIN
// Can't specialize templates inside a template class, so we move the impl here.
template <typename OutT>
void RoundDoubleToIntegralIfNecessary(double val, OutT* ret)
{ // OutT is integral -- clamp and round
  if (!vtkMath::IsNan(val))
  {
    double min = static_cast<double>(vtkTypeTraits<OutT>::Min());
    double max = static_cast<double>(vtkTypeTraits<OutT>::Max());
    val = vtkMath::ClampValue(val, min, max);
    *ret = static_cast<OutT>((val >= 0.0) ? (val + 0.5) : (val - 0.5));
  }
  else
    *ret = 0;
}
template <>
inline void RoundDoubleToIntegralIfNecessary(double val, double* retVal)
{ // OutT is double: passthrough
  *retVal = val;
}
template <>
inline void RoundDoubleToIntegralIfNecessary(double val, float* retVal)
{ // OutT is float -- just clamp (as doubles, then the cast to float is well-defined.)
  if (!vtkMath::IsNan(val))
  {
    double min = static_cast<double>(vtkTypeTraits<float>::Min());
    double max = static_cast<double>(vtkTypeTraits<float>::Max());
    val = vtkMath::ClampValue(val, min, max);
  }

  *retVal = static_cast<float>(val);
}
VTK_ABI_NAMESPACE_END
} // end namespace vtk_detail

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
#if defined(VTK_HAS_ISINF) || defined(VTK_HAS_STD_ISINF)
#define VTK_MATH_ISINF_IS_INLINE
inline vtkTypeBool vtkMath::IsInf(double x)
{
#if defined(VTK_HAS_STD_ISINF)
  return std::isinf(x);
#else
  return (isinf(x) != 0);    // Force conversion to bool
#endif
}
#endif

//-----------------------------------------------------------------------------
#if defined(VTK_HAS_ISNAN) || defined(VTK_HAS_STD_ISNAN)
#define VTK_MATH_ISNAN_IS_INLINE
inline vtkTypeBool vtkMath::IsNan(double x)
{
#if defined(VTK_HAS_STD_ISNAN)
  return std::isnan(x);
#else
  return (isnan(x) != 0);    // Force conversion to bool
#endif
}
#endif

//-----------------------------------------------------------------------------
#if defined(VTK_HAS_ISFINITE) || defined(VTK_HAS_STD_ISFINITE) || defined(VTK_HAS_FINITE)
#define VTK_MATH_ISFINITE_IS_INLINE
inline bool vtkMath::IsFinite(double x)
{
#if defined(VTK_HAS_STD_ISFINITE)
  return std::isfinite(x);
#elif defined(VTK_HAS_ISFINITE)
  return (isfinite(x) != 0); // Force conversion to bool
#else
  return (finite(x) != 0); // Force conversion to bool
#endif
}
#endif

VTK_ABI_NAMESPACE_END
#endif
