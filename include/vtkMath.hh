/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#ifndef __vtkMath_hh
#define __vtkMath_hh

#include <math.h>

class vtkMath
{
public:
  vtkMath() {};

  static float Pi() {return 3.14159265358979;};
  static float DegreesToRadians() {return 0.017453292;};

  // some common operations
  static float Dot(float x[3], float y[3]);
  static void Cross(float x[3], float y[3], float z[3]);
  static float Norm(float x[3]);
  static float Normalize(float x[3]);
  static float Distance2BetweenPoints(float x[3], float y[3]);

  // matrix stuff
  static float Determinant2x2(float c1[2], float c2[2]);
  static double Determinant2x2(double a, double b, double c, double d);
  static float Determinant3x3(float c1[3], float c2[3], float c3[3]);
  static double Determinant3x3(double a1, double a2, double a3, 
			       double b1, double b2, double b3, 
			       double c1, double c2, double c3);
  static int SolveLinearSystem(double **A, double *x, int size);
  static int InvertMatrix(double **A, double **AI, int size);
  static int LUFactorLinearSystem(double **A, int *index, int size);
  static void LUSolveLinearSystem(double **A, int *index, double *x, int size);
  static double EstimateMatrixCondition(double **A, int size);

  // Random number generation
  static void RandomSeed(long s);  
  static float Random();  
  static float Random(float min, float max);

  // Eigenvalue/vector extraction for 3x3 matrices
  static int Jacobi(float **a, float *d, float **v);

protected:
  static long Seed;
};

// Description:
// Dot product of two 3-vectors.
inline float vtkMath::Dot(float x[3], float y[3]) 
{
  return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);
}

// Description:
// Compute the norm of 3-vector.
inline float vtkMath::Norm(float x[3])
{
  return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
}

// Description:
// Normalize (in place) a 3-vector. Returns norm of vector.
inline float vtkMath::Normalize(float x[3])
{
  float den; 
  if ( (den = vtkMath::Norm(x)) != 0.0 ) for (int i=0; i < 3; i++) x[i] /= den;
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
