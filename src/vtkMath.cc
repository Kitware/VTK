/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include "vtkMath.hh"
#include "vtkSetGet.hh"

long vtkMath::Seed = 1177; // One authors home address

//
// some constants we need
//
#define K_A 16807
#define K_M 2147483647			/* Mersenne prime 2^31 -1 */
#define K_Q 127773			/* K_M div K_A */
#define K_R 2836			/* K_M mod K_A */

// Description:
// Generate random numbers between 0.0 and 1.0.
// This is used to provide portability across different systems.
float vtkMath::Random()
{
  long hi, lo;
    
  // Based on code in "Random Number Generators: Good Ones are Hard to Find,"
  // by Stephen K. Park and Keith W. Miller in Communications of the ACM,
  // 31, 10 (Oct. 1988) pp. 1192-1201.
  // Borrowed from: Fuat C. Baran, Columbia University, 1988.
  hi = this->Seed / K_Q;
  lo = this->Seed % K_Q;
  if ((this->Seed = K_A * lo - K_R * hi) <= 0) Seed += K_M;
  return ((float) this->Seed / K_M);
}

// Description:
// Initialize seed value. NOTE: Random() has the bad property that 
// the first random number returned after RandomSeed() is called 
// is proportional to the seed value! To help solve this, call 
// RandomSeed() a few times inside seed. This doesn't ruin the 
// repeatability of Random().
//
void vtkMath::RandomSeed(long s)
{
  this->Seed = s;

  vtkMath::Random();
  vtkMath::Random();
  vtkMath::Random();
}

// Description:
// Cross product of two 3-vectors. Result vector in z[3].
void vtkMath::Cross(float x[3], float y[3], float z[3])
{
  float Zx = x[1]*y[2] - x[2]*y[1]; 
  float Zy = x[2]*y[0] - x[0]*y[2];
  float Zz = x[0]*y[1] - x[1]*y[0];
  z[0] = Zx; z[1] = Zy; z[2] = Zz; 
}

#define VTK_SMALL_NUMBER 1.0e-12

// Description:
// Solve linear equations Ax = b using Crout's method. Input is square matrix A
// and load vector x. Solution x is written over load vector. The dimension of
// the matrix is specified in size. If error is found, method returns a 0.
int vtkMath::SolveLinearSystem(double **A, double *x, int size)
{
  static int *index = NULL, maxSize=0;
//
// Check on allocation of working vectors
//
  if ( index == NULL ) 
    {
    index = new int[size];
    maxSize = size;
    } 
  else if ( size > maxSize ) 
    {
    delete [] index;
    index = new int[size];
    maxSize = size;
    }
//
// Factor and solve matrix
//
  if ( this->LUFactorLinearSystem(A, index, size) == 0 ) return 0;
  this->LUSolveLinearSystem(A,index,x,size);

  return 1;
}

// Description:
// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
int vtkMath::InvertMatrix(double **A, double **AI, int size)
{
  static int *index = NULL, maxSize=0;
  static double *column = NULL;
  int i, j;
//
// Check on allocation of working vectors
//
  if ( index == NULL ) 
    {
    index = new int[size];
    column = new double[size];
    maxSize = size;
    } 
  else if ( size > maxSize ) 
    {
    delete [] index; delete [] column;
    index = new int[size];
    column = new double[size];
    maxSize = size;
    }
//
// Factor matrix; then begin solving for inverse one column at a time.
//
  if ( this->LUFactorLinearSystem(A, index, size) == 0 ) return 0;
  
  for ( i=0; i < size; i++ )
    {
    for ( j=0; j < size; j++ ) column[j] = 0.0;
    column[i] = 1.0;

    this->LUSolveLinearSystem(A,index,column,size);

    for ( j=0; j < size; j++ ) AI[i][j] = column[j];
    }

  return 1;
}

// Description:
// Factor linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is 
// found, method returns 0. 
int vtkMath::LUFactorLinearSystem(double **A, int *index, int size)
{
  static double *scale = NULL;
  static maxSize=0;
  int i, j, k;
  int maxI = 0;
  double largest, temp1, temp2, sum;
//
// Check on allocation of working vectors
//
  if ( scale == NULL ) 
    {
    scale = new double[size];
    maxSize = size;
    } 
  else if ( size > maxSize ) 
    {
    delete [] scale; 
    scale = new double[size];
    maxSize = size;
    }
//
// Loop over rows to get implicit scaling information
//
  for ( i = 0; i < size; i++ ) 
    {
    for ( largest = 0.0, j = 0; j < size; j++ ) 
      {
      if ( (temp2 = fabs(A[i][j])) > largest ) largest = temp2;
      }

      if ( largest == 0.0 ) return 0;
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
      for ( k = 0; k < i; k++ ) sum -= A[i][k] * A[k][j];
      A[i][j] = sum;
      }
//
// Begin search for largest pivot element
//
    for ( largest = 0.0, i = j; i < size; i++ ) 
      {
      sum = A[i][j];
      for ( k = 0; k < j; k++ ) sum -= A[i][k] * A[k][j];
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

    if ( fabs(A[j][j]) <= VTK_SMALL_NUMBER ) return 0;

    if ( j != (size-1) ) 
      {
      temp1 = 1.0 / A[j][j];
      for ( i = j + 1; i < size; i++ ) A[i][j] *= temp1;
      }
    }

  return 1;
}


// Description:
// Solve linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// factored matrix A=LU, integer array of pivot indices index[0->n-1],
// load vector x[0->n-1], and size of square matrix n. Note that A=LU and
// index[] are generated from method LUFactorLinearSystem). Also, solution
// vector is written directly over input load vector.
void vtkMath::LUSolveLinearSystem(double **A, int *index, double *x, int size)
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
      for ( j = ii; j <= (i-1); j++ ) sum -= A[i][j]*x[j];
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
    for ( j = i + 1; j < size; j++ ) sum -= A[i][j]*x[j];
    x[i] = sum / A[i][i];
    }
}

#undef VTK_SMALL_NUMBER

#define VTK_ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
        a[k][l]=h+s*(g-h*tau);

#define VTK_MAX_ROTATIONS 20

// Description:
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
// real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
// and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
// in decreasing order; eigenvectors are normalized.
int vtkMath::Jacobi(float **a, float *w, float **v)
{
  int i, j, k, iq, ip, numPos;
  float tresh, theta, tau, t, sm, s, h, g, c;
  float b[3], z[3], tmp;

  // initialize
  for (ip=0; ip<3; ip++) 
    {
    for (iq=0; iq<3; iq++) v[ip][iq] = 0.0;
    v[ip][ip] = 1.0;
    }
  for (ip=0; ip<3; ip++) 
    {
    b[ip] = w[ip] = a[ip][ip];
    z[ip] = 0.0;
    }

  // begin rotation sequence
  for (i=0; i<VTK_MAX_ROTATIONS; i++) 
    {
    sm = 0.0;
    for (ip=0; ip<2; ip++) 
      {
      for (iq=ip+1; iq<3; iq++) sm += fabs(a[ip][iq]);
      }
    if (sm == 0.0) break;

    if (i < 4) tresh = 0.2*sm/(9);
    else tresh = 0.0;

    for (ip=0; ip<2; ip++) 
      {
      for (iq=ip+1; iq<3; iq++) 
        {
        g = 100.0*fabs(a[ip][iq]);
        if (i > 4 && (fabs(w[ip])+g) == fabs(w[ip])
        && (fabs(w[iq])+g) == fabs(w[iq]))
          {
          a[ip][iq] = 0.0;
          }
        else if (fabs(a[ip][iq]) > tresh) 
          {
          h = w[iq] - w[ip];
          if ( (fabs(h)+g) == fabs(h)) t = (a[ip][iq]) / h;
          else 
            {
            theta = 0.5*h / (a[ip][iq]);
            t = 1.0 / (fabs(theta)+sqrt(1.0+theta*theta));
            if (theta < 0.0) t = -t;
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
          for (j=0;j<ip-1;j++) 
            {
            VTK_ROTATE(a,j,ip,j,iq)
            }
          for (j=ip+1;j<iq-1;j++) 
            {
            VTK_ROTATE(a,ip,j,j,iq)
            }
          for (j=iq+1; j<3; j++) 
            {
            VTK_ROTATE(a,ip,j,iq,j)
            }
          for (j=0; j<3; j++) 
            {
            VTK_ROTATE(v,j,ip,j,iq)
            }
          }
        }
      }

    for (ip=0; ip<3; ip++) 
      {
      b[ip] += z[ip];
      w[ip] = b[ip];
      z[ip] = 0.0;
      }
    }

  if ( i >= VTK_MAX_ROTATIONS )
    {
    cerr << "vtkMath::Jacobi: Error extracting eigenfunctions\n";
    return 0;
    }

  // sort eigenfunctions
  for (j=0; j<3; j++) 
    {
    k = j;
    tmp = w[k];
    for (i=j; i<3; i++)
      {
      if (w[i] >= tmp) 
        {
        k = i;
        tmp = w[k];
        }
      }
    if (k != j) 
      {
      w[k] = w[j];
      w[j] = tmp;
      for (i=0; i<3; i++) 
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
  for (j=0; j<3; j++)
    {
    for (numPos=0, i=0; i<3; i++) if ( v[i][j] >= 0.0 ) numPos++;
    if ( numPos < 2 ) for(i=0; i<3; i++) v[i][j] *= -1.0;
    }

  return 1;
}
#undef VTK_ROTATE
#undef VTK_MAX_ROTATIONS

// Description:
// Estimate the condition number of a LU factored matrix. Used to judge the
// accuracy of the solution. The matrix A must have been previously factored
// using the method LUFactorLinearSystem. The condition number is the ratio
// of the infinity matrix norm (i.e., maximum value of matrix component)
// divided by the minimum diagonal value. (This works for triangular matrices
// only: see Conte and de Boor, Elementary Numerical Analysis.)
double vtkMath::EstimateMatrixCondition(double **A, int size)
{
  int i;
  int j = 0;
  double min=VTK_LARGE_FLOAT, max=(-VTK_LARGE_FLOAT);

  // find the maximum value
  for (i=0; i < size; i++)
    {
    for (j=i; j < size; j++)
      {
      if ( fabs(A[i][j]) > max ) max = fabs(A[i][j]);
      }
    }

  // find the minimum diagonal value
  for (i=0; i < size; i++)
    {
    if ( fabs(A[i][i]) < min ) min = fabs(A[i][j]);
    }

  if ( min == 0.0 ) return VTK_LARGE_FLOAT;
  else return (max/min);
}

