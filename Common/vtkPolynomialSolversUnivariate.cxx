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

vtkCxxRevisionMacro(vtkPolynomialSolversUnivariate, "1.1");
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

  if(degP < 0) 
  {
    os << "0\n";
    return os;
  }
  if(degP == 0)
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
  if ( fabs( x - y ) < absolute0 )
    {
    return true;
    }
  
  double rErr;
  if ( fabs( x ) > fabs( y ) )
    {
    rErr = fabs( ( y - x ) / x );
    }
  else
    {
    rErr = fabs( ( y - x ) / y );
    }

  return rErr > rTol ? false : true;
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
int polynomialEucliDiv( double* A, int m, double* B, int n, double* Q, double* R, double rtol )
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
static int polynomialEucliDivOppositeR( double* A, int m, double* B, int n, double* mR, double rtol )
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
// Evaluate the value of the degree d univariate polynomial P at x
// using Horner's algorithm.
inline double evaluateHorner( double* P, int d, double x )
{
  double val = P[0];
  for ( int i = 1; i <= d; ++ i )
    {
    val = val*x + P[i];
    }

  return val;
}

int vtkGetSignChanges(double* P, int* degP, int* offsets, int count, double val, bool debug)
{
  int oldVal = 0;
  double v;
  int changes = 0;
  // When removing debug stuff remove this variable.
  double min = 1.;

  for(int i = 0; i < count; i++)
    {
      v = evaluateHorner(P+offsets[i], degP[i], val);
      if(v==0) 
        {
        continue;
        }

      if(debug)
        {
        double lv = log(fabs(v));
        if(lv < min) 
          {
          min = lv;
          }
        }

      if(v*oldVal < 0) 
        {
        changes++;
        oldVal = 0-oldVal;
        }

      if(oldVal == 0)
        {
        if(v < 0) oldVal = -1;
        else oldVal = 1;
        }
    }

  return changes;
}

int vtkGetSignChanges(double* P, int* degP, int* offsets, int count, double val)
{
  return vtkGetSignChanges(P, degP, offsets, count, val, false);
}

// ----------------------------------------------------------
// Gets the sturm sequence. SSS and degrees and offsets are expected to 
// be large enough and the number of non-zero items
// is returned. P is expected to have degree at least 1.
int vtkGetSturmSequence(double * P, int d, double * SSS, int* degrees, int* offsets, double rtol )
{
  degrees[0] = d;
  offsets[0] = 0;

  int dp1 = d+1;
  degrees[1] = d - 1;
  offsets[1] = dp1;
  int offset = dp1;

  // nSSS will keep track of the number of the index of
  // the last item in our list.
  int nSSS = 1;

  // Set the first two elements SSS = {P, P'}.
  for(int k=0; k < d; k++)
    {
    SSS[k] = P[k];
    SSS[k + offset] = static_cast<double>(d-k)*P[k];
    }
  SSS[d] = P[d];

  int degree = d-1;
  while(degrees[nSSS] > 0)
    {
    nSSS++;
    degrees[nSSS] = polynomialEucliDivOppositeR(SSS + offsets[nSSS - 2], degrees[nSSS - 2],
      SSS + offset, degree, SSS + offset + degree + 1, rtol );

    offsets[nSSS] = offset + 2*degree - degrees[nSSS];

    offset = offsets[nSSS];
    degree = degrees[nSSS];

    }
    

  // If the last element is zero then we ignore it.
  if(degrees[nSSS] < 0)
    {
    return nSSS;
    }
  // Otherwise we include it in our count, because it 
  // is a constant. (We are returning the number of
  // nonzero items, so 1 plus the index of the last).
  return nSSS + 1;
}

extern "C"
{
int vtkPolynomialSolversUnivariateCompareRoots(const void* a, const void* b);
}

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
int vtkSturmBisectionSolve(double* P, int d, double* a, double *upperBnds, double tol, 
  int intervalType, int divideGCD)
{
  // 0. Stupidity checks

  if ( tol <= 0 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolversUnivariate::SturmBisectionSolve: Tolerance must be positive");
    return -1;
    }

  if ( IsZero( P[0] ) )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolversUnivariate::SturmBisectionSolve: Zero leading coefficient");
    return -1;
    }

  if ( d < 1 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolversUnivariate::SturmBisectionSolve: Degree < 1");
    return -1;
    }

  if ( a[1] < a[0] + tol )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolversUnivariate::SturmBisectionSolve: Erroneous interval endpoints and/or tolerance");
    return -1;
    }

  // Check for 0 as a root and reduce the degree if so.
  bool zeroroot = false;
  if(P[d] == 0)
    {
    zeroroot = true;
    while(P[d] == 0)
      d--;
    }

  double bounds[] = { a[0], a[1] };
  // Create one large array to hold all the 
  // polynomials.
  double* SSS = new double[( d + 1 ) * ( d + 2 ) / 2];
  int* degrees = new int[d + 2];
  int* offsets = new int[d + 2];
  
  int nSSS;

  nSSS = vtkGetSturmSequence(P, d, SSS, degrees, offsets, vtkPolynomialSolversUnivariate::GetDivisionTolerance() );

  // If degrees[count-1] > 0 then we have degenerate roots.
  // We could possibly then find the degenerate roots.
  // Maybe we could more or less remove the degenerate roots.
  // Lets do some testwork to see.
  if( degrees[nSSS-1] > 0 && divideGCD == 1 )
    {
    double* R = new double[d+1];
    double* Q = new double[d+1];

    // Get the quotient and call this function again using the
    // quotient.
    int deg = polynomialEucliDiv(SSS, d, SSS+offsets[nSSS-1], degrees[nSSS-1], Q, R, 
      vtkPolynomialSolversUnivariate::GetDivisionTolerance());
    deg = d-degrees[nSSS - 1];


    delete [] SSS;
    delete [] degrees;
    delete [] offsets;
    delete [] R;
      
    int rval = vtkSturmBisectionSolve(Q, deg, a, upperBnds, tol, intervalType, 0);
    delete [] Q;
    if(zeroroot)
      {
      upperBnds[rval] = 0;
      return rval+1;
      }
    return rval;
    }

  // Move away from zeros on the edges. We can also slightly speed up 
  // computation by keeping the fact that these are roots and
  // continuing on.
  
  double perturbation = .5*tol/ static_cast<double>(d);

  int varSgn[] = {0,0};
  varSgn[0] = vtkGetSignChanges(SSS, degrees, offsets, nSSS, bounds[0]);
  varSgn[1] = vtkGetSignChanges(SSS, degrees, offsets, nSSS, bounds[1]);
  
  for(int k=0; k<=1; k++)
    {
    if(IsZero(evaluateHorner(SSS, d, bounds[k])))
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
      while(IsZero(evaluateHorner(SSS, d, leftx)) || IsZero(evaluateHorner(SSS, d, rightx)) || leftVarSgn <= rightVarSgn ||
            ((leftVarSgn == varSgn[k] || rightVarSgn == varSgn[k]) && leftVarSgn - rightVarSgn != 1))
        {
        leftx -= perturbation;
        rightx += perturbation;
        leftVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, leftx);
        rightVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, rightx);
        }

      // Move properly according to what kind of sequence we are searching.
      if((!(intervalType & 2) && k==1) || ((intervalType & 1) && k==0))
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
    if(zeroroot)
      return 1;
    return 0;
    }

  // 2. Root bracketing

  // Initialize the bounds for the
  // root intervals. The interval
  // ]lowerBnds[i], upperBnds[i][ 
  // contains the i+1 root. We will
  // see if we can completely 
  // separate the roots. Of course
  // the interval ]bounds[0], bounds[1][
  // contains all the roots. Afterwards if
  // some intervals are the same all but
  // one will be removed.
  int i;
  double* lowerBnds = new double[nRoots];
  
  for(i = 0; i < nRoots; i++)
    {
    upperBnds[i] = bounds[1];
    lowerBnds[i] = bounds[0];
    }


  int leftVarSgn, rightVarSgn, tempSgn;
  double leftx, rightx;
  bool hitroot;
  int nloc = nRoots-1;
  while ( nloc >= 1)
    {
    // Only one root according to Sturm or the interval is 
    // small enough to consider the same root.
    if(upperBnds[nloc]-lowerBnds[nloc] <= tol 
        || ((nloc < 1 || (upperBnds[nloc-1] < lowerBnds[nloc] - tol) )
        && ((nloc >= nRoots-1) || (upperBnds[nloc]) < lowerBnds[nloc+1]-tol)))
      {
      nloc--;
      continue;
      }


    // We begin with leftx and rightx being equal and change them only if
    // leftx (rightx) is a root. Then we can bracket the root. We do this
    // because roots can cause problems (our sequence is inexact so
    // even single roots can cause problems. Furthermore if we hit a 

    // root we may as well bracket it within tol so that we don't have to
    // worry about it later.
    leftx = (upperBnds[nloc]+lowerBnds[nloc])/2;
    rightx = leftx;
    hitroot = false;
    
    leftVarSgn = rightVarSgn = tempSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, rightx);

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
    if(IsZero(evaluateHorner(SSS, d, leftx)))
      {
      int step = 2;
      int pos = 1;
      double p2 = 4.;
      double mid = upperBnds[nloc]/p2 + (p2-pos)*lowerBnds[nloc]/p2;
      bool found = false;
      leftVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, lowerBnds[nloc]); 
      rightVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, upperBnds[nloc]);
      tempSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, mid);
      while(
        // 3.
        step < 10 && 
        // 2a.
        ((tempSgn > leftVarSgn)  ||
        // 2b. 
        (tempSgn < rightVarSgn) ||
        // 1
        IsZero(evaluateHorner(SSS, d, mid))))
        {
        pos += 2;
        if(pos > p2)
          {
          pos = 1;
          step++;
          p2 *= 2.;
          }
        mid = pos*upperBnds[nloc]/p2 + (p2-pos)*lowerBnds[nloc]/p2;
        tempSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, mid);
        }

      if(step < 10)
        {
        found = true;
        leftx = rightx = mid;
        leftVarSgn = rightVarSgn = tempSgn;
        // Set the new information for the current.
        if(varSgn[0] - leftVarSgn <= nloc)
          lowerBnds[nloc] = leftx;
        if(varSgn[0] - rightVarSgn > nloc)
          upperBnds[nloc] = rightx;

        }

      hitroot = !found;
      // Make sure all our measures change around the root. This is another place where
      // the perturbation may be too small and cause slowness. This could
      // theoretically cause an infinite loop. (It has!).
      while(!found && (
        // 1
        IsZero(evaluateHorner(SSS, d, leftx)) || IsZero(evaluateHorner(SSS, d, rightx))
        // 2 and 3c.
        || leftVarSgn <= rightVarSgn
        // 3a and 3b
        || leftVarSgn > varSgn[0] || rightVarSgn < varSgn[1]))
        {
        leftx -= perturbation;
        rightx += perturbation;
        // Take care of 4.
        if (rightx - leftx > 2*tol)
          break;
        leftVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, leftx);
        rightVarSgn = vtkGetSignChanges(SSS, degrees, offsets, nSSS, rightx);
        }
      // Now we must take care of our possible blunders.
        if(rightx - leftx > 2*tol)
          {
          // If leftx <= upperBnds[nloc-1] then we have to gracefully
          // clean up the mess.
          // For now assume we moved well enough...
          if(leftVarSgn > varSgn[0])
            leftVarSgn = varSgn[0];

          if(rightVarSgn < varSgn[1])
            rightVarSgn = varSgn[1];

          if(rightVarSgn > varSgn[0])
            rightVarSgn = varSgn[0]-nloc+1;

          if(leftVarSgn < varSgn[1])
            leftVarSgn = varSgn[0]-nloc;
          rightx += tol;
          leftx -= tol;
          }
        if(hitroot)
          {
          lowerBnds[nloc] = mid;
          upperBnds[nloc] = mid;
          }
      }
    else
      {
      // Set the new information for the current.
      if(varSgn[0] - leftVarSgn <= nloc)
        lowerBnds[nloc] = leftx;
      if(varSgn[0] - rightVarSgn > nloc)
        upperBnds[nloc] = rightx;
      }

    // We have isolated the rightVarSgn to the leftVarSgn roots.
    if(rightx != leftx)
      {
      for(i = varSgn[0] - leftVarSgn; i<= varSgn[0]-rightVarSgn-1; i++)
        {
        if(i > 0 && lowerBnds[i-1] < leftx) lowerBnds[i] = leftx;
        if(upperBnds[i] > rightx) upperBnds[i] = rightx;
        }
      }

    // Set the new lower bounds for the intervals to the right of the new and the left of the current.
    // We have to do the check because we can get crazy information with some points.
    for(i=varSgn[0]-rightVarSgn; i>=0 && i< nRoots; i++)
      if(lowerBnds[i] < rightx && upperBnds[i] > rightx) lowerBnds[i] = rightx;

    // We have to do the check because we can get crazy information with some points.
    // Set the new upper bounds for the intervals to the left of the new interval.
    for(i=0; i < varSgn[0]-leftVarSgn && varSgn[0] - leftVarSgn <= nloc; i++)
      if(upperBnds[i] > leftx && lowerBnds[i] < leftx) upperBnds[i] = leftx;


    if(leftVarSgn-rightVarSgn==1 || hitroot) nloc--;
    }

  int nIntervals = nRoots;
  bool* bisection = new bool[nRoots];

  // 3. Root polishing (if needed)
  for(nloc = 0; nloc < nRoots; nloc++)
    {
    if(upperBnds[nloc] - lowerBnds[nloc] < tol) 
      {
      continue;
      }

    double zv = evaluateHorner(P, d, upperBnds[nloc]);
    double z;

    if(IsZero(zv)) 
      {
      lowerBnds[nloc] = upperBnds[nloc];
      continue;
      }
    
    // If we can, use bisection.
    if(zv*evaluateHorner(P, d, lowerBnds[nloc])<0)
      {
      double tempu = zv;
      while(upperBnds[nloc] - lowerBnds[nloc] > tol)
        {
        z = (upperBnds[nloc] + lowerBnds[nloc])/2;
        // Sometimes the tolerance can be poorly chosen causing an infinite
        // loop, this should fix that. That is, u - l > tol, but (u+l)/2 == l|u.
        if( z >= upperBnds[nloc] || z <= lowerBnds[nloc] )
          {
          break;
          }
        zv = evaluateHorner(P, d, z);
        if(zv*tempu > 0)
          {
          tempu = zv;
          upperBnds[nloc] = z;
          }
        else
          lowerBnds[nloc] = z;
        }
      bisection[nloc] = true;
      }
    // Otherwise use a sequence. This is of course MUCH slower.
    else
      {
      while(upperBnds[nloc] - lowerBnds[nloc] > tol)
        {
        z = (upperBnds[nloc] + lowerBnds[nloc])/2;
        // Sometimes the tolerance can be poorly chosen causing an infinite
        // loop, this should fix that. That is, u - l > tol, but (u+l)/2 == l|u.
        if( z >= upperBnds[nloc] || z <= lowerBnds[nloc] )
          {
          break;
          }
        if((varSgn[0] - vtkGetSignChanges(SSS, degrees, offsets, nSSS, z) == nloc + 1)) 
          upperBnds[nloc] = z;
        else
          lowerBnds[nloc] = z;
        }
      bisection[nloc] = false;
      }
    }

  // Though theoretically this shouldn't happen, sometimes the roots are out of 
  // order. Lets sort them just in case, because it does happen sometimes.
  qsort(upperBnds, nIntervals, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots);
  qsort(lowerBnds, nIntervals, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots);

  // Remove duplicate roots.
  for(int j = 1; j < nIntervals; j++)
    {
    if((upperBnds[j] < upperBnds[j-1]+2*tol || lowerBnds[j] < lowerBnds[j-1]+2*tol || (zeroroot && fabs(upperBnds[j]) < 2*tol)))
      {
      for(int k = j+1; k < nIntervals; k++)
        {
        upperBnds[k-1] = upperBnds[k];
        lowerBnds[k-1] = lowerBnds[k];
        }
      j--;
      nIntervals--;
      continue;
      }
    }

  //if(lowerBnds)
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
// d-th degree polynomial using the Sturm sequence.
// intervalType specifies as follows (in binary)
// 0 = 00 = ]a,b[
// 1 = 10 = [a,b[
// 2 = 01 = ]a,b]
// 3 = 11 = [a,b]
int vtkPolynomialSolversUnivariate::SturmBisectionSolve( double* P, int d, double* a, 
  double *upperBnds, double tol, int intervalType, bool divideGCD )
{
  return vtkSturmBisectionSolve(P, d, a, upperBnds, tol, intervalType, divideGCD?1:0);
}

int vtkPolynomialSolversUnivariate::SturmBisectionSolve( double* P, int d, double* a, 
  double *upperBnds, double tol, int intervalType )
{
  return vtkSturmBisectionSolve(P, d, a, upperBnds, tol, intervalType, 0);
}

int vtkPolynomialSolversUnivariate::SturmBisectionSolve( double* P, int d, double* a, 
  double *upperBnds, double tol )
{
  return vtkSturmBisectionSolve(P, d, a, upperBnds, tol, 0, 0);
}

// Assume that dP = {f} and p is the degree of f.
// Furthermore assume that dP is large enough.
// Stores
// {f,f',f^(2)/2!,f^(3)/3!,...,f^(p)/p!}.
void vtkGetDerivativeSequence(double* dP, int p)
{
  int offsetA = 0;
  int offsetB = p+1;

  for(int i = 1; i <= p; i++)
    {
    for(int j = 0; j <= p-i; j++)
      dP[offsetB + j] = static_cast<double>(p-i-j+1)*dP[offsetA + j]/i;

    offsetA = offsetB;
    offsetB += p-i+1;
    }
}

int vtkGetSignChangesForDerivativeSequence( double* dP, int count, double val)
{
  int oldVal = 0;
  double v;
  int changes = 0;
  int offset = 0;

  for(int i = 0; i <= count; i++)
    {
    v = evaluateHorner(dP + offset, count-i, val);

    if(v*oldVal < 0)
      {
      changes++;
      oldVal = 0-oldVal;
      }
    if(oldVal == 0)
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
    offset += count-i+1;
    }

  return changes;
}

int vtkPolynomialSolversUnivariate::FilterRoots(double* P, int d, double *upperBnds, int rootcount, double diameter)
{ 
  // Sort the roots.
  qsort(upperBnds, rootcount, sizeof(double), vtkPolynomialSolversUnivariateCompareRoots);

  // Remove duplicates.
  for(int j = 1; j < rootcount; j++)
    {
    if( upperBnds[j] < upperBnds[j-1]+2e-3)
      {
      for(int k = j+1; k < rootcount; k++)
        {
        upperBnds[k-1] = upperBnds[k];
        }
      j--;
      rootcount--;
      continue;
      }
    }

  if( rootcount == 0 )
    {
    return 0;
    }

  // Ignore 0 as a root.
  for ( int i = d; i >= 0; i-- )
    {
    if( IsZero(P[i]) )
      {
      d--;
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

extern "C" {
  int vtkPolynomialSolversUnivariateCompareRoots(const void* a, const void* b)
  {
    return (*((const double*)a)) < (*((const double*)b)) ? -1 : 1; 
  }
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
        int nr = vtkMath::SolveQuadratic( cc, r, m );
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
  int nr = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( cc, cr, cm, tol );

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

void vtkPolynomialSolversUnivariate::SetDivisionTolerance( double tol )
{
  vtkPolynomialSolversUnivariate::DivisionTolerance = tol;
}

double vtkPolynomialSolversUnivariate::GetDivisionTolerance()
{
  return vtkPolynomialSolversUnivariate::DivisionTolerance;
}
