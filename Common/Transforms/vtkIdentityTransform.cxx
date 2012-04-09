/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentityTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdentityTransform.h"

#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkIdentityTransform);

//----------------------------------------------------------------------------
vtkIdentityTransform::vtkIdentityTransform()
{
}

//----------------------------------------------------------------------------
vtkIdentityTransform::~vtkIdentityTransform()
{
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalDeepCopy(vtkAbstractTransform *)
{
  // nothin' to do
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkIdentityTransform::MakeTransform()
{
  return vtkIdentityTransform::New();
}

//------------------------------------------------------------------------
template<class T2, class T3>
void vtkIdentityTransformPoint(T2 in[3], T3 out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

//------------------------------------------------------------------------
template<class T2, class T3, class T4>
void vtkIdentityTransformDerivative(T2 in[3], T3 out[3],
                                    T4 derivative[3][3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];

  vtkMath::Identity3x3(derivative);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const float in[3],
                                                  float out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const double in[3],
                                                  double out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const float in[3],
                                                   float out[3])
{
  vtkIdentityTransformPoint(in,out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const double in[3],
                                                   double out[3])
{
  vtkIdentityTransformPoint(in,out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const float in[3],
                                                   float out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const double in[3],
                                                   double out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(const float in[3],
                                                       float out[3],
                                                       float derivative[3][3])
{
  vtkIdentityTransformDerivative(in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(const double in[3],
                                                       double out[3],
                                                       double derivative[3][3])
{
  vtkIdentityTransformDerivative(in,out,derivative);
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
void vtkIdentityTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
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
void vtkIdentityTransform::TransformPoints(vtkPoints *inPts,
                                           vtkPoints *outPts)
{
  int n = inPts->GetNumberOfPoints();
  double point[3];

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);
    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::TransformNormals(vtkDataArray *inNms,
                                            vtkDataArray *outNms)
{
  int n = inNms->GetNumberOfTuples();
  double normal[3];

  for (int i = 0; i < n; i++)
    {
    inNms->GetTuple(i,normal);
    outNms->InsertNextTuple(normal);
    }
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::TransformVectors(vtkDataArray *inNms,
                                            vtkDataArray *outNms)
{
  int n = inNms->GetNumberOfTuples();
  double vect[3];

  for (int i = 0; i < n; i++)
    {
    inNms->GetTuple(i,vect);
    outNms->InsertNextTuple(vect);
    }
}



