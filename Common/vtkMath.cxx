/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2005 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@ca.sandia.gov,dcthomp@sandia.gov,

=========================================================================*/
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"

vtkCxxRevisionMacro(vtkMath, "1.106");
vtkStandardNewMacro(vtkMath);

long vtkMath::Seed = 1177; // One authors home address
static const double sqrt3 = sqrt( (double)3. );
static const double inv3 = 1 / 3.;

//
// some constants we need
//
#define VTK_K_A 16807
#define VTK_K_M 2147483647              /* Mersenne prime 2^31 -1 */
#define VTK_K_Q 127773                  /* VTK_K_M div VTK_K_A */
#define VTK_K_R 2836                    /* VTK_K_M mod VTK_K_A */

//
// Some useful macros and functions
//
#define VTK_SIGN(x)              (( (x) < 0 )?( -1 ):( 1 ))
// avoid dll boundary problems

//----------------------------------------------------------------------------
// Generate random numbers between 0.0 and 1.0.
// This is used to provide portability across different systems.
double vtkMath::Random()
{
  long hi, lo;
    
  // Based on code in "Random Number Generators: Good Ones are Hard to Find,"
  // by Stephen K. Park and Keith W. Miller in Communications of the ACM,
  // 31, 10 (Oct. 1988) pp. 1192-1201.
  // Borrowed from: Fuat C. Baran, Columbia University, 1988.
  hi = vtkMath::Seed / VTK_K_Q;
  lo = vtkMath::Seed % VTK_K_Q;
  if ((vtkMath::Seed = VTK_K_A * lo - VTK_K_R * hi) <= 0)
    {
    Seed += VTK_K_M;
    }
  return ((double) vtkMath::Seed / VTK_K_M);
}

//----------------------------------------------------------------------------
// Initialize seed value. NOTE: Random() has the bad property that 
// the first random number returned after RandomSeed() is called 
// is proportional to the seed value! To help solve this, call 
// RandomSeed() a few times inside seed. This doesn't ruin the 
// repeatability of Random().
//
void vtkMath::RandomSeed(long s)
{
  vtkMath::Seed = s;

  vtkMath::Random();
  vtkMath::Random();
  vtkMath::Random();
}

//----------------------------------------------------------------------------
// The number of combinations of n objects from a pool of m objects (m>n).
//
vtkTypeInt64 vtkMath::Binomial( int m, int n )
{
  if ( m < n )
    {
    return -1;
    }
  else if ( m == n )
    {
    return 1;
    }

  int n1 = n;
  int n2 = m - n;
  if ( n2 > n1 )
    {
    n1 = n2;
    n2 = n;
    }
  vtkTypeInt64 r = 1;
  while ( m > n1 )
    {
    r *= m--;
    }
  while ( n2 > 1 )
    {
    r /= n2--;
    }
  return r;
}

//----------------------------------------------------------------------------
// Start iterating over "m choose n" objects.
// This function returns an array of n integers, each from 0 to m-1.
// These integers represent the n items chosen from the set [0,m[. 
//
int* vtkMath::BeginCombination( int m, int n )
{
  if ( m < n )
    {
    return 0;
    }

  int* r = new int[ n ];
  for ( int i=0; i<n; ++i )
    {
    r[i] = i;
    }
  return r;
}

//----------------------------------------------------------------------------
// Given \a m, \a n, and a valid \a combination of \a n integers in
// the range [0,m[, this function alters the integers into the next
// combination in a sequence of all combinations of \a n items from
// a pool of \a m.
// If the \a combination is the last item in the sequence on input,
// then \a combination is unaltered and 0 is returned.
// Otherwise, 1 is returned and \a combination is updated.
//
int vtkMath::NextCombination( int m, int n, int* r )
{
  int a = n - 1;
  if ( r[a] == m - 1 ) {
    int i = 1;
    while ( (a >= 0) && (r[a] == m - i) )
      {
      --a;
      ++i;
      }
    if ( a < 0 )
      {
      // we're done
      return 1;
      }
    r[a]++;
    for ( i=a+1; i<=n-1; ++i )
      {
      r[i] = r[i-1] + 1;
      }
  } else {
    r[a]++;
  }
  return 0;
}

//----------------------------------------------------------------------------
// Free the "iterator" array created by vtkMath::BeginCombination.
//
void vtkMath::FreeCombination( int* r )
{
  delete [] r;
}

//----------------------------------------------------------------------------
// Find unit vectors which is perpendicular to this on and to
// each other.
void vtkMath::Perpendiculars(const double x[3], double y[3], double z[3],
                             double theta)
{
  int dx,dy,dz;
  double x2 = x[0]*x[0];
  double y2 = x[1]*x[1];
  double z2 = x[2]*x[2];
  double r = sqrt(x2 + y2 + z2);

  // transpose the vector to avoid divide-by-zero error
  if (x2 > y2 && x2 > z2)
  {
    dx = 0; dy = 1; dz = 2;
  }
  else if (y2 > z2) 
  {
    dx = 1; dy = 2; dz = 0;
  }
  else 
  {
    dx = 2; dy = 0; dz = 1;
  }

  double a = x[dx]/r;
  double b = x[dy]/r;
  double c = x[dz]/r;

  double tmp = sqrt(a*a+c*c);

  if (theta != 0)
    {
    double sintheta = sin(theta);
    double costheta = cos(theta);

    if (y)
    {
      y[dx] = (c*costheta - a*b*sintheta)/tmp;
      y[dy] = sintheta*tmp;
      y[dz] = (-a*costheta - b*c*sintheta)/tmp;
    }

    if (z)
      {
      z[dx] = (-c*sintheta - a*b*costheta)/tmp;
      z[dy] = costheta*tmp;
      z[dz] = (a*sintheta - b*c*costheta)/tmp;
      }
    }
  else
    {
    if (y)
    {
      y[dx] = c/tmp;
      y[dy] = 0;
      y[dz] = -a/tmp;
    }

    if (z)
      {
      z[dx] = -a*b/tmp;
      z[dy] = tmp;
      z[dz] = -b*c/tmp;
      }
    }      
}

//----------------------------------------------------------------------------
// Find unit vectors which are perpendicular to this one and to
// each other.
void vtkMath::Perpendiculars(const float x[3], float y[3], float z[3],
                             double theta)
{
  int dx,dy,dz;
  double x2 = x[0]*x[0];
  double y2 = x[1]*x[1];
  double z2 = x[2]*x[2];
  double r = sqrt(x2 + y2 + z2);

  // transpose the vector to avoid divide-by-zero error
  if (x2 > y2 && x2 > z2)
  {
    dx = 0; dy = 1; dz = 2;
  }
  else if (y2 > z2) 
  {
    dx = 1; dy = 2; dz = 0;
  }
  else 
  {
    dx = 2; dy = 0; dz = 1;
  }

  double a = x[dx]/r;
  double b = x[dy]/r;
  double c = x[dz]/r;

  double tmp = sqrt(a*a+c*c);

  if (theta != 0)
    {
    double sintheta = sin(theta);
    double costheta = cos(theta);

    if (y)
    {
      y[dx] = (c*costheta - a*b*sintheta)/tmp;
      y[dy] = sintheta*tmp;
      y[dz] = (-a*costheta - b*c*sintheta)/tmp;
    }

    if (z)
      {
      z[dx] = (-c*sintheta - a*b*costheta)/tmp;
      z[dy] = costheta*tmp;
      z[dz] = (a*sintheta - b*c*costheta)/tmp;
      }
    }
  else
    {
    if (y)
    {
      y[dx] = c/tmp;
      y[dy] = 0;
      y[dz] = -a/tmp;
    }

    if (z)
      {
      z[dx] = -a*b/tmp;
      z[dy] = tmp;
      z[dz] = -b*c/tmp;
      }
    }      
}

#define VTK_SMALL_NUMBER 1.0e-12

//----------------------------------------------------------------------------
// Solve linear equations Ax = b using Crout's method. Input is square matrix A
// and load vector x. Solution x is written over load vector. The dimension of
// the matrix is specified in size. If error is found, method returns a 0.
int vtkMath::SolveLinearSystem(double **A, double *x, int size)
{
  // if we solving something simple, just solve it
  //
  if (size == 2)
    {
    double det, y[2];

    det = vtkMath::Determinant2x2(A[0][0], A[0][1], A[1][0], A[1][1]);

    if (det == 0.0)
      {
      // Unable to solve linear system
      return 0;
      }

    y[0] = (A[1][1]*x[0] - A[0][1]*x[1]) / det;
    y[1] = (-A[1][0]*x[0] + A[0][0]*x[1]) / det;

    x[0] = y[0];
    x[1] = y[1];
    return 1;
    }
  else if (size == 1)
    {
    if (A[0][0] == 0.0)
      {
      // Unable to solve linear system
      return 0;
      }
    
    x[0] /= A[0][0];
    return 1;
    }

  //
  // System of equations is not trivial, use Crout's method
  //
  
  // Check on allocation of working vectors
  //
  int *index, scratch[10];
  index = ( size < 10 ? scratch : new int[size] );

  //
  // Factor and solve matrix
  //
  if ( vtkMath::LUFactorLinearSystem(A, index, size) == 0 )
    {
    return 0;
    }
  vtkMath::LUSolveLinearSystem(A,index,x,size);

  if (size >= 10 ) delete [] index;
  return 1;
}

//----------------------------------------------------------------------------
// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
int vtkMath::InvertMatrix(double **A, double **AI, int size)
{
  int *index, iScratch[10];
  double *column, dScratch[10];

  // Check on allocation of working vectors
  //
  if ( size <= 10 ) 
    {
    index = iScratch;
    column = dScratch;
    }
  else
    {
    index = new int[size];
    column = new double[size];
    }

  int retVal = vtkMath::InvertMatrix(A, AI, size, index, column);

  if ( size > 10 ) 
    {
    delete [] index;
    delete [] column;
    } 
  
  return retVal;
}

//----------------------------------------------------------------------------
// Factor linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is 
// found, method returns 0. 
int vtkMath::LUFactorLinearSystem(double **A, int *index, int size)
{
  double scratch[10];
  double *scale = (size<10 ? scratch : new double[size]);

  int i, j, k;
  int maxI = 0;
  double largest, temp1, temp2, sum;

  //
  // Loop over rows to get implicit scaling information
  //
  for ( i = 0; i < size; i++ ) 
    {
    for ( largest = 0.0, j = 0; j < size; j++ ) 
      {
      if ( (temp2 = fabs(A[i][j])) > largest )
        {
        largest = temp2;
        }
      }

    if ( largest == 0.0 )
      {
      vtkGenericWarningMacro(<<"Unable to factor linear system");
      return 0;
      }
      scale[i] = 1.0 / largest;
    }
  //
  // Loop over all columns using Crout's method
  //
  for ( j = 0; j < size; j++ ) 
    {
    for (i = 0; i < j; i++) 
      {
      sum = A[i][j];
      for ( k = 0; k < i; k++ )
        {
        sum -= A[i][k] * A[k][j];
        }
      A[i][j] = sum;
      }
    //
    // Begin search for largest pivot element
    //
    for ( largest = 0.0, i = j; i < size; i++ ) 
      {
      sum = A[i][j];
      for ( k = 0; k < j; k++ )
        {
        sum -= A[i][k] * A[k][j];
        }
      A[i][j] = sum;

      if ( (temp1 = scale[i]*fabs(sum)) >= largest ) 
        {
        largest = temp1;
        maxI = i;
        }
      }
    //
    // Check for row interchange
    //
    if ( j != maxI ) 
      {
      for ( k = 0; k < size; k++ ) 
        {
        temp1 = A[maxI][k];
        A[maxI][k] = A[j][k];
        A[j][k] = temp1;
        }
      scale[maxI] = scale[j];
      }
    //
    // Divide by pivot element and perform elimination
    //
    index[j] = maxI;

    if ( fabs(A[j][j]) <= VTK_SMALL_NUMBER )
      {
      vtkGenericWarningMacro(<<"Unable to factor linear system");
      return 0;
      }

    if ( j != (size-1) ) 
      {
      temp1 = 1.0 / A[j][j];
      for ( i = j + 1; i < size; i++ )
        {
        A[i][j] *= temp1;
        }
      }
    }

  if (size >= 10 ) delete [] scale;

  return 1;
}

//----------------------------------------------------------------------------
// Solve linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// factored matrix A=LU, integer array of pivot indices index[0->n-1],
// load vector x[0->n-1], and size of square matrix n. Note that A=LU and
// index[] are generated from method LUFactorLinearSystem). Also, solution
// vector is written directly over input load vector.
void vtkMath::LUSolveLinearSystem(double **A, int *index, 
                                  double *x, int size)
{
  int i, j, ii, idx;
  double sum;
//
// Proceed with forward and backsubstitution for L and U
// matrices.  First, forward substitution.
//
  for ( ii = -1, i = 0; i < size; i++ ) 
    {
    idx = index[i];
    sum = x[idx];
    x[idx] = x[i];

    if ( ii >= 0 )
      {
      for ( j = ii; j <= (i-1); j++ )
        {
        sum -= A[i][j]*x[j];
        }
      }
    else if (sum)
      {
      ii = i;
      }

    x[i] = sum;
  }
//
// Now, back substitution
//
  for ( i = size-1; i >= 0; i-- ) 
    {
    sum = x[i];
    for ( j = i + 1; j < size; j++ )
      {
      sum -= A[i][j]*x[j];
      }
    x[i] = sum / A[i][i];
    }
}

#undef VTK_SMALL_NUMBER

#define VTK_ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
        a[k][l]=h+s*(g-h*tau)

#define VTK_MAX_ROTATIONS 20

//#undef VTK_MAX_ROTATIONS

//#define VTK_MAX_ROTATIONS 50

// Jacobi iteration for the solution of eigenvectors/eigenvalues of a nxn
// real symmetric matrix. Square nxn matrix a; size of matrix in n;
// output eigenvalues in w; and output eigenvectors in v. Resulting
// eigenvalues/vectors are sorted in decreasing order; eigenvectors are
// normalized.
template<class T>
int vtkJacobiN(T **a, int n, T *w, T **v)
{
  int i, j, k, iq, ip, numPos;
  T tresh, theta, tau, t, sm, s, h, g, c, tmp;
  T bspace[4], zspace[4];
  T *b = bspace;
  T *z = zspace;

  // only allocate memory if the matrix is large
  if (n > 4)
    {
    b = new T[n];
    z = new T[n]; 
    }

  // initialize
  for (ip=0; ip<n; ip++) 
    {
    for (iq=0; iq<n; iq++)
      {
      v[ip][iq] = 0.0;
      }
    v[ip][ip] = 1.0;
    }
  for (ip=0; ip<n; ip++) 
    {
    b[ip] = w[ip] = a[ip][ip];
    z[ip] = 0.0;
    }

  // begin rotation sequence
  for (i=0; i<VTK_MAX_ROTATIONS; i++) 
    {
    sm = 0.0;
    for (ip=0; ip<n-1; ip++) 
      {
      for (iq=ip+1; iq<n; iq++)
        {
        sm += fabs(a[ip][iq]);
        }
      }
    if (sm == 0.0)
      {
      break;
      }

    if (i < 3)                                // first 3 sweeps
      {
      tresh = 0.2*sm/(n*n);
      }
    else
      {
      tresh = 0.0;
      }

    for (ip=0; ip<n-1; ip++) 
      {
      for (iq=ip+1; iq<n; iq++) 
        {
        g = 100.0*fabs(a[ip][iq]);

        // after 4 sweeps
        if (i > 3 && (fabs(w[ip])+g) == fabs(w[ip])
        && (fabs(w[iq])+g) == fabs(w[iq]))
          {
          a[ip][iq] = 0.0;
          }
        else if (fabs(a[ip][iq]) > tresh) 
          {
          h = w[iq] - w[ip];
          if ( (fabs(h)+g) == fabs(h))
            {
            t = (a[ip][iq]) / h;
            }
          else 
            {
            theta = 0.5*h / (a[ip][iq]);
            t = 1.0 / (fabs(theta)+sqrt(1.0+theta*theta));
            if (theta < 0.0)
              {
              t = -t;
              }
            }
          c = 1.0 / sqrt(1+t*t);
          s = t*c;
          tau = s/(1.0+c);
          h = t*a[ip][iq];
          z[ip] -= h;
          z[iq] += h;
          w[ip] -= h;
          w[iq] += h;
          a[ip][iq]=0.0;

          // ip already shifted left by 1 unit
          for (j = 0;j <= ip-1;j++) 
            {
            VTK_ROTATE(a,j,ip,j,iq);
            }
          // ip and iq already shifted left by 1 unit
          for (j = ip+1;j <= iq-1;j++) 
            {
            VTK_ROTATE(a,ip,j,j,iq);
            }
          // iq already shifted left by 1 unit
          for (j=iq+1; j<n; j++) 
            {
            VTK_ROTATE(a,ip,j,iq,j);
            }
          for (j=0; j<n; j++) 
            {
            VTK_ROTATE(v,j,ip,j,iq);
            }
          }
        }
      }

    for (ip=0; ip<n; ip++) 
      {
      b[ip] += z[ip];
      w[ip] = b[ip];
      z[ip] = 0.0;
      }
    }

  //// this is NEVER called
  if ( i >= VTK_MAX_ROTATIONS )
    {
    vtkGenericWarningMacro(
       "vtkMath::Jacobi: Error extracting eigenfunctions");
    return 0;
    }

  // sort eigenfunctions                 these changes do not affect accuracy 
  for (j=0; j<n-1; j++)                  // boundary incorrect
    {
    k = j;
    tmp = w[k];
    for (i=j+1; i<n; i++)                // boundary incorrect, shifted already
      {
      if (w[i] >= tmp)                   // why exchage if same?
        {
        k = i;
        tmp = w[k];
        }
      }
    if (k != j) 
      {
      w[k] = w[j];
      w[j] = tmp;
      for (i=0; i<n; i++) 
        {
        tmp = v[i][j];
        v[i][j] = v[i][k];
        v[i][k] = tmp;
        }
      }
    }
  // insure eigenvector consistency (i.e., Jacobi can compute vectors that
  // are negative of one another (.707,.707,0) and (-.707,-.707,0). This can
  // reek havoc in hyperstreamline/other stuff. We will select the most
  // positive eigenvector.
  int ceil_half_n = (n >> 1) + (n & 1);
  for (j=0; j<n; j++)
    {
    for (numPos=0, i=0; i<n; i++)
      {
      if ( v[i][j] >= 0.0 )
        {
        numPos++;
        }
      }
//    if ( numPos < ceil(double(n)/double(2.0)) )
    if ( numPos < ceil_half_n)
      {
      for(i=0; i<n; i++)
        {
        v[i][j] *= -1.0;
        }
      }
    }

  if (n > 4)
    {
    delete [] b;
    delete [] z;
    }
  return 1;
}

#undef VTK_ROTATE
#undef VTK_MAX_ROTATIONS

//----------------------------------------------------------------------------
int vtkMath::JacobiN(float **a, int n, float *w, float **v)
{
  return vtkJacobiN(a,n,w,v);
}

//----------------------------------------------------------------------------
int vtkMath::JacobiN(double **a, int n, double *w, double **v)
{
  return vtkJacobiN(a,n,w,v);
}


//----------------------------------------------------------------------------
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
// real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
// and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
// in decreasing order; eigenvectors are normalized.
int vtkMath::Jacobi(float **a, float *w, float **v)
{
  return vtkMath::JacobiN(a, 3, w, v);
}

//----------------------------------------------------------------------------
int vtkMath::Jacobi(double **a, double *w, double **v)
{
  return vtkMath::JacobiN(a, 3, w, v);
}

//----------------------------------------------------------------------------
// Estimate the condition number of a LU factored matrix. Used to judge the
// accuracy of the solution. The matrix A must have been previously factored
// using the method LUFactorLinearSystem. The condition number is the ratio
// of the infinity matrix norm (i.e., maximum value of matrix component)
// divided by the minimum diagonal value. (This works for triangular matrices
// only: see Conte and de Boor, Elementary Numerical Analysis.)
double vtkMath::EstimateMatrixCondition(double **A, int size)
{
  int i;
  int j;
  double min=VTK_LARGE_FLOAT, max=(-VTK_LARGE_FLOAT);

  // find the maximum value
  for (i=0; i < size; i++)
    {
    for (j=i; j < size; j++)
      {
      if ( fabs(A[i][j]) > max )
        {
        max = fabs(A[i][j]);
        }
      }
    }

  // find the minimum diagonal value
  for (i=0; i < size; i++)
    {
    if ( fabs(A[i][i]) < min )
      {
      min = fabs(A[i][i]);
      }
    }

  if ( min == 0.0 )
    {
    return VTK_LARGE_FLOAT;
    }
  else
    {
    return (max/min);
    }
}

//----------------------------------------------------------------------------
// Solves a cubic equation c0*t^3  + c1*t^2  + c2*t + c3 = 0 when
// c0, c1, c2, and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Return array contains number of (real) roots (counting multiple roots as one)
// followed by roots themselves. The value in roots[4] is a integer giving
// further information about the roots (see return codes for int SolveCubic()).
double* vtkMath::SolveCubic( double c0, double c1, double c2, double c3 ) 
{
  static double roots[5];
  roots[1] = 0.0;
  roots[2] = 0.0;
  roots[3] = 0.0;
  int num_roots;

  roots[4] = vtkMath::SolveCubic(c0, c1, c2, c3, 
                                 &roots[1], &roots[2], &roots[3], &num_roots );
  roots[0] = num_roots;
  return roots;
}

//----------------------------------------------------------------------------
// Solves a \a d -th degree polynomial equation using Lin-Bairstow's method.
//
int vtkMath::LinBairstowSolve( double* c, int d, double* r, double& tolerance )
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
        if ( ! ( nIterations % 200 ) ) tolerance *= 10.;
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

static int vtkMathCompareRoots( const void* a, const void* b )
{
  return (*((const double*)a)) < (*((const double*)b)) ? -1 : 1; 
}

// Algebraically extracts REAL roots of the quartic polynomial with 
// REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
// and stores them (when they exist) and their respective multiplicities
// in the \a r and \a m arrays.
int vtkMath::FerrariSolve( double* c, double* r, int* m )
{
  // step 0: eliminate trivial cases up to numerical noise
  if ( fabs( c[3] ) < VTK_DBL_EPSILON )
    {
    if ( fabs( c[2] ) < VTK_DBL_EPSILON )
      {
      if ( fabs( c[1] ) < VTK_DBL_EPSILON )
        {
        if ( fabs( c[0] ) < VTK_DBL_EPSILON )
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
        double cc[2];
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
      int nr = vtkMath::TartagliaCardanSolve( c - 1, r, m );
      r[nr] = 0.;
      m[nr] = 1;
      return nr + 1;
      }
    }
  if ( ( fabs( c[0] ) < VTK_DBL_EPSILON ) && ( fabs( c[2] ) < VTK_DBL_EPSILON ) )
    {
    if ( fabs( c[1] ) < VTK_DBL_EPSILON )
      {
      if ( c[3] < 0. ) return 0;
      r[0] = sqrt( sqrt( c[3] ) );
      m[0] = 4;
      return 1;
      }
    double cc[2], cr[2];
    int cm[2];
    cc[0] = 1.;
    cc[1] = c[1];
    cc[2] = c[3];
    int nr1 = vtkMath::SolveQuadratic( cc, cr, cm );
    int nr = 0;
    int i;
    for ( i = 0; i < nr1; ++ i )
      {
      if ( fabs( cr[i] ) < VTK_DBL_EPSILON )
        {
        r[nr] = 0.;
        m[nr ++] = 2 * cm[i];
        }
      else 
        {
        if ( cr[i] > VTK_DBL_EPSILON )
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
  if ( fabs( b ) < VTK_DBL_EPSILON )
    {
    double cc[2], cr[2];
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
      if ( fabs( cr[i] ) < VTK_DBL_EPSILON )
        {
        r[nr] = shift;
        m[nr ++] = 2 * cm[i];
        }
      else 
        {
        if ( cr[i] > VTK_DBL_EPSILON )
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
  double cc[4], cr[3];
  double unsorted[8];
  int cm[3];
  cc[1] = 2. * a;
  cc[2] = a * a - 4. * d;
  cc[3] = - b * b;
  int nr = vtkMath::TartagliaCardanSolve( cc, cr, cm );

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
  qsort( unsorted, nr, 2*sizeof( double ), vtkMathCompareRoots );
  r[0] = unsorted[0];
  m[0] = ( int ) unsorted[1];
  nr1 = 1;
  for ( i = 1; i < nr; ++ i )
    {
    if ( unsorted[2*i] == unsorted[2*i - 2] )
      {
      m[i - 1] += ( int ) unsorted[2*i + 1]; 
      continue;
      }
    r[nr1] = unsorted[2*i];
    m[nr1++] = ( int ) unsorted[2*i + 1];
    }
  double shift = - c[0] * .25;
  for ( i = 0; i < nr1; ++ i ) r[i] += shift;

  return nr1;
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
int vtkMath::SolveCubic( double c0, double c1, double c2, double c3, 
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
        *r2 = -2.0*sqrt(Q)*cos( (theta + 2.0*3.141592653589)/3.0) - c1/3.0;
        *r3 = -2.0*sqrt(Q)*cos( (theta - 2.0*3.141592653589)/3.0) - c1/3.0;

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
        if( (*r1 == *r2) )
          {
          *num_roots = 1;
          }
        }
      return *num_roots;
      }
    else //single real and complex conjugate pair
      {
      A = -VTK_SIGN(R) * pow(fabs(R) + sqrt(R_squared - Q_cubed),0.33333333);

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
    return vtkMath::SolveQuadratic( c1, c2, c3, r1, r2, num_roots );
    }
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the cubic polynomial with 
// REAL coefficients X^3 + c[1] X^2 + c[2] X + c[3]
// and stores them (when they exist) and their respective multiplicities.
// The main differences with SolveCubic are that (1) the polynomial must have
// unit leading coefficient, (2) no information is returned regarding complex
// roots, and (3) non-simple roots are stored only once -- this is a
// specialized solver.
// Returns the number of roots.
// 
int vtkMath::TartagliaCardanSolve( double* c, double* r, int* m )
{
  // step 0: eliminate trivial cases up to numerical noise
  if ( fabs( c[3] ) < VTK_DBL_EPSILON )
    {
    r[0] = 0.;
    if ( fabs( c[2] ) < VTK_DBL_EPSILON )
      {
      if ( fabs( c[1] ) < VTK_DBL_EPSILON )
        {
        m[0] = 3;
        return 1;
        }
      else
        {
        m[0] = 2;
        r[1] = - c[1];
        m[1] = 1;
        return 2;
        }
      }
    else
      {
      m[0] = 1;
      double delta = c[1] * c[1] - 4. * c[2];
      if ( delta > VTK_DBL_EPSILON )
        {
        delta = sqrt( delta );
        r[1] = ( - delta - c[1] ) * 0.5;
        m[1] = 1;
        r[2] = ( delta - c[1] ) * 0.5;
        m[2] = 1;
        return 3;
        }
      else
        {
        if ( delta < - VTK_DBL_EPSILON ) return 1;
        r[1] = - c[1] * 0.5;
        m[1] = 2;
        return 2;
        }
      }
    }

  // step 1: reduce to X^3 + pX + q
  double shift = - c[1] / 3.;
  double a2 = c[1] * c[1];
  double p = c[2] - a2 / 3.;
  double q = c[1] * ( 2. * a2 / 9. - c[2] ) / 3. + c[3];

  // step 2: compute the trivial real roots if p or q are 0
  // case 2.1: p = 0: 1 triple real root
  if ( fabs( p ) < VTK_DBL_EPSILON )
    {
    if ( fabs( q ) < VTK_DBL_EPSILON )
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
  if ( fabs( q ) < VTK_DBL_EPSILON )
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
  if ( fabs( D ) < VTK_DBL_EPSILON )
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
// Solves a quadratic equation c1*t^2 + c2*t + c3 = 0 when c1, c2, and
// c3 are REAL.  Solution is motivated by Numerical Recipes In C 2nd
// Ed.  Return array contains number of (real) roots (counting
// multiple roots as one) followed by roots themselves. Note that 
// roots[3] contains a return code further describing solution - see
// documentation for SolveCubic() for meaining of return codes.
double* vtkMath::SolveQuadratic( double c1, double c2, double c3) 
{
  static double roots[4];
  roots[0] = 0.0;
  roots[1] = 0.0;
  roots[2] = 0.0;
  int num_roots;

  roots[3] = vtkMath::SolveQuadratic( c1, c2, c3, &roots[1], &roots[2], 
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
int vtkMath::SolveQuadratic( double c1, double c2, double c3, 
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
    return vtkMath::SolveLinear( c2, c3, r1, num_roots );
    }
}

//----------------------------------------------------------------------------
// Algebraically extracts REAL roots of the quadratic polynomial with 
// REAL coefficients c[0] X^2 + c[1] X + c[2]
// and stores them (when they exist) and their respective multiplicities.
// Returns either the number of roots, or -1 if ininite number of roots.
int vtkMath::SolveQuadratic( double* c, double* r, int* m )
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
double* vtkMath::SolveLinear( double c2, double c3) 
{
  static double roots[3];
  int num_roots;
  roots[1] = 0.0;
  roots[2] = vtkMath::SolveLinear( c2, c3, &roots[1], &num_roots );
  roots[0] = num_roots;
  return roots;
}

//----------------------------------------------------------------------------
// Solves a linear equation c2*t + c3 = 0 when c2 and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Root and number of (real) roots are stored in user provided variables
// r2 and num_roots.
int vtkMath::SolveLinear( double c2, double c3, double *r1, int *num_roots )
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
// Solves for the least squares best fit matrix for the homogeneous equation X'M' = 0'.
// Uses the method described on pages 40-41 of Computer Vision by 
// Forsyth and Ponce, which is that the solution is the eigenvector 
// associated with the minimum eigenvalue of T(X)X, where T(X) is the
// transpose of X.
// The inputs and output are transposed matrices.
//    Dimensions: X' is numberOfSamples by xOrder,
//                M' dimension is xOrder by 1.
// M' should be pre-allocated. All matrices are row major. The resultant
// matrix M' should be pre-multiplied to X' to get 0', or transposed and
// then post multiplied to X to get 0
int vtkMath::SolveHomogeneousLeastSquares(int numberOfSamples, double **xt, int xOrder,
                                double **mt)
  {
  // check dimensional consistency
  if (numberOfSamples < xOrder)
    {
    vtkGenericWarningMacro("Insufficient number of samples. Underdetermined.");
    return 0;
    }

  int i, j, k;

  // set up intermediate variables
  // Allocate matrix to hold X times transpose of X
  double **XXt = new double *[xOrder];     // size x by x
  // Allocate the array of eigenvalues and eigenvectors
  double *eigenvals = new double [xOrder];
  double **eigenvecs = new double *[xOrder];


  // Clear the upper triangular region (and btw, allocate the eigenvecs as well)
  for (i = 0; i < xOrder; i++)
    {
    eigenvecs[i] = new double[xOrder];
    XXt[i] = new double[xOrder];
    for (j = 0; j < xOrder; j++)
      {
      XXt[i][j] = 0.0;
      }
    }

  // Calculate XXt upper half only, due to symmetry
  for (k = 0; k < numberOfSamples; k++)
    {
    for (i = 0; i < xOrder; i++)
      {
      for (j = i; j < xOrder; j++)
        {
        XXt[i][j] += xt[k][i] * xt[k][j];
        }
      }
    }

  // now fill in the lower half of the XXt matrix
  for (i = 0; i < xOrder; i++)
    {
    for (j = 0; j < i; j++)
      {
      XXt[i][j] = XXt[j][i];
      }
    }

  // Compute the eigenvectors and eigenvalues
  vtkMath::JacobiN(XXt, xOrder, eigenvals, eigenvecs);

  // Smallest eigenval is at the end of the list (xOrder-1), and solution is
  // corresponding eigenvec. 
  for (i=0; i<xOrder; i++)
    {
    mt[i][0] = eigenvecs[i][xOrder-1];
    }

  // Clean up:
  for (i=0; i<xOrder; i++)
    {
    delete [] XXt[i];
    delete [] eigenvecs[i];
    }
  delete [] XXt;
  delete [] eigenvecs;
  delete [] eigenvals;
  
  return 1;
  }
#define VTK_SMALL_NUMBER 1.0e-12

//----------------------------------------------------------------------------
// Solves for the least squares best fit matrix for the equation X'M' = Y'.
// Uses pseudoinverse to get the ordinary least squares. 
// The inputs and output are transposed matrices.
//    Dimensions: X' is numberOfSamples by xOrder,
//                Y' is numberOfSamples by yOrder,
//                M' dimension is xOrder by yOrder.
// M' should be pre-allocated. All matrices are row major. The resultant
// matrix M' should be pre-multiplied to X' to get Y', or transposed and
// then post multiplied to X to get Y
// By default, this method checks for the homogeneous condition where Y==0, and
// if so, invokes SolveHomogeneousLeastSquares. For better performance when
// the system is known not to be homogeneous, invoke with checkHomogeneous=0.
int vtkMath::SolveLeastSquares(int numberOfSamples, double **xt, int xOrder,
                               double **yt, int yOrder, double **mt, int checkHomogeneous)
{
  // check dimensional consistency
  if ((numberOfSamples < xOrder) || (numberOfSamples < yOrder))
    {
    vtkGenericWarningMacro("Insufficient number of samples. Underdetermined.");
    return 0;
    }

  int i, j, k;

  int someHomogeneous = 0;
  int allHomogeneous = 1;
  double **hmt = NULL;
  int homogRC = 0;
  int *homogenFlags = new int[yOrder];

  // Ok, first init some flags check and see if all the systems are homogeneous
  if (checkHomogeneous)
    {
    // If Y' is zero, it's a homogeneous system and can't be solved via
    // the pseudoinverse method. Detect this case, warn the user, and
    // invoke SolveHomogeneousLeastSquares instead. Note that it doesn't
    // really make much sense for yOrder to be greater than one in this case,
    // since that's just yOrder occurrences of a 0 vector on the RHS, but
    // we allow it anyway. N


    // Initialize homogeneous flags on a per-right-hand-side basis
    for (j=0; j<yOrder; j++)
      {
      homogenFlags[j] = 1;
      }
    for (i=0; i<numberOfSamples; i++)
      {
      for (j=0; j<yOrder; j++)
        {
        if (fabs(yt[i][j]) > VTK_SMALL_NUMBER)
          {
          allHomogeneous = 0;
          homogenFlags[j] = 0;
          }
        }
      }

    // If we've got one system, and it's homogeneous, do it and bail out quickly.
    if (allHomogeneous && yOrder == 1)
      {
      vtkGenericWarningMacro("Detected homogeneous system (Y=0), calling SolveHomogeneousLeastSquares()");
      return vtkMath::SolveHomogeneousLeastSquares(numberOfSamples, xt, xOrder, mt);
      }


    // Ok, we've got more than one system of equations.
    // Figure out if we need to calculate the homogeneous equation solution for 
    // any of them.
    if (allHomogeneous)
      {
      someHomogeneous = 1;
      }
    else
      {
      for (j=0; j<yOrder; j++)
        {
        if (homogenFlags[j])
          {
          someHomogeneous = 1;
          }
        }
      }
    }

  // If necessary, solve the homogeneous problem
  if (someHomogeneous)
    {
    // hmt is the homogeneous equation version of mt, the general solution.
    hmt = new double *[xOrder];
    for (j=0; j<xOrder; j++)
      {
      // Only allocate 1 here, not yOrder, because here we're going to solve
      // just the one homogeneous equation subset of the entire problem
      hmt[j] = new double [1]; 
      }

    // Ok, solve the homogeneous problem
    homogRC = vtkMath::SolveHomogeneousLeastSquares(numberOfSamples, xt, xOrder, hmt);
    }
  

  // set up intermediate variables
  double **XXt = new double *[xOrder];     // size x by x
  double **XXtI = new double *[xOrder];    // size x by x
  double **XYt = new double *[xOrder];     // size x by y
  for (i = 0; i < xOrder; i++)
    {
    XXt[i] = new double[xOrder];
    XXtI[i] = new double[xOrder];

    for (j = 0; j < xOrder; j++)
      {
      XXt[i][j] = 0.0;
      XXtI[i][j] = 0.0;
      }

    XYt[i] = new double[yOrder];
    for (j = 0; j < yOrder; j++)
      {
      XYt[i][j] = 0.0;
      }
    }

  // first find the pseudoinverse matrix
  for (k = 0; k < numberOfSamples; k++)
    {
    for (i = 0; i < xOrder; i++)
      {
      // first calculate the XXt matrix, only do the upper half (symmetrical)
      for (j = i; j < xOrder; j++)
        {
        XXt[i][j] += xt[k][i] * xt[k][j];
        }

      // now calculate the XYt matrix
      for (j = 0; j < yOrder; j++)
        {
        XYt[i][j] += xt[k][i] * yt[k][j];
        }
      }
    }
  
  // now fill in the lower half of the XXt matrix
  for (i = 0; i < xOrder; i++)
    {
    for (j = 0; j < i; j++)
      {
      XXt[i][j] = XXt[j][i];
      }
    }
  
  // next get the inverse of XXt
  if (!(vtkMath::InvertMatrix(XXt, XXtI, xOrder)))
    {
    return 0;
    }
  
  // next get m
  for (i = 0; i < xOrder; i++)
    {
    for (j = 0; j < yOrder; j++)
      {
      mt[i][j] = 0.0;
      for (k = 0; k < xOrder; k++)
        {
        mt[i][j] += XXtI[i][k] * XYt[k][j];
        }
      }
    }

  // Fix up any of the solutions that correspond to the homogeneous equation
  // problem.
  if (someHomogeneous)
    {
    for (j=0; j<yOrder; j++)
      {
      if (homogenFlags[j])
        {
        // Fix this one
        for (i=0; i<xOrder; i++)
          {
          mt[i][j] = hmt[i][0];
          }
        }
      }

    // Clean up
    for (i=0; i<xOrder; i++)
      {
      delete [] hmt[i];
      }
    delete [] hmt;
    }

  // clean up:
  // set up intermediate variables
  for (i = 0; i < xOrder; i++)
    {
    delete [] XXt[i];
    delete [] XXtI[i];

    delete [] XYt[i];
    }
  delete [] XXt;
  delete [] XXtI;
  delete [] XYt;
  delete [] homogenFlags;
  
  if (someHomogeneous)
    {
    return homogRC;
    }
  else
    {
    return 1;
    }
}

//=============================================================================
// Thread safe versions of math methods.
//=============================================================================


// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
// -----------------------
// For thread safe behavior, temporary arrays tmp1SIze and tmp2Size
// of length size must be passsed in.
int vtkMath::InvertMatrix(double **A, double **AI, int size,
                          int *tmp1Size, double *tmp2Size)
{
  int i, j;

  //
  // Factor matrix; then begin solving for inverse one column at a time.
  // Note: tmp1Size returned value is used later, tmp2Size is just working
  // memory whose values are not used in LUSolveLinearSystem
  //
  if ( vtkMath::LUFactorLinearSystem(A, tmp1Size, size, tmp2Size) == 0 )
    {
    return 0;
    }
  
  for ( j=0; j < size; j++ )
    {
    for ( i=0; i < size; i++ )
      {
      tmp2Size[i] = 0.0;
      }
    tmp2Size[j] = 1.0;

    vtkMath::LUSolveLinearSystem(A,tmp1Size,tmp2Size,size);

    for ( i=0; i < size; i++ )
      {
      AI[i][j] = tmp2Size[i];
      }
    }

  return 1;
}




// Factor linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is 
// found, method returns 0.
//------------------------------------------------------------------
// For thread safe, temporary memory array tmpSize of length size
// must be passed in.
int vtkMath::LUFactorLinearSystem(double **A, int *index, int size,
                                  double *tmpSize)
{
  int i, j, k;
  int maxI = 0;
  double largest, temp1, temp2, sum;

  //
  // Loop over rows to get implicit scaling information
  //
  for ( i = 0; i < size; i++ ) 
    {
    for ( largest = 0.0, j = 0; j < size; j++ ) 
      {
      if ( (temp2 = fabs(A[i][j])) > largest )
        {
        largest = temp2;
        }
      }

    if ( largest == 0.0 )
      {
      vtkGenericWarningMacro(<<"Unable to factor linear system");
      return 0;
      }
      tmpSize[i] = 1.0 / largest;
    }
  //
  // Loop over all columns using Crout's method
  //
  for ( j = 0; j < size; j++ ) 
    {
    for (i = 0; i < j; i++) 
      {
      sum = A[i][j];
      for ( k = 0; k < i; k++ )
        {
        sum -= A[i][k] * A[k][j];
        }
      A[i][j] = sum;
      }
    //
    // Begin search for largest pivot element
    //
    for ( largest = 0.0, i = j; i < size; i++ ) 
      {
      sum = A[i][j];
      for ( k = 0; k < j; k++ )
        {
        sum -= A[i][k] * A[k][j];
        }
      A[i][j] = sum;

      if ( (temp1 = tmpSize[i]*fabs(sum)) >= largest ) 
        {
        largest = temp1;
        maxI = i;
        }
      }
    //
    // Check for row interchange
    //
    if ( j != maxI ) 
      {
      for ( k = 0; k < size; k++ ) 
        {
        temp1 = A[maxI][k];
        A[maxI][k] = A[j][k];
        A[j][k] = temp1;
        }
      tmpSize[maxI] = tmpSize[j];
      }
    //
    // Divide by pivot element and perform elimination
    //
    index[j] = maxI;

    if ( fabs(A[j][j]) <= VTK_SMALL_NUMBER )
      {
      vtkGenericWarningMacro(<<"Unable to factor linear system");
      return 0;
      }

    if ( j != (size-1) ) 
      {
      temp1 = 1.0 / A[j][j];
      for ( i = j + 1; i < size; i++ )
        {
        A[i][j] *= temp1;
        }
      }
    }

  return 1;
}

#undef VTK_SMALL_NUMBER

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// All of the following methods are for dealing with 3x3 matrices
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// helper function, swap two 3-vectors
template<class T>
inline void vtkSwapVectors3(T v1[3], T v2[3])
{
  for (int i = 0; i < 3; i++)
    {
    T tmp = v1[i];
    v1[i] = v2[i];
    v2[i] = tmp;
    }
}

//----------------------------------------------------------------------------
// Unrolled LU factorization of a 3x3 matrix with pivoting.
// This decomposition is non-standard in that the diagonal
// elements are inverted, to convert a division to a multiplication
// in the backsubstitution.
template<class T>
inline void vtkLUFactor3x3(T A[3][3], int index[3])
{
  int i,maxI;
  T tmp,largest;
  T scale[3];

  // Loop over rows to get implicit scaling information

  for ( i = 0; i < 3; i++ ) 
    {
    largest =  fabs(A[i][0]);
    if ((tmp = fabs(A[i][1])) > largest)
      {
      largest = tmp;
      }
    if ((tmp = fabs(A[i][2])) > largest)
      {
      largest = tmp;
      }
    scale[i] = T(1.0)/largest;
    }
  
  // Loop over all columns using Crout's method

  // first column
  largest = scale[0]*fabs(A[0][0]);
  maxI = 0;
  if ((tmp = scale[1]*fabs(A[1][0])) >= largest) 
    {
    largest = tmp;
    maxI = 1;
    }
  if ((tmp = scale[2]*fabs(A[2][0])) >= largest) 
    {
    maxI = 2;
    }
  if (maxI != 0) 
    {
    vtkSwapVectors3(A[maxI],A[0]);
    scale[maxI] = scale[0];
    }
  index[0] = maxI;

  A[0][0] = T(1.0)/A[0][0];
  A[1][0] *= A[0][0];
  A[2][0] *= A[0][0];
    
  // second column
  A[1][1] -= A[1][0]*A[0][1];
  A[2][1] -= A[2][0]*A[0][1];
  largest = scale[1]*fabs(A[1][1]);
  maxI = 1;
  if ((tmp = scale[2]*fabs(A[2][1])) >= largest) 
    {
    maxI = 2;
    vtkSwapVectors3(A[2],A[1]);
    scale[2] = scale[1];
    }
  index[1] = maxI;
  A[1][1] = T(1.0)/A[1][1];
  A[2][1] *= A[1][1];

  // third column
  A[1][2] -= A[1][0]*A[0][2];
  A[2][2] -= A[2][0]*A[0][2] + A[2][1]*A[1][2];
  largest = scale[2]*fabs(A[2][2]);
  index[2] = 2;
  A[2][2] = T(1.0)/A[2][2];
}

//----------------------------------------------------------------------------
void vtkMath::LUFactor3x3(float A[3][3], int index[3])
{
  vtkLUFactor3x3(A,index);
}

//----------------------------------------------------------------------------
void vtkMath::LUFactor3x3(double A[3][3], int index[3])
{
  vtkLUFactor3x3(A,index);
}

//----------------------------------------------------------------------------
// Backsubsitution with an LU-decomposed matrix.  This is the standard
// LU decomposition, except that the diagonals elements have been inverted.
template<class T1, class T2>
inline void vtkLUSolve3x3(const T1 A[3][3], const int index[3], T2 x[3])
{
  T2 sum;

  // forward substitution
  
  sum = x[index[0]];
  x[index[0]] = x[0];
  x[0] = sum;

  sum = x[index[1]];
  x[index[1]] = x[1];
  x[1] = sum - A[1][0]*x[0];

  sum = x[index[2]];
  x[index[2]] = x[2];
  x[2] = sum - A[2][0]*x[0] - A[2][1]*x[1];

  // back substitution
  
  x[2] = x[2]*A[2][2];
  x[1] = (x[1] - A[1][2]*x[2])*A[1][1];
  x[0] = (x[0] - A[0][1]*x[1] - A[0][2]*x[2])*A[0][0];
}  

//----------------------------------------------------------------------------
void vtkMath::LUSolve3x3(const float A[3][3], 
                         const int index[3], float x[3])
{
  vtkLUSolve3x3(A,index,x);
}

//----------------------------------------------------------------------------
void vtkMath::LUSolve3x3(const double A[3][3], 
                         const int index[3], double x[3])
{
  vtkLUSolve3x3(A,index,x);
}

//----------------------------------------------------------------------------
// this method solves Ay = x for y
template<class T1, class T2, class T3>
inline void vtkLinearSolve3x3(const T1 A[3][3], const T2 x[3], T3 y[3])
{
  int index[3];
  T3 B[3][3];
  for (int i = 0; i < 3; i++)
    {
    B[i][0] = A[i][0];
    B[i][1] = A[i][1];
    B[i][2] = A[i][2];
    y[i] = x[i];
    }

  vtkMath::LUFactor3x3(B,index);
  vtkMath::LUSolve3x3(B,index,y);
}

//----------------------------------------------------------------------------
void vtkMath::LinearSolve3x3(const float A[3][3], 
                             const float x[3], float y[3])
{
  vtkLinearSolve3x3(A,x,y);
}

//----------------------------------------------------------------------------
void vtkMath::LinearSolve3x3(const double A[3][3], 
                             const double x[3], double y[3])
{
  vtkLinearSolve3x3(A,x,y);
}

//----------------------------------------------------------------------------
template<class T1, class T2, class T3>
inline void vtkMultiply3x3(const T1 A[3][3], const T2 v[3], T3 u[3])
{
  T3 x = A[0][0]*v[0] + A[0][1]*v[1] + A[0][2]*v[2];
  T3 y = A[1][0]*v[0] + A[1][1]*v[1] + A[1][2]*v[2];
  T3 z = A[2][0]*v[0] + A[2][1]*v[1] + A[2][2]*v[2];

  u[0] = x;
  u[1] = y;
  u[2] = z;
}

//----------------------------------------------------------------------------
void vtkMath::Multiply3x3(const float A[3][3], const float v[3], float u[3])
{
  vtkMultiply3x3(A,v,u);
}

//----------------------------------------------------------------------------
void vtkMath::Multiply3x3(const double A[3][3], const double v[3], double u[3])
{
  vtkMultiply3x3(A,v,u);
}

//----------------------------------------------------------------------------
template<class T, class T2, class T3>
inline void vtkMultiplyMatrix3x3(const T A[3][3], const T2 B[3][3],
                                        T3 C[3][3])
{
  T3 D[3][3];

  for (int i = 0; i < 3; i++)
    {
    D[0][i] = A[0][0]*B[0][i] + A[0][1]*B[1][i] + A[0][2]*B[2][i];
    D[1][i] = A[1][0]*B[0][i] + A[1][1]*B[1][i] + A[1][2]*B[2][i];
    D[2][i] = A[2][0]*B[0][i] + A[2][1]*B[1][i] + A[2][2]*B[2][i];
    }

  for (int j = 0; j < 3; j++)
    {
    C[j][0] = D[j][0];
    C[j][1] = D[j][1];
    C[j][2] = D[j][2];
    }
}

//----------------------------------------------------------------------------
void vtkMath::Multiply3x3(const float A[3][3], 
                          const float B[3][3], float C[3][3])
{
  vtkMultiplyMatrix3x3(A,B,C);
}

//----------------------------------------------------------------------------
void vtkMath::Multiply3x3(const double A[3][3], 
                          const double B[3][3], double C[3][3])
{
  vtkMultiplyMatrix3x3(A,B,C);
}

//----------------------------------------------------------------------------
template<class T1, class T2>
inline void vtkTranspose3x3(const T1 A[3][3], T2 AT[3][3])
{
  T2 tmp;
  tmp = A[1][0];
  AT[1][0] = A[0][1];
  AT[0][1] = tmp;
  tmp = A[2][0];
  AT[2][0] = A[0][2];
  AT[0][2] = tmp;
  tmp = A[2][1];
  AT[2][1] = A[1][2];
  AT[1][2] = tmp;

  AT[0][0] = A[0][0];
  AT[1][1] = A[1][1];
  AT[2][2] = A[2][2];
}

//----------------------------------------------------------------------------
void vtkMath::Transpose3x3(const float A[3][3], float AT[3][3])
{
  vtkTranspose3x3(A,AT);
}

//----------------------------------------------------------------------------
void vtkMath::Transpose3x3(const double A[3][3], double AT[3][3])
{
  vtkTranspose3x3(A,AT);
}

//----------------------------------------------------------------------------
template<class T1, class T2>
inline void vtkInvert3x3(const T1 A[3][3], T2 AI[3][3])
{
  int index[3];
  T2 tmp[3][3];

  for (int k = 0; k < 3; k++)
    {
    AI[k][0] = A[k][0];
    AI[k][1] = A[k][1];
    AI[k][2] = A[k][2];
    }
  // invert one column at a time
  vtkMath::LUFactor3x3(AI,index);
  for (int i = 0; i < 3; i++)
    {
    T2 *x = tmp[i];
    x[0] = x[1] = x[2] = 0.0;
    x[i] = 1.0;
    vtkMath::LUSolve3x3(AI,index,x);
    }
  for (int j = 0; j < 3; j++)
    {
    T2 *x = tmp[j];
    AI[0][j] = x[0];
    AI[1][j] = x[1];
    AI[2][j] = x[2];      
    }
}

//----------------------------------------------------------------------------
void vtkMath::Invert3x3(const float A[3][3], float AI[3][3])
{
  vtkInvert3x3(A,AI);
}

//----------------------------------------------------------------------------
void vtkMath::Invert3x3(const double A[3][3], double AI[3][3])
{
  vtkInvert3x3(A,AI);
}

//----------------------------------------------------------------------------
template<class T>
 inline void vtkIdentity3x3(T A[3][3])
{
  for (int i = 0; i < 3; i++)
    {
    A[i][0] = A[i][1] = A[i][2] = T(0.0);
    A[i][i] = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkMath::Identity3x3(float A[3][3])
{
  vtkIdentity3x3(A);
}

//----------------------------------------------------------------------------
void vtkMath::Identity3x3(double A[3][3])
{
  vtkIdentity3x3(A);
}

//----------------------------------------------------------------------------
template<class T1, class T2>
 inline void vtkQuaternionToMatrix3x3(T1 quat[4], T2 A[3][3])
{
  T2 ww = quat[0]*quat[0];
  T2 wx = quat[0]*quat[1];
  T2 wy = quat[0]*quat[2];
  T2 wz = quat[0]*quat[3];

  T2 xx = quat[1]*quat[1];
  T2 yy = quat[2]*quat[2];
  T2 zz = quat[3]*quat[3];

  T2 xy = quat[1]*quat[2];
  T2 xz = quat[1]*quat[3];
  T2 yz = quat[2]*quat[3];

  T2 rr = xx + yy + zz;
  // normalization factor, just in case quaternion was not normalized
  T2 f = T2(1)/T2(sqrt(ww + rr));
  T2 s = (ww - rr)*f;
  f *= 2;

  A[0][0] = xx*f + s;
  A[1][0] = (xy + wz)*f;
  A[2][0] = (xz - wy)*f;

  A[0][1] = (xy - wz)*f;
  A[1][1] = yy*f + s;
  A[2][1] = (yz + wx)*f;

  A[0][2] = (xz + wy)*f;
  A[1][2] = (yz - wx)*f;
  A[2][2] = zz*f + s;
}

//----------------------------------------------------------------------------
void vtkMath::QuaternionToMatrix3x3(const float quat[4], float A[3][3])
{
  vtkQuaternionToMatrix3x3(quat,A);
}

//----------------------------------------------------------------------------
void vtkMath::QuaternionToMatrix3x3(const double quat[4], double A[3][3])
{
  vtkQuaternionToMatrix3x3(quat,A);
}

//----------------------------------------------------------------------------
//  The solution is based on
//  Berthold K. P. Horn (1987),
//  "Closed-form solution of absolute orientation using unit quaternions,"
//  Journal of the Optical Society of America A, 4:629-642
template<class T1, class T2>
inline void vtkMatrix3x3ToQuaternion(const T1 A[3][3], T2 quat[4])
{
  T2 N[4][4];

  // on-diagonal elements
  N[0][0] =  A[0][0]+A[1][1]+A[2][2];
  N[1][1] =  A[0][0]-A[1][1]-A[2][2];
  N[2][2] = -A[0][0]+A[1][1]-A[2][2];
  N[3][3] = -A[0][0]-A[1][1]+A[2][2];

  // off-diagonal elements
  N[0][1] = N[1][0] = A[2][1]-A[1][2];
  N[0][2] = N[2][0] = A[0][2]-A[2][0];
  N[0][3] = N[3][0] = A[1][0]-A[0][1];

  N[1][2] = N[2][1] = A[1][0]+A[0][1];
  N[1][3] = N[3][1] = A[0][2]+A[2][0];
  N[2][3] = N[3][2] = A[2][1]+A[1][2];

  T2 eigenvectors[4][4],eigenvalues[4];
  
  // convert into format that JacobiN can use,
  // then use Jacobi to find eigenvalues and eigenvectors
  T2 *NTemp[4],*eigenvectorsTemp[4];
  for (int i = 0; i < 4; i++)
    {
    NTemp[i] = N[i];
    eigenvectorsTemp[i] = eigenvectors[i];
    }
  vtkMath::JacobiN(NTemp,4,eigenvalues,eigenvectorsTemp);
  
  // the first eigenvector is the one we want
  quat[0] = eigenvectors[0][0];
  quat[1] = eigenvectors[1][0];
  quat[2] = eigenvectors[2][0];
  quat[3] = eigenvectors[3][0];
}
  
//----------------------------------------------------------------------------
void vtkMath::Matrix3x3ToQuaternion(const float A[3][3], float quat[4])
{
  vtkMatrix3x3ToQuaternion(A,quat);
}

//----------------------------------------------------------------------------
void vtkMath::Matrix3x3ToQuaternion(const double A[3][3], double quat[4])
{
  vtkMatrix3x3ToQuaternion(A,quat);
}
  
//----------------------------------------------------------------------------
//  The orthogonalization is done via quaternions in order to avoid
//  having to use a singular value decomposition algorithm.  
template <class T1, class T2>
inline void vtkOrthogonalize3x3(const T1 A[3][3], T2 B[3][3])
{
  int i;

  // copy the matrix
  for (i = 0; i < 3; i++)
    {
    B[0][i] = A[0][i];
    B[1][i] = A[1][i];
    B[2][i] = A[2][i];
    }

  // Pivot the matrix to improve accuracy
  T2 scale[3];
  int index[3];
  T2 tmp, largest;

  // Loop over rows to get implicit scaling information
  for (i = 0; i < 3; i++)
    {
    largest = fabs(B[i][0]);
    if ((tmp = fabs(B[i][1])) > largest)
      {
      largest = tmp;
      }
    if ((tmp = fabs(B[i][2])) > largest)
      {
      largest = tmp;
      }
    scale[i] = 1.0;
    if (largest != 0)
      {
      scale[i] = T2(1.0)/largest;
      }
    }

  // first column
  index[0] = 0;
  largest = scale[0]*fabs(B[0][0]);
  if ((tmp = scale[1]*fabs(B[1][0])) >= largest) 
    {
    largest = tmp;
    index[0] = 1;
    }
  if ((tmp = scale[2]*fabs(B[2][0])) >= largest) 
    {
    index[0] = 2;
    }
  if (index[0] != 0) 
    {
    vtkSwapVectors3(B[index[0]],B[0]);
    scale[index[0]] = scale[0];
    }

  // second column
  index[1] = 1;
  largest = scale[1]*fabs(B[1][1]);
  if ((tmp = scale[2]*fabs(B[2][1])) >= largest) 
    {
    index[1] = 2;
    vtkSwapVectors3(B[2],B[1]);
    }

  // third column
  index[2] = 2;

  // A quaternian can only describe a pure rotation, not
  // a rotation with a flip, therefore the flip must be
  // removed before the matrix is converted to a quaternion.
  T2 d = vtkDeterminant3x3(B);
  if (d < 0)
    {
    for (i = 0; i < 3; i++)
      {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
      }
    }

  // Do orthogonalization using a quaternion intermediate
  // (this, essentially, does the orthogonalization via
  // diagonalization of an appropriately constructed symmetric
  // 4x4 matrix rather than by doing SVD of the 3x3 matrix)
  T2 quat[4];
  vtkMath::Matrix3x3ToQuaternion(B,quat);
  vtkMath::QuaternionToMatrix3x3(quat,B);

  // Put the flip back into the orthogonalized matrix.
  if (d < 0)
    {
    for (i = 0; i < 3; i++)
      {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
      }
    }

  // Undo the pivoting
  if (index[1] != 1)
    {
    vtkSwapVectors3(B[index[1]],B[1]);
    }
  if (index[0] != 0)
    {
    vtkSwapVectors3(B[index[0]],B[0]);
    }
}

//----------------------------------------------------------------------------
void vtkMath::Orthogonalize3x3(const float A[3][3], float B[3][3])
{
  vtkOrthogonalize3x3(A,B);
}

//----------------------------------------------------------------------------
void vtkMath::Orthogonalize3x3(const double A[3][3], double B[3][3])
{
  vtkOrthogonalize3x3(A,B);
}

//----------------------------------------------------------------------------
float vtkMath::Norm(const float* x, int n)
{
  double sum=0;
  for (int i=0; i<n; i++)
    {
    sum += x[i]*x[i];
    }

  return sqrt(sum);
}

//----------------------------------------------------------------------------
double vtkMath::Norm(const double* x, int n)
{
  double sum=0;
  for (int i=0; i<n; i++)
    {
    sum += x[i]*x[i];
    }

  return sqrt(sum);
}

//----------------------------------------------------------------------------
// Extract the eigenvalues and eigenvectors from a 3x3 matrix.
// The eigenvectors (the columns of V) will be normalized. 
// The eigenvectors are aligned optimally with the x, y, and z
// axes respectively.
template <class T1, class T2>
inline void vtkDiagonalize3x3(const T1 A[3][3], T2 w[3], T2 V[3][3])
{
  int i,j,k,maxI;
  T2 tmp, maxVal;

  // do the matrix[3][3] to **matrix conversion for Jacobi
  T2 C[3][3];
  T2 *ATemp[3],*VTemp[3];
  for (i = 0; i < 3; i++)
    {
    C[i][0] = A[i][0];
    C[i][1] = A[i][1];
    C[i][2] = A[i][2];
    ATemp[i] = C[i];
    VTemp[i] = V[i];
    }

  // diagonalize using Jacobi
  vtkMath::JacobiN(ATemp,3,w,VTemp);
  
  // if all the eigenvalues are the same, return identity matrix
  if (w[0] == w[1] && w[0] == w[2])
    {
    vtkMath::Identity3x3(V);
    return;
    }

  // transpose temporarily, it makes it easier to sort the eigenvectors
  vtkMath::Transpose3x3(V,V);

  // if two eigenvalues are the same, re-orthogonalize to optimally line
  // up the eigenvectors with the x, y, and z axes
  for (i = 0; i < 3; i++)
    {
    if (w[(i+1)%3] == w[(i+2)%3]) // two eigenvalues are the same
      {
      // find maximum element of the independant eigenvector
      maxVal = fabs(V[i][0]);
      maxI = 0;
      for (j = 1; j < 3; j++)
        {
        if (maxVal < (tmp = fabs(V[i][j])))
          {
          maxVal = tmp;
          maxI = j;
          }
        }
      // swap the eigenvector into its proper position
      if (maxI != i)
        {
        tmp = w[maxI];
        w[maxI] = w[i];
        w[i] = tmp;
        vtkSwapVectors3(V[i],V[maxI]);
        }
      // maximum element of eigenvector should be positive
      if (V[maxI][maxI] < 0)
        {
        V[maxI][0] = -V[maxI][0];
        V[maxI][1] = -V[maxI][1];
        V[maxI][2] = -V[maxI][2];
        }
      
      // re-orthogonalize the other two eigenvectors
      j = (maxI+1)%3;
      k = (maxI+2)%3;

      V[j][0] = 0.0; 
      V[j][1] = 0.0; 
      V[j][2] = 0.0;
      V[j][j] = 1.0;
      vtkMath::Cross(V[maxI],V[j],V[k]);
      vtkMath::Normalize(V[k]);
      vtkMath::Cross(V[k],V[maxI],V[j]);

      // transpose vectors back to columns
      vtkMath::Transpose3x3(V,V);
      return;
      }
    }

  // the three eigenvalues are different, just sort the eigenvectors
  // to align them with the x, y, and z axes

  // find the vector with the largest x element, make that vector
  // the first vector
  maxVal = fabs(V[0][0]);
  maxI = 0;
  for (i = 1; i < 3; i++)
    {
    if (maxVal < (tmp = fabs(V[i][0])))
      {
      maxVal = tmp;
      maxI = i;
      }
    }
  // swap eigenvalue and eigenvector
  if (maxI != 0)
    {
    tmp = w[maxI];
    w[maxI] = w[0];
    w[0] = tmp;
    vtkSwapVectors3(V[maxI],V[0]);
    }
  // do the same for the y element
  if (fabs(V[1][1]) < fabs(V[2][1]))
    {
    tmp = w[2];
    w[2] = w[1];
    w[1] = tmp;
    vtkSwapVectors3(V[2],V[1]);
    }

  // ensure that the sign of the eigenvectors is correct
  for (i = 0; i < 2; i++)
    {
    if (V[i][i] < 0)
      {
      V[i][0] = -V[i][0];
      V[i][1] = -V[i][1];
      V[i][2] = -V[i][2];
      }
    }
  // set sign of final eigenvector to ensure that determinant is positive
  if (vtkMath::Determinant3x3(V) < 0)
    {
    V[2][0] = -V[2][0];
    V[2][1] = -V[2][1];
    V[2][2] = -V[2][2];
    }

  // transpose the eigenvectors back again
  vtkMath::Transpose3x3(V,V);
}

//----------------------------------------------------------------------------
void vtkMath::Diagonalize3x3(const float A[3][3], float w[3], float V[3][3])
{
  vtkDiagonalize3x3(A,w,V);
}

//----------------------------------------------------------------------------
void vtkMath::Diagonalize3x3(const double A[3][3],double w[3],double V[3][3])
{
  vtkDiagonalize3x3(A,w,V);
}

//----------------------------------------------------------------------------
// Perform singular value decomposition on the matrix A:
//    A = U * W * VT
// where U and VT are orthogonal W is diagonal (the diagonal elements
// are returned in vector w).
// The matrices U and VT will both have positive determinants.
// The scale factors w are ordered according to how well the
// corresponding eigenvectors (in VT) match the x, y and z axes
// respectively.
//
// The singular value decomposition is used to decompose a linear
// transformation into a rotation, followed by a scale, followed
// by a second rotation.  The scale factors w will be negative if
// the determinant of matrix A is negative.
//
// Contributed by David Gobbi (dgobbi@irus.rri.on.ca)
template <class T1, class T2>
inline void vtkSingularValueDecomposition3x3(const T1 A[3][3], 
                                                    T2 U[3][3], T2 w[3],
                                                    T2 VT[3][3])
{
  int i;
  T2 B[3][3];

  // copy so that A can be used for U or VT without risk
  for (i = 0; i < 3; i++)
    {
    B[0][i] = A[0][i];
    B[1][i] = A[1][i];
    B[2][i] = A[2][i];
    }

  // temporarily flip if determinant is negative
  T2 d = vtkMath::Determinant3x3(B);
  if (d < 0)
    {
    for (i = 0; i < 3; i++)
      {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
      }
    }

  // orthogonalize, diagonalize, etc.
  vtkMath::Orthogonalize3x3(B, U);
  vtkMath::Transpose3x3(B, B);
  vtkMath::Multiply3x3(B, U, VT);
  vtkMath::Diagonalize3x3(VT, w, VT);
  vtkMath::Multiply3x3(U, VT, U);
  vtkMath::Transpose3x3(VT, VT);

  // re-create the flip
  if (d < 0)
    {
    w[0] = -w[0];
    w[1] = -w[1];
    w[2] = -w[2];
    }

  /* paranoia check: recombine to ensure that the SVD is correct
  vtkMath::Transpose3x3(B, B);

  if (d < 0)
    {
    for (i = 0; i < 3; i++)
      {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
      }
    }

  int j;
  T2 maxerr = 0;
  T2 tmp;
  T2 M[3][3];
  T2 W[3][3];
  vtkMath::Identity3x3(W);
  W[0][0] = w[0]; W[1][1] = w[1]; W[2][2] = w[2];
  vtkMath::Identity3x3(M);
  vtkMath::Multiply3x3(M, U, M);
  vtkMath::Multiply3x3(M, W, M);
  vtkMath::Multiply3x3(M, VT, M);

  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
      if ((tmp = fabs(B[i][j] - M[i][j])) > maxerr)
        {
        maxerr = tmp;
        }
      }
    }

  vtkGenericWarningMacro("SingularValueDecomposition max error = " << maxerr);
  */
}

//----------------------------------------------------------------------------
void vtkMath::SingularValueDecomposition3x3(const float A[3][3], 
                                            float U[3][3], float w[3],
                                            float VT[3][3])
{
  vtkSingularValueDecomposition3x3(A,U,w,VT);
}

//----------------------------------------------------------------------------
void vtkMath::SingularValueDecomposition3x3(const double A[3][3], 
                                            double U[3][3], double w[3],
                                            double VT[3][3])
{
  vtkSingularValueDecomposition3x3(A,U,w,VT);
}

//----------------------------------------------------------------------------
void vtkMath::RGBToHSV(float r, float g, float b, 
                       float *h, float *s, float *v)
{
  double dh,ds,dv;
  vtkMath::RGBToHSV(r,g,b,&dh,&ds,&dv);
  *h = static_cast<float>(dh);
  *s = static_cast<float>(ds);
  *v = static_cast<float>(dv);
}

//----------------------------------------------------------------------------
double* vtkMath::RGBToHSV(double rgb[3])
{
  return vtkMath::RGBToHSV(rgb[0], rgb[1], rgb[2]);
}

//----------------------------------------------------------------------------
double* vtkMath::RGBToHSV(double r, double g, double b)
{
  static double hsv[3];
  vtkMath::RGBToHSV(r, g, b, hsv, hsv + 1, hsv + 2);
  return hsv;
}

//----------------------------------------------------------------------------
void vtkMath::RGBToHSV(double r, double g, double b, 
                       double *h, double *s, double *v)
{
  double onethird = 1.0 / 3.0;
  double onesixth = 1.0 / 6.0;
  double twothird = 2.0 / 3.0;

  double cmax, cmin;
  
  cmax = r;
  cmin = r;
  if (g > cmax)
    {
    cmax = g;
    }
  else if (g < cmin)
    {
    cmin = g;
    }
  if (b > cmax)
    {
    cmax = b;
    }
  else if (b < cmin)
    {
    cmin = b;
    }
  *v = cmax;

  if (*v > 0.0)
    {
    *s = (cmax - cmin) / cmax;
    }
  else 
    {
    *s = 0.0;
    }
  if (*s > 0)
    {
    if (r == cmax)
      {
      *h = onesixth * (g - b) / (cmax - cmin);
      }
    else if (g == cmax)
      {
      *h = onethird + onesixth * (b - r) / (cmax - cmin);
      }
    else
      {
      *h = twothird + onesixth * (r - g) / (cmax - cmin);
      }
    if (*h < 0.0)
      {
      *h += 1.0;
      }
    }
  else
    {
    *h = 0.0;
    }
}

//----------------------------------------------------------------------------
void vtkMath::HSVToRGB(float h, float s, float v, 
                       float *r, float *g, float *b)
{
  double dr,dg,db;
  vtkMath::HSVToRGB(h,s,v,&dr,&dg,&db);
  *r = static_cast<float>(dr);
  *g = static_cast<float>(dg);
  *b = static_cast<float>(db);
}

//----------------------------------------------------------------------------
double* vtkMath::HSVToRGB(double hsv[3])
{
  return vtkMath::HSVToRGB(hsv[0], hsv[1], hsv[2]);
}

//----------------------------------------------------------------------------
double* vtkMath::HSVToRGB(double h, double s, double v)
{
  static double rgb[3];
  vtkMath::HSVToRGB(h, s, v, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkMath::HSVToRGB(double h, double s, double v, 
                       double *r, double *g, double *b)
{
  double onethird = 1.0 / 3.0;
  double onesixth = 1.0 / 6.0;
  double twothird = 2.0 / 3.0;
  double fivesixth = 5.0 / 6.0;

  // compute RGB from HSV
  if (h > onesixth && h <= onethird) // green/red
    {
    *g = 1.0;
    *r = (onethird - h) / onesixth;
    *b = 0.0;
    }
  else if (h > onethird && h <= 0.5) // green/blue
    {
    *g = 1.0;
    *b = (h - onethird) / onesixth;
    *r = 0.0;
    }
  else if (h > 0.5 && h <= twothird) // blue/green
    {
    *b = 1.0;
    *g = (twothird - h) / onesixth;
    *r = 0.0;
    }
  else if (h > twothird && h <= fivesixth) // blue/red
    {
    *b = 1.0;
    *r = (h - twothird) / onesixth;
    *g = 0.0;
    }
  else if (h > fivesixth && h <= 1.0) // red/blue
    {
    *r = 1.0;
    *b = (1.0 - h) / onesixth;
    *g = 0.0;
    }
  else // red/green
    {
    *r = 1.0;
    *g = h / onesixth;
    *b = 0.0;
    }
  
  // add Saturation to the equation.
  *r = (s * *r + (1.0 - s));
  *g = (s * *g + (1.0 - s));
  *b = (s * *b + (1.0 - s));
  
  *r *= v;
  *g *= v;
  *b *= v;
}

//----------------------------------------------------------------------------
void vtkMath::LabToXYZ(double lab[3], double xyz[3])
{
  //LAB to XYZ
  double var_Y = ( lab[0] + 16 ) / 116;
  double var_X = lab[1] / 500 + var_Y;
  double var_Z = var_Y - lab[2] / 200;
    
  if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
  else var_Y = ( var_Y - 16 / 116 ) / 7.787;
                                                            
  if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
  else var_X = ( var_X - 16 / 116 ) / 7.787;

  if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
  else var_Z = ( var_Z - 16 / 116 ) / 7.787;
  double ref_X =  95.047;
  double ref_Y = 100.000;
  double ref_Z = 108.883;
  xyz[0] = ref_X * var_X;     //ref_X =  95.047  Observer= 2 Illuminant= D65
  xyz[1] = ref_Y * var_Y;     //ref_Y = 100.000
  xyz[2] = ref_Z * var_Z;     //ref_Z = 108.883
}


//----------------------------------------------------------------------------
void vtkMath::XYZToRGB(double xyz[3], double rgb[3])
{
  
  //double ref_X =  95.047;        //Observer = 2° Illuminant = D65
  //double ref_Y = 100.000;
  //double ref_Z = 108.883;
 
  double var_X = xyz[0] / 100;        //X = From 0 to ref_X
  double var_Y = xyz[1] / 100;        //Y = From 0 to ref_Y
  double var_Z = xyz[2] / 100;        //Z = From 0 to ref_Y
 
  double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
  double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
  double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;
 
  if ( var_R > 0.0031308 ) var_R = 1.055 * ( pow(var_R, ( 1 / 2.4 )) ) - 0.055;
  else var_R = 12.92 * var_R;
  if ( var_G > 0.0031308 ) var_G = 1.055 * ( pow(var_G ,( 1 / 2.4 )) ) - 0.055;
  else  var_G = 12.92 * var_G;
  if ( var_B > 0.0031308 ) var_B = 1.055 * ( pow(var_B, ( 1 / 2.4 )) ) - 0.055;
  else var_B = 12.92 * var_B;
                                                                                                 
  rgb[0] = var_R;
  rgb[1] = var_G;
  rgb[2] = var_B;
  
  //clip colors. ideally we would do something different for colors
  //out of gamut, but not really sure what to do atm.
  if (rgb[0]<0) rgb[0]=0;
  if (rgb[1]<0) rgb[1]=0;
  if (rgb[2]<0) rgb[2]=0;
  if (rgb[0]>1) rgb[0]=1;
  if (rgb[1]>1) rgb[1]=1;
  if (rgb[2]>1) rgb[2]=1;

}
//----------------------------------------------------------------------------
void vtkMath::ClampValues(double *values, 
                          int nb_values, 
                          const double range[2])
{
  if (!values || nb_values <= 0 || !range)
    {
    return;
    }

  const double *values_end = values + nb_values;
  while (values < values_end)
    {
    if (*values < range[0])
      {
      *values = range[0];
      }
    else if (*values > range[1])
      {
      *values = range[1];
      }
    values++;
    }
}

//----------------------------------------------------------------------------
void vtkMath::ClampValues(const double *values, 
                          int nb_values, 
                          const double range[2],
                          double *clamped_values)
{
  if (!values || nb_values <= 0 || !range || !clamped_values)
    {
    return;
    }

  const double *values_end = values + nb_values;
  while (values < values_end)
    {
    if (*values < range[0])
      {
      *clamped_values = range[0];
      }
    else if (*values > range[1])
      {
      *clamped_values = range[1];
      }
    else
      {
      *clamped_values = *values;
      }
    values++;
    clamped_values++;
    }
}

//----------------------------------------------------------------------------
int vtkMath::GetScalarTypeFittingRange(
  double range_min, double range_max, double scale, double shift)
{
  class TypeRange
  {
  public:
    int Type;
    double Min;
    double Max;
  };

  TypeRange FloatTypes[] = 
    {
      { VTK_FLOAT,          VTK_FLOAT_MIN,          VTK_FLOAT_MAX },
      { VTK_DOUBLE,         VTK_DOUBLE_MIN,         VTK_DOUBLE_MAX }
    };

  TypeRange IntTypes[] = 
    {
      { VTK_BIT,            VTK_BIT_MIN,            VTK_BIT_MAX },
      { VTK_CHAR,           VTK_CHAR_MIN,           VTK_CHAR_MAX },
      { VTK_SIGNED_CHAR,    VTK_SIGNED_CHAR_MIN,    VTK_SIGNED_CHAR_MAX },
      { VTK_UNSIGNED_CHAR,  VTK_UNSIGNED_CHAR_MIN,  VTK_UNSIGNED_CHAR_MAX },
      { VTK_SHORT,          VTK_SHORT_MIN,          VTK_SHORT_MAX },
      { VTK_UNSIGNED_SHORT, VTK_UNSIGNED_SHORT_MIN, VTK_UNSIGNED_SHORT_MAX },
      { VTK_INT,            VTK_INT_MIN,            VTK_INT_MAX },
      { VTK_UNSIGNED_INT,   VTK_UNSIGNED_INT_MIN,   VTK_UNSIGNED_INT_MAX },
      { VTK_LONG,           VTK_LONG_MIN,           VTK_LONG_MAX },
      { VTK_UNSIGNED_LONG,  VTK_UNSIGNED_LONG_MIN,  VTK_UNSIGNED_LONG_MAX }
#if defined(VTK_TYPE_USE_LONG_LONG)
      ,
      { VTK_LONG_LONG,      VTK_LONG_LONG_MIN,      VTK_LONG_LONG_MAX },
      { VTK_UNSIGNED_LONG_LONG,
        VTK_UNSIGNED_LONG_LONG_MIN, VTK_UNSIGNED_LONG_LONG_MAX }
#endif
#if defined(VTK_TYPE_USE___INT64)
      ,
      { VTK___INT64,      VTK___INT64_MIN,      VTK___INT64_MAX }
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
      ,
      { VTK_UNSIGNED___INT64,
        VTK_UNSIGNED___INT64_MIN, VTK_UNSIGNED___INT64_MAX }
# endif
#endif
    };

  // If the range, scale or shift are decimal number, just browse
  // the decimal types

  double intpart;

  int range_min_is_int = (modf(range_min, &intpart) == 0.0);
  int range_max_is_int = (modf(range_max, &intpart) == 0.0);
  int scale_is_int = (modf(scale, &intpart) == 0.0);
  int shift_is_int = (modf(shift, &intpart) == 0.0);

  range_min = range_min * scale + shift;
  range_max = range_max * scale + shift;

  if (range_min_is_int && range_max_is_int && scale_is_int && shift_is_int)
    {
    for (unsigned int i = 0; i < sizeof(IntTypes) / sizeof(TypeRange); i++)
      {
      if (IntTypes[i].Min <= range_min && range_max <= IntTypes[i].Max)
        {
        return IntTypes[i].Type;
        }
      }
    }

  for (unsigned int i = 0; i < sizeof(FloatTypes) / sizeof(TypeRange); i++)
    {
    if (FloatTypes[i].Min <= range_min && range_max <= FloatTypes[i].Max)
      {
      return FloatTypes[i].Type;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkMath::GetAdjustedScalarRange(
  vtkDataArray *array, int comp, double range[2])
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
    {
    return 0;
    }
  
  array->GetRange(range, comp);

  switch (array->GetDataType())
    {
    case VTK_UNSIGNED_CHAR:
      range[0] = (double)array->GetDataTypeMin();
      range[1] = (double)array->GetDataTypeMax();
      break;

    case VTK_UNSIGNED_SHORT:
      range[0] = (double)array->GetDataTypeMin();
      if (range[1] <= 4095.0)
        {
        if (range[1] > VTK_UNSIGNED_CHAR_MAX)
          {
          range[1] = 4095.0;
          }
        }
      else
        {
        range[1] = (double)array->GetDataTypeMax();
        }
      break;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMath::ExtentIsWithinOtherExtent(int extent1[6], int extent2[6])
{
  if (!extent1 || !extent2)
    {
    return 0;
    }
  
  int i;
  for (i = 0; i < 6; i += 2)
    {
    if (extent1[i]     < extent2[i] || extent1[i]     > extent2[i + 1] ||
        extent1[i + 1] < extent2[i] || extent1[i + 1] > extent2[i + 1])
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------

int vtkMath::BoundsIsWithinOtherBounds(double bounds1[6], double bounds2[6], double delta[3])
{
  if(!bounds1 || !bounds2)
    {
    return 0;
    }
  for(int i=0;i<6;i+=2)
    {
    
    if(bounds1[i]+delta[i/2] < bounds2[i] || bounds1[i]-delta[i/2] > bounds2[i+1] ||
       bounds1[i+1]+delta[i/2] < bounds2[i] || bounds1[i+1]-delta[i/2] > bounds2[i+1])
      return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMath::PointIsWithinBounds(double point[3], double bounds[6], double delta[3])
{
  if(!point || !bounds || !delta)
    {
    return 0;
    }
  for(int i=0;i<3;i++)
    {
    if(point[i]+delta[i] < bounds[2*i] || point[i]-delta[i] > bounds[2*i+1])
      {
      return 0;
      }
    }
  return 1;
  
} 

//----------------------------------------------------------------------------
void vtkMath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Seed: " << this->Seed << "\n";
}
