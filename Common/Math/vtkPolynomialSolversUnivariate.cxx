/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolynomialSolversUnivariate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov

=========================================================================*/
#include "vtkPolynomialSolversUnivariate.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkMath.h"

#ifndef isnan
// This is compiler specific not platform specific: MinGW doesn't need that.
# if defined(_MSC_VER) || defined(__BORLANDC__)
#  include <float.h>
#  define isnan(x) _isnan(x)
# endif
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined (__SUNPRO_C) || defined(__SUNPRO_CC)
# define fmax(a,b) ( (a) >= (b) ? (a) : (b) )
#endif

#define VTK_SIGN(x)              (( (x) < 0 )?( -1 ):( 1 ))

vtkStandardNewMacro(vtkPolynomialSolversUnivariate);

static const double sqrt3 = sqrt( static_cast<double>( 3. ) );
static const double inv3 = 1 / 3.;
static const double absolute0 = 10. * VTK_DBL_MIN;

double vtkPolynomialSolversUnivariate::DivisionTolerance = 1e-8;//sqrt( VTK_DBL_EPSILON );

//----------------------------------------------------------------------------
void vtkPolynomialSolversUnivariate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "(s) DivisionTolerance: " << this->GetDivisionTolerance() << "\n";
}

//----------------------------------------------------------------------------
// Print the polynomial for debuggery
ostream& vtkPolynomialSolversUnivariate::PrintPolynomial( ostream& os, double* P, int degP )
{
  os << "\n";
  os << "The polynomial has degree " << degP << "\n";

  if ( degP < 0 )
    {
    os << "0\n";
    return os;
    }

  if ( degP == 0 )
    {
    os << P[0] << "\n";
    return os;
    }

  unsigned int degPm1 = degP - 1;
  for ( unsigned int i = 0; i < degPm1; ++ i )
    {
    if ( P[i] > 0 )
      {
      if ( i )
        {
        os << "+";
        }
      if ( P[i] != 1. )
        {
        os << P[i] << "*";
        }
      os << "x**" << degP - i;
      }
    else if ( P[i] < 0 )
      {
      os << P[i] << "*x**" << degP - i;
      }
    }

  if ( degP > 0 )
    {
    if ( P[degPm1] > 0 )
      {
      os << "+" << P[degPm1] << "*x";
      }
    else if ( P[degPm1] < 0 )
      {
      os << P[degPm1] << "*x";
      }
    }

  if ( P[degP] > 0 )
    {
    os << "+" << P[degP];
    }
  else if ( P[degP] < 0 )
    {
    os << P[degP];
    }

  os << "\n";
  return os;
}

//----------------------------------------------------------------------------
// Double precision comparison with 0
inline bool IsZero( double x )
{
  return ( fabs( x ) < absolute0 ) ? true : false;
}

//----------------------------------------------------------------------------
// Double precision comparison
inline bool AreEqual( double x, double y, double rTol )
{
  double delta = fabs( x - y );

  // First, handle "absolute" zeros. This is to eliminate the
  // case (x+t - -x ) / x = 2 + t/x even when both x and t are small.
  if (  delta < absolute0 )
    {
    return true;
    }

  // Second, handle "relative" equalities.
  double absx = fabs( x );
  double absy = fabs( y );
  if ( absx > absy )
    {
    return delta > rTol * absx ? false : true;
    }
  else
    {
    return delta > rTol * absy ? false : true;
    }
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
static int polynomialEucliDiv(
  double* A, int m, double* B, int n, double* Q, double* R, double rtol )
{
  // Note: for execution speed, no sanity checks are performed on A and B.
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    Q[0] = 0.;
    for ( i = 0; i <= m; ++ i )
      {
      R[i] = A[i];
      }

    return m;
    }

  double iB0 = 1. / B[0];
  if ( ! n )
    {
    for ( i = 0; i <= m; ++ i )
      {
      Q[i] = A[i] * iB0;
      }

    return -1;
    }

  int nj;
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j )
      {
      Q[i] -= B[j] * Q[i - j];
      }
    Q[i] *= iB0;
    }

  bool nullCoeff = false;
  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    double sum = 0;
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j )
      {
      sum += B[n - i + 1 + j] * Q[mMn - j];
      }

    if ( ! AreEqual( A[m - i + 1], sum, rtol ) )
      {
      R[n - i] = A[m - i + 1] - sum;
      r = i - 1;
      }
    else
      {
      R[n - i] = 0.;
      if ( n == i )
        {
        nullCoeff = true;
        }
      }
    }

  if ( ! r && nullCoeff )
    {
    return -1;
    }

  return r;
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
// Does not store Q and stores -R instead of R
static int polynomialEucliDivOppositeR(
  double* A, int m, double* B, int n, double* mR, double rtol )
{
  // Note: for execution speed, no sanity checks are performed on A and B.
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    for ( i = 0; i <= m; ++ i )
      {
      mR[i] = A[i];
      }
    return m;
    }

  if ( ! n )
    {
    return -1;
    }

  int nj;
  double iB0 = 1. / B[0];
  double* Q = new double[mMn + 1];
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j )
      {
      Q[i] -= B[j] * Q[i - j];
      }
    Q[i] *= iB0;
    }

  bool nullCoeff = false;
  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    double sum = 0;
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j )
      {
      sum += B[n - i + 1 + j] * Q[mMn - j];
      }

    if ( ! AreEqual( A[m - i + 1], sum, rtol ) )
      {
      mR[n - i] = sum - A[m - i + 1];
      r = i - 1;
      }
    else
      {
      mR[n - i] = 0.;
      if ( n == i )
        {
        nullCoeff = true;
        }
      }
    }
  delete [] Q;

  if ( ! r && nullCoeff )
    {
    r = -1;
    }

  return r;
}

//----------------------------------------------------------------------------
inline double vtkNormalizePolyCoeff( double d, double* div = 0 )
{
  static const double high = 18446744073709551616.; // 2^64
  static const double reallyBig = 1.e300;
  static const double reallyBigInv = 1 / reallyBig;
  static const double notThatBig = 1.e30;
  static const double notThatBigInv = 1.e-30;

  if ( fabs( d ) < reallyBig )
    {
    while ( fabs( d ) > notThatBig )
      {
      d /= high;
      if ( div )
        {
        *div /= high;
        }
      }
    }
  if ( fabs( d ) > reallyBigInv )
    {
    while(fabs( d ) < notThatBigInv )
      {
      d *= high;
      if ( div )
        {
        *div *= high;
        }
      }
    }
  return d;
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
// Does not store Q and stores -R instead of R. This premultiplies Ai by mul and
// then divides mR by div. mR MUST have at least the size of m+1, because it is
// used as temporary storage besides as the return value.
// This shouldn't be too  much slower than the version without mul and div. It
// has to copy mul*Ai into mR at the beginning. So it has m+1 extra
// multiplications and assignments. When it assigns final values into mR it
// divides by div. So it has deg(mR) extra divisions.
static int polynomialEucliDivOppositeR(
  double mul, double* Ai, int m, double* B, int n, double div, double* mR,
  double rtol )
{
  // Note: for execution speed, no sanity checks are performed on A and B.
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  // To save space we will use mR *instead* of Ai. Just have to be sure that
  // when we write to mR we are writing to a spot that will not be used again.
  for ( i = 0; i <= m; ++ i )
    {
    mR[i] = mul*Ai[i];
    }

  // Remember that mMn >= 0 implies m-n >= 0 implies m  >= n and so
  // m + 1 > n.
  if ( mMn < 0 )
    {
    return m;
    }

  if ( ! n )
    {
    return -1;
    }

  div = 1 / div;
  int nj;
  double iB0 = 1. / B[0];
  double* Q = new double[mMn + 1];

  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = mR[i];
    for ( int j = 1; j <= nj; ++ j )
      {
      Q[i] -= B[j] * Q[i - j];
      }
    Q[i] *= iB0;
    }

  bool nullCoeff = false;
  int r = 0;
  for ( i = n; i >= 1; -- i )
    {
    double sum = 0;
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j )
      {
      sum += B[n - i + 1 + j] * Q[mMn - j];
      }

    if ( ! AreEqual( mR[m - i + 1], sum, rtol ) )
      {
      // Now we have m + 1 > n implies n - i < m - i + 1. Thus since i is
      // decreasing (hence m - i + 1 increasing) we will never use n - i again,
      // so we can safely write over it.
      mR[n - i] = ( sum - mR[m - i + 1] ) * div;
      // We want the non-zero term with the largest index. So we only write to r
      // once.
      if ( r == 0 )
        {
        mR[n-i] = vtkNormalizePolyCoeff( mR[n - i], &div );
        r = i - 1;
        }
      }
    else
      {
      // See previous comment.
      mR[n - i] = 0.;
      if ( n == i )
        {
        nullCoeff = true;
        }
      }
    }
  delete [] Q;

  if ( ! r && nullCoeff )
    {
    r = -1;
    }

  return r;
}




//----------------------------------------------------------------------------
// Evaluate the value of the degree d univariate polynomial P at x
// using Horner's algorithm.
inline double evaluateHorner( double* P, int d, double x )
{
  if ( d == -1 )
    {
    return 0.;
    }
  double val = P[0];
  for ( int i = 1; i <= d; ++ i )
    {
    val = val * x + P[i];
    }

  return val;
}

int vtkGetSignChanges(
  double* P, int* degP, int* offsets, int count, double val, int* fsign = 0 )
{
  int oldVal = 0;
  double v;
  int changes = 0;

  for ( int i = 0; i < count; ++ i )
    {
    v = evaluateHorner( P + offsets[i], degP[i], val );

    if ( fsign && i == 0 )
      {
      if ( IsZero( v ) )
        {
        *fsign = 0;
        }
      else if ( v > 0 )
        {
        *fsign = 1;
        }
      else
        {
        *fsign = -1;
        }
      }

    if ( v == 0 )
      {
      continue;
      }

    if ( v * oldVal < 0 )
      {
      ++ changes;
      oldVal = -oldVal;
      }

    if ( oldVal == 0 )
      {
      oldVal = ( v < 0 ? -1 : 1 );
      }
    }

  return changes;
}


// ----------------------------------------------------------
// Gets the Habicht sequence. SSS and degrees and ofsets are expected to be
// large enough and the number of non-zero items is returned. P is expected to
// have degree at least 1.
//
// This algorithm is modified from
// BPR, Algorithms in Real Algebraic Geometry, page 318
int vtkGetHabichtSequence(
  double* P, int d, double * SSS, int* degrees, int* offsets, double rtol )
{
  //const static double high = pow(2.,256);
  degrees[0] = d;
  offsets[0] = 0;

  int dp1 = d+1;
  double* t = new double[dp1];
  double* s = new double[dp1];

  degrees[1] = d-1;
  offsets[1] = dp1;
  int offset = dp1;

  // Set the first two elements SSS = {P, P'}.
  for ( int m = 0; m < d; ++ m )
    {
    SSS[m] = P[m];
    SSS[m + offset] = static_cast<double>( d - m ) * SSS[m];
    }
  SSS[d] = P[d];
  t[0] = s[0] = ( P[0] > 0. ? 1. : -1. );
  t[1] = s[1] = SSS[offset];

  int j = 0;
  int k = 0;

  int deg = d;
  int degree = d - 1;
  int jp1 = 1;
  int ip1 = 0;
  while ( degree > 0  && j < d - 1 )
    {
    k = deg - degree;
    if ( k == jp1 )
      {
      s[jp1] = t[jp1];

      degrees[k+1] = polynomialEucliDivOppositeR(
        s[jp1] * s[jp1], SSS + offsets[ip1], degrees[ip1], SSS + offset,
        degree, s[j] * t[ip1], SSS + offsets[k] + degree + 1, rtol );
      offsets[k+1] = offset + 2 * degree - degrees[k + 1];
      }
    else
      {
      s[jp1] = 0;
      for( int delta = 1; delta < k - j; ++ delta )
        {
        t[jp1 + delta] = ( t[jp1] * t[j + delta]) / s[j];
        t[jp1 + delta] = vtkNormalizePolyCoeff( t[jp1 + delta] );
        if ( delta % 2 )
          {
          t[jp1 + delta] *= -1;
          }
        }
      s[k] = t[k];

      offsets[k] = offsets[jp1] + degrees[jp1] + 1;
      degrees[k] = degrees[jp1];
      for ( int dg = 0; dg <= degree; ++ dg )
        {
        SSS[offsets[k] + dg] = ( s[k] * SSS[offset + dg] ) / t[jp1];
        }

      for ( int l = j+2; l < k; ++ l )
        {
        degrees[l] = -1;
        offsets[l] = offsets[k];
        s[l] = 0;
        }

      degrees[k + 1] = polynomialEucliDivOppositeR(
        t[jp1] * s[k], SSS + offsets[ip1], degrees[ip1], SSS + offset,
        degree, s[j] * t[ip1], SSS + offsets[k] + degrees[k] + 1, rtol );
      offsets[k + 1] = offsets[k] + 2 * degrees[k] - degrees[k + 1];
      }
    t[k + 1] = SSS[offsets[k + 1]];
    ip1 = jp1;
    j = k;
    jp1 = j+1;
    degree = degrees[jp1];
    offset = offsets[jp1];
    }

  delete [] s;
  delete [] t;

  if ( degree == 0 )
    {
    return jp1 + 1;
    }
  else
    {
    while ( degrees[jp1] < 0 )
      {
      -- jp1;
      }
    return jp1 + 1;
    }
}

// ----------------------------------------------------------
// Gets the sturm sequence. SSS and degrees and offsets are expected to
// be large enough and the number of non-zero items
// is returned. P is expected to have degree at least 1.
int vtkGetSturmSequence(
  double* P, int d, double* SSS, int* degrees, int* offsets, double rtol )
{
  degrees[0] = d;
  offsets[0] = 0;

  int dp1 = d + 1;
  int dm1 = d - 1;
  degrees[1] = dm1;
  offsets[1] = dp1;
  int offset = dp1;

  // nSSS will keep track of the number of the index of
  // the last item in our list.
  int nSSS = 1;

  // Set the first two elements SSS = {P, P'}.
  for ( int k = 0; k < d; ++ k )
    {
    SSS[k] = P[k];
    SSS[k + offset] = static_cast<double>( d - k ) * P[k];
    }
  SSS[d] = P[d];

  int degree = dm1;
  while ( degrees[nSSS] > 0 )
    {
    nSSS++;
    degrees[nSSS] = polynomialEucliDivOppositeR(
      SSS + offsets[nSSS - 2], degrees[nSSS - 2],
      SSS + offset, degree, SSS + offset + degree + 1, rtol );

    offsets[nSSS] = offset + 2 * degree - degrees[nSSS];

    offset = offsets[nSSS];
    degree = degrees[nSSS];
    }


  // If the last element is zero then we ignore it.
  if ( degrees[nSSS] < 0 )
    {
    return nSSS;
    }

  // Otherwise we include it in our count, because it
  // is a constant. (We are returning the number of
  // nonzero items, so 1 plus the index of the last).
  return nSSS + 1;
}

extern "C" {

int vtkPolynomialSolversUnivariateCompareRoots( const void* a, const void* b )
{
  return ( *( (const double*) a ) ) < ( *( (const double*) b ) ) ? -1 : 1;
}

} // extern "C"


// ------------------------------------------------------------
// upperBnds is expected to be large enough.
// intervalType finds roots as follows
// 0 = ]a,b[
// 1 = [a,b[
// 2 = ]a,a]
// 3 = [a,b]
//
// divideGCD uses an integer in case in the future someone wants to add logic to
// whether the gcd is divided. For example something like
// divideGCD == 0 -> never divide
// divideGCD == 1 -> use logic
// divideGCD == 2 -> divide as long as non-constant (ignore the logic).
//
// It probably would have been better to originally have had the tolerance be a
// relative tolerance rather than an absolute tolerance.
int vtkHabichtOrSturmBisectionSolve(
  double* P, int d, double* a, double* upperBnds, double tol,
  int intervalType, int divideGCD, int method )
{
  // Pretend to be one solver or the other (for error reporting)
  static const char title1[] = "vtkPolynomialSolversUnivariate::SturmBisectionSolve";
  static const char title2[] = "vtkPolynomialSolversUnivariate::HabichtBisectionSolve";
  const char* title = ( method == 0 ? title1 : title2 );

  // 0. Stupidity checks
  if ( tol <= 0 )
    {
    vtkGenericWarningMacro(<< title << ": Tolerance must be positive" );
    return -1;
    }

  if ( IsZero( P[0] ) )
    {
    vtkGenericWarningMacro(<< title << ": Zero leading coefficient" );
    return -1;
    }

  if ( d < 1 )
    {
    vtkGenericWarningMacro(<< title << ": Degree (" << d << ") < 1" );
    return -1;
    }

  if ( a[1] < a[0] + tol )
    {
    vtkGenericWarningMacro(<< title << ": Erroneous interval endpoints and/or tolerance" );
    return -1;
    }

  // Check for 0 as a root and reduce the degree if so.
  bool zeroroot = false;
  if ( IsZero(  P[d] ) )
    {
    zeroroot = true;
    while ( IsZero( P[d] ) )
      {
      -- d;
      }
    }

  // Take care of constant polynomials and polynomials of the form a*x^d.
  if ( d == 0 )
    {
    if ( zeroroot )
      {
      upperBnds[0] = 0.;
      return 1;
      }
    else
      {
      return 0;
      }
    }

  // Create one large array to hold all the
  // polynomials.
  //
  // We need two extra spaces because the habicht division uses R to temporarily
  // hold mul*A. So there must be at least size(A) space when dividing, which can be
  // 2 floats larger than R.
  double* SSS = new double[( ( d + 1 ) * ( d + 2 ) ) / 2 + 2];
  int* degrees = new int[d + 1];
  int* offsets = new int[d + 1];

  double bounds[] = { a[0], a[1] };
  int nSSS;

  if ( method == 0 )
    {
    nSSS = vtkGetSturmSequence( P, d, SSS, degrees, offsets, vtkPolynomialSolversUnivariate::GetDivisionTolerance() );
    }
  else
    {
    nSSS = vtkGetHabichtSequence( P, d, SSS, degrees, offsets, vtkPolynomialSolversUnivariate::GetDivisionTolerance() );
    }

  // If degrees[count-1] > 0 then we have degenerate roots.
  // We could possibly then find the degenerate roots.
  // Maybe we could more or less remove the degenerate roots.
  // Lets do some testwork to see.
  if ( degrees[nSSS-1] > 0 && divideGCD == 1 )
    {
    double* R = new double[d + 1];
    double* Q = new double[d + 1];

    // Get the quotient and call this function again using the
    // quotient.
    int deg = polynomialEucliDiv(
      SSS, d, SSS + offsets[nSSS - 1], degrees[nSSS - 1], Q, R,
      vtkPolynomialSolversUnivariate::GetDivisionTolerance() );
    deg = d - degrees[nSSS - 1];


    delete [] R;
    // The Habicht sequence will occasionally get infinite coeffs and cause
    // unpleasant events to happen with the sequence. In that case Q[0] == 0, thus
    // the division is not used.
    if ( ! IsZero( Q[0] ) )
      {
      delete [] SSS;
      delete [] degrees;
      delete [] offsets;

      int rval = vtkHabichtOrSturmBisectionSolve( Q, deg, a, upperBnds, tol, intervalType, 0, method );
      delete [] Q;
      if ( zeroroot )
        {
        upperBnds[rval] = 0;
        return rval + 1;
        }
      return rval;
      }
    else
      {
      delete Q;
      }
    }

  // Move away from zeros on the edges. We can also slightly speed up
  // computation by keeping the fact that these are roots and
  // continuing on.

  // If perturbation is too small compared to the bounds then we won't move when
  // we perturb.
  double perturbation =
    fmax(
      fmax( fabs( bounds[0] ) * 1e-12, fabs( bounds[1] ) * 1e-12 ),
      .5 * tol / static_cast<double>( d ) );

  int varSgn[] = { 0, 0 };
  varSgn[0] = vtkGetSignChanges( SSS, degrees, offsets, nSSS, bounds[0] );
  varSgn[1] = vtkGetSignChanges( SSS, degrees, offsets, nSSS, bounds[1] );

  for ( int k = 0; k <= 1; ++ k )
    {
    if ( IsZero(evaluateHorner( SSS, d, bounds[k] ) ) )
      {
      int leftVarSgn = varSgn[k];
      int rightVarSgn = varSgn[k];
      double leftx = bounds[k];
      double rightx = bounds[k];
      // Make sure we move far enough away that everything still works. That is
      // we needs to be non-zero and have the sequence realize that we've
      // got a zero in the interval. It is possible that this causes our code
      // to slow down. Should probably play around with how large the
      // perturbation should be. Also it is possible (but unlikely) that our inexact
      // sturm sequence doesn't realize there is a zero here.
      //
      // JUST AS WITH THE BISECTING, NEED TO MAKE SURE WE DON'T HAVE AN
      // INFINITE LOOP.
      while (
        IsZero( evaluateHorner( SSS, d, leftx ) ) ||
        IsZero( evaluateHorner( SSS, d, rightx ) ) ||
        leftVarSgn <= rightVarSgn ||
        ( ( leftVarSgn == varSgn[k] || rightVarSgn == varSgn[k] ) && leftVarSgn - rightVarSgn != 1 ) )
        {
        leftx -= perturbation;
        rightx += perturbation;
        leftVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, leftx );
        rightVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, rightx );
        }

      // Move properly according to what kind of sequence we are searching.
      if ( ( ! ( intervalType & 2 ) && k==1 ) || ( ( intervalType & 1 ) && k == 0 ) )
        {
        bounds[k] = leftx;
        varSgn[k] = leftVarSgn;
        }
      else
        {
        bounds[k] = rightx;
        varSgn[k] = rightVarSgn;
        }
      }
    }

  // If we don't have roots then leave here.
  int nRoots = varSgn[0] - varSgn[1];
  if ( nRoots < 1 )
    {
    upperBnds[0] = 0;
    delete [] SSS;
    delete [] degrees;
    delete [] offsets;
    if ( zeroroot )
      return 1;
    return 0;
    }

  // 2. Root bracketing

  // Initialize the bounds for the root intervals. The interval
  // ]lowerBnds[i], upperBnds[i][ contains the i+1 root. We will
  // see if we can completely separate the roots. Of course
  // the interval ]bounds[0], bounds[1][ contains all the roots.
  // Afterwards if some intervals are the same all but
  // one will be removed.
  int i;
  double* lowerBnds = new double[nRoots];

  for ( i = 0; i < nRoots; ++ i )
    {
    upperBnds[i] = bounds[1];
    lowerBnds[i] = bounds[0];
    }

  int leftVarSgn, rightVarSgn, tempSgn;
  double leftx, rightx;
  bool hitroot;
  int nloc = nRoots - 1;
  while ( nloc >= 1)
    {
    // Only one root according to Sturm or the interval is
    // small enough to consider the same root.
    if (
      upperBnds[nloc] - lowerBnds[nloc] <= tol ||
      ( ( nloc < 1 || ( upperBnds[nloc-1] < lowerBnds[nloc] - tol ) )
        && ( ( nloc >= nRoots - 1 ) || ( upperBnds[nloc] ) < lowerBnds[nloc + 1] - tol ) ) )
      {
      -- nloc;
      continue;
      }


    // We begin with leftx and rightx being equal and change them only if
    // leftx (rightx) is a root. Then we can bracket the root. We do this
    // because roots can cause problems (our sequence is inexact so
    // even single roots can cause problems. Furthermore if we hit a

    // root we may as well bracket it within tol so that we don't have to
    // worry about it later.
    leftx = ( upperBnds[nloc] + lowerBnds[nloc] ) / 2;
    // If we are going nowhere then quit.
    if ( leftx >= upperBnds[nloc] || leftx <= lowerBnds[nloc] )
      {
      nloc--;
      continue;
      }
    rightx = leftx;
    hitroot = false;

    leftVarSgn = rightVarSgn = tempSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, rightx );

    // Move away if we have a root, just like we did with the initial endpoints.
    // Maybe could write a function to do this, but it may be slower.
    // After moving away we want the following things to be handled:
    // 1 Not zero at the endpoints (leftx or rightx)
    // 2 Sign changes at left != sign changes at right.
    // 3 No "crazy" values:
    //   a. sign[left] > sign[0]
    //   b. sign[right] < sign[1]
    //   c. sign[right] > sign[left].
    // 4 Does not take too long.
    //
    // It would be convenient if we could get sign[left]-sign[right] = 1, but
    // in reality we may be too close for our values to even be worthwhile.
    //
    // The question remains, "What happens if we move away too far?" There must
    // be some distance after which we quit perturbing and deal with the fact
    // that our sequence is not perfect. This will take care of 4, but we still
    // need to handle what has just happened.
    //
    // Option 2: In this step we are only worried about bracketing the
    // roots. If the midpoint is a root, we could try this. Suppose a=0, b=1.
    // Then if we get 1/2 is a root, we check whether 1/4 and 3/4 work. If
    // one of them does then call that our "mid". Then in the next step we
    // will be moved away from 1/2. If neither of them work then we could
    // do what we are doing here, or try 1/8, 3/8, 5/8, and 7/8. In this latter
    // case we should make sure we don't come back to that messy point later.
    // This option removes the use of one of leftx or rightx.
    // So we need to have the following not happen.
    //
    // 1. Not zero at mid.
    // 2. No "crazy" valus:
    //    a. sign[mid] > sign[0]
    //    b. sign[mid] < sign[1].
    // 3. Does no take too long.
    //
    // This method is better because the "perturbation" is always relative to
    // the endpoints. Unlike the other method, because the perturbation
    // variable is relative to the larger endpoing.
    if (
      IsZero( leftx ) ||
      IsZero( evaluateHorner( SSS, d, leftx ) ) ||
      ( tempSgn > varSgn[0] ) ||
      ( tempSgn < nloc ) )
      {
      int step = 2;
      int pos = 1;
      double p2 = 4.;
      double mid = upperBnds[nloc] / p2 + ( p2 - pos ) * lowerBnds[nloc] / p2;
      bool found = false;
      leftVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, lowerBnds[nloc] );
      rightVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, upperBnds[nloc] );
      tempSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, mid );
      while (
        // 3.
        step < 10 &&
        // 2a.
        ( (tempSgn > leftVarSgn )  ||
        // 2b.
          ( tempSgn < rightVarSgn) ||
        // 1
          IsZero( evaluateHorner( SSS, d, mid ) ) ||
          IsZero( mid ) ) )
        {
        pos += 2;
        if ( pos > p2 )
          {
          pos = 1;
          ++ step;
          p2 *= 2.;
          }
        mid = pos * upperBnds[nloc] / p2 + (p2 - pos) * lowerBnds[nloc] / p2;
        tempSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, mid );
        }

      if ( step < 10 )
        {
        found = true;
        leftx = rightx = mid;
        leftVarSgn = rightVarSgn = tempSgn;
        // Set the new information for the current.
        if ( varSgn[0] - leftVarSgn <= nloc )
          lowerBnds[nloc] = leftx;
        if ( varSgn[0] - rightVarSgn > nloc )
          upperBnds[nloc] = rightx;
        }

      hitroot = ! found;
      // Make sure all our measures change around the root. This is another place where
      // the perturbation may be too small and cause slowness. This could
      // theoretically cause an infinite loop. (It has!).
      while ( !found && (
        // 1
        IsZero( evaluateHorner( SSS, d, leftx )) || IsZero( evaluateHorner( SSS, d, rightx ) )
        // 2 and 3c.
        || leftVarSgn <= rightVarSgn
        // 3a and 3b
        || leftVarSgn > varSgn[0] || rightVarSgn < varSgn[1] ) )
        {
        leftx -= perturbation;
        rightx += perturbation;
        // Take care of 4.
        if ( rightx - leftx > 2 * tol )
          break;
        leftVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, leftx );
        rightVarSgn = vtkGetSignChanges( SSS, degrees, offsets, nSSS, rightx );
        }
      // Now we must take care of our possible blunders.
        if ( rightx - leftx > 2 * tol )
          {
          // If leftx <= upperBnds[nloc-1] then we have to gracefully
          // clean up the mess.
          // For now assume we moved well enough...
          if ( leftVarSgn > varSgn[0] )
            leftVarSgn = varSgn[0];

          if ( rightVarSgn < varSgn[1] )
            rightVarSgn = varSgn[1];

          if ( rightVarSgn > varSgn[0] )
            rightVarSgn = varSgn[0] - nloc + 1;

          if ( leftVarSgn < varSgn[1] )
            leftVarSgn = varSgn[0] - nloc;

          rightx += tol;
          leftx -= tol;
          }

        if ( hitroot )
          {
          lowerBnds[nloc] = mid;
          upperBnds[nloc] = mid;
          }
      }
    else
      {
      // Set the new information for the current.
      if ( varSgn[0] - leftVarSgn <= nloc )
        lowerBnds[nloc] = leftx;
      if ( varSgn[0] - rightVarSgn > nloc )
        upperBnds[nloc] = rightx;
      }

    // We have isolated the rightVarSgn to the leftVarSgn roots.
    if ( rightx != leftx )
      {
      for ( i = varSgn[0] - leftVarSgn; i <= varSgn[0] - rightVarSgn - 1; ++ i )
        {
        if ( i > 0 && lowerBnds[i - 1] < leftx )
          {
          lowerBnds[i] = leftx;
          }
        if ( upperBnds[i] > rightx )
          {
          upperBnds[i] = rightx;
          }
        }
      }

    // Set the new lower bounds for the intervals to the right of the new and the left of the current.
    // We have to do the check because we can get crazy information with some points.
    for ( i = varSgn[0] - rightVarSgn; i >=0 && i < nRoots; ++ i )
      if ( lowerBnds[i] < rightx && upperBnds[i] > rightx)
        lowerBnds[i] = rightx;

    // We have to do the check because we can get crazy information with some points.
    // Set the new upper bounds for the intervals to the left of the new interval.
    for ( i = 0; i < varSgn[0] - leftVarSgn && varSgn[0] - leftVarSgn <= nloc; ++ i )
      if ( upperBnds[i] > leftx && lowerBnds[i] < leftx )
        upperBnds[i] = leftx;

    if ( leftVarSgn - rightVarSgn == 1 || hitroot )
      -- nloc;
    }

  int nIntervals = nRoots;
  bool* bisection = new bool[nRoots];

  // 3. Root polishing (if needed)
  for ( nloc = 0; nloc < nRoots; ++ nloc )
    {
    if ( upperBnds[nloc] - lowerBnds[nloc] < tol )
      {
      continue;
      }

    double zv = evaluateHorner( P, d, upperBnds[nloc] );
    double lv = evaluateHorner( P, d, lowerBnds[nloc] );
    // These are for the possibility that we get sign change differences when
    // using the sequence.
    int ls, zs;
    double z;

    // Neither of the next two conditions should ever be met, but when using
    // floats we still want to be sure.
    if ( IsZero( zv ) )
      {
      lowerBnds[nloc] = upperBnds[nloc];
      continue;
      }

    if ( IsZero( lv ) )
      {
      upperBnds[nloc] = lowerBnds[nloc];
      continue;
      }

    int us = ( zv > 0 ) ? 1 : -1;
    ls = ( lv > 0 ) ? 1 : -1;

    bool bisect = false;
    // Check to see if we should use the sequence, which is MUCH slower.
    if ( us * ls > 0 )
      {
      int zc;
      while ( upperBnds[nloc] - lowerBnds[nloc] > tol )
        {
        z = ( upperBnds[nloc] + lowerBnds[nloc] ) / 2;
        // Sometimes the tolerance can be poorly chosen causing an infinite
        // loop, this should fix that. That is, u - l > tol, but (u+l)/2 == l|u.
        if( z >= upperBnds[nloc] || z <= lowerBnds[nloc] )
          {
          break;
          }
        zc = vtkGetSignChanges( SSS, degrees, offsets, nSSS, z, &zs );

        // If that point is a zero go ahead and quit.
        if ( zs == 0 )
          {
          upperBnds[nloc] = lowerBnds[nloc] = z;
          break;
          }

        // Decide which side it goes on.
        if ( ( varSgn[0] - zc )  == nloc + 1 )
          {
          us = zs;
          upperBnds[nloc] = z;
          }
        else
          {
          ls = zs;
          lowerBnds[nloc] = z;
          }

        // Maybe we are *now* close enough to use bisection. In which case DO THAT!
        // One extra multiplication is worth it.
        if( us * ls < 0 )
          {
          bisect = true;
          // Quit this loop so that we can use bisection.
          break;
          }

        } // While ub[nloc] - lb[nloc] > tol

      bisection[nloc] = false;
      if ( bisect == false )
        {
        // Move on to next interval.
        continue;
        }
      } // If ub and lb have the same sign.
    else
      {
      bisect = true;
      }

    // If we can, use bisection.
    if ( bisect )
      {
      double tempu = zv;
      while ( upperBnds[nloc] - lowerBnds[nloc] > tol )
        {
        z = ( upperBnds[nloc] + lowerBnds[nloc] ) / 2;
        // Sometimes the tolerance can be poorly chosen causing an infinite
        // loop, this should fix that. That is, u - l > tol, but (u+l)/2 == l|u.
        if ( z >= upperBnds[nloc] || z <= lowerBnds[nloc] )
          {
          break;
          }
        zv = evaluateHorner( P, d, z );
        if ( IsZero( zv ) )
          {
          upperBnds[nloc] = lowerBnds[nloc] = z;
          break;
          }

        if ( zv * tempu > 0 )
          {
          tempu = zv;
          upperBnds[nloc] = z;
          }
        else
          lowerBnds[nloc] = z;
        }
      bisection[nloc] = true;
      } // While ub[nloc] - lb[nloc] > tol
    } // If we can bisect

  // Though theoretically this shouldn't happen, sometimes the roots are out of
  // order. Lets sort them just in case, because it does happen sometimes.
  qsort( upperBnds, nIntervals, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots );
  qsort( lowerBnds, nIntervals, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots );

  // Remove duplicate roots.
  for ( int j = 1; j < nIntervals; ++ j )
    {
    if (
        upperBnds[j] < ( upperBnds[j - 1] + 2 * tol ) ||
        lowerBnds[j] < ( lowerBnds[j - 1] + 2 * tol ) ||
        ( zeroroot && fabs( upperBnds[j] ) < 2 * tol ) )
      {
      for ( int k = j + 1; k < nIntervals; ++ k )
        {
        upperBnds[k - 1] = upperBnds[k];
        lowerBnds[k - 1] = lowerBnds[k];
        }
      -- j;
      -- nIntervals;
      continue;
      }
    }

  delete [] bisection;
  delete [] lowerBnds;

  delete [] degrees;
  delete [] SSS;
  delete [] offsets;

  // Make sure the first root isn't zero.
  if ( zeroroot && ( fabs( upperBnds[0] ) < 2 * tol ) )
    {
    for ( int k = 1; k < nIntervals; ++ k )
      {
      upperBnds[i-1] = upperBnds[i];
      }
    }

  if ( zeroroot )
    {
    upperBnds[nIntervals] = 0;
    ++ nIntervals;
    }

  return nIntervals;
}

//----------------------------------------------------------------------------
// Find all real roots in ] a[0] ; a[1] [ of a real
// d-th degree polynomial using the Habicht sequence.
// intervalType specifies as follows (in binary)
// 0 = 00 = ]a,b[
// 1 = 10 = [a,b[
// 2 = 01 = ]a,b]
// 3 = 11 = [a,b]
int vtkPolynomialSolversUnivariate::HabichtBisectionSolve(
  double* P, int d, double* a, double* upperBnds, double tol )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, 0, 0, 1 );
}

int vtkPolynomialSolversUnivariate::HabichtBisectionSolve(
  double* P, int d, double* a, double* upperBnds, double tol,
  int intervalType )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, intervalType, 0, 1 );
}

int vtkPolynomialSolversUnivariate::HabichtBisectionSolve(
  double* P, int d, double* a, double* upperBnds, double tol,
  int intervalType, bool divideGCD )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, intervalType, divideGCD ? 1 : 0, 1 );
}

//----------------------------------------------------------------------------
// Find all real roots in ] a[0] ; a[1] [ of a real
// d-th degree polynomial using the Sturm sequence.
// intervalType specifies as follows (in binary)
// 0 = 00 = ]a,b[
// 1 = 10 = [a,b[
// 2 = 01 = ]a,b]
// 3 = 11 = [a,b]
int vtkPolynomialSolversUnivariate::SturmBisectionSolve(
  double* P, int d, double* a, double *upperBnds, double tol )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, 0, 0, 0 );
}

int vtkPolynomialSolversUnivariate::SturmBisectionSolve(
  double* P, int d, double* a, double *upperBnds, double tol,
  int intervalType )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, intervalType, 0, 0 );
}

int vtkPolynomialSolversUnivariate::SturmBisectionSolve(
  double* P, int d, double* a, double *upperBnds, double tol,
  int intervalType, bool divideGCD )
{
  return vtkHabichtOrSturmBisectionSolve( P, d, a, upperBnds, tol, intervalType, divideGCD ? 1 : 0, 0 );
}

// Assume that dP = {f} and p is the degree of f.
// Furthermore assume that dP is large enough.
// Stores
// {f,f',f^(2)/2!,f^(3)/3!,...,f^(p)/p!}.
void vtkGetDerivativeSequence( double* dP, int p )
{
  int offsetA = 0;
  int offsetB = p+1;

  for ( int i = 1; i <= p; ++ i )
    {
    for ( int j = 0; j <= p - i; ++ j )
      dP[offsetB + j] = static_cast<double>( p - i - j + 1 ) * dP[offsetA + j] / i;

    offsetA = offsetB;
    offsetB += p - i + 1;
    }
}

int vtkGetSignChangesForDerivativeSequence( double* dP, int count, double val )
{
  int oldVal = 0;
  double v;
  int changes = 0;
  int offset = 0;

  for ( int i = 0; i <= count; ++ i )
    {
    v = evaluateHorner( dP + offset, count - i, val );

    if ( v * oldVal < 0 )
      {
      ++ changes;
      oldVal = -oldVal;
      }
    if ( oldVal == 0 )
      {
      if( v < 0 )
        {
        oldVal = -1;
        }
      else
        {
        oldVal = 1;
        }
      }
    offset += count - i + 1;
    }

  return changes;
}

int vtkPolynomialSolversUnivariate::FilterRoots(
  double* P, int d, double* upperBnds, int rootcount, double diameter )
{
  // Sort the roots.
  qsort( upperBnds, rootcount, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots );

  // Remove duplicates.
  for ( int j = 1; j < rootcount; ++ j )
    {
    if ( upperBnds[j] < upperBnds[j - 1] + diameter )
      {
      for ( int k = j + 1; k < rootcount; ++ k )
        {
        upperBnds[k - 1] = upperBnds[k];
        }
      -- j;
      -- rootcount;
      continue;
      }
    }

  if ( rootcount == 0 )
    {
    return 0;
    }

  // Ignore 0 as a root.
  for ( int i = d; i >= 0; -- i )
    {
    if ( IsZero( P[i] ) )
      {
      -- d;
      }
    else
      {
      break;
      }
    }


  double* dp = new double[( ( d + 2 ) * ( d + 1 ) ) / 2];
  for ( int i = 0; i <= d; ++ i )
    {
    dp[i] = P[i];
    }

  vtkGetDerivativeSequence( dp, d );

  // I could tag now and remove later. It'd be faster when there are a lot of
  // roots. But I doubt any of these root finders would work on a system with
  // enough roots for that method to be any better.
  for ( int i = 0; i < rootcount; ++ i )
    {
    if ( fabs( upperBnds[i] ) < diameter )
      {
      continue;
      }

    if (
      vtkGetSignChangesForDerivativeSequence( dp, d, upperBnds[i] ) ==
      vtkGetSignChangesForDerivativeSequence( dp, d, upperBnds[i] - diameter ) )
      {
      // Remove the root.
      for ( int j = i + 1; j < rootcount; ++ j )
        {
        upperBnds[j - 1] = upperBnds[j];
        }
      -- i;
      -- rootcount;
      }
    }
  delete [] dp;
  return rootcount;
}

//----------------------------------------------------------------------------
// Solves a d-th degree polynomial equation using Lin-Bairstow's method.
//
int vtkPolynomialSolversUnivariate::LinBairstowSolve( double* c, int d, double* r, double& tolerance )
{
  if ( IsZero( c[0] ) )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolversUnivariate::LinBairstowSolve: Zero leading coefficient");
    return 0;
    }

  int i;
  int dp1 = d + 1;
  for ( i = 1 ; i < dp1; ++ i )
    {
    c[i] /= c[0];
    }

  double* div1 = new double[dp1];
  double* div2 = new double[dp1];
  div1[0] = div2[0] = 1;
  for ( i = d ; i > 2; i -= 2 )
    {
    double det, detR, detS;
    double R = 0.;
    double S = 0.;
    double dR = 1.;
    double dS = 0.;
    int nIterations = 1;

    while ( ( fabs( dR ) + fabs( dS ) ) > tolerance )
      {
      // relax tolerance after 100 iterations did not suffice to converge
      // within the current tolerance
      if ( ! ( nIterations % 100 ) )
        {
        R = vtkMath::Random( 0., 2. );
        if ( ! ( nIterations % 200 ) )
          {
          tolerance *= 4.;
          }
        }

      div1[1] = c[1] - R;
      div2[1] = div1[1] - R;

      for ( int j = 2; j <= i; ++ j )
        {
        div1[j] = c[j] - R * div1[j - 1] - S * div1[j - 2];
        div2[j] = div1[j] - R * div2[j - 1] - S * div2[j - 2];
        }

      double u = div2[i - 1] * div2[i - 3];
      double v = div2[i - 2] * div2[i - 2];
      if ( AreEqual ( u, v, 1.e-6 ) )
        {
        det = detR = detS = 1.;
        }
      else
        {
        det  = u - v;
        detR = div1[i]     * div2[i - 3] - div1[i - 1] * div2[i - 2];
        detS = div1[i - 1] * div2[i - 1] - div1[i]     * div2[i - 2];
        }

      dR = detR / det;
      dS = detS / det;

      // prevent Jacobian from exploding faster than tolerance can be relaxed
      // by the means of a crude limiter
      if ( fabs( dR ) + fabs( dS ) > 10. )
        {
        dR = vtkMath::Random( -1., 1. );
        dS = vtkMath::Random( -1., 1. );
        }

      R += dR;
      S += dS;
      ++ nIterations;
      }

    for ( int j = 0; j < i - 1; ++ j )
      {
      c[j] = div1[j];
      }
    c[i] = S;
    c[i - 1] = R;
    }

  int nr = 0;
  for ( i = d; i >= 2; i -= 2 )
    {
    double delta = c[i - 1] * c[i - 1] - 4. * c[i];
    if ( delta >= 0 )
      {
      // check whether there are 2 simple roots or 1 double root
      if ( delta )
        {
        delta = sqrt( delta );
        // we have 2 simple real roots
        r[nr ++] = ( - c[i - 1] - delta ) / 2.;
        // insert 2nd simple real root
        r[nr ++] = ( - c[i - 1] + delta ) / 2.;
        }
      else
        {
        // we have a double real root
        r[nr ++] = - c[1];
        r[nr ++] = - c[1];
        }
      }
    }
  if ( ( d % 2 ) == 1 )
    {
    // what's left when degree is odd
    r[nr ++] = - c[1];
    }

  delete [] div1;
  delete [] div2;
  return nr;
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the quartic polynomial with
// REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
// and stores them (when they exist) and their respective multiplicities
// in the r and m arrays.
int vtkPolynomialSolversUnivariate::FerrariSolve( double* c, double* r, int* m, double tol )
{
  // step 0: eliminate trivial cases up to numerical noise
  if ( fabs( c[3] ) <= tol )
    {
    if ( fabs( c[2] ) <= tol )
      {
      if ( fabs( c[1] ) <= tol )
        {
        if ( fabs( c[0] ) <= tol )
          {
          r[0] = 0.;
          m[0] = 4;
          return 1;
          }
        else
          {
          r[0] = - c[1];
          m[0] = 1;
          r[1] = 0.;
          m[1] = 3;
          return 2;
          }
        }
      else
        {
        double cc[3];
        cc[0] = 1.;
        cc[1] = c[0];
        cc[2] = c[1];
        int nr = vtkPolynomialSolversUnivariate::SolveQuadratic( cc, r, m );
        r[nr] = 0.;
        m[nr] = 2;
        return nr + 1;
        }
      }
    else
      {
      int nr = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( c, r, m, tol );
      r[nr] = 0.;
      m[nr] = 1;
      return nr + 1;
      }
    }
  if ( ( fabs( c[0] ) <= tol ) && ( fabs( c[2] ) <= tol ) )
    {
    if ( fabs( c[1] ) <= tol )
      {
      if ( c[3] < 0. )
        {
        return 0;
        }
      r[0] = sqrt( sqrt( c[3] ) );
      m[0] = 4;
      return 1;
      }
    double cc[3], cr[2];
    int cm[2];
    cc[0] = 1.;
    cc[1] = c[1];
    cc[2] = c[3];
    int nr1 = vtkPolynomialSolversUnivariate::SolveQuadratic( cc, cr, cm );
    int nr = 0;
    int i;
    for ( i = 0; i < nr1; ++ i )
      {
      if ( fabs( cr[i] ) <= tol )
        {
        r[nr] = 0.;
        m[nr ++] = 2 * cm[i];
        }
      else
        {
        if ( cr[i] > tol )
          {
          r[nr] = sqrt( cr[i] );
          m[nr ++] = cm[i];
          r[nr] = - sqrt( cr[i] );
          m[nr ++] = cm[i];
          }
        }
      }
    return nr;
    }

  // step 1: reduce to X^4 + aX^2 + bX + d
  double p2d8 = c[0] * c[0] * .125 ;
  double qd2 = c[1] * .5;
  double a = c[1] - 3 * p2d8;
  double b = c[0] * ( p2d8 - qd2 ) + c[2];
  double d = p2d8 * ( qd2 - .75 * p2d8 ) - c[0] * c[2] * .25 + c[3];
  // expedite the case when the reduced equation is biquadratic
  if ( fabs( b ) <= tol )
    {
    double cc[3], cr[2];
    int cm[2];
    cc[0] = 1.;
    cc[1] = a;
    cc[2] = d;
    int nr1 = vtkPolynomialSolversUnivariate::SolveQuadratic( cc, cr, cm );
    int nr = 0;
    double shift = - c[0] * .25;
    int i;
    for ( i = 0; i < nr1; ++ i )
      {
      if ( fabs( cr[i] ) <= tol )
        {
        r[nr] = shift;
        m[nr ++] = 2 * cm[i];
        }
      else
        {
        if ( cr[i] > tol )
          {
          r[nr] = sqrt( cr[i] ) + shift;
          m[nr ++] = cm[i];
          r[nr] = - sqrt( cr[i] ) + shift;
          m[nr ++] = cm[i];
          }
        }
      }
    return nr;
    }

  // step 2: solve the companion cubic
  double cc[3], cr[3];
  double unsorted[8];
  int cm[3];
  cc[0] = 2. * a;
  cc[1] = a * a - 4. * d;
  cc[2] = - b * b;
  int nr = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( cc, cr, cm, tol );

  // step 3: figure alpha^2
  double alpha2 = cr[-- nr];
  while ( alpha2 < 0. && nr ) alpha2 = cr[-- nr];

  // step 4: solve the quadratics
  cc[0] = 1.;
  cc[1] = sqrt( alpha2 );
  double rho = - b / cc[1];
  cc[2] = ( a + alpha2 + rho ) * .5;
  int nr1 = vtkPolynomialSolversUnivariate::SolveQuadratic( cc, r, m );
  cc[1] = - cc[1];
  cc[2] -= rho;
  nr = nr1 + vtkPolynomialSolversUnivariate::SolveQuadratic( cc, r + nr1, m + nr1 );
  if ( ! nr )
    {
    return 0;
    }

  // step 5: sort, filter and shift roots (if any)
  int i;
  for ( i = 0; i < nr; ++ i )
    {
    unsorted[2*i] = r[i];
    unsorted[2*i + 1] = m[i];
    }
  qsort( unsorted, nr, 2*sizeof( double ), vtkPolynomialSolversUnivariateCompareRoots );
  r[0] = unsorted[0];
  m[0] = static_cast<int>(unsorted[1]);
  nr1 = 1;
  for ( i = 1; i < nr; ++ i )
    {
    if ( unsorted[2*i] == unsorted[2*i - 2] )
      {
      m[i - 1] += static_cast<int>(unsorted[2*i + 1]);
      continue;
      }
    r[nr1] = unsorted[2*i];
    m[nr1++] = static_cast<int>(unsorted[2*i + 1]);
    }
  double shift = - c[0] * .25;
  for ( i = 0; i < nr1; ++ i )
    {
    r[i] += shift;
    }

  return nr1;
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the cubic polynomial with
// REAL coefficients X^3 + c[0] X^2 + c[1] X + c[2]
// and stores them (when they exist) and their respective multiplicities.
// The main differences with SolveCubic are that (1) the polynomial must have
// unit leading coefficient, (2) no information is returned regarding complex
// roots, and (3) non-simple roots are stored only once -- this is a
// specialized solver.
// Returns the number of roots.
//
int vtkPolynomialSolversUnivariate::TartagliaCardanSolve( double* c, double* r, int* m, double tol )
{
  // step 0: eliminate trivial cases up to numerical noise
  if ( fabs( c[2] ) <= tol )
    {
    r[0] = 0.;
    if ( fabs( c[1] ) <= tol )
      {
      if ( fabs( c[0] ) <= tol )
        {
        m[0] = 3;
        return 1;
        }
      else
        {
        m[0] = 2;
        r[1] = - c[0];
        m[1] = 1;
        return 2;
        }
      }
    else
      {
      m[0] = 1;
      double a2 = c[0] * c[0];
      double fourc1 = 4. * c[1];
      double delta = a2 - fourc1;
      double threshold = tol * ( a2 > fabs( fourc1 ) ? a2 : fabs( fourc1 ) );
      if ( delta > threshold )
        {
        delta = sqrt( delta );
        r[1] = ( - delta - c[0] ) * 0.5;
        m[1] = 1;
        r[2] = ( delta - c[0] ) * 0.5;
        m[2] = 1;
        return 3;
        }
      else
        {
        if ( delta < - threshold )
          {
          return 1;
          }
        r[1] = - c[0] * 0.5;
        m[1] = 2;
        return 2;
        }
      }
    }

  // step 1: reduce to X^3 + pX + q
  double shift = - c[0] / 3.;
  double a2 = c[0] * c[0];
  double p = c[1] - a2 / 3.;
  double q = c[0] * ( 2. * a2 / 9. - c[1] ) / 3. + c[2];

  // step 2: compute the trivial real roots if p or q are 0
  // case 2.1: p = 0: 1 triple real root
  if ( fabs( p ) <= tol )
    {
    if ( fabs( q ) <= tol )
      {
      r[0] = + shift;
      m[0] = 3;
      return 1;
      }
    double x;
    x = q < 0 ? pow( - q, inv3 ) : - pow( q, inv3 );
    r[0] = x + shift;
    m[0] = 3;
    return 1;
    }

  // case 2.2: q = 0: 1 ( p > 0 ) or 3 ( p < 0 ) simple real root(s)
  if ( fabs( q ) <= tol )
    {
    r[0] = + shift;
    m[0] = 1;
    if ( p < 0 )
      {
      double x = sqrt( - p );
      r[1] =  x + shift;
      r[2] =  - x + shift;
      m[1] = m[2] = 1;
      return 3;
      }
    return 1;
    }

  // step 3: compute discriminant
  double p_3 = p * inv3;
  double q_2 = q * 0.5;
  double D = p_3 * p_3 * p_3 + q_2 * q_2;

  // step 4: compute roots depending on the discriminant
  double u;
  // 4.1: case D = 0: 1 simple and 1 double real roots
  if ( fabs( D ) <= tol )
    {
    u = q > 0 ? - pow( q_2, inv3 ) : pow( - q_2, inv3 );
    r[0] =  2. * u + shift;
    m[0] = 1;
    r[1] =  - u + shift;
    m[1] = 2;
    return 2;
    }
  // 4.2: case D > 0: 1 simple real root
  if ( D > 0 )
    {
    u = sqrt( D ) - q_2;
    u = u < 0 ? - pow( - u, inv3 ) : pow( u, inv3 );
    r[0] = u - p_3 / u + shift;
    m[0] = 1;
    return 1;
    }
  // 5.3: case D < 0: 3 simple real roots
  double smp_3 = sqrt( - p_3 );
  double argu  = acos( q_2 / ( p_3 * smp_3 ) ) * inv3;
  double x1 = cos( argu );
  double x2 = sqrt3 * sqrt( 1. - x1 * x1 );
  x1 *= smp_3;
  x2 *= smp_3;
  r[0] = 2. * x1 + shift;
  r[1] = x2 - x1 + shift;
  r[2] = r[1]  - 2. * x2;
  m[0] = m[1] = m[2] = 1;
  return 3;
}

//----------------------------------------------------------------------------
// Solves a cubic equation c0*t^3  + c1*t^2  + c2*t + c3 = 0 when
// c0, c1, c2, and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Return array contains number of (real) roots (counting multiple roots as one)
// followed by roots themselves. The value in roots[4] is a integer giving
// further information about the roots (see return codes for int SolveCubic()).
double* vtkPolynomialSolversUnivariate::SolveCubic( double c0, double c1, double c2, double c3 )
{
  static double roots[5];
  roots[1] = 0.0;
  roots[2] = 0.0;
  roots[3] = 0.0;
  int num_roots;

  roots[4] = vtkPolynomialSolversUnivariate::SolveCubic(c0, c1, c2, c3,
                                                        &roots[1], &roots[2], &roots[3], &num_roots );
  roots[0] = num_roots;
  return roots;
}

//----------------------------------------------------------------------------
// Solves a cubic equation when c0, c1, c2, And c3 Are REAL.  Solution
// is motivated by Numerical Recipes In C 2nd Ed.  Roots and number of
// real roots are stored in user provided variables r1, r2, r3, and
// num_roots. Note that the function can return the following integer
// values describing the roots: (0)-no solution; (-1)-infinite number
// of solutions; (1)-one distinct real root of multiplicity 3 (stored
// in r1); (2)-two distinct real roots, one of multiplicity 2 (stored
// in r1 & r2); (3)-three distinct real roots; (-2)-quadratic equation
// with complex conjugate solution (real part of root returned in r1,
// imaginary in r2); (-3)-one real root and a complex conjugate pair
// (real root in r1 and real part of pair in r2 and imaginary in r3).
int vtkPolynomialSolversUnivariate::SolveCubic( double c0, double c1, double c2, double c3,
                                                double *r1, double *r2, double *r3, int *num_roots )
{
  double        Q, R;
  double        R_squared;      /* R*R */
  double        Q_cubed;        /* Q*Q*Q */
  double        theta;
  double        A, B;

  // Cubic equation: c0*t^3  + c1*t^2  + c2*t + c3 = 0
  //
  //   r1, r2, r3 are roots and num_roots is the number
  //   of real roots

  // Make Sure This Is A Bonafide Cubic Equation
  if( c0 != 0.0 )
    {
    //Put Coefficients In Right Form
    c1 = c1/c0;
    c2 = c2/c0;
    c3 = c3/c0;

    Q = ((c1*c1) - 3*c2)/9.0;

    R = (2.0*(c1*c1*c1) - 9.0*(c1*c2) + 27.0*c3)/54.0;

    R_squared = R*R;
    Q_cubed   = Q*Q*Q;

    if( R_squared <= Q_cubed )
      {
      if( Q_cubed == 0.0 )
        {
        *r1 = -c1/3.0;
        *r2 = *r1;
        *r3 = *r1;
        *num_roots = 1;
        return 1;
        }
      else
        {
        theta = acos( R / (sqrt(Q_cubed) ) );

        *r1 = -2.0*sqrt(Q)*cos( theta/3.0 ) - c1/3.0;
        *r2 = -2.0*sqrt(Q)*cos( (theta + 2.0 * vtkMath::Pi())/3.0) - c1/3.0;
        *r3 = -2.0*sqrt(Q)*cos( (theta - 2.0 * vtkMath::Pi())/3.0) - c1/3.0;

        *num_roots = 3;

        // Reduce Number Of Roots To Two
        if( *r1 == *r2 )
          {
          *num_roots = 2;
          *r2 = *r3;
          }
        else if( *r1 == *r3 )
          {
          *num_roots = 2;
          }

        if( (*r2 == *r3) && (*num_roots == 3) )
          {
          *num_roots = 2;
          }

        // Reduce Number Of Roots To One
        if( *r1 == *r2 )
          {
          *num_roots = 1;
          }
        }
      return *num_roots;
      }
    else //single real and complex conjugate pair
      {
      A = -VTK_SIGN(R) * pow(fabs(R) + sqrt(R_squared - Q_cubed), 1.0/3);

      if( A == 0.0 )
        {
        B = 0.0;
        }
      else
        {
        B = Q/A;
        }

      *r1 =  (A + B) - c1/3.0;
      *r2 = -0.5*(A + B) - c1/3.0;
      *r3 = sqrt(3.0)/2.0*(A - B);

      *num_roots = 1;
      return (-3);
      }
    } //if cubic equation

  else // Quadratic Equation: c1*t  + c2*t + c3 = 0
    {
    // Okay this was not a cubic - lets try quadratic
    return vtkPolynomialSolversUnivariate::SolveQuadratic( c1, c2, c3, r1, r2, num_roots );
    }
}

//----------------------------------------------------------------------------
// Solves a quadratic equation c1*t^2 + c2*t + c3 = 0 when c1, c2, and
// c3 are REAL.  Solution is motivated by Numerical Recipes In C 2nd
// Ed.  Return array contains number of (real) roots (counting
// multiple roots as one) followed by roots themselves. Note that
// roots[3] contains a return code further describing solution - see
// documentation for SolveCubic() for meaining of return codes.
double* vtkPolynomialSolversUnivariate::SolveQuadratic( double c1, double c2, double c3)
{
  static double roots[4];
  roots[0] = 0.0;
  roots[1] = 0.0;
  roots[2] = 0.0;
  int num_roots;

  roots[3] = vtkPolynomialSolversUnivariate::SolveQuadratic( c1, c2, c3, &roots[1], &roots[2],
                                      &num_roots );
  roots[0] = num_roots;
  return roots;
}

//----------------------------------------------------------------------------
// Solves A Quadratic Equation c1*t^2  + c2*t  + c3 = 0 when
// c1, c2, and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Roots and number of roots are stored in user provided variables
// r1, r2, num_roots
int vtkPolynomialSolversUnivariate::SolveQuadratic( double c1, double c2, double c3,
                                                    double *r1, double *r2, int *num_roots )
{
  double        Q;
  double        determinant;

  // Quadratic equation: c1*t^2 + c2*t + c3 = 0

  // Make sure this is a quadratic equation
  if( c1 != 0.0 )
    {
    determinant = c2*c2 - 4*c1*c3;

    if( determinant >= 0.0 )
      {
      Q = -0.5 * (c2 + VTK_SIGN(c2)*sqrt(determinant));

      *r1 = Q / c1;

      if( Q == 0.0 )
        {
        *r2 = 0.0;
        }
      else
        {
        *r2 = c3 / Q;
        }

      *num_roots = 2;

      // Reduce Number Of Roots To One
      if( *r1 == *r2 )
        {
        *num_roots = 1;
        }
      return *num_roots;
      }
    else        // Equation Does Not Have Real Roots
      {
      *num_roots = 0;
      return (-2);
      }
    }

  else // Linear Equation: c2*t + c3 = 0
    {
    // Okay this was not quadratic - lets try linear
    return vtkPolynomialSolversUnivariate::SolveLinear( c2, c3, r1, num_roots );
    }
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the quadratic polynomial with
// REAL coefficients c[0] X^2 + c[1] X + c[2]
// and stores them (when they exist) and their respective multiplicities.
// Returns either the number of roots, or -1 if ininite number of roots.
int vtkPolynomialSolversUnivariate::SolveQuadratic( double* c, double* r, int* m )
{
  if( ! c[0] )
    {
    if( c[1] )
      {
      r[0] = -c[2] / c[1];
      m[0] = 1;
      return 1;
      }
    else
      {
      if ( c[2] ) return 0;
      else return -1;
      }
    }

  double delta = c[1] * c[1] - 4. * c[0] * c[2];

  if ( delta >= 0. )
    {
    double fac = 1. / ( 2. *  c[0] );
    // check whether there are 2 simple or 1 double root(s)
    if ( delta )
      {
      delta = sqrt( delta );
      // insert 1st simple real root
      r[0] = ( - delta - c[1] ) * fac;
      m[0] = 1;
      // insert 2nd simple real root
      r[1] = ( delta - c[1] ) * fac ;
      m[1] = 1;
      return 2;
      }
    else
      {
      // insert single double real root
      r[0] = - c[1] * fac;
      m[0] = 2;
      return 1;
      }
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
// Solves a linear equation c2*t  + c3 = 0 when c2 and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Return array contains number of roots followed by roots themselves.
double* vtkPolynomialSolversUnivariate::SolveLinear( double c2, double c3)
{
  static double roots[3];
  int num_roots;
  roots[1] = 0.0;
  roots[2] = vtkPolynomialSolversUnivariate::SolveLinear( c2, c3, &roots[1], &num_roots );
  roots[0] = num_roots;
  return roots;
}

//----------------------------------------------------------------------------
// Solves a linear equation c2*t + c3 = 0 when c2 and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Root and number of (real) roots are stored in user provided variables
// r2 and num_roots.
int vtkPolynomialSolversUnivariate::SolveLinear( double c2, double c3, double *r1, int *num_roots )
{
  // Linear equation: c2*t + c3 = 0
  // Now this had better be linear
  if( c2 != 0.0 )
    {
    *r1 = -c3 / c2;
    *num_roots = 1;
    return *num_roots;
    }
  else
    {
    *num_roots = 0;
    if ( c3 == 0.0 )
      {
      return (-1);
      }
    }

  return *num_roots;
}

//----------------------------------------------------------------------------
void vtkPolynomialSolversUnivariate::SetDivisionTolerance( double tol )
{
  vtkPolynomialSolversUnivariate::DivisionTolerance = tol;
}

//----------------------------------------------------------------------------
double vtkPolynomialSolversUnivariate::GetDivisionTolerance()
{
  return vtkPolynomialSolversUnivariate::DivisionTolerance;
}
