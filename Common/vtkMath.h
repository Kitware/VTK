/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkMath : public vtkObject
{
public:
  static vtkMath *New();
  vtkTypeMacro(vtkMath,vtkObject);
  
  // Description:
  // Useful constants.
  static float Pi() {return 3.14159265358979;};
  static float DegreesToRadians() {return 0.017453292;};

  // Description:
  // Useful constants. (double-precision version)
  static double DoubleDegreesToRadians() {return 0.017453292519943295;};

  // Description:
  // Dot product of two 3-vectors (float version).
  static float Dot(const float x[3], const float y[3]) {
    return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);};

  // Description:
  // Dot product of two 3-vectors (double-precision version).
  static double Dot(const double x[3], const double y[3]) {
    return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);};
  
  // Description:
  // Cross product of two 3-vectors. Result vector in z[3].
  static void Cross(const float x[3], const float y[3], float z[3]);

  // Description:
  // Cross product of two 3-vectors. Result vector in z[3]. (double-precision
  // version)
  static void Cross(const double x[3], const double y[3], double z[3]);

  // Description:
  // Compute the norm of n-vector.
  static float Norm(const float* x, int n); 

  // Description:
  // Compute the norm of 3-vector.
  static float Norm(const float x[3]) {
    return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);};
  
  // Description:
  // Compute the norm of 3-vector (double-precision version).
  static double Norm(const double x[3]) {
    return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);};
  
  // Description:
  // Normalize (in place) a 3-vector. Returns norm of vector.
  static float Normalize(float x[3]);

  // Description:
  // Normalize (in place) a 3-vector. Returns norm of vector
  // (double-precision version).
  static double Normalize(double x[3]);

  // Description:
  // Given a unit vector x, find two unit vectors y and z such that 
  // x cross y = z (i.e. the vectors are perpendicular to each other).
  // There is an infinite number of such vectors, specify an angle theta 
  // to choose one set.  If you want only one perpendicular vector, 
  // specify NULL for z.
  static void Perpendiculars(const double x[3], double y[3], double z[3], 
			     double theta);
  static void Perpendiculars(const float x[3], float y[3], float z[3],
			     double theta);

  // Description:
  // Compute distance squared between two points.
  static float Distance2BetweenPoints(const float x[3], const float y[3]);

  // Description:
  // Compute distance squared between two points (double precision version).
  static double Distance2BetweenPoints(const double x[3], const double y[3]);

  // Description:
  // Dot product of two 2-vectors. The third (z) component is ignored.
  static float Dot2D(const float x[3], const float y[3]) {
    return (x[0]*y[0] + x[1]*y[1]);};
  
  // Description:
  // Dot product of two 2-vectors. The third (z) component is
  // ignored (double-precision version).
  static double Dot2D(const double x[3], const double y[3]) {
    return (x[0]*y[0] + x[1]*y[1]);};

  // Description:
  // Compute the norm of a 2-vector. Ignores z-component.
  static float Norm2D(const float x[3]) {
    return sqrt(x[0]*x[0] + x[1]*x[1]);};

  // Description:
  // Compute the norm of a 2-vector. Ignores z-component
  // (double-precision version).
  static double Norm2D(const double x[3]) {
    return sqrt(x[0]*x[0] + x[1]*x[1]);};

  // Description:
  // Normalize (in place) a 2-vector. Returns norm of vector. Ignores
  // z-component.
  static float Normalize2D(float x[3]);

  // Description:
  // Normalize (in place) a 2-vector. Returns norm of vector. Ignores
  // z-component (double-precision version).
  static double Normalize2D(double x[3]);

  // Description:
  // Compute determinant of 2x2 matrix. Two columns of matrix are input.
  static float Determinant2x2(const float c1[2], const float c2[2]) {
    return (c1[0]*c2[1] - c2[0]*c1[1]);};

  // Description:
  // Calculate the determinant of a 2x2 matrix: | a b | | c d |
  static double Determinant2x2(double a, double b, double c, double d) {
    return (a * d - b * c);};

  // Description:
  // LU Factorization of a 3x3 matrix.  The diagonal elements are the
  // multiplicative inverse of those in the standard LU factorization.
  static void LUFactor3x3(float A[3][3], int index[3]);
  static void LUFactor3x3(double A[3][3], int index[3]);

  // Description:
  // LU back substitution for a 3x3 matrix.  The diagonal elements are the
  // multiplicative inverse of those in the standard LU factorization.
  static void LUSolve3x3(const float A[3][3], const int index[3], 
			 float x[3]);
  static void LUSolve3x3(const double A[3][3], const int index[3], 
			 double x[3]);

  // Description:
  // Solve Ay = x for y and place the result in y.  The matrix A is
  // destroyed in the process.
  static void LinearSolve3x3(const float A[3][3], const float x[3], 
			     float y[3]);
  static void LinearSolve3x3(const double A[3][3], const double x[3], 
			     double y[3]);

  // Description:
  // Multiply a vector by a 3x3 matrix.  The result is placed in out.
  static void Multiply3x3(const float A[3][3], const float in[3], 
			  float out[3]);
  static void Multiply3x3(const double A[3][3], const double in[3], 
			  double out[3]);
  
  // Description:
  // Mutltiply one 3x3 matrix by another according to C = AB.
  static void Multiply3x3(const float A[3][3], const float B[3][3], 
			  float C[3][3]);
  static void Multiply3x3(const double A[3][3], const double B[3][3], 
			  double C[3][3]);

  // Description:
  // Transpose a 3x3 matrix.
  static void Transpose3x3(const float A[3][3], float AT[3][3]);
  static void Transpose3x3(const double A[3][3], double AT[3][3]);

  // Description:
  // Invert a 3x3 matrix.
  static void Invert3x3(const float A[3][3], float AI[3][3]);
  static void Invert3x3(const double A[3][3], double AI[3][3]);

  // Description:
  // Set A to the identity matrix.
  static void Identity3x3(float A[3][3]);
  static void Identity3x3(double A[3][3]);

  // Description:
  // Return the determinant of a 3x3 matrix.
  static double Determinant3x3(float A[3][3]);
  static double Determinant3x3(double A[3][3]);

  // Description:
  // Compute determinant of 3x3 matrix. Three columns of matrix are input.
  static float Determinant3x3(const float c1[3], 
			      const float c2[3], 
			      const float c3[3]);

  // Description:
  // Calculate the determinant of a 3x3 matrix in the form:
  //     | a1,  b1,  c1 |
  //     | a2,  b2,  c2 |
  //     | a3,  b3,  c3 |
  static double Determinant3x3(double a1, double a2, double a3, 
			       double b1, double b2, double b3, 
			       double c1, double c2, double c3);

  // Description:
  // Convert a quaternion to a 3x3 rotation matrix.  The quaternion
  // does not have to be normalized beforehand.
  static void QuaternionToMatrix3x3(const float quat[4], float A[3][3]); 
  static void QuaternionToMatrix3x3(const double quat[4], double A[3][3]); 

  // Description:
  // Convert a 3x3 matrix into a quaternion.  This will provide the
  // best possible answer even if the matrix is not a pure rotation matrix.
  // The method used is that of B.K.P. Horn.
  static void Matrix3x3ToQuaternion(const float A[3][3], float quat[4]);
  static void Matrix3x3ToQuaternion(const double A[3][3], double quat[4]);
  
  // Description:
  // Orthogonalize a 3x3 matrix and put the result in B.  If matrix A
  // has a negative determinant, then B will be a rotation plus a flip
  // i.e. it will have a determinant of -1.
  static void Orthogonalize3x3(const float A[3][3], float B[3][3]);
  static void Orthogonalize3x3(const double A[3][3], double B[3][3]);

  // Description:
  // Diagonalize a symmetric 3x3 matrix and return the eigenvalues in
  // w and the eigenvectors in the columns of V.  The matrix V will 
  // have a positive determinant, and the three eigenvectors will be
  // aligned as closely as possible with the x, y, and z axes.
  static void Diagonalize3x3(const float A[3][3], float w[3], float V[3][3]);
  static void Diagonalize3x3(const double A[3][3],double w[3],double V[3][3]);

  // Description:
  // Perform singular value decomposition on a 3x3 matrix.  This is not
  // done using a conventional SVD algorithm, instead it is done using
  // Orthogonalize3x3 and Diagonalize3x3.  Both output matrices U and VT
  // will have positive determinants, and the w values will be arranged
  // such that the three rows of VT are aligned as closely as possible
  // with the x, y, and z axes respectively.  If the determinant of A is
  // negative, then the three w values will be negative.
  static void SingularValueDecomposition3x3(const float A[3][3],
					    float U[3][3], float w[3],
					    float VT[3][3]);
  static void SingularValueDecomposition3x3(const double A[3][3],
					    double U[3][3], double w[3],
					    double VT[3][3]);
  
  // Description:
  // Solve linear equations Ax = b using Crout's method. Input is square
  // matrix A and load vector x. Solution x is written over load vector. The
  // dimension of the matrix is specified in size. If error is found, method
  // returns a 0.
  static int SolveLinearSystem(double **A, double *x, int size);

  // Description:
  // Invert input square matrix A into matrix AI. 
  // Note that A is modified during
  // the inversion. The size variable is the dimension of the matrix. Returns 0
  // if inverse not computed.
  static int InvertMatrix(double **A, double **AI, int size);

  // Description:
  // Thread safe version of InvertMatrix method.
  // Working memory arrays tmp1SIze and tmp2Size
  // of length size must be passed in.
  static int InvertMatrix(double **A, double **AI, int size,
			  int *tmp1Size, double *tmp2Size);

  // Description:
  // Factor linear equations Ax = b using LU decomposition A = LU where L is
  // lower triangular matrix and U is upper triangular matrix. Input is 
  // square matrix A, integer array of pivot indices index[0->n-1], and size
  // of square matrix n. Output factorization LU is in matrix A. If error is 
  // found, method returns 0. 
  static int LUFactorLinearSystem(double **A, int *index, int size);

  // Description:
  // Thread safe version of LUFactorLinearSystem method.
  // Working memory array tmpSize of length size
  // must be passed in.
  static int LUFactorLinearSystem(double **A, int *index, int size,
				  double *tmpSize);

  // Description:
  // Solve linear equations Ax = b using LU decomposition A = LU where L is
  // lower triangular matrix and U is upper triangular matrix. Input is 
  // factored matrix A=LU, integer array of pivot indices index[0->n-1],
  // load vector x[0->n-1], and size of square matrix n. Note that A=LU and
  // index[] are generated from method LUFactorLinearSystem). Also, solution
  // vector is written directly over input load vector.
  static void LUSolveLinearSystem(double **A, int *index, 
				  double *x, int size);

  // Description:
  // Estimate the condition number of a LU factored matrix. Used to judge the
  // accuracy of the solution. The matrix A must have been previously factored
  // using the method LUFactorLinearSystem. The condition number is the ratio
  // of the infinity matrix norm (i.e., maximum value of matrix component)
  // divided by the minimum diagonal value. (This works for triangular matrices
  // only: see Conte and de Boor, Elementary Numerical Analysis.)
  static double EstimateMatrixCondition(double **A, int size);

  // Description:
  // Initialize seed value. NOTE: Random() has the bad property that 
  // the first random number returned after RandomSeed() is called 
  // is proportional to the seed value! To help solve this, call 
  // RandomSeed() a few times inside seed. This doesn't ruin the 
  // repeatability of Random().
  static void RandomSeed(long s);  

  // Description:
  // Generate random numbers between 0.0 and 1.0.
  // This is used to provide portability across different systems.
  static float Random();  

  // Description:
  // Generate random number between (min,max).
  static float Random(float min, float max);

  // Description:
  // Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
  // real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
  // and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
  // in decreasing order; eigenvectors are normalized.
  static int Jacobi(float **a, float *w, float **v);
  static int Jacobi(double **a, double *w, double **v);

  // Description:
  // JacobiN iteration for the solution of eigenvectors/eigenvalues of a nxn
  // real symmetric matrix. Square nxn matrix a; size of matrix in n; output
  // eigenvalues in w; and output eigenvectors in v. Resulting
  // eigenvalues/vectors are sorted in decreasing order; eigenvectors are
  // normalized.  w and v need to be allocated previously
  static int JacobiN(float **a, int n, float *w, float **v);
  static int JacobiN(double **a, int n, double *w, double **v);

  // Description:
  // Solves a cubic equation c0*t^3 + c1*t^2 + c2*t + c3 = 0 when c0, c1, c2,
  // and c3 are REAL.  Solution is motivated by Numerical Recipes In C 2nd
  // Ed.  Return array contains number of (real) roots (counting multiple
  // roots as one) followed by roots themselves. The value in roots[4] is a
  // integer giving further information about the roots (see return codes for
  // int SolveCubic()).
  static double* SolveCubic(double c0, double c1, double c2, double c3);

  // Description:
  // Solves a quadratic equation c1*t^2 + c2*t + c3 = 0 when c1, c2, and c3
  // are REAL.  Solution is motivated by Numerical Recipes In C 2nd Ed.
  // Return array contains number of (real) roots (counting multiple roots as
  // one) followed by roots themselves. Note that roots[3] contains a return
  // code further describing solution - see documentation for SolveCubic()
  // for meaning of return codes.
  static double* SolveQuadratic(double c0, double c1, double c2);

  // Description:
  // Solves a linear equation c2*t  + c3 = 0 when c2 and c3 are REAL.
  // Solution is motivated by Numerical Recipes In C 2nd Ed.
  // Return array contains number of roots followed by roots themselves.
  static double* SolveLinear(double c0, double c1);

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
  static int SolveCubic(double c0, double c1, double c2, double c3, 
			double *r1, double *r2, double *r3, int *num_roots);

  // Description:
  // Solves A Quadratic Equation c1*t^2  + c2*t  + c3 = 0 when 
  // c1, c2, and c3 are REAL.
  // Solution is motivated by Numerical Recipes In C 2nd Ed.
  // Roots and number of roots are stored in user provided variables
  // r1, r2, num_roots
  static int SolveQuadratic(double c0, double c1, double c2, 
			    double *r1, double *r2, int *num_roots);
  
  // Description:
  // Solves a linear equation c2*t + c3 = 0 when c2 and c3 are REAL.
  // Solution is motivated by Numerical Recipes In C 2nd Ed.
  // Root and number of (real) roots are stored in user provided variables
  // r2 and num_roots.
  static int SolveLinear(double c0, double c1, double *r1, int *num_roots);


  // Description:
  // Solves for the least squares best fit matrix for the equation X'M' = Y'.
  // Uses pseudoinverse to get the ordinary least squares. 
  // The inputs and output are transposed matrices.
  //    Dimensions: X' is numberOfSamples by xOrder,
  //                Y' is numberOfSamples by yOrder,
  //                M' dimension is xOrder by yOrder.
  // M' should be pre-allocated. All matrices are row major. The resultant
  // matrix M' should be pre-multiplied to X' to get Y', or transposed and
  // then post multiplied to X to get Y
  static int SolveLeastSquares(int numberOfSamples, double **xt, int xOrder,
                               double **yt, int yOrder, double **mt);
  
protected:
  vtkMath() {};
  ~vtkMath() {};
  vtkMath(const vtkMath&);
  void operator=(const vtkMath&);
  
  static long Seed;
};

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
inline double vtkMath::Normalize(double x[3])
{
  double den; 
  if ( (den = vtkMath::Norm(x)) != 0.0 )
    {
    for (int i=0; i < 3; i++)
      {
      x[i] /= den;
      }
    }
  return den;
}

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

inline double vtkMath::Normalize2D(double x[3])
{
  double den; 
  if ( (den = vtkMath::Norm2D(x)) != 0.0 )
    {
    for (int i=0; i < 2; i++)
      {
      x[i] /= den;
      }
    }
  return den;
}

inline float vtkMath::Determinant3x3(const float c1[3], 
				     const float c2[3], 
				     const float c3[3])
{
  return c1[0]*c2[1]*c3[2] + c2[0]*c3[1]*c1[2] + c3[0]*c1[1]*c2[2] -
         c1[0]*c3[1]*c2[2] - c2[0]*c1[1]*c3[2] - c3[0]*c2[1]*c1[2];
}

inline double vtkMath::Determinant3x3(double a1, double a2, double a3, 
				      double b1, double b2, double b3, 
				      double c1, double c2, double c3)
{
    return ( a1 * vtkMath::Determinant2x2( b2, b3, c2, c3 )
	   - b1 * vtkMath::Determinant2x2( a2, a3, c2, c3 )
           + c1 * vtkMath::Determinant2x2( a2, a3, b2, b3 ) );
}

inline float vtkMath::Distance2BetweenPoints(const float x[3], 
					     const float y[3])
{
  return ((x[0]-y[0])*(x[0]-y[0]) + (x[1]-y[1])*(x[1]-y[1]) +
          (x[2]-y[2])*(x[2]-y[2]));
}
inline double vtkMath::Distance2BetweenPoints(const double x[3], 
					      const double y[3])
{
  return ((x[0]-y[0])*(x[0]-y[0]) + (x[1]-y[1])*(x[1]-y[1]) +
          (x[2]-y[2])*(x[2]-y[2]));
}

inline float vtkMath::Random(float min, float max)
{
  return (min + vtkMath::Random()*(max-min));
}

// Cross product of two 3-vectors. Result vector in z[3].
inline void vtkMath::Cross(const float x[3], const float y[3], float z[3])
{
  float Zx = x[1]*y[2] - x[2]*y[1]; 
  float Zy = x[2]*y[0] - x[0]*y[2];
  float Zz = x[0]*y[1] - x[1]*y[0];
  z[0] = Zx; z[1] = Zy; z[2] = Zz; 
}

// Cross product of two 3-vectors. Result vector in z[3].
inline void vtkMath::Cross(const double x[3], const double y[3], double z[3])
{
  double Zx = x[1]*y[2] - x[2]*y[1]; 
  double Zy = x[2]*y[0] - x[0]*y[2];
  double Zz = x[0]*y[1] - x[1]*y[0];
  z[0] = Zx; z[1] = Zy; z[2] = Zz; 
}

//BTX
//----------------------------------------------------------------------------
template<class T>
static inline double vtkDeterminant3x3(T A[3][3])
{
  return A[0][0]*A[1][1]*A[2][2] + A[1][0]*A[2][1]*A[0][2] + 
         A[2][0]*A[0][1]*A[1][2] - A[0][0]*A[2][1]*A[1][2] - 
         A[1][0]*A[0][1]*A[2][2] - A[2][0]*A[1][1]*A[0][2];
}
//ETX

inline double vtkMath::Determinant3x3(float A[3][3])
{
  return vtkDeterminant3x3(A);
}

inline double vtkMath::Determinant3x3(double A[3][3])
{
  return vtkDeterminant3x3(A);
}


#endif
