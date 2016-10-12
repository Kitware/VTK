/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransform2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransform2D.h"
#include "vtkPoints2D.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <cstdlib>

vtkStandardNewMacro(vtkTransform2D);

//----------------------------------------------------------------------------
vtkTransform2D::vtkTransform2D()
{
  this->Matrix = vtkMatrix3x3::New();
  this->InverseMatrix = vtkMatrix3x3::New();
}

//----------------------------------------------------------------------------
vtkTransform2D::~vtkTransform2D()
{
  if (this->Matrix)
  {
    this->Matrix->Delete();
    this->Matrix = NULL;
  }
  if (this->InverseMatrix)
  {
    this->InverseMatrix->Delete();
    this->InverseMatrix = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Matrix:" << endl;
  this->Matrix->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkTransform2D::Identity()
{
  this->Matrix->Identity();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTransform2D::Inverse()
{
  this->Matrix->Invert();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTransform2D::InternalDeepCopy(vtkTransform2D *transform)
{
  // copy the input
  this->Matrix->DeepCopy(transform->Matrix);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkTransform2D::GetMTime()
{
  return this->Matrix->GetMTime();
}

//----------------------------------------------------------------------------
void vtkTransform2D::Translate(double x, double y)
{
  if (x == 0.0 && y == 0.0)
  {
    return;
  }
  double matrix[3][3];
  vtkMatrix3x3::Identity(*matrix);
  matrix[0][2] = x;
  matrix[1][2] = y;
  this->Matrix->Multiply3x3(this->Matrix->GetData(), *matrix,
                            this->Matrix->GetData());
  this->Matrix->Modified();
}

//----------------------------------------------------------------------------
void vtkTransform2D::Rotate(double angle)
{
  if (angle == 0.0)
  {
    return;
  }

  // convert to radians
  angle = vtkMath::RadiansFromDegrees(angle);
  double c = cos(angle);
  double s = sin(angle);

  double matrix[3][3];
  vtkMatrix3x3::Identity(*matrix);
  matrix[0][0] = c;
  matrix[0][1] = s;
  matrix[1][0] = -s;
  matrix[1][1] = c;
  this->Matrix->Multiply3x3(this->Matrix->GetData(), *matrix,
                            this->Matrix->GetData());
  this->Matrix->Modified();
}

//----------------------------------------------------------------------------
void vtkTransform2D::Scale(double x, double y)
{
  if (x == 1.0 && y == 1.0)
  {
    return;
  }
  double matrix[3][3];
  vtkMatrix3x3::Identity(*matrix);
  matrix[0][0] = x;
  matrix[1][1] = y;
  this->Matrix->Multiply3x3(this->Matrix->GetData(), *matrix,
                            this->Matrix->GetData());
  this->Matrix->Modified();
}

//----------------------------------------------------------------------------
void vtkTransform2D::SetMatrix(const double elements[9])
{
  this->Matrix->DeepCopy(elements);
}

//----------------------------------------------------------------------------
void vtkTransform2D::GetMatrix(vtkMatrix3x3 *matrix)
{
  matrix->DeepCopy(this->Matrix);
}

//----------------------------------------------------------------------------
void vtkTransform2D::GetPosition(double position[2])
{
  position[0] = this->Matrix->GetElement(0, 2);
  position[1] = this->Matrix->GetElement(1, 2);
}

//----------------------------------------------------------------------------
void vtkTransform2D::GetScale(double scale[2])
{
  scale[0] = this->Matrix->GetElement(0, 0);
  scale[1] = this->Matrix->GetElement(1, 1);
}

//----------------------------------------------------------------------------
// Return the inverse of the current transformation matrix.
void vtkTransform2D::GetInverse(vtkMatrix3x3 *inverse)
{
  vtkMatrix3x3::Invert(this->GetMatrix(),inverse);
}

//----------------------------------------------------------------------------
// Obtain the transpose of the current transformation matrix.
void vtkTransform2D::GetTranspose(vtkMatrix3x3 *transpose)
{
  vtkMatrix3x3::Transpose(this->GetMatrix(),transpose);
}

//----------------------------------------------------------------------------
namespace { // Anonmymous namespace

template <class T1, class T2, class T3>
inline double vtkHomogeneousTransformPoint2D(T1 M[9],
                                             const T2 in[2], T3 out[2])
{
  double x = M[0]*in[0] + M[1]*in[1] + M[2];
  double y = M[3]*in[0] + M[4]*in[1] + M[5];
  double w = M[6]*in[0] + M[7]*in[1] + M[8];

  double f = 1.0/w;
  out[0] = static_cast<T3>(x*f);
  out[1] = static_cast<T3>(y*f);

  return f;
}

} // End anonymous namespace

//----------------------------------------------------------------------------
void vtkTransform2D::TransformPoints(const float *inPts, float *outPts, int n)
{
  double *M = this->Matrix->GetData();

  for (int i = 0; i < n; ++i)
  {
    vtkHomogeneousTransformPoint2D(M, &inPts[2*i], &outPts[2*i]);
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::TransformPoints(const double *inPts, double *outPts, int n)
{
  double *M = this->Matrix->GetData();

  for (int i = 0; i < n; ++i)
  {
    vtkHomogeneousTransformPoint2D(M, &inPts[2*i], &outPts[2*i]);
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::TransformPoints(vtkPoints2D *inPts, vtkPoints2D *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  outPts->SetNumberOfPoints(n);
  double *M = this->Matrix->GetData();
  double point[2];

  for (int i = 0; i < n; ++i)
  {
    inPts->GetPoint(i,point);
    vtkHomogeneousTransformPoint2D(M,point,point);
    outPts->SetPoint(i, point);
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::InverseTransformPoints(const float *inPts, float *outPts,
                                            int n)
{
  if (this->Matrix->GetMTime() > this->InverseMatrix->GetMTime())
  {
    this->Matrix->Invert(this->Matrix, this->InverseMatrix);
  }
  double *M = this->InverseMatrix->GetData();

  for (int i = 0; i < n; ++i)
  {
    vtkHomogeneousTransformPoint2D(M, &inPts[2*i], &outPts[2*i]);
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::InverseTransformPoints(const double *inPts, double *outPts,
                                            int n)
{
  if (this->Matrix->GetMTime() > this->InverseMatrix->GetMTime())
  {
    this->Matrix->Invert(this->Matrix, this->InverseMatrix);
  }
  double *M = this->InverseMatrix->GetData();

  for (int i = 0; i < n; ++i)
  {
    vtkHomogeneousTransformPoint2D(M, &inPts[2*i], &outPts[2*i]);
  }
}

//----------------------------------------------------------------------------
void vtkTransform2D::InverseTransformPoints(vtkPoints2D *inPts, vtkPoints2D *outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  outPts->SetNumberOfPoints(n);
  if (this->Matrix->GetMTime() > this->InverseMatrix->GetMTime())
  {
    this->Matrix->Invert(this->Matrix, this->InverseMatrix);
  }
  double *M = this->InverseMatrix->GetData();
  double point[2];

  for (int i = 0; i < n; ++i)
  {
    inPts->GetPoint(i,point);
    vtkHomogeneousTransformPoint2D(M,point,point);
    outPts->SetPoint(i, point);
  }
}
