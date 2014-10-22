/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorOperators.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVectorOperators_h
#define vtkVectorOperators_h

// This set of operators enhance the vtkVector classes, allowing various
// operator overloads one might expect.
#include "vtkVector.h"

// Description:
// Unary minus / negation of vector.
template<typename A, int Size>
vtkVector<A, Size> operator-(const vtkVector<A, Size>& v)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = -v[i];
    }
  return ret;
}

// Description:
// Performs addition of vectors of the same basic type.
template<typename A, int Size>
vtkVector<A, Size> operator+(const vtkVector<A, Size>& v1,
                             const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = v1[i] + v2[i];
    }
  return ret;
}

// Description:
// Performs subtraction of vectors of the same basic type.
template<typename A, int Size>
vtkVector<A, Size> operator-(const vtkVector<A, Size>& v1,
                             const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = v1[i] - v2[i];
    }
  return ret;
}

// Description:
// Performs multiplication of vectors of the same basic type.
template<typename A, int Size>
vtkVector<A, Size> operator*(const vtkVector<A, Size>& v1,
                             const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = v1[i] * v2[i];
    }
  return ret;
}

// Description:
// Performs multiplication of vectors by a scalar value.
template<typename A, typename B, int Size>
vtkVector<A, Size> operator*(const vtkVector<A, Size>& v1,
                             const B& scalar)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = v1[i] * scalar;
    }
  return ret;
}

// Description:
// Performs divisiom of vectors of the same type.
template<typename A, int Size>
vtkVector<A, Size> operator/(const vtkVector<A, Size>& v1,
                             const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
    {
    ret[i] = v1[i] / v2[i];
    }
  return ret;
}

// Description:
// Several macros to define the various operator overloads for the vectors.
#define vtkVectorOperatorNegate(vectorType, type, size) \
inline vectorType operator-(const vectorType& v) \
{ \
  return vectorType((-static_cast<vtkVector<type, size> >(v)).GetData()); \
}
#define vtkVectorOperatorPlus(vectorType, type, size) \
inline vectorType operator+(const vectorType& v1, const vectorType& v2) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) + \
    static_cast<vtkVector<type, size> >(v2)).GetData()); \
}
#define vtkVectorOperatorMinus(vectorType, type, size) \
inline vectorType operator-(const vectorType& v1, const vectorType& v2) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) - \
    static_cast<vtkVector<type, size> >(v2)).GetData()); \
}
#define vtkVectorOperatorMultiply(vectorType, type, size) \
inline vectorType operator*(const vectorType& v1, const vectorType& v2) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) * \
    static_cast<vtkVector<type, size> >(v2)).GetData()); \
}
#define vtkVectorOperatorMultiplyScalar(vectorType, type, size) \
template<typename B> \
inline vectorType operator*(const vectorType& v1, const B& scalar) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) * scalar).GetData()); \
}
#define vtkVectorOperatorMultiplyScalarPre(vectorType, type, size) \
template<typename B> \
inline vectorType operator*(const B& scalar, const vectorType& v1) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) * scalar).GetData()); \
}
#define vtkVectorOperatorDivide(vectorType, type, size) \
inline vectorType operator/(const vectorType& v1, const vectorType& v2) \
{ \
  return vectorType((static_cast<vtkVector<type, size> >(v1) / \
    static_cast<vtkVector<type, size> >(v2)).GetData()); \
}

#define vtkVectorOperatorMacro(vectorType, type, size) \
vtkVectorOperatorNegate(vectorType, type, size) \
vtkVectorOperatorPlus(vectorType, type, size) \
vtkVectorOperatorMinus(vectorType, type, size) \
vtkVectorOperatorMultiply(vectorType, type, size) \
vtkVectorOperatorMultiplyScalar(vectorType, type, size) \
vtkVectorOperatorMultiplyScalarPre(vectorType, type, size) \
vtkVectorOperatorDivide(vectorType, type, size)

// Description:
// Overload the operators for the common types.
vtkVectorOperatorMacro(vtkVector2i, int,    2)
vtkVectorOperatorMacro(vtkVector2f, float,  2)
vtkVectorOperatorMacro(vtkVector2d, double, 2)
vtkVectorOperatorMacro(vtkVector3i, int,    3)
vtkVectorOperatorMacro(vtkVector3f, float,  3)
vtkVectorOperatorMacro(vtkVector3d, double, 3)

#endif
// VTK-HeaderTest-Exclude: vtkVectorOperators.h
