/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributesErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAttributesErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include <assert.h>

vtkCxxRevisionMacro(vtkAttributesErrorMetric,"1.8");
vtkStandardNewMacro(vtkAttributesErrorMetric);

//-----------------------------------------------------------------------------
vtkAttributesErrorMetric::vtkAttributesErrorMetric()
{
  this->AttributeTolerance = 0.1; // arbitrary
  this->AbsoluteAttributeTolerance = 0.1; // arbitrary
  
  this->Range=0;
  this->SquareAbsoluteAttributeTolerance=0;
}

//-----------------------------------------------------------------------------
vtkAttributesErrorMetric::~vtkAttributesErrorMetric()
{
}

//-----------------------------------------------------------------------------
// Description:
// Set the absolute attribute accuracy to `value'. See
// GetAbsoluteAttributeTolerance() for details.
// \pre valid_range_value: value>0
void vtkAttributesErrorMetric::SetAbsoluteAttributeTolerance(double value)
{
  assert("pre: valid_range_value" && value>0);
  if(this->AbsoluteAttributeTolerance!=value)
    {
    this->AbsoluteAttributeTolerance=value;
    this->Modified();
    this->SquareAbsoluteAttributeTolerance=this->AbsoluteAttributeTolerance*this->AbsoluteAttributeTolerance;
    this->SquareAbsoluteAttributeToleranceComputeTime.Modified();
    this->Range=0;
    }
}

//-----------------------------------------------------------------------------
// Description:
// Set the relative attribute accuracy to `value'. See
// GetAttributeTolerance() for details.
// \pre valid_range_value: value>0 && value<1
void vtkAttributesErrorMetric::SetAttributeTolerance(double value)
{
  assert("pre: valid_range_value" && value>0 && value<1);
  if(this->AttributeTolerance!=value)
    {
    this->AttributeTolerance=value;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkAttributesErrorMetric::RequiresEdgeSubdivision(double *leftPoint,
                                                      double *midPoint,
                                                      double *rightPoint,
                                                      double alpha)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  assert("pre: clamped_alpha" && alpha>0 && alpha<1);
  
  int result;
  double ae;
  vtkGenericAttributeCollection *ac;
  
  this->ComputeSquareAbsoluteAttributeTolerance();
  
  const int ATTRIBUTE_OFFSET=6;
  
  ac=this->DataSet->GetAttributes();
  vtkGenericAttribute *a=ac->GetAttribute(ac->GetActiveAttribute());
  
  if(this->GenericCell->IsAttributeLinear(a))
    {
    //don't need to do anything:
    ae=0;
    }
  else
    {
    int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ac->GetActiveComponent()+ATTRIBUTE_OFFSET;
    double tmp=leftPoint[i]+alpha*(rightPoint[i]-leftPoint[i])-midPoint[i];
    ae=tmp*tmp;
    assert("check: positive_ae" && ae>=0);
    }
  
  if(this->SquareAbsoluteAttributeTolerance==0)
    {
    result=fabs(ae)>0.0001;
    }
  else
    {
    result=ae>this->SquareAbsoluteAttributeTolerance;
    }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Return the error at the mid-point. The type of error depends on the state
// of the concrete error metric. For instance, it can return an absolute
// or relative error metric.
// See RequiresEdgeSubdivision() for a description of the arguments.
// \post positive_result: result>=0
double vtkAttributesErrorMetric::GetError(double *leftPoint,
                                          double *midPoint,
                                          double *rightPoint,
                                          double alpha)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  assert("pre: clamped_alpha" && alpha>0 && alpha<1);
  
  double ae;
  vtkGenericAttributeCollection *ac;
  
  this->ComputeSquareAbsoluteAttributeTolerance();
  
  const int ATTRIBUTE_OFFSET=6;
  
  ac=this->DataSet->GetAttributes();
  vtkGenericAttribute *a=ac->GetAttribute(ac->GetActiveAttribute());
  
 
  if(this->GenericCell->IsAttributeLinear(a))
    {
    //don't need to do anything:
    ae=0;
    }
  else
    {
    int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ac->GetActiveComponent()+ATTRIBUTE_OFFSET;
    double tmp=leftPoint[i]+alpha*(rightPoint[i]-leftPoint[i])-midPoint[i];
    ae=tmp*tmp;
    }
  
  double result;
  
  if(this->Range!=0)
    {
    result=sqrt(ae)/this->Range;
    }
  else
    {
    result=0;
    }
  
  assert("post: positive_result" && result>=0);
  
  return result;
}

//-----------------------------------------------------------------------------
void vtkAttributesErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AttributeTolerance: "  << this->AttributeTolerance << endl;
  os << indent << "AbsoluteAttributeTolerance: "  << this->AbsoluteAttributeTolerance << endl;
}

//-----------------------------------------------------------------------------
// Description:
// Compute the absolute attribute tolerance, only if the cached value is
// obsolete.
void vtkAttributesErrorMetric::ComputeSquareAbsoluteAttributeTolerance()
{
  if ( this->GetMTime() > this->SquareAbsoluteAttributeToleranceComputeTime )
    {
    vtkGenericAttributeCollection *ac=this->DataSet->GetAttributes();
    vtkGenericAttribute *a=ac->GetAttribute(ac->GetActiveAttribute());
    
    int i=ac->GetActiveComponent();
    
    double r[2];
    
    a->GetRange(i,r);
    
    double tmp=(r[1]-r[0])*this->AttributeTolerance;
    
    this->Range=r[1]-r[0];
    
    this->SquareAbsoluteAttributeTolerance=tmp*tmp;
    this->SquareAbsoluteAttributeToleranceComputeTime.Modified();
    this->AbsoluteAttributeTolerance=sqrt(this->SquareAbsoluteAttributeTolerance);
    }
}
