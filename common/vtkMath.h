/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkMath - performs common math operations
// .SECTION Description
// vtkMath is provides methods to perform common math operations. These 
// include providing constants such as Pi; conversion from degrees to 
// radians; vector operations such as dot and cross products and vector 
// norm; matrix determinant for 2x2 and 3x3 matrices; and random 
// number generation.

#ifndef __vtkMath_h
#define __vtkMath_h

#include <math.h>
#include "vtkObject.h"

class VTK_EXPORT vtkMath : public vtkObject
{
public:
  static vtkMath *New() {return new vtkMath;};
  virtual const char *GetClassName() {return "vtkMath";};
  
  // constants
  static float Pi() {return 3.14159265358979;};
  static float DegreesToRadians() {return 0.017453292;};

  // some common methods
  static float Dot(float x[3], float y[3]);
  static double Dot(double x[3], double y[3]);

// Description:
// Cross product of two 3-vectors. Result vector in z[3].
  static void Cross(float x[3], float y[3], float z[3]);

  static float Norm(float x[3]);
  static float Normalize(float x[3]);
  static float Distance2BetweenPoints(float x[3], float y[3]);

  // special methods for 2D operations
  static float Dot2D(float x[3], float y[3]);
  static double Dot2D(double x[3], double y[3]);
  static float Norm2D(float x[3]);
  static float Normalize2D(float x[3]);

  // matrix stuff
  static float Determinant2x2(float c1[2], float c2[2]);
  static double Determinant2x2(double a, double b, double c, double d);
  static float Determinant3x3(float c1[3], float c2[3], float c3[3]);
  static double Determinant3x3(double a1, double a2, double a3, 
			       double b1, double b2, double b3, 
			       double c1, double c2, double c3);

// Description:
// Solve linear equations Ax = b using Crout's method. Input is square matrix A
// and load vector x. Solution x is written over load vector. The dimension of
// the matrix is specified in size. If error is found, method returns a 0.
  static int SolveLinearSystem(double **A, double *x, int size);


// Description:
// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
  static int InvertMatrix(double **A, double **AI, int size);


// Description:
// Factor linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is 
// found, method returns 0. 
  static int LUFactorLinearSystem(double **A, int *index, int size);


// Description:
// Solve linear equations Ax = b using LU decompostion A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is 
// factored matrix A=LU, integer array of pivot indices index[0->n-1],
// load vector x[0->n-1], and size of square matrix n. Note that A=LU and
// index[] are generated from method LUFactorLinearSystem). Also, solution
// vector is written directly over input load vector.
  static void LUSolveLinearSystem(double **A, int *index, double *x, int size);


// Description:
// Estimate the condition number of a LU factored matrix. Used to judge the
// accuracy of the solution. The matrix A must have been previously factored
// using the method LUFactorLinearSystem. The condition number is the ratio
// of the infinity matrix norm (i.e., maximum value of matrix component)
// divided by the minimum diagonal value. (This works for triangular matrices
// only: see Conte and de Boor, Elementary Numerical Analysis.)
  static double EstimateMatrixCondition(double **A, int size);


  // Random number generation

// Description:
// Initialize seed value. NOTE: Random() has the bad property that 
// the first random number returned after RandomSeed() is called 
// is proportional to the seed value! To help solve this, call 
// RandomSeed() a few times inside seed. This doesn't ruin the 
// repeatability of Random().
//
  static void RandomSeed(long s);  


// Description:
// Generate random numbers between 0.0 and 1.0.
// This is used to provide portability across different systems.
  static float Random();  

  static float Random(float min, float max);

  // Eigenvalue/vector extraction for 3x3 matrices

// Description:
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
// real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
// and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
// in decreasing order; eigenvectors are normalized.
  static int Jacobi(float **a, float *d, float **v);


  // Roots of polinomial equations
  // 

// Description:
// Solves a cubic equation c0*t^3  + c1*t^2  + c2*t + c3 = 0 when
// c0, c1, c2, and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Return array contains number of (real) roots (counting multiple roots as one)
// followed by roots themselves. The value in roots[4] is a integer giving
// further information about the roots (see return codes for int SolveCubic()).
  static double* SolveCubic( double c0, double c1, double c2, double c3 );


// Description:
// Solves a quadratic equation c1*t^2 + c2*t + c3 = 0 when c1, c2, and
// c3 are REAL.  Solution is motivated by Numerical Recipes In C 2nd
// Ed.  Return array contains number of (real) roots (counting
// multiple roots as one) followed by roots themselves. Note that 
// roots[3] contains a return code further describing solution - see
// documentation for SolveCubic() for meaining of return codes.
  static double* SolveQuadratic( double c0, double c1, double c2 );


// Description:
// Solves a linear equation c2*t  + c3 = 0 when c2 and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Return array contains number of roots followed by roots themselves.
  static double* SolveLinear( double c0, double c1 );



// Description:
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
  static int SolveCubic( double c0, double c1, double c2, double c3, 
			  double *r1, double *r2, double *r3, int *num_roots );


// Description:
// Solves A Quadratic Equation c1*t^2  + c2*t  + c3 = 0 when 
// c1, c2, and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Roots and number of roots are stored in user provided variables
// r1, r2, num_roots
  static int SolveQuadratic( double c0, double c1, double c2, 
			      double *r1, double *r2, int *num_roots );



// Description:
// Solves a linear equation c2*t + c3 = 0 when c2 and c3 are REAL.
// Solution is motivated by Numerical Recipes In C 2nd Ed.
// Root and number of (real) roots are stored in user provided variables
// r2 and num_roots.
  static int SolveLinear( double c0, double c1, double *r1, int *num_roots );


protected:
  static long Seed;
};

// Description:
// Dot product of two 3-vectors (float version).
inline float vtkMath::Dot(float x[3], float y[3]) 
{
  return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);
}

// Description:
// Dot product of two 3-vectors (double-precision version).
inline double vtkMath::Dot(double x[3], double y[3]) 
{
  return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);
}

// Description:
// Dot product of two 2-vectors. The third (z) component is ignored.
inline float vtkMath::Dot2D(float x[3], float y[3]) 
{
  return (x[0]*y[0] + x[1]*y[1]);
}

// Description:
// Dot product of two 2-vectors. The third (z) component is ignored. (Double-precision
// version.)
inline double vtkMath::Dot2D(double x[3], double y[3]) 
{
  return (x[0]*y[0] + x[1]*y[1]);
}

// Description:
// Compute the norm of 3-vector.
inline float vtkMath::Norm(float x[3])
{
  return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
}

// Description:
// Compute the norm of a 2-vector. Ignores z-component.
inline float vtkMath::Norm2D(float x[3])
{
  return sqrt(x[0]*x[0] + x[1]*x[1]);
}

// Description:
// Normalize (in place) a 3-vector. Returns norm of vector.
inline float vtkMath::Normalize(float x[3])
{
  float den; 
  if ( (den = vtkMath::Norm(x)) != 0.0 )
    {
    for (int i=0; i < 3; i++)
      {
      x[i] /= den;
      }
    }
  return den;
}

// Description:
// Normalize (in place) a 2-vector. Returns norm of vector. Ignores z-component.
inline float vtkMath::Normalize2D(float x[3])
{
  float den; 
  if ( (den = vtkMath::Norm2D(x)) != 0.0 )
    {
    for (int i=0; i < 2; i++)
      {
      x[i] /= den;
      }
    }
  return den;
}

// Description:
// Compute determinant of 2x2 matrix. Two columns of matrix are input.
inline float vtkMath::Determinant2x2(float c1[2], float c2[2])
{
  return (c1[0]*c2[1] - c2[0]*c1[1]);
}

// Description:
// Calculate the determinant of a 2x2 matrix: | a b |
//                                            | c d |
inline double vtkMath::Determinant2x2(double a, double b, double c, double d)
{
  return (a * d - b * c);
}

// Description:
// Compute determinant of 3x3 matrix. Three columns of matrix are input.
inline float vtkMath::Determinant3x3(float c1[3], float c2[3], float c3[3])
{
  return c1[0]*c2[1]*c3[2] + c2[0]*c3[1]*c1[2] + c3[0]*c1[1]*c2[2] -
         c1[0]*c3[1]*c2[2] - c2[0]*c1[1]*c3[2] - c3[0]*c2[1]*c1[2];
}

// Description:
// Calculate the determinent of a 3x3 matrix in the form:
//     | a1,  b1,  c1 |
//     | a2,  b2,  c2 |
//     | a3,  b3,  c3 |
inline double vtkMath::Determinant3x3(double a1, double a2, double a3, 
                                     double b1, double b2, double b3, 
                                     double c1, double c2, double c3)
{
    return ( a1 * vtkMath::Determinant2x2( b2, b3, c2, c3 )
            - b1 * vtkMath::Determinant2x2( a2, a3, c2, c3 )
            + c1 * vtkMath::Determinant2x2( a2, a3, b2, b3 ) );
}

// Description:
// Compute distance squared between two points.
inline float vtkMath::Distance2BetweenPoints(float x[3], float y[3])
{
  return ((x[0]-y[0])*(x[0]-y[0]) + (x[1]-y[1])*(x[1]-y[1]) +
          (x[2]-y[2])*(x[2]-y[2]));
}

// Description:
// Generate random number between (min,max).
inline float vtkMath::Random(float min, float max)
{
  return (min + vtkMath::Random()*(max-min));
}

#endif
