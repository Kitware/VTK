/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHomogeneousTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHomogeneousTransform.h"

#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"


//----------------------------------------------------------------------------
vtkHomogeneousTransform::vtkHomogeneousTransform()
{
  this->Matrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkHomogeneousTransform::~vtkHomogeneousTransform()
{
  if (this->Matrix)
    {
    this->Matrix->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Matrix: (" << this->Matrix << ")\n";
  if (this->Matrix)
    {
    this->Matrix->PrintSelf(os, indent.GetNextIndent());
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline double vtkHomogeneousTransformPoint(T1 M[4][4],
                                           T2 in[3], T3 out[3])
{
  double x = M[0][0]*in[0] + M[0][1]*in[1] + M[0][2]*in[2] + M[0][3];
  double y = M[1][0]*in[0] + M[1][1]*in[1] + M[1][2]*in[2] + M[1][3];
  double z = M[2][0]*in[0] + M[2][1]*in[1] + M[2][2]*in[2] + M[2][3];
  double w = M[3][0]*in[0] + M[3][1]*in[1] + M[3][2]*in[2] + M[3][3];

  double f = 1.0/w;
  out[0] = static_cast<T3>(x*f);
  out[1] = static_cast<T3>(y*f);
  out[2] = static_cast<T3>(z*f);

  return f;
}

//------------------------------------------------------------------------
// computes a coordinate transformation and also returns the Jacobian matrix
template <class T1, class T2, class T3, class T4>
inline void vtkHomogeneousTransformDerivative(T1 M[4][4],
                                              T2 in[3], T3 out[3],
                                              T4 derivative[3][3])
{
  double f = vtkHomogeneousTransformPoint(M,in,out);

  for (int i = 0; i < 3; i++)
    { 
    derivative[0][i] = static_cast<T4>((M[0][i] - M[3][i]*out[0])*f);
    derivative[1][i] = static_cast<T4>((M[1][i] - M[3][i]*out[1])*f);
    derivative[2][i] = static_cast<T4>((M[2][i] - M[3][i]*out[2])*f);
    }
}

//------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformPoint(const float in[3], 
                                                     float out[3])
{
  vtkHomogeneousTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformPoint(const double in[3], 
                                                     double out[3])
{
  vtkHomogeneousTransformPoint(this->Matrix->Element,in,out);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformDerivative(const float in[3], 
                                                    float out[3],
                                                    float derivative[3][3])
{
  vtkHomogeneousTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformDerivative(const double in[3], 
                                                    double out[3],
                                                    double derivative[3][3])
{
  vtkHomogeneousTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::TransformPoints(vtkPoints *inPts, 
                                              vtkPoints *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  double (*M)[4] = this->Matrix->Element;
  double point[3];  

  this->Update();

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkHomogeneousTransformPoint(M,point,point);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 
void vtkHomogeneousTransform::TransformPointsNormalsVectors(vtkPoints *inPts, 
                                                            vtkPoints *outPts,
                                                            vtkDataArray *inNms, 
                                                            vtkDataArray *outNms,
                                                            vtkDataArray *inVrs, 
                                                            vtkDataArray *outVrs)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  double (*M)[4] = this->Matrix->Element;
  double L[4][4];
  double inPnt[3],outPnt[3],inNrm[3],outNrm[3],inVec[3],outVec[3];
  double w;
  
  this->Update();

  if (inNms)
    { // need inverse of the matrix to calculate normals
    vtkMatrix4x4::DeepCopy(*L,this->Matrix);  
    vtkMatrix4x4::Invert(*L,*L);
    vtkMatrix4x4::Transpose(*L,*L);
    }

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,inPnt);

    // do the coordinate transformation, get 1/w
    double f = vtkHomogeneousTransformPoint(M,inPnt,outPnt);
    outPts->InsertNextPoint(outPnt);

    if (inVrs)
      { 
      inVrs->GetTuple(i,inVec);

      // do the linear homogeneous transformation
      outVec[0] = M[0][0]*inVec[0] + M[0][1]*inVec[1] + M[0][2]*inVec[2];
      outVec[1] = M[1][0]*inVec[0] + M[1][1]*inVec[1] + M[1][2]*inVec[2];
      outVec[2] = M[2][0]*inVec[0] + M[2][1]*inVec[1] + M[2][2]*inVec[2];
      w =         M[3][0]*inVec[0] + M[3][1]*inVec[1] + M[3][2]*inVec[2];

      // apply homogeneous correction: note that the f we are using
      // is the one we calculated in the point transformation
      outVec[0] = (outVec[0]-w*outPnt[0])*f;
      outVec[1] = (outVec[1]-w*outPnt[1])*f;
      outVec[2] = (outVec[2]-w*outPnt[2])*f;

      outVrs->InsertNextTuple(outVec);
      }

    if (inNms)
      { 
      inNms->GetTuple(i,inNrm);

      // calculate the w component of the normal
      w = -(inNrm[0]*inPnt[0] + inNrm[1]*inPnt[1] + inNrm[2]*inPnt[2]);

      // perform the transformation in homogeneous coordinates
      outNrm[0] = L[0][0]*inNrm[0]+L[0][1]*inNrm[1]+L[0][2]*inNrm[2]+L[0][3]*w;
      outNrm[1] = L[1][0]*inNrm[0]+L[1][1]*inNrm[1]+L[1][2]*inNrm[2]+L[1][3]*w;
      outNrm[2] = L[2][0]*inNrm[0]+L[2][1]*inNrm[1]+L[2][2]*inNrm[2]+L[2][3]*w;

      // re-normalize
      vtkMath::Normalize(outNrm);
      outNms->InsertNextTuple(outNrm);
      }
    }
}

//----------------------------------------------------------------------------
// update and copy out the current matrix
void vtkHomogeneousTransform::GetMatrix(vtkMatrix4x4 *m)
{
  this->Update(); 
  m->DeepCopy(this->Matrix); 
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkHomogeneousTransform *t =
    static_cast<vtkHomogeneousTransform *>(transform);

  this->Matrix->DeepCopy(t->Matrix);
}

