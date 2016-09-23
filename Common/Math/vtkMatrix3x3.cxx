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
#include <cmath>

vtkStandardNewMacro(vtkMatrix3x3);

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
void vtkMatrix3x3::Zero(double elements[9])
{
  for (int i = 0; i < 9; ++i)
  {
    elements[i] = 0.0;
  }
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::Identity(double elements[9])
{
  elements[0] = elements[4] = elements[8] = 1.0;
  elements[1] = elements[2] = elements[3] = elements[5] =
    elements[6] = elements[7] = 0.0;
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
void vtkMatrix3x3::MultiplyPoint(const double elements[9],
                                 const float in[3], float result[3])
{
  vtkMatrix3x3MultiplyPoint(elements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::MultiplyPoint(const double elements[9],
                                 const double in[3], double result[3])
{
  vtkMatrix3x3MultiplyPoint(elements,in,result);
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double *vtkMatrix3x3::operator[](const unsigned int i)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::operator[], "VTK 7.0");
  return &(this->Element[i][0]);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
const double *vtkMatrix3x3::operator[](unsigned int i) const
{
  VTK_LEGACY_BODY(vtkMatrix3x3::operator[], "VTK 7.0");
  return &(this->Element[i][0]);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
bool vtkMatrix3x3::operator==(const vtkMatrix3x3 &other)
{
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (Element[i][j] != other.Element[i][j])
      {
        return false;
      }
    }
  }
  return true;
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
bool vtkMatrix3x3::operator!=(const vtkMatrix3x3 &other)
{
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (Element[i][j] != other.Element[i][j])
      {
        return true;
      }
    }
  }
  return false;
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix3x3::Adjoint(vtkMatrix3x3 &in, vtkMatrix3x3 &out)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::Adjoint, "VTK 7.0");
  this->Adjoint(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double vtkMatrix3x3::Determinant(vtkMatrix3x3 &in)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::Determinant, "VTK 7.0");
  return vtkMatrix3x3::Determinant(*in.Element);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
double vtkMatrix3x3::Determinant(vtkMatrix3x3 *in)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::Determinant, "VTK 7.0");
  return vtkMatrix3x3::Determinant(*in->Element);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix3x3::Invert(vtkMatrix3x3 &in, vtkMatrix3x3 &out)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::Invert, "VTK 7.0");
  this->Invert(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix3x3::Transpose(vtkMatrix3x3 &in, vtkMatrix3x3 &out)
{
  VTK_LEGACY_BODY(vtkMatrix3x3::Transpose, "VTK 7.0");
  this->Transpose(&in, &out);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix3x3::PointMultiply(const double elements[9],
                                 const float in[3], float result[3])
{
  VTK_LEGACY_BODY(vtkMatrix3x3::PointMultiply, "VTK 7.0");
  double newElements[9];
  vtkMatrix3x3::Transpose(elements, newElements);
  vtkMatrix3x3::MultiplyPoint(newElements, in, result);
}
#endif

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkMatrix3x3::PointMultiply(const double elements[9],
                                 const double in[3], double result[3])
{
  VTK_LEGACY_BODY(vtkMatrix3x3::PointMultiply, "VTK 7.0");
  double newElements[9];
  vtkMatrix3x3::Transpose(elements, newElements);
  vtkMatrix3x3::MultiplyPoint(newElements, in, result);
}
#endif

//----------------------------------------------------------------------------
// Multiplies matrices a and b and stores the result in c.
void vtkMatrix3x3::Multiply3x3(const double a[9], const double b[9],
                               double c[9])
{
  double accum[9];

  for (int i = 0; i < 9; i += 3)
  {
    for (int k = 0; k < 3; k++)
    {
      accum[i + k] = a[i + 0] * b[k + 0] +
                     a[i + 1] * b[k + 3] +
                     a[i + 2] * b[k + 6];
    }
  }

  // Copy to final dest
  for (int j = 0; j < 9; j++)
  {
    c[j] = accum[j];
  }
}

//----------------------------------------------------------------------------
// Matrix Inversion (adapted from Richard Carling in "Graphics Gems,"
// Academic Press, 1990).
void vtkMatrix3x3::Invert(const double inElements[9],
                          double outElements[9])
{
  // inverse( original_matrix, inverse_matrix )
  // calculate the inverse of a 3x3 matrix
  //
  //     -1
  //     A  = ___1__ adjoint A
  //         det A
  //

  // calculate the 3x3 determinent
  // if the determinent is zero,
  // then the inverse matrix is not unique.

  double det = vtkMatrix3x3::Determinant(inElements);
  if (det == 0.0)
  {
    return;
  }

  // calculate the adjoint matrix
  vtkMatrix3x3::Adjoint(inElements, outElements);

  // scale the adjoint matrix to get the inverse
  for (int i = 0; i < 9; i++)
  {
    outElements[i] /= det;
  }
}

//----------------------------------------------------------------------------
double vtkMatrix3x3::Determinant(const double elements[9])
{
  return vtkMath::Determinant3x3(
    elements[0], elements[1], elements[2],
    elements[3], elements[4], elements[5],
    elements[6], elements[7], elements[8]);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::Adjoint(const double inElements[9], double outElements[9])
{
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
  a1 = inElements[0]; b1 = inElements[1]; c1 = inElements[2];
  a2 = inElements[3]; b2 = inElements[4]; c2 = inElements[5];
  a3 = inElements[6]; b3 = inElements[7]; c3 = inElements[8];

  // row column labeling reversed since we transpose rows & columns

  outElements[0] =   vtkMath::Determinant2x2(b2, b3, c2, c3);
  outElements[3] = - vtkMath::Determinant2x2(a2, a3, c2, c3);
  outElements[6] =   vtkMath::Determinant2x2(a2, a3, b2, b3);

  outElements[1] = - vtkMath::Determinant2x2(b1, b3, c1, c3);
  outElements[4] =   vtkMath::Determinant2x2(a1, a3, c1, c3);
  outElements[7] = - vtkMath::Determinant2x2(a1, a3, b1, b3);

  outElements[2] =   vtkMath::Determinant2x2(b1, b2, c1, c2);
  outElements[5] = - vtkMath::Determinant2x2(a1, a2, c1, c2);
  outElements[8] =   vtkMath::Determinant2x2(a1, a2, b1, b2);
}

//----------------------------------------------------------------------------
void vtkMatrix3x3::DeepCopy(double elements[9], const double newElements[9])
{
  for (int i = 0; i < 9; ++i)
  {
    elements[i] = newElements[i];
  }
}

//----------------------------------------------------------------------------
// Transpose the matrix and put it into out.
void vtkMatrix3x3::Transpose(const double inElements[9],
                             double outElements[9])
{
  for (int i = 0; i < 3; ++i)
  {
    for(int j = i; j < 3; ++j)
    {
      double temp = inElements[3*i + j];
      outElements[3*i + j] = inElements[3*j + i];
      outElements[3*j + i] = temp;
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
