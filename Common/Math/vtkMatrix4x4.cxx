/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <cstdlib>
#include <cmath>

vtkStandardNewMacro(vtkMatrix4x4);

//----------------------------------------------------------------------------
void vtkMatrix4x4::Zero(double elements[16])
{
  for (int i = 0; i < 16; i++)
  {
    elements[i] = 0.0;
  }
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::Identity(double elements[16])
{
  elements[0] = elements[5] = elements[10] = elements[15] = 1.0;
  elements[1] = elements[2] = elements[3] = elements[4] =
    elements[6] = elements[7] = elements[8] = elements[9] =
    elements[11] = elements[12] = elements[13] = elements[14] = 0.0;
}

//----------------------------------------------------------------------------
namespace { // Enclose private helper function in anonymous namespace

template<class T1, class T2, class T3>
void vtkMatrix4x4MultiplyPoint(T1 elem[16], T2 in[4], T3 out[4])
{
  T3 v1 = in[0];
  T3 v2 = in[1];
  T3 v3 = in[2];
  T3 v4 = in[3];

  out[0] = v1*elem[0]  + v2*elem[1]  + v3*elem[2]  + v4*elem[3];
  out[1] = v1*elem[4]  + v2*elem[5]  + v3*elem[6]  + v4*elem[7];
  out[2] = v1*elem[8]  + v2*elem[9]  + v3*elem[10] + v4*elem[11];
  out[3] = v1*elem[12] + v2*elem[13] + v3*elem[14] + v4*elem[15];
}

} // End anonymous namespace

//----------------------------------------------------------------------------
// Multiply this matrix by a point (in homogeneous coordinates).
// and return the result in result. The in[4] and result[4]
// arrays must both be allocated but they can be the same array.
void vtkMatrix4x4::MultiplyPoint(const double elements[16],
                                 const float in[4], float result[4])
{
  vtkMatrix4x4MultiplyPoint(elements, in, result);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::MultiplyPoint(const double elements[16],
                                 const double in[4], double result[4])
{
  vtkMatrix4x4MultiplyPoint(elements, in, result);
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double *vtkMatrix4x4::operator[](const unsigned int i)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::operator[], "VTK 7.0");
  return &(this->Element[i][0]);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
const double *vtkMatrix4x4::operator[](unsigned int i) const
{
  VTK_LEGACY_BODY(vtkMatrix4x4::operator[], "VTK 7.0");
  return &(this->Element[i][0]);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix4x4::Adjoint(vtkMatrix4x4 &in, vtkMatrix4x4 &out)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::Adjoint, "VTK 7.0");
  this->Adjoint(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double vtkMatrix4x4::Determinant(vtkMatrix4x4 &in)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::Determinant, "VTK 7.0");
  return vtkMatrix4x4::Determinant(*in.Element);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double vtkMatrix4x4::Determinant(vtkMatrix4x4 *in)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::Determinant, "VTK 7.0");
  return vtkMatrix4x4::Determinant(*in->Element);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix4x4::Invert(vtkMatrix4x4 &in, vtkMatrix4x4 &out)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::Invert, "VTK 7.0");
  this->Invert(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix4x4::Transpose(vtkMatrix4x4 &in, vtkMatrix4x4 &out)
{
  VTK_LEGACY_BODY(vtkMatrix4x4::Transpose, "VTK 7.0");
  this->Transpose(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix4x4::PointMultiply(const double elements[16],
                                 const float in[4], float result[4])
{
  VTK_LEGACY_BODY(vtkMatrix4x4::PointMultiply, "VTK 7.0");
  double newElements[16];
  vtkMatrix4x4::Transpose(elements, newElements);
  vtkMatrix4x4::MultiplyPoint(newElements, in, result);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix4x4::PointMultiply(const double elements[16],
                                 const double in[4], double result[4])
{
  VTK_LEGACY_BODY(vtkMatrix4x4::PointMultiply, "VTK 7.0");
  double newElements[16];
  vtkMatrix4x4::Transpose(elements, newElements);
  vtkMatrix4x4::MultiplyPoint(newElements, in, result);
}
#endif

//----------------------------------------------------------------------------
// Matrix Inversion (adapted from Richard Carling in "Graphics Gems,"
// Academic Press, 1990).
void vtkMatrix4x4::Invert(const double inElements[16],
                          double outElements[16])
{
  // inverse( original_matrix, inverse_matrix )
  // calculate the inverse of a 4x4 matrix
  //
  //     -1
  //     A  = ___1__ adjoint A
  //         det A
  //

  // calculate the 4x4 determinent
  // if the determinent is zero,
  // then the inverse matrix is not unique.

  double det = vtkMatrix4x4::Determinant(inElements);
  if (det == 0.0)
  {
    return;
  }

  // calculate the adjoint matrix
  vtkMatrix4x4::Adjoint(inElements, outElements);

  // scale the adjoint matrix to get the inverse
  for (int i = 0; i < 16; i++)
  {
    outElements[i] /= det;
  }
}

//----------------------------------------------------------------------------
double vtkMatrix4x4::Determinant(const double elem[16])
{
  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  // assign to individual variable names to aid selecting
  //  correct elements

  a1 = elem[0];  b1 = elem[1];  c1 = elem[2];  d1 = elem[3];
  a2 = elem[4];  b2 = elem[5];  c2 = elem[6];  d2 = elem[7];
  a3 = elem[8];  b3 = elem[9];  c3 = elem[10]; d3 = elem[11];
  a4 = elem[12]; b4 = elem[13]; c4 = elem[14]; d4 = elem[15];

  return a1 * vtkMath::Determinant3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)
       - b1 * vtkMath::Determinant3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)
       + c1 * vtkMath::Determinant3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)
       - d1 * vtkMath::Determinant3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::Adjoint(const double elem[16], double outElem[16])
{
  //
  //   adjoint( original_matrix, inverse_matrix )
  //
  //     calculate the adjoint of a 4x4 matrix
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
  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  // assign to individual variable names to aid
  // selecting correct values

  a1 = elem[0];  b1 = elem[1];  c1 = elem[2];  d1 = elem[3];
  a2 = elem[4];  b2 = elem[5];  c2 = elem[6];  d2 = elem[7];
  a3 = elem[8];  b3 = elem[9];  c3 = elem[10]; d3 = elem[11];
  a4 = elem[12]; b4 = elem[13]; c4 = elem[14]; d4 = elem[15];

  // row column labeling reversed since we transpose rows & columns

  outElem[0]  =
    vtkMath::Determinant3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
  outElem[4]  =
    - vtkMath::Determinant3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
  outElem[8]  =
    vtkMath::Determinant3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
  outElem[12]  =
    - vtkMath::Determinant3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

  outElem[1]  =
    - vtkMath::Determinant3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
  outElem[5]  =
    vtkMath::Determinant3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
  outElem[9]  =
    - vtkMath::Determinant3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
  outElem[13]  =
    vtkMath::Determinant3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

  outElem[2]  =
    vtkMath::Determinant3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
  outElem[6]  =
    - vtkMath::Determinant3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
  outElem[10]  =
    vtkMath::Determinant3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
  outElem[14]  =
    - vtkMath::Determinant3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

  outElem[3]  =
    - vtkMath::Determinant3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
  outElem[7]  =
    vtkMath::Determinant3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
  outElem[11]  =
    - vtkMath::Determinant3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
  outElem[15]  =
    vtkMath::Determinant3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::DeepCopy(double destination[16], const double source[16])
{
  for (int i = 0; i < 16; i++)
  {
    destination[i] = source[i];
  }
}

//----------------------------------------------------------------------------
// Transpose the matrix and put it into out.
void vtkMatrix4x4::Transpose(const double inElements[16],
                             double outElements[16])
{
  for (int i = 0; i < 4; i++)
  {
    for(int j = i; j < 4; j++)
    {
      double temp = inElements[4*i + j];
      outElements[4*i + j] = inElements[4*j + i];
      outElements[4*j + i] = temp;
    }
  }
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Elements:\n";
  for (int i = 0; i < 4; i++)
  {
    os << indent << indent;
    for (int j = 0; j < 4; j++)
    {
      os << this->Element[i][j] << " ";
    }
    os << "\n";
  }
}
