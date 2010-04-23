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
  this->TransformPoints(inPts,outPts);
  if (inNms)
    {
    this->TransformNormals(inNms,outNms);
    }
  if (inVrs)
    {
    this->TransformVectors(inVrs,outVrs);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformPoints(vtkPoints *inPts, 
                                         vtkPoints *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  double (*matrix)[4] = this->Matrix->Element;
  double point[3];  

  this->Update();

  for (vtkIdType i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkLinearTransformPoint(matrix,point,point);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkDataArray *inNms, 
                                          vtkDataArray *outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  double norm[3];
  double matrix[4][4];
  
  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);  
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  for (vtkIdType i = 0; i < n; i++)
    {
    inNms->GetTuple(i,norm);

    // use TransformVector because matrix is already transposed & inverted
    vtkLinearTransformVector(matrix,norm,norm);
    vtkMath::Normalize(norm);

    outNms->InsertNextTuple(norm);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkDataArray *inNms, 
                                          vtkDataArray *outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  double vec[3];
  
  this->Update();

  double (*matrix)[4] = this->Matrix->Element;

  for (vtkIdType i = 0; i < n; i++)
    {
    inNms->GetTuple(i,vec);

    vtkLinearTransformVector(matrix,vec,vec);

    outNms->InsertNextTuple(vec);
    }
}

