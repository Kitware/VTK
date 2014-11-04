/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix3x3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMatrix3x3.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <cstdlib>
#include <math.h>

vtkStandardNewMacro(vtkMatrix3x3);

// Useful for viewing a double[9] as a double[3][3]
typedef double (*SqMatPtr)[3];

//----------------------------------------------------------------------------
vtkMatrix3x3::vtkMatrix3x3()
{
  vtkMatrix3x3::Identity(*this->Element);
}

//----------------------------------------------------------------------------
vtkMatrix3x3::~vtkMatrix3x3()
{
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::Zero(double Elements[9])
{
  SqMatPtr elem  = (SqMatPtr)Elements;
  int i,j;
  for (i = 0; i < 3; ++i)
    {
    for (j = 0; j < 3; ++j)
      {
      elem[i][j] = 0.0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::Identity(double Elements[9])
{
  Elements[0] = Elements[4] = Elements[8] = 1.0;
  Elements[1] = Elements[2] = Elements[3] = Elements[5] =
    Elements[6] = Elements[7] = 0.0;
}

//----------------------------------------------------------------------------
namespace { // Enclose private helper function in anonymous namespace

template<class T1, class T2, class T3>
void vtkMatrix3x3MultiplyPoint(T1 elem[9], T2 in[3], T3 out[3])
{
  T3 v1 = in[0];
  T3 v2 = in[1];
  T3 v3 = in[2];

  out[0] = v1*elem[0]  + v2*elem[1]  + v3*elem[2];
  out[1] = v1*elem[3]  + v2*elem[4]  + v3*elem[5];
  out[2] = v1*elem[6]  + v2*elem[7]  + v3*elem[8];
}

} // End anonymous namespace

//----------------------------------------------------------------------------
// Multiply this matrix by a point (in homogeneous coordinates).
// and return the result in result. The in[3] and result[3]
// arrays must both be allocated but they can be the same array.
void vtkMatrix3x3::MultiplyPoint(const double Elements[9],
                                 const float in[3], float result[3])
{
  vtkMatrix3x3MultiplyPoint(Elements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::MultiplyPoint(const double Elements[9],
                                 const double in[3], double result[3])
{
  vtkMatrix3x3MultiplyPoint(Elements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::PointMultiply(const double Elements[9],
                                 const float in[3], float result[3])
{
  double newElements[9];
  vtkMatrix3x3::Transpose(Elements,newElements);
  vtkMatrix3x3::MultiplyPoint(newElements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::PointMultiply(const double Elements[9],
                                 const double in[3], double result[3])
{
  double newElements[9];
  vtkMatrix3x3::Transpose(Elements,newElements);
  vtkMatrix3x3::MultiplyPoint(newElements,in,result);
}

//----------------------------------------------------------------------------
// Multiplies matrices a and b and stores the result in c.
void vtkMatrix3x3::Multiply3x3(const double a[9], const double b[9],
                               double c[9])
{
  SqMatPtr aMat = (SqMatPtr) a;
  SqMatPtr bMat = (SqMatPtr) b;
  SqMatPtr cMat = (SqMatPtr) c;
  int i, k;
  double Accum[3][3];

  for (i = 0; i < 3; ++i)
    {
    for (k = 0; k < 3; ++k)
      {
      Accum[i][k] = aMat[i][0] * bMat[0][k] +
                    aMat[i][1] * bMat[1][k] +
                    aMat[i][2] * bMat[2][k];
      }
    }

  // Copy to final dest
  for (i = 0; i < 3; ++i)
    {
    cMat[i][0] = Accum[i][0];
    cMat[i][1] = Accum[i][1];
    cMat[i][2] = Accum[i][2];
    }

}

//----------------------------------------------------------------------------
// Matrix Inversion (adapted from Richard Carling in "Graphics Gems,"
// Academic Press, 1990).
void vtkMatrix3x3::Invert(const double inElements[9],
                          double outElements[9])
{
  SqMatPtr outElem = (SqMatPtr)outElements;

  // inverse( original_matrix, inverse_matrix )
  // calculate the inverse of a 3x3 matrix
  //
  //     -1
  //     A  = ___1__ adjoint A
  //         det A
  //

  int i, j;
  double det;

  // calculate the 3x3 determinent
  // if the determinent is zero,
  // then the inverse matrix is not unique.

  det = vtkMatrix3x3::Determinant(inElements);
  if ( det == 0.0 )
    {
    return;
    }

  // calculate the adjoint matrix
  vtkMatrix3x3::Adjoint(inElements, outElements );

  // scale the adjoint matrix to get the inverse
  for (i=0; i<3; ++i)
    {
    for(j=0; j<3; ++j)
      {
      outElem[i][j] = outElem[i][j] / det;
      }
    }
}

//----------------------------------------------------------------------------
double vtkMatrix3x3::Determinant(const double Elements[9])
{
  SqMatPtr elem = (SqMatPtr)Elements;

  return vtkMath::Determinant3x3(elem);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::Adjoint(const double inElements[9], double outElements[9])
{
  SqMatPtr inElem = (SqMatPtr) inElements;
  SqMatPtr outElem = (SqMatPtr) outElements;

  //
  //   adjoint( original_matrix, inverse_matrix )
  //
  //     calculate the adjoint of a 3x3 matrix
  //
  //      Let  a   denote the minor determinant of matrix A obtained by
  //           ij
  //
  //      deleting the ith row and jth column from A.
  //
  //                    i+j
  //     Let  b   = (-1)    a
  //          ij            ji
  //
  //    The matrix B = (b  ) is the adjoint of A
  //                     ij
  //
  double a1, a2, a3, b1, b2, b3, c1, c2, c3;

  // assign to individual variable names to aid
  // selecting correct values
  a1 = inElem[0][0]; b1 = inElem[0][1]; c1 = inElem[0][2];
  a2 = inElem[1][0]; b2 = inElem[1][1]; c2 = inElem[1][2];
  a3 = inElem[2][0]; b3 = inElem[2][1]; c3 = inElem[2][2];

  // row column labeling reversed since we transpose rows & columns

  outElem[0][0]  =   vtkMath::Determinant2x2( b2, b3, c2, c3);
  outElem[1][0]  = - vtkMath::Determinant2x2( a2, a3, c2, c3);
  outElem[2][0]  =   vtkMath::Determinant2x2( a2, a3, b2, b3);

  outElem[0][1]  = - vtkMath::Determinant2x2( b1, b3, c1, c3);
  outElem[1][1]  =   vtkMath::Determinant2x2( a1, a3, c1, c3);
  outElem[2][1]  = - vtkMath::Determinant2x2( a1, a3, b1, b3);

  outElem[0][2]  =   vtkMath::Determinant2x2( b1, b2, c1, c2);
  outElem[1][2]  = - vtkMath::Determinant2x2( a1, a2, c1, c2);
  outElem[2][2]  =   vtkMath::Determinant2x2( a1, a2, b1, b2);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::DeepCopy(double Elements[9], const double newElements[9])
{
  for (int i = 0; i < 9; ++i)
    {
    Elements[i] = newElements[i];
    }
}

//----------------------------------------------------------------------------
// Transpose the matrix and put it into out.
void vtkMatrix3x3::Transpose(const double inElements[9],
                             double outElements[9])
{
  SqMatPtr inElem = (SqMatPtr)inElements;
  SqMatPtr outElem = (SqMatPtr)outElements;
  double temp;

  for (int i=0; i<3; ++i)
    {
    for(int j=i; j<3; ++j)
      {
      temp = inElem[i][j];
      outElem[i][j] = inElem[j][i];
      outElem[j][i] = temp;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Elements:\n";
  for (int i = 0; i < 3; ++i)
    {
    os << indent;
    for (int j = 0; j < 3; ++j)
      {
      os << "\t" << this->Element[i][j];
      }
    os << "\n";
    }
}
