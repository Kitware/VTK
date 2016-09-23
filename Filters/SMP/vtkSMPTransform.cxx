/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPTransform.h"
#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"
#include "vtkObjectFactory.h"

#include "vtkSMPTools.h"

#include <cstdlib>

vtkStandardNewMacro(vtkSMPTransform);

//----------------------------------------------------------------------------
void vtkSMPTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkSMPTransformPoint(T1 matrix[4][4],
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
inline void vtkSMPTransformDerivative(T1 matrix[4][4],
                                      T2 in[3], T3 out[3],
                                      T4 derivative[3][3])
{
  vtkSMPTransformPoint(matrix,in,out);

  for (int i = 0; i < 3; i++)
  {
    derivative[0][i] = static_cast<T4>(matrix[0][i]);
    derivative[1][i] = static_cast<T4>(matrix[1][i]);
    derivative[2][i] = static_cast<T4>(matrix[2][i]);
  }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkSMPTransformVector(T1 matrix[4][4],
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
inline void vtkSMPTransformNormal(T1 mat[4][4],
                                  T2 in[3], T3 out[3])
{
  // to transform the normal, multiply by the transposed inverse matrix
  T1 matrix[4][4];
  memcpy(*matrix,*mat,16*sizeof(T1));
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  vtkSMPTransformVector(matrix,in,out);

  vtkMath::Normalize(out);
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
class TranformAllFunctor
{
public:
  vtkPoints* inPts;
  vtkPoints* outPts;
  vtkDataArray* inNms;
  vtkDataArray* outNms;
  vtkDataArray* inVcs;
  vtkDataArray* outVcs;
  double (*matrix)[4];
  double (*matrixInvTr)[4];
  void operator()( vtkIdType begin, vtkIdType end ) const
  {
    for (vtkIdType id=begin; id<end; id++)
    {
      double point[3];
      inPts->GetPoint(id, point);
      vtkSMPTransformPoint(matrix, point, point);
      outPts->SetPoint(id, point);
      if (inVcs)
      {
        inVcs->GetTuple(id, point);
        vtkSMPTransformVector(matrix, point, point);
        outVcs->SetTuple(id, point);
      }
      if (inNms)
      {
        inNms->GetTuple(id, point);
        vtkSMPTransformVector(matrixInvTr, point, point);
        vtkMath::Normalize( point );
        outNms->SetTuple(id, point);
      }
    }
  }
};

void vtkSMPTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
                                                    vtkPoints *outPts,
                                                    vtkDataArray *inNms,
                                                    vtkDataArray *outNms,
                                                    vtkDataArray *inVrs,
                                                    vtkDataArray *outVrs)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  double matrix[4][4];
  this->Update();

  TranformAllFunctor functor;
  functor.inPts = inPts;
  functor.outPts = outPts;
  functor.inNms = inNms;
  functor.outNms = outNms;
  functor.inVcs = inVrs;
  functor.outVcs = outVrs;
  functor.matrix = this->Matrix->Element;
  if (inNms)
  {
    vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);
    vtkMatrix4x4::Invert(*matrix,*matrix);
    vtkMatrix4x4::Transpose(*matrix,*matrix);
    functor.matrixInvTr = matrix;
  }

  vtkSMPTools::For( 0, n, functor );
}

//----------------------------------------------------------------------------
class TransformPointsFunctor
{
public:
  vtkPoints* inPts;
  vtkPoints* outPts;
  double (*matrix)[4];
  void operator () ( vtkIdType begin, vtkIdType end ) const
  {
    for (vtkIdType id=begin; id<end; id++)
    {
      double point[3];
      inPts->GetPoint( id, point );
      vtkSMPTransformPoint( matrix, point, point );
      outPts->SetPoint( id, point );
    }
  }
};

void vtkSMPTransform::TransformPoints(vtkPoints *inPts,
                                         vtkPoints *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  this->Update();

  TransformPointsFunctor functor;
  functor.inPts = inPts;
  functor.outPts = outPts;
  functor.matrix = this->Matrix->Element;

  vtkSMPTools::For( 0, n, functor );
}

//----------------------------------------------------------------------------
class TransformNormalsFunctor
{
public:
  vtkDataArray* inNms;
  vtkDataArray* outNms;
  double (*matrix)[4];
  void operator () ( vtkIdType begin, vtkIdType end ) const
  {
    for(vtkIdType id=begin; id<end; id++)
    {
      double norm[3];
      inNms->GetTuple( id, norm );
      vtkSMPTransformVector( matrix, norm, norm );
      vtkMath::Normalize( norm );
      outNms->SetTuple( id, norm );
    }
  }
};

void vtkSMPTransform::TransformNormals(vtkDataArray *inNms,
                                       vtkDataArray *outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  double matrix[4][4];

  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  TransformNormalsFunctor functor;
  functor.inNms = inNms;
  functor.outNms = outNms;
  functor.matrix = matrix;

  vtkSMPTools::For( 0, n, functor );
}

//----------------------------------------------------------------------------
class TransformVectorsFunctor
{
public:
  vtkDataArray* inVcs;
  vtkDataArray* outVcs;
  double (*matrix)[4];
  void operator () ( vtkIdType begin, vtkIdType end) const
  {
    for(vtkIdType id=begin; id<end; id++)
    {
      double vec[3];
      inVcs->GetTuple( id, vec );
      vtkSMPTransformVector( matrix, vec, vec );
      outVcs->SetTuple( id, vec );
    }
  }
};

void vtkSMPTransform::TransformVectors(vtkDataArray *inNms,
                                          vtkDataArray *outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  this->Update();

  TransformVectorsFunctor functor;
  functor.inVcs = inNms;
  functor.outVcs = outNms;
  functor.matrix = this->Matrix->Element;

  vtkSMPTools::For( 0, n, functor );
}
