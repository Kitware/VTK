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

vtkCxxRevisionMacro(vtkGenericSubdivisionErrorMetric,"1.2");
vtkStandardNewMacro(vtkGenericSubdivisionErrorMetric);

//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::vtkGenericSubdivisionErrorMetric()
{
  this->GeometricTolerance = 1; // arbitrary
  this->PixelTolerance = 0.25; // pixels do match
  this->AttributeTolerance = 0.1; // arbitrary
  
  this->AttributeCollection = NULL;
  this->GenericCell = NULL;
  this->Edge1Cache = this->Edge2Cache = NULL;
}

//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::~vtkGenericSubdivisionErrorMetric()
{
  if( this->Edge1Cache || this->Edge2Cache )
    {
    delete[] this->Edge1Cache;
    delete[] this->Edge2Cache;
    }
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
  
  if( this->Edge1Cache || this->Edge2Cache )
    {
    delete[] this->Edge1Cache;
    delete[] this->Edge2Cache;
    }
  
  if(this->AttributeCollection!=0)
    {
    int numComp = this->AttributeCollection->GetNumberOfComponents();
    this->Edge1Cache = new double[numComp+3];
    this->Edge2Cache = new double[numComp+3];
    }
  else
    {
    this->Edge1Cache = 0;
    this->Edge2Cache = 0;
    }
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
bool vtkGenericSubdivisionErrorMetric::EvaluateEdge( double *e1, double *e2 )
{
  bool result=0;
  this->ComputeCoordinates(e1,e2); 
  double ge = this->EvaluateGeometricError(e1, e2);
  result=ge>this->GeometricTolerance;
  
  if(!result)
    {
    
    double se = this->EvaluateScreenError(e1, e2);
    result=se>this->PixelTolerance;
    
    if(!result)
      {
      double ae = this->EvaluateAttributesError(e1, e2);
      this->ComputeAbsoluteAttributeTolerance();    
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
// Description:
// Compute world coordinates of the vertices `e1' and `e2' defining the edge.
// The result is in Edge1Cache and Edge2Cache. The middle of the straight line
// is InterpolatedCenterCache, the middle of the arc is RealCenterCache.
void vtkGenericSubdivisionErrorMetric::ComputeCoordinates(double *e1,
                                                          double *e2)
{ 
  if(!this->GenericCell->IsGeometryLinear())
    {
    this->GenericCell->EvaluateLocation(0, e1, this->Edge1Cache);
    this->GenericCell->EvaluateLocation(0, e2, this->Edge2Cache);
    
    double pcoord[3];
    
    int i=0;
    while(i<3)
      {
      this->InterpolatedCenterCache[i] = (this->Edge1Cache[i]
                                        + this->Edge2Cache[i])*0.5;
      
      // parametric center
      pcoord[i] = (e1[i] + e2[i]) *0.5;
      ++i;
      }
    
    //Now evalute real value at center point
    this->GenericCell->EvaluateLocation(0, pcoord, this->RealCenterCache);
    }
}

//-----------------------------------------------------------------------------
// EvaluateGeometricError
double vtkGenericSubdivisionErrorMetric::EvaluateGeometricError(double *vtkNotUsed(e1),
                                                                double *vtkNotUsed(e2))
{
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }
  
  return Distance2LinePoint(this->Edge1Cache,this->Edge2Cache,
                            this->RealCenterCache);
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
double vtkGenericSubdivisionErrorMetric::EvaluateAttributesError(double *e1,
                                                                 double *e2)
{
  // Since VTK is now using 1D texture mapping is this really usefull to refine
  // based on attributes ?

  vtkGenericAttribute *a=this->AttributeCollection->GetAttribute(
    this->AttributeCollection->GetActiveAttribute());
  
  if(this->GenericCell->IsAttributeLinear(a))
    {
    //don't need to do anything:
    return 0;
    }
  
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
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "GeometricTolerance: "  << this->GeometricTolerance << endl;
  os << indent << "PixelTolerance: "  << this->PixelTolerance << endl;
  os << indent << "AttributeTolerance: "  << this->AttributeTolerance << endl;
  
//  os << indent << "Error: "  << this->Error << endl;
  
//  os << indent << "AttributeCollection: "  << this->AttributeCollection << endl;
//  os << indent << "GenericCell: "  << this->GenericCell << endl;
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
