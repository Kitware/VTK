/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLinearTransform.h"

#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"


//------------------------------------------------------------------------
void vtkLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformPoint(T1 matrix[4][4],
                                    T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(
    matrix[0][0]*in[0]+matrix[0][1]*in[1]+matrix[0][2]*in[2]+matrix[0][3]);
  T3 y = static_cast<T3>(
    matrix[1][0]*in[0]+matrix[1][1]*in[1]+matrix[1][2]*in[2]+matrix[1][3]);
  T3 z = static_cast<T3>(
    matrix[2][0]*in[0]+matrix[2][1]*in[1]+matrix[2][2]*in[2]+matrix[2][3]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3, class T4>
inline void vtkLinearTransformDerivative(T1 matrix[4][4],
                                         T2 in[3], T3 out[3],
                                         T4 derivative[3][3])
{
  vtkLinearTransformPoint(matrix,in,out);

  for (int i = 0; i < 3; i++)
    {
    derivative[0][i] = static_cast<T4>(matrix[0][i]);
    derivative[1][i] = static_cast<T4>(matrix[1][i]);
    derivative[2][i] = static_cast<T4>(matrix[2][i]);
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformVector(T1 matrix[4][4],
                                     T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(
    matrix[0][0]*in[0] + matrix[0][1]*in[1] + matrix[0][2]*in[2]);
  T3 y = static_cast<T3>(
    matrix[1][0]*in[0] + matrix[1][1]*in[1] + matrix[1][2]*in[2]);
  T3 z = static_cast<T3>(
    matrix[2][0]*in[0] + matrix[2][1]*in[1] + matrix[2][2]*in[2]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformNormal(T1 mat[4][4],
                                            T2 in[3], T3 out[3])
{
  // to transform the normal, multiply by the transposed inverse matrix
  T1 matrix[4][4];
  memcpy(*matrix,*mat,16*sizeof(T1));
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  vtkLinearTransformVector(matrix,in,out);

  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformPoints(
  T1 matrix[4][4], T2 *in, T3 *out, vtkIdType n)
{
  for (vtkIdType i = 0; i < n; i++)
    {
    vtkLinearTransformPoint(matrix, in, out);
    in += 3;
    out += 3;
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformVectors(
  T1 matrix[4][4], T2 *in, T3 *out, vtkIdType n)
{
  for (vtkIdType i = 0; i < n; i++)
    {
    vtkLinearTransformVector(matrix, in, out);
    in += 3;
    out += 3;
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformNormals(
  T1 matrix[4][4], T2 *in, T3 *out, vtkIdType n)
{
  for (vtkIdType i = 0; i < n; i++)
    {
    // matrix has been transposed & inverted, so use TransformVector
    vtkLinearTransformVector(matrix, in, out);
    vtkMath::Normalize(out);
    in += 3;
    out += 3;
    }
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const float in[3],
                                                float out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const double in[3],
                                                double out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const float in[3],
                                                 float out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const double in[3],
                                                 double out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const float in[3],
                                                 float out[3])
{
  vtkLinearTransformVector(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const double in[3],
                                                 double out[3])
{
  vtkLinearTransformVector(this->Matrix->Element,in,out);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const float in[3],
                                                     float out[3],
                                                     float derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const double in[3],
                                                     double out[3],
                                                     double derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
void vtkLinearTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
                                                       vtkPoints *outPts,
                                                       vtkDataArray *inNms,
                                                       vtkDataArray *outNms,
                                                       vtkDataArray *inVrs,
                                                       vtkDataArray *outVrs)
{
  this->TransformPoints(inPts, outPts);
  if (inNms)
    {
    this->TransformNormals(inNms, outNms);
    }
  if (inVrs)
    {
    this->TransformVectors(inVrs, outVrs);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformPoints(vtkPoints *inPts,
                                         vtkPoints *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  vtkIdType m = outPts->GetNumberOfPoints();
  double (*matrix)[4] = this->Matrix->Element;

  this->Update();

  // operate directly on the memory to avoid GetPoint()/SetPoint() calls.
  vtkDataArray *inArray = inPts->GetData();
  vtkDataArray *outArray = outPts->GetData();
  int inType = inArray->GetDataType();
  int outType = outArray->GetDataType();
  void *inPtr = inArray->GetVoidPointer(0);
  void *outPtr = outArray->WriteVoidPointer(3*m, 3*n);

  if (inType == VTK_FLOAT && outType == VTK_FLOAT)
    {
    vtkLinearTransformPoints(matrix,
      static_cast<float *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_FLOAT && outType == VTK_DOUBLE)
    {
    vtkLinearTransformPoints(matrix,
      static_cast<float *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_FLOAT)
    {
    vtkLinearTransformPoints(matrix,
      static_cast<double *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_DOUBLE)
    {
    vtkLinearTransformPoints(matrix,
      static_cast<double *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else
    {
    double point[3];

    for (vtkIdType i = 0; i < n; i++)
      {
      inPts->GetPoint(i, point);

      vtkLinearTransformPoint(matrix, point, point);

      outPts->SetPoint(m + i, point);
      }
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkDataArray *inNms,
                                          vtkDataArray *outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  vtkIdType m = outNms->GetNumberOfTuples();
  double matrix[4][4];

  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  // operate directly on the memory to avoid GetTuple()/SetPoint() calls.
  int inType = inNms->GetDataType();
  int outType = outNms->GetDataType();
  void *inPtr = inNms->GetVoidPointer(0);
  void *outPtr = outNms->WriteVoidPointer(3*m, 3*n);

  if (inType == VTK_FLOAT && outType == VTK_FLOAT)
    {
    vtkLinearTransformNormals(matrix,
      static_cast<float *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_FLOAT && outType == VTK_DOUBLE)
    {
    vtkLinearTransformNormals(matrix,
      static_cast<float *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_FLOAT)
    {
    vtkLinearTransformNormals(matrix,
      static_cast<double *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_DOUBLE)
    {
    vtkLinearTransformNormals(matrix,
      static_cast<double *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else
    {
    for (vtkIdType i = 0; i < n; i++)
      {
      double norm[3];

      inNms->GetTuple(i, norm);

      // use TransformVector because matrix is already transposed & inverted
      vtkLinearTransformVector(matrix, norm, norm);
      vtkMath::Normalize(norm);

      outNms->SetTuple(m + i, norm);
      }
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkDataArray *inVrs,
                                          vtkDataArray *outVrs)
{
  vtkIdType n = inVrs->GetNumberOfTuples();
  vtkIdType m = outVrs->GetNumberOfTuples();

  double (*matrix)[4] = this->Matrix->Element;

  this->Update();

  // operate directly on the memory to avoid GetTuple()/SetTuple() calls.
  int inType = inVrs->GetDataType();
  int outType = outVrs->GetDataType();
  void *inPtr = inVrs->GetVoidPointer(0);
  void *outPtr = outVrs->WriteVoidPointer(3*m, 3*n);

  if (inType == VTK_FLOAT && outType == VTK_FLOAT)
    {
    vtkLinearTransformVectors(matrix,
      static_cast<float *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_FLOAT && outType == VTK_DOUBLE)
    {
    vtkLinearTransformVectors(matrix,
      static_cast<float *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_FLOAT)
    {
    vtkLinearTransformVectors(matrix,
      static_cast<double *>(inPtr), static_cast<float *>(outPtr), n);
    }
  else if (inType == VTK_DOUBLE && outType == VTK_DOUBLE)
    {
    vtkLinearTransformVectors(matrix,
      static_cast<double *>(inPtr), static_cast<double *>(outPtr), n);
    }
  else
    {
    for (vtkIdType i = 0; i < n; i++)
      {
      double vec[3];

      inVrs->GetTuple(i, vec);

      vtkLinearTransformVector(matrix, vec, vec);

      outVrs->SetTuple(m + i, vec);
      }
    }
}
