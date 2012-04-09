/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewDependentErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkViewDependentErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include <assert.h>
#include "vtkCoordinate.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkViewDependentErrorMetric);

//-----------------------------------------------------------------------------
vtkViewDependentErrorMetric::vtkViewDependentErrorMetric()
{
  this->PixelTolerance = 0.25; // arbitrary positive value
  this->Viewport = 0;
  this->Coordinate = vtkCoordinate::New();
  this->Coordinate->SetCoordinateSystemToWorld();
}

//-----------------------------------------------------------------------------
vtkViewDependentErrorMetric::~vtkViewDependentErrorMetric()
{
  this->Coordinate->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Set the squared screen-based geometric accuracy measured in pixels.
// Subdivision will be required if the square distance between the projection
// of the real point and the straight line passing through the projection
// of the vertices of the edge is greater than `value'.
// For instance, 0.25 will give better result than 1.
// \pre positive_value: value>0
void vtkViewDependentErrorMetric::SetPixelTolerance(double value)
{
  assert("pre: positive_value" && value>0);
  if(this->PixelTolerance!=value)
    {
    this->PixelTolerance=value;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
// Avoid reference loop
void vtkViewDependentErrorMetric::SetViewport(vtkViewport *viewport)
{
  if(this->Viewport!=viewport)
    {
    this->Viewport = viewport;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkViewDependentErrorMetric::RequiresEdgeSubdivision(double *leftPoint,
                                                         double *midPoint,
                                                         double *rightPoint,
                                                         double vtkNotUsed(alpha))
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
//  assert("pre: clamped_alpha" && alpha>0 && alpha<1); // or else true
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }
#if 0
  if( !this->RayFrustumIntersection(leftPoint,rightPoint,frustum) && !this->PointFrustumIntersection(midPoint,frustum))
    {
    // not in the frustum, don't need subdivision
    return 0;
    }
#endif

  // Get the projection of the left, mid and right points
  double leftProjPoint[2];
  double midProjPoint[2];
//  double rightProjPoint[2];

  this->Coordinate->SetValue(leftPoint);
  double *pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // pix is a volatile pointer
  leftProjPoint[0] = pix[0];
  leftProjPoint[1] = pix[1];

  this->Coordinate->SetValue(midPoint);
  pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // pix is a volatile pointer
  midProjPoint[0] = pix[0];
  midProjPoint[1] = pix[1];

  this->Coordinate->SetValue(rightPoint);
  pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // distance between the line (leftProjPoint,rightProjPoint) and the point midProjPoint.
  return this->Distance2LinePoint(leftProjPoint,pix,midProjPoint)>this->PixelTolerance;
}

//-----------------------------------------------------------------------------
// Description:
// Return the error at the mid-point. The type of error depends on the state
// of the concrete error metric. For instance, it can return an absolute
// or relative error metric.
// See RequiresEdgeSubdivision() for a description of the arguments.
// \pre leftPoint_exists: leftPoint!=0
// \pre midPoint_exists: midPoint!=0
// \pre rightPoint_exists: rightPoint!=0
// \pre clamped_alpha: alpha>0 && alpha<1
// \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
//          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
// \post positive_result: result>=0
double vtkViewDependentErrorMetric::GetError(double *leftPoint,
                                             double *midPoint,
                                             double *rightPoint,
                                             double vtkNotUsed(alpha))
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
//  assert("pre: clamped_alpha" && alpha>0 && alpha<1); // or else true
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }

  // Get the projection of the left, mid and right points
  double leftProjPoint[2];
  double midProjPoint[2];
//  double rightProjPoint[2];

  this->Coordinate->SetValue(leftPoint);
  double *pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // pix is a volatile pointer
  leftProjPoint[0] = pix[0];
  leftProjPoint[1] = pix[1];

  this->Coordinate->SetValue(midPoint);
  pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // pix is a volatile pointer
  midProjPoint[0] = pix[0];
  midProjPoint[1] = pix[1];

  this->Coordinate->SetValue(rightPoint);
  pix = this->Coordinate->GetComputedDoubleDisplayValue(this->Viewport);

  // distance between the line (leftProjPoint,rightProjPoint) and the point midProjPoint.
  return this->Distance2LinePoint(leftProjPoint,pix,midProjPoint);
}

//-----------------------------------------------------------------------------
// Description:
// Square distance between a straight line (defined by points x and y)
// and a point z. Property: if x and y are equal, the line is a point and
// the result is the square distance between points x and z.
double vtkViewDependentErrorMetric::Distance2LinePoint(double x[2],
                                                       double y[2],
                                                       double z[2])
{
  double u[2];
  double v[2];
  double w[2];

  u[0] = y[0] - x[0];
  u[1] = y[1] - x[1];

  vtkMath::Normalize2D(u);

  v[0] = z[0] - x[0];
  v[1] = z[1] - x[1];

  double dot = vtkMath::Dot2D(u,v);

  w[0] = v[0] - dot*u[0];
  w[1] = v[1] - dot*u[1];

  return vtkMath::Dot2D(w,w);
}

//-----------------------------------------------------------------------------
void vtkViewDependentErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PixelTolerance: "  << this->PixelTolerance << endl;
  os << indent << "ViewPort: ";
  if( this->Viewport )
    {
    this->Viewport->PrintSelf( os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
