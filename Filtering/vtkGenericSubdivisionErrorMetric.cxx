/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericSubdivisionErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericSubdivisionErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include <assert.h>

vtkCxxRevisionMacro(vtkGenericSubdivisionErrorMetric,"1.3");
vtkStandardNewMacro(vtkGenericSubdivisionErrorMetric);

//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::vtkGenericSubdivisionErrorMetric()
{
  this->GeometricTolerance = 1; // arbitrary
  this->PixelTolerance = 0.25; // pixels do match
  this->AttributeTolerance = 0.1; // arbitrary
  
  this->AttributeCollection = NULL;
  this->GenericCell = NULL;
  this->ActiveIndex=0;
}

//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::~vtkGenericSubdivisionErrorMetric()
{
}

//-----------------------------------------------------------------------------
// Description :
// Set the geometric accuracy with an absolute value.
// Subdivision will be required if the square distance is greater than
// value. For instance 0.01 will give better result than 0.1.
// \pre positive_value: value>0
void vtkGenericSubdivisionErrorMetric::SetAbsoluteGeometricTolerance(double value)
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
void vtkGenericSubdivisionErrorMetric::SetRelativeGeometricTolerance(double value,
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
// Description:
// Set the pixel accuracy to `value'. See GetPixelTolerance() for details.
// \pre valid_value: value>=0.25
void vtkGenericSubdivisionErrorMetric::SetPixelTolerance(double value)
{
  assert("pre: valid_value" && value>=0.25);
  
  this->PixelTolerance=value;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Description:
// Set the relative attribute accuracy to `value'. See
// GetAttributeTolerance() for details.
// \pre valid_range_value: value>0 && value<1
void vtkGenericSubdivisionErrorMetric::SetAttributeTolerance(double value)
{
  assert("pre: valid_range_value" && value>0 && value<1);
  this->AttributeTolerance=value;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::SetAttributeCollection(vtkGenericAttributeCollection* a)
{
  this->AttributeCollection=a;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::SetGenericCell(vtkGenericAdaptorCell *c)
{
  this->GenericCell=c;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Evaluate different type of error metric and return whether or not the edge
// needs to be split
bool vtkGenericSubdivisionErrorMetric::EvaluateEdge(double *leftPoint,
                                                    double *midPoint,
                                                    double *rightPoint)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  
  bool result=0;
  double ge = this->EvaluateGeometricError(leftPoint,midPoint,rightPoint);
  result=ge>this->GeometricTolerance;
  
  if(!result)
    {
    
    double se = this->EvaluateScreenError(leftPoint,midPoint,rightPoint);
    result=se>this->PixelTolerance;
    
    if(!result)
      {
      this->ComputeAbsoluteAttributeTolerance();
      double ae = this->EvaluateAttributesError(leftPoint,midPoint,rightPoint);
      if(this->AbsoluteAttributeTolerance==0)
        {
        result=fabs(ae)>0.0001;
        }
      else
        {
        result=ae>this->AbsoluteAttributeTolerance;
        }
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
// EvaluateGeometricError
double vtkGenericSubdivisionErrorMetric::EvaluateGeometricError(
  double *leftPoint,
  double *midPoint,
  double *rightPoint)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }
  return this->Distance2LinePoint(leftPoint,rightPoint,midPoint);
}

//-----------------------------------------------------------------------------
// Description:
// Square distance between a straight line (defined by points x and y)
// and a point z. Property: if x and y are equal, the line is a point and
// the result is the square distance between points x and z.
double vtkGenericSubdivisionErrorMetric::Distance2LinePoint(double x[3],
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
// EvaluateAttributesError
double vtkGenericSubdivisionErrorMetric::EvaluateAttributesError(
  double *leftPoint,
  double *midPoint,
  double *rightPoint)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  
  const int ATTRIBUTE_OFFSET=6;
  
  vtkGenericAttribute *a=this->AttributeCollection->GetAttribute(
    this->AttributeCollection->GetActiveAttribute());
  
  if(this->GenericCell->IsAttributeLinear(a))
    {
    //don't need to do anything:
    return 0;
    }

#if 0
  
  // Evaluate the field data at point a and b:
  double *w1 = this->Edge1Cache + 3;
  double *w2 = this->Edge2Cache + 3;

  this->GenericCell->InterpolateTuple( a, e1, w1);
  this->GenericCell->InterpolateTuple( a, e2, w2);
  
  int i=this->AttributeCollection->GetActiveComponent();
  
  double attributeAtPoint[10]; // FB: TODO
  
  double midPoint[3];
    
  int j=0;
    while(j<3)
      {
      // parametric center
      midPoint[j] = (e1[j] + e2[j]) *0.5;
      ++j;
      }
  
  
  this->GenericCell->InterpolateTuple(a,midPoint,attributeAtPoint);
  
  double tmp=(w1[i]+w2[i])*0.5-attributeAtPoint[i];

  return tmp*tmp;
#else
  int i=this->AttributeCollection->GetAttributeIndex(this->AttributeCollection->GetActiveAttribute())+this->AttributeCollection->GetActiveComponent()+ATTRIBUTE_OFFSET;
  double tmp=(leftPoint[i]+rightPoint[i])*0.5-midPoint[i];
  return tmp*tmp;
#endif
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "GeometricTolerance: "  << this->GeometricTolerance << endl;
  os << indent << "PixelTolerance: "  << this->PixelTolerance << endl;
  os << indent << "AttributeTolerance: "  << this->AttributeTolerance << endl;
  os << indent << "AttributeCollection: "  << this->AttributeCollection << endl;
  os << indent << "GenericCell: "  << this->GenericCell << endl;
}

//-----------------------------------------------------------------------------
// Description:
// Compute the absolute attribute tolerance, only if the cached value is
// obsolete.
void vtkGenericSubdivisionErrorMetric::ComputeAbsoluteAttributeTolerance()
{
  if ( this->GetMTime() > this->AbsoluteAttributeToleranceComputeTime )
    {
    vtkGenericAttribute *a=this->AttributeCollection->GetAttribute(
      this->AttributeCollection->GetActiveAttribute());
    
    int i=this->AttributeCollection->GetActiveComponent();
    
    double r[2];
    
    a->GetRange(i,r);
    
    double tmp=(r[1]-r[0])*this->AttributeTolerance;
    
    this->AbsoluteAttributeTolerance=tmp*tmp;
    this->AbsoluteAttributeToleranceComputeTime.Modified();
    }
}
