/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometricErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include <assert.h>

vtkCxxRevisionMacro(vtkGeometricErrorMetric,"1.2");
vtkStandardNewMacro(vtkGeometricErrorMetric);

//-----------------------------------------------------------------------------
vtkGeometricErrorMetric::vtkGeometricErrorMetric()
{
  this->GeometricTolerance = 1; // arbitrary positive value
}

//-----------------------------------------------------------------------------
vtkGeometricErrorMetric::~vtkGeometricErrorMetric()
{
}

//-----------------------------------------------------------------------------
// Description :
// Set the geometric accuracy with an absolute value.
// Subdivision will be required if the square distance is greater than
// value. For instance 0.01 will give better result than 0.1.
// \pre positive_value: value>0
void vtkGeometricErrorMetric::SetAbsoluteGeometricTolerance(double value)
{
  assert("pre: positive_value" && value>0);
  this->GeometricTolerance=value;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Description :
// Set the geometric accuracy with a value relative to the bounding box of
// the dataset. Internally compute the absolute tolerance.
// For instance 0.01 will give better result than 0.1.
// \pre valid_range_value: value>0 && value<1
// \pre ds_exists: ds!=0
void vtkGeometricErrorMetric::SetRelativeGeometricTolerance(double value,
                                                            vtkGenericDataSet *ds)
{
  assert("pre: valid_range_value" && value>0 && value<1);
  assert("pre: ds_exists" && ds!=0);

  double bounds[6];
  ds->GetBounds(bounds);
  double smallest;
  double length;
  smallest=bounds[1]-bounds[0];
  length=bounds[3]-bounds[2];
  if(length<smallest || smallest==0)
    {
    smallest=length;
    }
  length=bounds[5]-bounds[4];
  if(length<smallest|| smallest==0)
    {
    smallest=length;
    }
  length=ds->GetLength();
  if(length<smallest|| smallest==0)
    {
    smallest=length;
    }
  if(smallest==0)
    {
    smallest=1;
    }
  double tmp=value*smallest;

  this->GeometricTolerance=tmp*tmp;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkGeometricErrorMetric::NeedEdgeSubdivision(double *leftPoint,
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
  // distance between the line (leftPoint,rightPoint) and the point midPoint.
  return this->Distance2LinePoint(leftPoint,rightPoint,midPoint)>this->GeometricTolerance;
}

//-----------------------------------------------------------------------------
// Description:
// Square distance between a straight line (defined by points x and y)
// and a point z. Property: if x and y are equal, the line is a point and
// the result is the square distance between points x and z.
double vtkGeometricErrorMetric::Distance2LinePoint(double x[3],
                                                   double y[3],
                                                   double z[3])
{
  double u[3];
  double v[3];
  double w[3];
  double dot;
  
  u[0]=y[0]-x[0];
  u[1]=y[1]-x[1];
  u[2]=y[2]-x[2];
  
  vtkMath::Normalize(u);
  
  v[0]=z[0]-x[0];
  v[1]=z[1]-x[1];
  v[2]=z[2]-x[2];
  
  dot=vtkMath::Dot(u,v);
  
  w[0]=v[0]-dot*u[0];
  w[1]=v[1]-dot*u[1];
  w[2]=v[2]-dot*u[2];
  
  return vtkMath::Dot(w,w);
}

//-----------------------------------------------------------------------------
void vtkGeometricErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "GeometricTolerance: "  << this->GeometricTolerance << endl;
}
