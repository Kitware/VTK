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

vtkCxxRevisionMacro(vtkAttributesErrorMetric,"1.2");
vtkStandardNewMacro(vtkAttributesErrorMetric);

//-----------------------------------------------------------------------------
vtkAttributesErrorMetric::vtkAttributesErrorMetric()
{
  this->AttributeTolerance = 0.1; // arbitrary
}

//-----------------------------------------------------------------------------
vtkAttributesErrorMetric::~vtkAttributesErrorMetric()
{
}

//-----------------------------------------------------------------------------
// Description:
// Set the relative attribute accuracy to `value'. See
// GetAttributeTolerance() for details.
// \pre valid_range_value: value>0 && value<1
void vtkAttributesErrorMetric::SetAttributeTolerance(double value)
{
  assert("pre: valid_range_value" && value>0 && value<1);
  this->AttributeTolerance=value;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkAttributesErrorMetric::NeedEdgeSubdivision(double *leftPoint,
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
  
  this->ComputeAbsoluteAttributeTolerance();
  
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
  
  if(this->AbsoluteAttributeTolerance==0)
    {
    result=fabs(ae)>0.0001;
    }
  else
    {
    result=ae>this->AbsoluteAttributeTolerance;
    }
  return result;
}

//-----------------------------------------------------------------------------
void vtkAttributesErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AttributeTolerance: "  << this->AttributeTolerance << endl;
}

//-----------------------------------------------------------------------------
// Description:
// Compute the absolute attribute tolerance, only if the cached value is
// obsolete.
void vtkAttributesErrorMetric::ComputeAbsoluteAttributeTolerance()
{
  if ( this->GetMTime() > this->AbsoluteAttributeToleranceComputeTime )
    {
    vtkGenericAttributeCollection *ac=this->DataSet->GetAttributes();
    vtkGenericAttribute *a=ac->GetAttribute(ac->GetActiveAttribute());
    
    int i=ac->GetActiveComponent();
    
    double r[2];
    
    a->GetRange(i,r);
    
    double tmp=(r[1]-r[0])*this->AttributeTolerance;
    
    this->AbsoluteAttributeTolerance=tmp*tmp;
    this->AbsoluteAttributeToleranceComputeTime.Modified();
    }
}
