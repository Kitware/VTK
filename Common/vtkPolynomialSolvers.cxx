/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolynomialSolvers.cxx

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
#include "vtkPolynomialSolvers.h"
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

vtkCxxRevisionMacro(vtkPolynomialSolvers, "1.24");
vtkStandardNewMacro(vtkPolynomialSolvers);

static const double sqrt3 = sqrt( static_cast<double>(3.) );
static const double inv3 = 1 / 3.;

//----------------------------------------------------------------------------
void vtkPolynomialSolvers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
int polynomialEucliDiv( double* A, int m, double* B, int n, double* Q, double* R )
{
  // Note: for execution speed, no sanity checks are performed on A and B. 
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    Q[0] = 0.;
    for ( i = 0; i <= m; ++ i ) R[i] = A[i];

    return m;
    }
  
  double iB0 = 1. / B[0];
  if ( ! n )
    {
    for ( i = 0; i <= m; ++ i ) Q[i] = A[i] * iB0;

    return -1;
    }
  
  int nj;
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j ) Q[i] -= B[j] * Q[i - j] ;
    Q[i] *= iB0;
    }

  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    R[n - i] = A[m - i + 1];
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j ) R[n - i] -= B[n - i + 1 + j] * Q[mMn - j];

    if ( R[n - i] ) r = i - 1;
    }

  if ( ! r && ! R[0] ) return -1;

  return r;
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
// Does not store Q and stores -R instead of R
int polynomialEucliDivOppositeR( double* A, int m, double* B, int n, double* mR )
{
  // Note: for execution speed, no sanity checks are performed on A and B. 
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    for ( i = 0; i <= m; ++ i ) mR[i] = A[i];
    return m;
    }
  
  if ( ! n ) return -1;
  
  double iB0 = 1. / B[0];
  int nj;
  double* Q = new double[mMn + 1];
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j ) 
      {
      Q[i] -= B[j] * Q[i - j] ;
      }
    Q[i] *= iB0;
    }

  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    mR[n - i] = - A[m - i + 1];
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j ) mR[n - i] += B[n - i + 1 + j] * Q[mMn - j];

    if ( mR[n - i] ) 
      {
      r = i - 1;
      }
    }
  delete [] Q;
  
  if ( ! r && ( ! mR[0] || fabs ( mR[0] ) <= static_cast<double>( 2 * m ) * VTK_DBL_EPSILON * fabs ( A[m] ) ) )
    {
    mR[0] = 0.;
    return -1;
    }

  return r;
}

//----------------------------------------------------------------------------
// Evaluate the value of the degree d univariate polynomial P at x
// using Horner's algorithm.
inline double evaluateHorner( double* P, int d, double x )
{
  double val = P[0];
  for ( int i = 1; i <= d; ++ i ) val = val * x + P[i];

  return val;
}

//----------------------------------------------------------------------------
// Find all real roots in ] a[0] ; a[1] ] of a real 
// d-th degree polynomial using Sturm's theorem.
int vtkPolynomialSolvers::SturmBisectionSolve( double* P, int d, double* a, double *upperBnds, double tol )
{
  // 0. Stupidity checks

  if ( tol <= 0 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Tolerance must be positive");
    return -1;
    }

  if ( ! P[0] )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Zero leading coefficient");
    return -1;
    }

  if ( d < 1 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Degree < 1");
    return -1;
    }

  if ( a[1] < a[0] + tol )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Erroneous interval endpoints and/or tolerance");
    return -1;
    }

  double bounds[] = { a[0], a[1] };
  // 1. Root counting
  double* SSS = new double[( d + 1 ) * ( d + 2 ) / 2];
  int* degSSS = new int[d + 2];
  
  int offsetA = 0;
  degSSS[0] = d;
  SSS[0] = P[0];
  SSS[d] = P[d];

  int offsetB = d + 1;
  degSSS[1] = d - 1;
  SSS[offsetB] = static_cast<double>( d ) * P[0];
  
  int i, k;
  double oldVal[] = { P[0], P[0] };
  for ( i = 1; i < d; ++ i ) 
    {
    SSS[i] = P[i];
    SSS[offsetB + i] = static_cast<double>( d - i ) * P[i];
    for ( k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * bounds[k] + P[i];
    }
  for ( k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * bounds[k] + P[d];

  double perturbation = tol * .5 / static_cast<double>( d );
  while ( ! oldVal[0] && ! evaluateHorner( SSS + offsetB, d - 1, bounds[0] ) )
    {
    bounds[0] -= perturbation;
    oldVal[0] = evaluateHorner( SSS, d, bounds[0] );
    }
  while ( ! oldVal[1] && ! evaluateHorner( SSS + offsetB, d - 1, bounds[1] ) )
    {
    bounds[1] += perturbation;
    oldVal[1] = evaluateHorner( SSS, d, bounds[1] );
    }

  int varSgn[] = { 0, 0 };
  int offsetR;
  int nSSS = 1;
  for ( ; degSSS[nSSS] > -1; ++ nSSS )
    {
    double newVal[] = { SSS[offsetB], SSS[offsetB] };
    for ( k = 0; k < 2; ++ k )
      {
      for ( i = 1; i <= degSSS[nSSS]; ++ i ) newVal[k] = newVal[k] * bounds[k] + SSS[offsetB + i];
      if ( oldVal[k] * newVal[k] < 0. ) ++ varSgn[k];
      if ( newVal[k] ) oldVal[k] = newVal[k];
      }

    offsetR = offsetB + degSSS[nSSS] + 1;
    degSSS[nSSS + 1] = polynomialEucliDivOppositeR( SSS + offsetA, degSSS[nSSS - 1], SSS + offsetB, degSSS[nSSS], SSS + offsetR );
   
    offsetA = offsetB;
    offsetB = offsetR;
   }

  int nRoots = varSgn[0] - varSgn[1];
  if ( ! nRoots ) return 0;

  // 2. Root bracketing

  upperBnds[0] = bounds[1];
  double localTol = bounds[1] - bounds[0];

  int* lowerVarSgn = new int[nRoots];
  int* upperVarSgn = new int[nRoots];
  lowerVarSgn[0] = varSgn[0];
  upperVarSgn[0] = varSgn[1];

  int midVarSgn, nIntervals = 1;
  double x, xOldVal, xVal;
  while ( nIntervals < nRoots && localTol > tol )
    {
    localTol *= .5;
    int nloc = nIntervals;
    for ( i = 0; i < nloc; ++ i )
      {
      x = upperBnds[i] - localTol;
      offsetA = 0;
      xOldVal = 0.;
      midVarSgn = 0;
      for ( int j = 0; j < nSSS; offsetA += degSSS[j ++] + 1 )
        {
        xVal = evaluateHorner( SSS + offsetA, degSSS[j], x );
        if ( xOldVal * xVal < 0. ) ++ midVarSgn;
        if ( xVal ) xOldVal = xVal;
        }

      if ( midVarSgn == upperVarSgn[i] ) upperBnds[i] = x;
      else if ( midVarSgn != lowerVarSgn[i] )
        {
        upperBnds[nIntervals] = x;
        lowerVarSgn[nIntervals] = lowerVarSgn[i];
        upperVarSgn[nIntervals ++] = lowerVarSgn[i] = midVarSgn;
        }
      }
    }

  delete [] lowerVarSgn;

  // 3. Root polishing (if needed)
  if ( localTol > tol ) 
    {
    double* upperVals = new double[nIntervals];
    bool* multipleRoot = new bool[nIntervals];
    for ( i = 0; i < nIntervals; ++ i ) 
      {
      upperVals[i] = evaluateHorner( P, d, upperBnds[i] );
      multipleRoot[i] = upperVals[i] * evaluateHorner( P, d, upperBnds[i]  - localTol ) > 0. ? true : false;
      }

    while ( localTol > tol )
      {
      localTol *= .5;
      for ( i = 0; i < nIntervals; ++ i )
        {
        if ( ! upperVals[i] ) continue;

        x = upperBnds[i] - localTol;
        if ( multipleRoot[i] )
          {
          upperVarSgn[0] = varSgn[1];

          midVarSgn = offsetA = 0;
          xOldVal = 0.;
          midVarSgn = 0;
          for ( int j = 0; j < nSSS; offsetA += degSSS[j ++] + 1 )
            {
            xVal = evaluateHorner( SSS + offsetA, degSSS[j], x );
            if ( xOldVal * xVal < 0. ) ++ midVarSgn;
            if ( xVal ) xOldVal = xVal;
            }

          if ( midVarSgn == upperVarSgn[i] ) upperBnds[i] = x;

          }
        else
          {
          xVal = evaluateHorner( P, d, x );
          if ( upperVals[i] * xVal > 0. )
            {
            upperBnds[i] = x;
            upperVals[i] = xVal;
            }
          }
        }
      }
    delete [] upperVals;
    delete [] multipleRoot;
    }

  delete [] upperVarSgn;
  delete [] degSSS;
  delete [] SSS;

  return nIntervals;
}

//----------------------------------------------------------------------------
// Solves a d-th degree polynomial equation using Lin-Bairstow's method.
//
int vtkPolynomialSolvers::LinBairstowSolve( double* c, int d, double* r, double& tolerance )
{
  if ( ! c[0] )
    {
    vtkGenericWarningMacro(<<"vtkMath::LinBairstowSolve: Zero leading coefficient");
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
      if ( ! ( nIterations % 100 ) )
        {
        R = vtkMath::Random( 0., 2. );
        if ( ! ( nIterations % 200 ) ) tolerance *= 4.;
        }

      div1[1] = c[1] - R;
      div2[1] = div1[1] - R;

      for ( int j = 2; j <= i; ++ j )
        {
        div1[j] = c[j] - R * div1[j - 1] - S * div1[j - 2];
        div2[j] = div1[j] - R * div2[j - 1] - S * div2[j - 2];
        }

      det  = div2[i - 1] * div2[i -3]  - div2[i - 2] * div2[i - 2];
      detR = div1[i]     * div2[i -3]  - div1[i - 1] * div2[i - 2];
      detS = div1[i - 1] * div2[i - 1] - div1[i]     * div2[i - 2];

      if ( fabs( det ) < VTK_DBL_EPSILON )
        {
        det = detR = detS = 1.;
        }

      dR = detR / det;
      dS = detS / det;
      R += dR;
      S += dS;
      ++ nIterations;
      }

    for ( int j = 0; j < i - 1; ++ j ) c[j] = div1[j];
    c[i] = S;
    c[i - 1] = R;
    }

  int nr = 0;  
  for ( i = d; i >= 2; i -= 2 )
    {
    double delta = c[i - 1] * c[i - 1] - 4. * c[i];
    if ( delta >= 0 )
      {
      // check whether there is 2 simple or 1 double root(s)
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

extern "C" {
  int vtkPolynomialSolversCompareRoots(const void* a, const void* b)
  {
    return (*((const double*)a)) < (*((const double*)b)) ? -1 : 1; 
  }
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the quartic polynomial with 
// REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
// and stores them (when they exist) and their respective multiplicities
// in the r and m arrays.
int vtkPolynomialSolvers::FerrariSolve( double* c, double* r, int* m, double tol )
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
        int nr = vtkMath::SolveQuadratic( cc, r, m );
        r[nr] = 0.;
        m[nr] = 2;
        return nr + 1;
        }
      }
    else
      {
      int nr = vtkPolynomialSolvers::TartagliaCardanSolve( c, r, m, tol );
      r[nr] = 0.;
      m[nr] = 1;
      return nr + 1;
      }
    }
  if ( ( fabs( c[0] ) <= tol ) && ( fabs( c[2] ) <= tol ) )
    {
    if ( fabs( c[1] ) <= tol )
      {
      if ( c[3] < 0. ) return 0;
      r[0] = sqrt( sqrt( c[3] ) );
      m[0] = 4;
      return 1;
      }
    double cc[3], cr[2];
    int cm[2];
    cc[0] = 1.;
    cc[1] = c[1];
    cc[2] = c[3];
    int nr1 = vtkMath::SolveQuadratic( cc, cr, cm );
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
    int nr1 = vtkMath::SolveQuadratic( cc, cr, cm );
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
  int nr = vtkPolynomialSolvers::TartagliaCardanSolve( cc, cr, cm, tol );

  // step 3: figure alpha^2
  double alpha2 = cr[-- nr];
  while ( alpha2 < 0. && nr ) alpha2 = cr[-- nr];

  // step 4: solve the quadratics
  cc[0] = 1.;
  cc[1] = sqrt( alpha2 );
  double rho = - b / cc[1];
  cc[2] = ( a + alpha2 + rho ) * .5;
  int nr1 = vtkMath::SolveQuadratic( cc, r, m );
  cc[1] = - cc[1];
  cc[2] -= rho;
  nr = nr1 + vtkMath::SolveQuadratic( cc, r + nr1, m + nr1 );
  if ( ! nr ) return 0;

  // step 5: sort, filter and shift roots (if any)
  int i;
  for ( i = 0; i < nr; ++ i )
    {
    unsorted[2*i] = r[i];
    unsorted[2*i + 1] = m[i];
    }
  qsort( unsorted, nr, 2*sizeof( double ), vtkPolynomialSolversCompareRoots );
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
  for ( i = 0; i < nr1; ++ i ) r[i] += shift;

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
int vtkPolynomialSolvers::TartagliaCardanSolve( double* c, double* r, int* m, double tol )
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
      double delta = c[0] * c[0] - 4. * c[1];
      if ( delta > VTK_DBL_EPSILON )
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
        if ( delta < - VTK_DBL_EPSILON ) return 1;
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
