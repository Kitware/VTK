/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlane.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkDataArrayAccessor.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"

#include <algorithm>

vtkStandardNewMacro(vtkPlane);

//-----------------------------------------------------------------------------
// Construct plane passing through origin and normal to z-axis.
vtkPlane::vtkPlane()
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

//-----------------------------------------------------------------------------
double vtkPlane::DistanceToPlane(double x[3])
{
  return this->DistanceToPlane(x, this->GetNormal(), this->GetOrigin());
}

//-----------------------------------------------------------------------------
void vtkPlane::ProjectPoint(double x[3], double origin[3],
                            double normal[3], double xproj[3])
{
  double t, xo[3];

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);

  xproj[0] = x[0] - t * normal[0];
  xproj[1] = x[1] - t * normal[1];
  xproj[2] = x[2] - t * normal[2];
}

//-----------------------------------------------------------------------------
void vtkPlane::ProjectPoint(double x[3], double xproj[3])
{
  this->ProjectPoint(x, this->GetOrigin(), this->GetNormal(), xproj);
}

//-----------------------------------------------------------------------------
void vtkPlane::ProjectVector(
  double v[3], double vtkNotUsed(origin)[3], double normal[3],
  double vproj[3])
{
  double t = vtkMath::Dot(v, normal);
  double n2 = vtkMath::Dot(normal, normal);
  if (n2 == 0)
  {
    n2 = 1.0;
  }
  vproj[0] = v[0] - t * normal[0] / n2;
  vproj[1] = v[1] - t * normal[1] / n2;
  vproj[2] = v[2] - t * normal[2] / n2;
}

//-----------------------------------------------------------------------------
void vtkPlane::ProjectVector(double v[3], double vproj[3])
{
  this->ProjectVector(v, this->GetOrigin(), this->GetNormal(), vproj);
}


//-----------------------------------------------------------------------------
void vtkPlane::Push(double distance)
{
  int i;

  if ( distance == 0.0 )
  {
    return;
  }
  for (i=0; i < 3; i++ )
  {
    this->Origin[i] += distance * this->Normal[i];
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
// Project a point x onto plane defined by origin and normal. The
// projected point is returned in xproj. NOTE : normal NOT required to
// have magnitude 1.
void vtkPlane::GeneralizedProjectPoint(double x[3], double origin[3],
                                       double normal[3], double xproj[3])
{
  double t, xo[3], n2;

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);
  n2 = vtkMath::Dot(normal, normal);

  if (n2 != 0)
  {
    xproj[0] = x[0] - t * normal[0]/n2;
    xproj[1] = x[1] - t * normal[1]/n2;
    xproj[2] = x[2] - t * normal[2]/n2;
  }
  else
  {
    xproj[0] = x[0];
    xproj[1] = x[1];
    xproj[2] = x[2];
  }
}

//-----------------------------------------------------------------------------
void vtkPlane::GeneralizedProjectPoint(double x[3], double xproj[3])
{
  this->GeneralizedProjectPoint(x, this->GetOrigin(), this->GetNormal(), xproj);
}

//-----------------------------------------------------------------------------
// Evaluate plane equation for point x[3].
double vtkPlane::EvaluateFunction(double x[3])
{
  return ( this->Normal[0]*(x[0]-this->Origin[0]) +
           this->Normal[1]*(x[1]-this->Origin[1]) +
           this->Normal[2]*(x[2]-this->Origin[2]) );
}

//-----------------------------------------------------------------------------
// Evaluate function gradient at point x[3].
void vtkPlane::EvaluateGradient(double vtkNotUsed(x)[3], double n[3])
{
  for (int i=0; i<3; i++)
  {
    n[i] = this->Normal[i];
  }
}

#define VTK_PLANE_TOL 1.0e-06

//-----------------------------------------------------------------------------
// Given a line defined by the two points p1,p2; and a plane defined by the
// normal n and point p0, compute an intersection. The parametric
// coordinate along the line is returned in t, and the coordinates of
// intersection are returned in x. A zero is returned if the plane and line
// do not intersect between (0<=t<=1). If the plane and line are parallel,
// zero is returned and t is set to VTK_LARGE_DOUBLE.
int vtkPlane::IntersectWithLine(double p1[3], double p2[3], double n[3],
                               double p0[3], double& t, double x[3])
{
  double num, den, p21[3];
  double fabsden, fabstolerance;

  // Compute line vector
  //
  p21[0] = p2[0] - p1[0];
  p21[1] = p2[1] - p1[1];
  p21[2] = p2[2] - p1[2];

  // Compute denominator.  If ~0, line and plane are parallel.
  //
  num = vtkMath::Dot(n,p0) - ( n[0]*p1[0] + n[1]*p1[1] + n[2]*p1[2] ) ;
  den = n[0]*p21[0] + n[1]*p21[1] + n[2]*p21[2];
  //
  // If denominator with respect to numerator is "zero", then the line and
  // plane are considered parallel.
  //

  // trying to avoid an expensive call to fabs()
  if (den < 0.0)
  {
    fabsden = -den;
  }
  else
  {
    fabsden = den;
  }
  if (num < 0.0)
  {
    fabstolerance = -num*VTK_PLANE_TOL;
  }
  else
  {
    fabstolerance = num*VTK_PLANE_TOL;
  }
  if ( fabsden <= fabstolerance )
  {
    t = VTK_DOUBLE_MAX;
    return 0;
  }

  // valid intersection
  t = num / den;

  x[0] = p1[0] + t*p21[0];
  x[1] = p1[1] + t*p21[1];
  x[2] = p1[2] + t*p21[2];

  if ( t >= 0.0 && t <= 1.0 )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Accelerate plane cutting operation
namespace {
template <typename InputArrayType, typename OutputArrayType> struct CutWorker
{
  typedef typename vtkDataArrayAccessor<InputArrayType>::APIType InputValueType;
  typedef typename vtkDataArrayAccessor<InputArrayType>::APIType OutputValueType;
  OutputValueType Normal[3];
  OutputValueType Origin[3];
  vtkDataArrayAccessor<InputArrayType> src;
  vtkDataArrayAccessor<OutputArrayType> dest;

  CutWorker(InputArrayType* in, OutputArrayType* out) : src(in), dest(out) {}
  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (vtkIdType tIdx = begin; tIdx < end; ++tIdx)
    {
      OutputValueType x[3];
      x[0] = static_cast<OutputValueType>(src.Get(tIdx, 0));
      x[1] = static_cast<OutputValueType>(src.Get(tIdx, 1));
      x[2] = static_cast<OutputValueType>(src.Get(tIdx, 2));
      OutputValueType out =
          Normal[0] * (x[0] - Origin[0]) +
          Normal[1] * (x[1] - Origin[1]) +
          Normal[2] * (x[2] - Origin[2]);
      dest.Set(tIdx, 0, out);
    }
  }
};

struct CutFunctionWorker
{
  double Normal[3];
  double Origin[3];
  CutFunctionWorker(double n[3], double o[3])
  {
    std::copy_n(n, 3, this->Normal);
    std::copy_n(o, 3, this->Origin);
  }
  template <typename InputArrayType, typename OutputArrayType>
  void operator()(InputArrayType* input, OutputArrayType* output)
  {
    VTK_ASSUME(input->GetNumberOfComponents() == 3);
    VTK_ASSUME(output->GetNumberOfComponents() == 1);
    vtkIdType numTuples = input->GetNumberOfTuples();
    CutWorker<InputArrayType, OutputArrayType> cut(input, output);
    std::copy_n(Normal, 3, cut.Normal);
    std::copy_n(Origin, 3, cut.Origin);
    vtkSMPTools::For(0, numTuples, cut);
  }
};
} // end anon namespace

void vtkPlane::EvaluateFunction(vtkDataArray* input, vtkDataArray* output)
{
  CutFunctionWorker worker(this->Normal, this->Origin);
  typedef vtkTypeList_Create_2(float, double) InputTypes;
  typedef vtkTypeList_Create_2(float, double) OutputTypes;
  typedef vtkArrayDispatch::Dispatch2ByValueType<InputTypes, OutputTypes> MyDispatch;
  if (!MyDispatch::Execute(input, output, worker))
  {
    worker(input, output); // Use vtkDataArray API if dispatch fails.
  }
}

//-----------------------------------------------------------------------------
int vtkPlane::IntersectWithLine(double p1[3], double p2[3], double& t, double x[3])
{
  return this->IntersectWithLine(p1, p2, this->GetNormal(), this->GetOrigin(), t, x);
}

//-----------------------------------------------------------------------------
void vtkPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Normal: (" << this->Normal[0] << ", "
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}
