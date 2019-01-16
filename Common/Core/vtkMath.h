/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov

=========================================================================*/
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
#include "vtkObject.h"
#include "vtkTypeTraits.h" // For type traits
#include "vtkSmartPointer.h" // For vtkSmartPointer.

#include "vtkMathConfigure.h" // For <cmath> and VTK_HAS_ISNAN etc.

#include <cassert> // assert() in inline implementations.
#include <algorithm> // for std::clamp

#ifndef DBL_MIN
#  define VTK_DBL_MIN    2.2250738585072014e-308
#else  // DBL_MIN
#  define VTK_DBL_MIN    DBL_MIN
#endif  // DBL_MIN

#ifndef DBL_EPSILON
#  define VTK_DBL_EPSILON    2.2204460492503131e-16
#else  // DBL_EPSILON
#  define VTK_DBL_EPSILON    DBL_EPSILON
#endif  // DBL_EPSILON

#ifndef VTK_DBL_EPSILON
#  ifndef DBL_EPSILON
#    define VTK_DBL_EPSILON    2.2204460492503131e-16
#  else  // DBL_EPSILON
#    define VTK_DBL_EPSILON    DBL_EPSILON
#  endif  // DBL_EPSILON
#endif  // VTK_DBL_EPSILON

class vtkDataArray;
class vtkPoints;
class vtkMathInternal;
class vtkMinimalStandardRandomSequence;
class vtkBoxMuellerRandomSequence;

namespace vtk_detail
{
// forward declaration
template <typename OutT>
void RoundDoubleToIntegralIfNecessary(double val, OutT* ret);
} // end namespace vtk_detail

class VTKCOMMONCORE_EXPORT vtkMath : public vtkObject
{
public:
  static vtkMath *New();
  vtkTypeMacro(vtkMath,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A mathematical constant. This version is atan(1.0) * 4.0
   */
  static double Pi() { return 3.141592653589793; }

  //@{
  /**
   * Convert degrees into radians
   */
  static float RadiansFromDegrees( float degrees);
  static double RadiansFromDegrees( double degrees);
  //@}

  //@{
  /**
   * Convert radians into degrees
   */
  static float DegreesFromRadians( float radians);
  static double DegreesFromRadians( double radians);
  //@}

  /**
   * Rounds a float to the nearest integer.
   */
#if 1
  static int Round(float f) {
    return static_cast<int>( f + ( f >= 0.0 ? 0.5 : -0.5 ) ); }
  static int Round(double f) {
    return static_cast<int>( f + ( f >= 0.0 ? 0.5 : -0.5 ) ); }
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
  template<class T>
  static T Min(const T & a, const T & b);

  /**
   * Returns the maximum of the two arguments provided. If either
   * argument is NaN, the first argument will always be returned.
   */
  template<class T>
  static T Max(const T & a, const T & b);

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
  static vtkTypeInt64 Factorial( int N );

  /**
   * The number of combinations of n objects from a pool of m objects (m>n).
   * This is commonly known as "m choose n" and sometimes denoted \f$_mC_n\f$
   * or \f$\left(\begin{array}{c}m \\ n\end{array}\right)\f$.
   */
  static vtkTypeInt64 Binomial( int m, int n );

  /**
   * Start iterating over "m choose n" objects.
   * This function returns an array of n integers, each from 0 to m-1.
   * These integers represent the n items chosen from the set [0,m[.

   * You are responsible for calling vtkMath::FreeCombination() once the iterator is no longer needed.

   * Warning: this gets large very quickly, especially when n nears m/2!
   * (Hint: think of Pascal's triangle.)
   */
  static int* BeginCombination( int m, int n );

  /**
   * Given \a m, \a n, and a valid \a combination of \a n integers in
   * the range [0,m[, this function alters the integers into the next
   * combination in a sequence of all combinations of \a n items from
   * a pool of \a m.

   * If the \a combination is the last item in the sequence on input,
   * then \a combination is unaltered and 0 is returned.
   * Otherwise, 1 is returned and \a combination is updated.
   */
  static int NextCombination( int m, int n, int* combination );

  /**
   * Free the "iterator" array created by vtkMath::BeginCombination.
   */
  static void FreeCombination( int* combination);

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
  static double Random( double min, double max );

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
  static double Gaussian( double mean, double std );

  /**
   * Addition of two 3-vectors (float version). Result is stored in c according to c = a + b.
   */
  static void Add(const float a[3], const float b[3], float c[3]) {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] + b[i];
    }
  }

  /**
   * Addition of two 3-vectors (double version). Result is stored in c according to c = a + b.
   */
  static void Add(const double a[3], const double b[3], double c[3]) {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] + b[i];
    }
  }

  /**
   * Subtraction of two 3-vectors (float version). Result is stored in c according to c = a - b.
   */
  static void Subtract(const float a[3], const float b[3], float c[3]) {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] - b[i];
    }
  }

  /**
   * Subtraction of two 3-vectors (double version). Result is stored in c according to c = a - b.
   */
  static void Subtract(const double a[3], const double b[3], double c[3]) {
    for (int i = 0; i < 3; ++i)
    {
      c[i] = a[i] - b[i];
    }
  }

  /**
   * Multiplies a 3-vector by a scalar (float version).
   * This modifies the input 3-vector.
   */
  static void MultiplyScalar(float a[3], float s) {
    for (int i = 0; i < 3; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 2-vector by a scalar (float version).
   * This modifies the input 2-vector.
   */
  static void MultiplyScalar2D(float a[2], float s) {
    for (int i = 0; i < 2; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 3-vector by a scalar (double version).
   * This modifies the input 3-vector.
   */
  static void MultiplyScalar(double a[3], double s) {
    for (int i = 0; i < 3; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Multiplies a 2-vector by a scalar (double version).
   * This modifies the input 2-vector.
   */
  static void MultiplyScalar2D(double a[2], double s) {
    for (int i = 0; i < 2; ++i)
    {
      a[i] *= s;
    }
  }

  /**
   * Dot product of two 3-vectors (float version).
   */
  static float Dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * Dot product of two 3-vectors (double version).
   */
  static double Dot(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * Outer product of two 3-vectors (float version).
   */
  static void Outer(const float a[3], const float b[3], float c[3][3]) {
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
  static void Outer(const double a[3], const double b[3], double c[3][3]) {
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
   * (float version)
   */
  static void Cross(const float a[3], const float b[3], float c[3]);

  /**
   * Cross product of two 3-vectors. Result (a x b) is stored in c.
   * (double version)
   */
  static void Cross(const double a[3], const double b[3], double c[3]);

  //@{
  /**
   * Compute the norm of n-vector. x is the vector, n is its length.
   */
  static float Norm(const float* x, int n);
  static double Norm(const double* x, int n);
  //@}

  /**
   * Compute the norm of 3-vector (float version).
   */
  static float Norm(const float v[3]) {
    return static_cast<float> (sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] ) );
  }

  /**
   * Compute the norm of 3-vector (double version).
   */
  static double Norm(const double v[3]) {
    return sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
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

  //@{
  /**
   * Given a unit vector v1, find two unit vectors v2 and v3 such that
   * v1 cross v2 = v3 (i.e. the vectors are perpendicular to each other).
   * There is an infinite number of such vectors, specify an angle theta
   * to choose one set.  If you want only one perpendicular vector,
   * specify nullptr for v3.
   */
  static void Perpendiculars(const double v1[3], double v2[3], double v3[3],
                             double theta);
  static void Perpendiculars(const float v1[3], float v2[3], float v3[3],
                             double theta);
  //@}

  //@{
  /**
   * Compute the projection of vector a on vector b and return it in projection[3].
   * If b is a zero vector, the function returns false and 'projection' is invalid.
   * Otherwise, it returns true.
   */
  static bool ProjectVector(const float a[3], const float b[3], float projection[3]);
  static bool ProjectVector(const double a[3], const double b[3], double projection[3]);
  //@}

  //@{
  /**
   * Compute the projection of 2D vector a on 2D vector b and returns the result
   * in projection[2].
   * If b is a zero vector, the function returns false and 'projection' is invalid.
   * Otherwise, it returns true.
   */
  static bool ProjectVector2D(const float a[2], const float b[2], float projection[2]);
  static bool ProjectVector2D(const double a[2], const double b[2], double projection[2]);
  //@}

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
   * Compute the amplitude of a Gaussian function with mean=0 and specified variance.
   * That is, 1./(sqrt(2 Pi * variance)) * exp(-distanceFromMean^2/(2.*variance)).
   */
  static double GaussianAmplitude(const double variance, const double distanceFromMean);

  /**
   * Compute the amplitude of a Gaussian function with specified mean and variance.
   * That is, 1./(sqrt(2 Pi * variance)) * exp(-(position - mean)^2/(2.*variance)).
   */
  static double GaussianAmplitude(const double mean, const double variance, const double position);

  /**
   * Compute the amplitude of an unnormalized Gaussian function with mean=0 and specified variance.
   * That is, exp(-distanceFromMean^2/(2.*variance)). When distanceFromMean = 0, this function
   * returns 1.
   */
  static double GaussianWeight(const double variance, const double distanceFromMean);

  /**
   * Compute the amplitude of an unnormalized Gaussian function with specified mean and variance.
   * That is, exp(-(position - mean)^2/(2.*variance)). When the distance from 'position' to 'mean'
   * is 0, this function returns 1.
   */
  static double GaussianWeight(const double mean, const double variance, const double position);

  /**
   * Dot product of two 2-vectors. (float version).
   */
  static float Dot2D(const float x[2], const float y[2]) {
    return x[0] * y[0] + x[1] * y[1];
  }

  /**
   * Dot product of two 2-vectors. (double version).
   */
  static double Dot2D(const double x[2], const double y[2]) {
    return x[0] * y[0] + x[1] * y[1];
  }

  /**
   * Outer product of two 2-vectors (float version).
   */
  static void Outer2D(const float x[2], const float y[2], float A[2][2])
  {
    for (int i=0; i < 2; ++i)
    {
      for (int j=0; j < 2; ++j)
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
    for (int i=0; i < 2; ++i)
    {
      for (int j=0; j < 2; ++j)
      {
        A[i][j] = x[i] * y[j];
      }
    }
  }

  /**
   * Compute the norm of a 2-vector.
   * (float version).
   */
  static float Norm2D(const float x[2]) {
    return static_cast<float> (sqrt( x[0] * x[0] + x[1] * x[1] ) );
  }

  /**
   * Compute the norm of a 2-vector.
   * (double version).
   */
  static double Norm2D(const double x[2]) {
    return sqrt( x[0] * x[0] + x[1] * x[1] );
  }

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
  static float Determinant2x2(const float c1[2], const float c2[2]) {
    return c1[0] * c2[1] - c2[0] * c1[1];
  }

  //@{
  /**
   * Calculate the determinant of a 2x2 matrix: | a b | | c d |
   */
  static double Determinant2x2(double a, double b, double c, double d) {
    return a * d - b * c;
  }
  static double Determinant2x2(const double c1[2], const double c2[2]) {
    return c1[0] * c2[1] - c2[0] * c1[1];
  }
  //@}

  //@{
  /**
   * LU Factorization of a 3x3 matrix.
   */
  static void LUFactor3x3(float A[3][3], int index[3]);
  static void LUFactor3x3(double A[3][3], int index[3]);
  //@}

  //@{
  /**
   * LU back substitution for a 3x3 matrix.
   */
  static void LUSolve3x3(const float A[3][3], const int index[3],
                         float x[3]);
  static void LUSolve3x3(const double A[3][3], const int index[3],
                         double x[3]);
  //@}

  //@{
  /**
   * Solve Ay = x for y and place the result in y.  The matrix A is
   * destroyed in the process.
   */
  static void LinearSolve3x3(const float A[3][3], const float x[3],
                             float y[3]);
  static void LinearSolve3x3(const double A[3][3], const double x[3],
                             double y[3]);
  //@}

  //@{
  /**
   * Multiply a vector by a 3x3 matrix.  The result is placed in out.
   */
  static void Multiply3x3(const float A[3][3], const float in[3],
                          float out[3]);
  static void Multiply3x3(const double A[3][3], const double in[3],
                          double out[3]);
  //@}

  //@{
  /**
   * Multiply one 3x3 matrix by another according to C = AB.
   */
  static void Multiply3x3(const float A[3][3], const float B[3][3],
                          float C[3][3]);
  static void Multiply3x3(const double A[3][3], const double B[3][3],
                          double C[3][3]);
  //@}

  /**
   * General matrix multiplication.  You must allocate output storage.
   * colA == rowB
   * and matrix C is rowA x colB
   */
  static void MultiplyMatrix(const double *const *A, const double *const *B,
                             unsigned int rowA, unsigned int colA,
                             unsigned int rowB, unsigned int colB,
                             double **C);

  //@{
  /**
   * Transpose a 3x3 matrix. The input matrix is A. The output
   * is stored in AT.
   */
  static void Transpose3x3(const float A[3][3], float AT[3][3]);
  static void Transpose3x3(const double A[3][3], double AT[3][3]);
  //@}

  //@{
  /**
   * Invert a 3x3 matrix. The input matrix is A. The output is
   * stored in AI.
   */
  static void Invert3x3(const float A[3][3], float AI[3][3]);
  static void Invert3x3(const double A[3][3], double AI[3][3]);
  //@}

  //@{
  /**
   * Set A to the identity matrix.
   */
  static void Identity3x3(float A[3][3]);
  static void Identity3x3(double A[3][3]);
  //@}

  //@{
  /**
   * Return the determinant of a 3x3 matrix.
   */
  static double Determinant3x3(const float A[3][3]);
  static double Determinant3x3(const double A[3][3]);
  //@}

  /**
   * Compute determinant of 3x3 matrix. Three columns of matrix are input.
   */
  static float Determinant3x3(const float c1[3],
                              const float c2[3],
                              const float c3[3]);

  /**
   * Compute determinant of 3x3 matrix. Three columns of matrix are input.
   */
  static double Determinant3x3(const double c1[3],
                               const double c2[3],
                               const double c3[3]);

  /**
   * Calculate the determinant of a 3x3 matrix in the form:
   * | a1,  b1,  c1 |
   * | a2,  b2,  c2 |
   * | a3,  b3,  c3 |
   */
  static double Determinant3x3(double a1, double a2, double a3,
                               double b1, double b2, double b3,
                               double c1, double c2, double c3);

  //@{
  /**
   * Convert a quaternion to a 3x3 rotation matrix.  The quaternion
   * does not have to be normalized beforehand.
   * The quaternion must be in the form [w, x, y, z].
   * @sa Matrix3x3ToQuaternion() MultiplyQuaternion()
   * @sa vtkQuaternion
   */
  static void QuaternionToMatrix3x3(const float quat[4], float A[3][3]);
  static void QuaternionToMatrix3x3(const double quat[4], double A[3][3]);
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Multiply two quaternions. This is used to concatenate rotations.
   * Quaternions are in the form [w, x, y, z].
   * @sa Matrix3x3ToQuaternion() QuaternionToMatrix3x3()
   * @sa vtkQuaternion
   */
  static void MultiplyQuaternion( const float q1[4], const float q2[4],  float q[4] );
  static void MultiplyQuaternion( const double q1[4], const double q2[4],  double q[4] );
  //@}

  //@{
  /**
   * rotate a vector by a normalized quaternion
   * using // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
   */
  static void RotateVectorByNormalizedQuaternion(const float v[3], const float q[4], float r[3]);
  static void RotateVectorByNormalizedQuaternion(const double v[3], const double q[4], double r[3]);
  //@}

  //@{
  /**
   * rotate a vector by WXYZ
   * using // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
   */
  static void RotateVectorByWXYZ(const float v[3], const float q[4], float r[3]);
  static void RotateVectorByWXYZ(const double v[3], const double q[4], double r[3]);
  //@}

  //@{
  /**
   * Orthogonalize a 3x3 matrix and put the result in B.  If matrix A
   * has a negative determinant, then B will be a rotation plus a flip
   * i.e. it will have a determinant of -1.
   */
  static void Orthogonalize3x3(const float A[3][3], float B[3][3]);
  static void Orthogonalize3x3(const double A[3][3], double B[3][3]);
  //@}

  //@{
  /**
   * Diagonalize a symmetric 3x3 matrix and return the eigenvalues in
   * w and the eigenvectors in the columns of V.  The matrix V will
   * have a positive determinant, and the three eigenvectors will be
   * aligned as closely as possible with the x, y, and z axes.
   */
  static void Diagonalize3x3(const float A[3][3], float w[3], float V[3][3]);
  static void Diagonalize3x3(const double A[3][3],double w[3],double V[3][3]);
  //@}

  //@{
  /**
   * Perform singular value decomposition on a 3x3 matrix.  This is not
   * done using a conventional SVD algorithm, instead it is done using
   * Orthogonalize3x3 and Diagonalize3x3.  Both output matrices U and VT
   * will have positive determinants, and the w values will be arranged
   * such that the three rows of VT are aligned as closely as possible
   * with the x, y, and z axes respectively.  If the determinant of A is
   * negative, then the three w values will be negative.
   */
  static void SingularValueDecomposition3x3(const float A[3][3],
                                            float U[3][3], float w[3],
                                            float VT[3][3]);
  static void SingularValueDecomposition3x3(const double A[3][3],
                                            double U[3][3], double w[3],
                                            double VT[3][3]);
  //@}

  /**
   * Solve linear equations Ax = b using Crout's method. Input is square
   * matrix A and load vector x. Solution x is written over load vector. The
   * dimension of the matrix is specified in size. If error is found, method
   * returns a 0.
   */
  static vtkTypeBool SolveLinearSystem(double **A, double *x, int size);

  /**
   * Invert input square matrix A into matrix AI.
   * Note that A is modified during
   * the inversion. The size variable is the dimension of the matrix. Returns 0
   * if inverse not computed.
   */
  static vtkTypeBool InvertMatrix(double **A, double **AI, int size);

  /**
   * Thread safe version of InvertMatrix method.
   * Working memory arrays tmp1SIze and tmp2Size
   * of length size must be passed in.
   */
  static vtkTypeBool InvertMatrix(double **A, double **AI, int size,
                                  int *tmp1Size, double *tmp2Size);

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
  static vtkTypeBool LUFactorLinearSystem(double **A, int *index, int size);

  /**
   * Thread safe version of LUFactorLinearSystem method.
   * Working memory array tmpSize of length size
   * must be passed in.
   */
  static vtkTypeBool LUFactorLinearSystem(double **A, int *index, int size,
                                          double *tmpSize);

  /**
   * Solve linear equations Ax = b using LU decomposition A = LU where L is
   * lower triangular matrix and U is upper triangular matrix. Input is
   * factored matrix A=LU, integer array of pivot indices index[0->n-1],
   * load vector x[0->n-1], and size of square matrix n. Note that A=LU and
   * index[] are generated from method LUFactorLinearSystem). Also, solution
   * vector is written directly over input load vector.
   */
  static void LUSolveLinearSystem(double **A, int *index,
                                  double *x, int size);

  /**
   * Estimate the condition number of a LU factored matrix. Used to judge the
   * accuracy of the solution. The matrix A must have been previously factored
   * using the method LUFactorLinearSystem. The condition number is the ratio
   * of the infinity matrix norm (i.e., maximum value of matrix component)
   * divided by the minimum diagonal value. (This works for triangular matrices
   * only: see Conte and de Boor, Elementary Numerical Analysis.)
   */
  static double EstimateMatrixCondition(const double *const *A, int size);

  //@{
  /**
   * Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
   * real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
   * and output eigenvectors in v arranged column-wise. Resulting
   * eigenvalues/vectors are sorted in decreasing order; the most positive
   * eigenvectors are selected for consistency; eigenvectors are normalized.
   * NOTE: the input matrix a is modified during the solution
   */
  static vtkTypeBool Jacobi(float **a, float *w, float **v);
  static vtkTypeBool Jacobi(double **a, double *w, double **v);
  //@}

  //@{
  /**
   * JacobiN iteration for the solution of eigenvectors/eigenvalues of a nxn
   * real symmetric matrix. Square nxn matrix a; size of matrix in n; output
   * eigenvalues in w; and output eigenvectors in v arranged column-wise.
   * Resulting eigenvalues/vectors are sorted in decreasing order; the most
   * positive eigenvectors are selected for consistency; and eigenvectors are
   * normalized. w and v need to be allocated previously.
   * NOTE: the input matrix a is modified during the solution
   */
  static vtkTypeBool JacobiN(float **a, int n, float *w, float **v);
  static vtkTypeBool JacobiN(double **a, int n, double *w, double **v);
  //@}

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
  static vtkTypeBool SolveHomogeneousLeastSquares(int numberOfSamples, double **xt,
                                                  int xOrder, double **mt);

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
  static vtkTypeBool SolveLeastSquares(int numberOfSamples, double **xt, int xOrder,
                                       double **yt, int yOrder, double **mt, int checkHomogeneous=1);

  //@{
  /**
   * Convert color in RGB format (Red, Green, Blue) to HSV format
   * (Hue, Saturation, Value). The input color is not modified.
   * The input RGB must be float values in the range [0, 1].
   * The output ranges are hue [0, 1], saturation [0, 1], and
   * value [0, 1].
   */
  static void RGBToHSV(const float rgb[3], float hsv[3]) {
    RGBToHSV(rgb[0], rgb[1], rgb[2], hsv, hsv+1, hsv+2);
  }
  static void RGBToHSV(float r, float g, float b, float *h, float *s, float *v);
  VTK_LEGACY(static double* RGBToHSV(const double rgb[3]) VTK_SIZEHINT(3));
  VTK_LEGACY(static double* RGBToHSV(double r, double g, double b) VTK_SIZEHINT(3));
  static void RGBToHSV(const double rgb[3], double hsv[3]) {
    RGBToHSV(rgb[0], rgb[1], rgb[2], hsv, hsv+1, hsv+2);
  }
  static void RGBToHSV(double r, double g, double b, double *h, double *s, double *v);
  //@}

  //@{
  /**
   * Convert color in HSV format (Hue, Saturation, Value) to RGB
   * format (Red, Green, Blue). The input color is not modified.
   * The input 'hsv' must be float values in the range [0, 1].
   * The elements of each component of the output 'rgb' are in
   * the range [0, 1].
   */
  static void HSVToRGB(const float hsv[3], float rgb[3]) {
    HSVToRGB(hsv[0], hsv[1], hsv[2], rgb, rgb+1, rgb+2);
  }
  static void HSVToRGB(float h, float s, float v, float *r, float *g, float *b);
  VTK_LEGACY(static double* HSVToRGB(const double hsv[3]) VTK_SIZEHINT(3));
  VTK_LEGACY(static double* HSVToRGB(double h, double s, double v) VTK_SIZEHINT(3));
  static void HSVToRGB(const double hsv[3], double rgb[3])
    { HSVToRGB(hsv[0], hsv[1], hsv[2], rgb, rgb+1, rgb+2); }
  static void HSVToRGB(double h, double s, double v, double *r, double *g, double *b);
  //@}

  //@{
  /**
   * Convert color from the CIE-L*ab system to CIE XYZ.
   */
  static void LabToXYZ(const double lab[3], double xyz[3]) {
    LabToXYZ(lab[0], lab[1], lab[2], xyz+0, xyz+1, xyz+2);
  }
  static void LabToXYZ(double L, double a, double b,
                       double *x, double *y, double *z);
  VTK_LEGACY(static double *LabToXYZ(const double lab[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Convert Color from the CIE XYZ system to CIE-L*ab.
   */
  static void XYZToLab(const double xyz[3], double lab[3]) {
    XYZToLab(xyz[0], xyz[1], xyz[2], lab+0, lab+1, lab+2);
  }
  static void XYZToLab(double x, double y, double z,
                       double *L, double *a, double *b);
  VTK_LEGACY(static double *XYZToLab(const double xyz[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Convert color from the CIE XYZ system to RGB.
   */
  static void XYZToRGB(const double xyz[3], double rgb[3]) {
    XYZToRGB(xyz[0], xyz[1], xyz[2], rgb+0, rgb+1, rgb+2);
  }
  static void XYZToRGB(double x, double y, double z,
                       double *r, double *g, double *b);
  VTK_LEGACY(static double *XYZToRGB(const double xyz[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Convert color from the RGB system to CIE XYZ.
   */
  static void RGBToXYZ(const double rgb[3], double xyz[3]) {
    RGBToXYZ(rgb[0], rgb[1], rgb[2], xyz+0, xyz+1, xyz+2);
  }
  static void RGBToXYZ(double r, double g, double b,
                       double *x, double *y, double *z);
  VTK_LEGACY(static double *RGBToXYZ(const double rgb[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Convert color from the RGB system to CIE-L*ab.
   * The input RGB must be values in the range [0, 1].
   * The output ranges of 'L' is [0, 100]. The output
   * range of 'a' and 'b' are approximately [-110, 110].
   */
  static void RGBToLab(const double rgb[3], double lab[3]) {
    RGBToLab(rgb[0], rgb[1], rgb[2], lab+0, lab+1, lab+2);
  }
  static void RGBToLab(double red, double green, double blue,
                       double *L, double *a, double *b);
  VTK_LEGACY(static double *RGBToLab(const double rgb[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Convert color from the CIE-L*ab system to RGB.
   */
  static void LabToRGB(const double lab[3], double rgb[3]) {
    LabToRGB(lab[0], lab[1], lab[2], rgb+0, rgb+1, rgb+2);
  }
  static void LabToRGB(double L, double a, double b,
                       double *red, double *green, double *blue);
  VTK_LEGACY(static double *LabToRGB(const double lab[3]) VTK_SIZEHINT(3));
  //@}

  //@{
  /**
   * Set the bounds to an uninitialized state
   */
  static void UninitializeBounds(double bounds[6]) {
    bounds[0] = 1.0;
    bounds[1] = -1.0;
    bounds[2] = 1.0;
    bounds[3] = -1.0;
    bounds[4] = 1.0;
    bounds[5] = -1.0;
  }
  //@}

  //@{
  /**
   * Are the bounds initialized?
   */
  static vtkTypeBool AreBoundsInitialized(const double bounds[6]) {
    if ( bounds[1] - bounds[0] < 0.0 )
    {
      return 0;
    }
    return 1;
  }
  //@}

  /**
   * Clamp some value against a range, return the result.
   * min must be less than or equal to max. Semantics the same as std::clamp.
   */
  template<class T>
  static T ClampValue(const T & value, const T & min, const T & max);

  //@{
  /**
   * Clamp some values against a range
   * The method without 'clamped_values' will perform in-place clamping.
   */
  static void ClampValue(double *value, const double range[2]);
  static void ClampValue(double value, const double range[2], double *clamped_value);
  static void ClampValues(
    double *values, int nb_values, const double range[2]);
  static void ClampValues(
    const double *values, int nb_values, const double range[2], double *clamped_values);
  //@}

  /**
   * Clamp a value against a range and then normalize it between 0 and 1.
   * If range[0]==range[1], the result is 0.
   * \pre valid_range: range[0]<=range[1]
   * \post valid_result: result>=0.0 && result<=1.0
   */
  static double ClampAndNormalizeValue(double value,
                                       const double range[2]);

  /**
   * Convert a 6-Component symmetric tensor into a 9-Component tensor, no allocation performed.
   * Symmetric tensor is expected to have the following order : XX, YY, ZZ, XY, YZ, XZ
   */
  template<class T1, class T2>
  static void TensorFromSymmetricTensor(const T1 symmTensor[6], T2 tensor[9]);

    /**
   * Convert a 6-Component symmetric tensor into a 9-Component tensor, overwriting
   * the tensor input.
   * Symmetric tensor is expected to have the following order : XX, YY, ZZ, XY, YZ, XZ
   */
  template<class T>
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
    double range_min, double range_max,
    double scale = 1.0, double shift = 0.0);

  /**
   * Get a vtkDataArray's scalar range for a given component.
   * If the vtkDataArray's data type is unsigned char (VTK_UNSIGNED_CHAR)
   * the range is adjusted to the whole data type range [0, 255.0].
   * Same goes for unsigned short (VTK_UNSIGNED_SHORT) but the upper bound
   * is also adjusted down to 4095.0 if was between ]255, 4095.0].
   * Return 1 on success, 0 otherwise.
   */
  static vtkTypeBool GetAdjustedScalarRange(
    vtkDataArray *array, int comp, double range[2]);

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
  static vtkTypeBool BoundsIsWithinOtherBounds(const double bounds1[6], const double bounds2[6], const double delta[3]);

  /**
   * Return true if point is within the given 3D bounds
   * Bounds is x-min, x-max, y-min, y-max, z-min, z-max
   * Delta is the error margin along each axis (usually a small number)
   */
  static vtkTypeBool PointIsWithinBounds(const double point[3], const double bounds[6], const double delta[3]);

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
    const double bounds[6],
    const double normal[3],
    const double point[3]);

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
    const double p1[3],
    const double p2[3],
    const double p3[3],
    double center[3]);

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
   * Test if a number has finite value i.e. it is normal, subnormal or zero, but not infinite or Nan.
   */
  static bool IsFinite(double x);
protected:
  vtkMath() {}
  ~vtkMath() override {}

  static vtkSmartPointer<vtkMathInternal> Internal;
private:
  vtkMath(const vtkMath&) = delete;
  void operator=(const vtkMath&) = delete;
};

//----------------------------------------------------------------------------
inline float vtkMath::RadiansFromDegrees( float x )
{
  return x * 0.017453292f;
}

//----------------------------------------------------------------------------
inline double vtkMath::RadiansFromDegrees( double x )
{
  return x * 0.017453292519943295;
}

//----------------------------------------------------------------------------
inline float vtkMath::DegreesFromRadians( float x )
{
  return x * 57.2957795131f;
}

//----------------------------------------------------------------------------
inline double vtkMath::DegreesFromRadians( double x )
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
  unsigned int z = ((x > 0) ? x - 1 : 0);
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
  return i - ( i > x );
}

//----------------------------------------------------------------------------
// Modify the trunc() operation provided by static_cast<int>() to get ceil(),
// Note that in C++ conditions evaluate to values of 1 or 0 (true or false).
inline int vtkMath::Ceil(double x)
{
  int i = static_cast<int>(x);
  return i + ( i < x );
}

//----------------------------------------------------------------------------
template<class T>
inline T vtkMath::Min(const T & a, const T & b)
{
  return (b <= a ? b : a);
}

//----------------------------------------------------------------------------
template<class T>
inline T vtkMath::Max(const T & a, const T & b)
{
  return (b > a ? b : a);
}

//----------------------------------------------------------------------------
inline float vtkMath::Normalize(float v[3])
{
  float den = vtkMath::Norm( v );
  if ( den != 0.0 )
  {
    for (int i=0; i < 3; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline double vtkMath::Normalize(double v[3])
{
  double den = vtkMath::Norm( v );
  if ( den != 0.0 )
  {
    for (int i=0; i < 3; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline float vtkMath::Normalize2D(float v[3])
{
  float den = vtkMath::Norm2D( v );
  if ( den != 0.0 )
  {
    for (int i=0; i < 2; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline double vtkMath::Normalize2D(double v[3])
{
  double den = vtkMath::Norm2D( v );
  if ( den != 0.0 )
  {
    for (int i=0; i < 2; ++i)
    {
      v[i] /= den;
    }
  }
  return den;
}

//----------------------------------------------------------------------------
inline float vtkMath::Determinant3x3(const float c1[3],
                                     const float c2[3],
                                     const float c3[3])
{
  return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
         c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const double c1[3],
                                      const double c2[3],
                                      const double c3[3])
{
  return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
         c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(double a1, double a2, double a3,
                                      double b1, double b2, double b3,
                                      double c1, double c2, double c3)
{
    return ( a1 * vtkMath::Determinant2x2( b2, b3, c2, c3 )
           - b1 * vtkMath::Determinant2x2( a2, a3, c2, c3 )
           + c1 * vtkMath::Determinant2x2( a2, a3, b2, b3 ) );
}

//----------------------------------------------------------------------------
inline float vtkMath::Distance2BetweenPoints(const float p1[3],
                                             const float p2[3])
{
  return ( ( p1[0] - p2[0] ) * ( p1[0] - p2[0] )
         + ( p1[1] - p2[1] ) * ( p1[1] - p2[1] )
         + ( p1[2] - p2[2] ) * ( p1[2] - p2[2] ) );
}

//----------------------------------------------------------------------------
inline double vtkMath::Distance2BetweenPoints(const double p1[3],
                                              const double p2[3])
{
  return ( ( p1[0] - p2[0] ) * ( p1[0] - p2[0] )
         + ( p1[1] - p2[1] ) * ( p1[1] - p2[1] )
         + ( p1[2] - p2[2] ) * ( p1[2] - p2[2] ) );
}

//----------------------------------------------------------------------------
// Cross product of two 3-vectors. Result (a x b) is stored in c[3].
inline void vtkMath::Cross(const float a[3], const float b[3], float c[3])
{
  float Cx = a[1] * b[2] - a[2] * b[1];
  float Cy = a[2] * b[0] - a[0] * b[2];
  float Cz = a[0] * b[1] - a[1] * b[0];
  c[0] = Cx; c[1] = Cy; c[2] = Cz;
}

//----------------------------------------------------------------------------
// Cross product of two 3-vectors. Result (a x b) is stored in c[3].
inline void vtkMath::Cross(const double a[3], const double b[3], double c[3])
{
  double Cx = a[1] * b[2] - a[2] * b[1];
  double Cy = a[2] * b[0] - a[0] * b[2];
  double Cz = a[0] * b[1] - a[1] * b[0];
  c[0] = Cx; c[1] = Cy; c[2] = Cz;
}

//----------------------------------------------------------------------------
template<class T>
inline double vtkDeterminant3x3(const T A[3][3])
{
  return A[0][0] * A[1][1] * A[2][2] + A[1][0] * A[2][1] * A[0][2] +
         A[2][0] * A[0][1] * A[1][2] - A[0][0] * A[2][1] * A[1][2] -
         A[1][0] * A[0][1] * A[2][2] - A[2][0] * A[1][1] * A[0][2];
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const float A[3][3])
{
  return vtkDeterminant3x3( A );
}

//----------------------------------------------------------------------------
inline double vtkMath::Determinant3x3(const double A[3][3])
{
  return vtkDeterminant3x3( A );
}

//----------------------------------------------------------------------------
template<class T>
inline T vtkMath::ClampValue(const T & value, const T & min, const T & max)
{
  assert("pre: valid_range" && min<=max);

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
inline void vtkMath::ClampValue(double *value, const double range[2])
{
  if (value && range)
  {
    assert("pre: valid_range" && range[0]<=range[1]);

    *value = vtkMath::ClampValue(*value, range[0], range[1]);
  }
}

//----------------------------------------------------------------------------
inline void vtkMath::ClampValue(
  double value, const double range[2], double *clamped_value)
{
  if (range && clamped_value)
  {
    assert("pre: valid_range" && range[0]<=range[1]);

    *clamped_value = vtkMath::ClampValue(value, range[0], range[1]);
  }
}

// ---------------------------------------------------------------------------
inline double vtkMath::ClampAndNormalizeValue(double value,
                                              const double range[2])
{
  assert("pre: valid_range" && range[0]<=range[1]);

  double result;
  if (range[0] == range[1])
  {
      result = 0.0;
  }
  else
  {
      // clamp
      result=vtkMath::ClampValue(value, range[0], range[1]);

      // normalize
      result=( result - range[0] ) / ( range[1] - range[0] );
  }

  assert("post: valid_result" && result>=0.0 && result<=1.0);

  return result;
}

//-----------------------------------------------------------------------------
template<class T1, class T2>
inline void vtkMath::TensorFromSymmetricTensor(const T1 symmTensor[9], T2 tensor[9])
{
  for (int i = 0; i < 3; ++i)
  {
    tensor[4*i] = symmTensor[i];
  }
  tensor[1] = tensor[3] = symmTensor[3];
  tensor[2] = tensor[6] = symmTensor[5];
  tensor[5] = tensor[7] = symmTensor[4];
}

//-----------------------------------------------------------------------------
template<class T>
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

namespace vtk_detail
{
// Can't specialize templates inside a template class, so we move the impl here.
template <typename OutT>
void RoundDoubleToIntegralIfNecessary(double val, OutT* ret)
{ // OutT is integral -- clamp and round
  double min = static_cast<double>(vtkTypeTraits<OutT>::Min());
  double max = static_cast<double>(vtkTypeTraits<OutT>::Max());
  val = vtkMath::ClampValue(val, min, max);
  *ret = static_cast<OutT>((val >= 0.0) ? (val + 0.5) : (val - 0.5));
}
template <>
inline void RoundDoubleToIntegralIfNecessary(double val, double* retVal)
{ // OutT is double: passthrough
  *retVal = val;
}
template <>
inline void RoundDoubleToIntegralIfNecessary(double val, float* retVal)
{ // OutT is float -- just clamp (as doubles, then the cast to float is well-defined.)
  double min = static_cast<double>(vtkTypeTraits<float>::Min());
  double max = static_cast<double>(vtkTypeTraits<float>::Max());
  val = vtkMath::ClampValue(val, min, max);
  *retVal = static_cast<float>(val);
}
} // end namespace vtk_detail

//-----------------------------------------------------------------------------
#if defined(VTK_HAS_ISINF) || defined(VTK_HAS_STD_ISINF)
#define VTK_MATH_ISINF_IS_INLINE
inline vtkTypeBool vtkMath::IsInf(double x)
{
#if defined(VTK_HAS_STD_ISINF)
  return std::isinf(x);
#else
  return (isinf(x) != 0); // Force conversion to bool
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
  return (isnan(x) != 0); // Force conversion to bool
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

#endif
