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
#include <cassert>

vtkStandardNewMacro(vtkAttributesErrorMetric);

//-----------------------------------------------------------------------------
vtkAttributesErrorMetric::vtkAttributesErrorMetric()
{
  this->AttributeTolerance = 0.1; // arbitrary
  this->AbsoluteAttributeTolerance = 0.1; // arbitrary
  this->SquareAbsoluteAttributeTolerance=this->AbsoluteAttributeTolerance*this->AbsoluteAttributeTolerance;
  this->Range=0;
  this->DefinedByAbsolute=1;
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
  if(this->AbsoluteAttributeTolerance!=value || !this->DefinedByAbsolute)
    {
    this->AbsoluteAttributeTolerance=value;
    this->SquareAbsoluteAttributeTolerance=this->AbsoluteAttributeTolerance*this->AbsoluteAttributeTolerance;
    this->Range=0;
    this->DefinedByAbsolute=1;
    this->Modified();
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
  if(this->AttributeTolerance!=value || this->DefinedByAbsolute)
    {
    this->AttributeTolerance=value;
    this->DefinedByAbsolute=0;
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
    if(ac->GetActiveComponent()>=0)
      {
      int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ac->GetActiveComponent()+ATTRIBUTE_OFFSET;
      double tmp=leftPoint[i]+alpha*(rightPoint[i]-leftPoint[i])-midPoint[i];
      ae=tmp*tmp;
      }
    else // module of the vector
      {
      int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ATTRIBUTE_OFFSET;
      int j=0;
      int c=ac->GetNumberOfComponents();
      double tmp;
#if 0
      // If x and y are two vectors, we compute: ||x|-|y||
      double interpolatedValueMod=0;
      double midValueMod=0;
      while(j<c)
        {
        tmp=leftPoint[i+j]+alpha*(rightPoint[i+j]-leftPoint[i+j]);
        interpolatedValueMod+=tmp*tmp;
        tmp=midPoint[i+j];
        midValueMod+=tmp*tmp;
        ++j;
        }
      tmp=sqrt(midValueMod)-sqrt(interpolatedValueMod);
      ae=tmp*tmp;
#else
      // If x and y are two vectors, we compute: |x-y|
      // We should compute ||x|-|y|| but |x-y| is usually enough
      // and tends to produce less degenerated edges.
      // Remind that: ||x|-|y||<=|x-y|
      ae=0;
      while(j<c)
        {
        tmp=leftPoint[i+j]+alpha*(rightPoint[i+j]-leftPoint[i+j])-midPoint[i+j];
        ae+=tmp*tmp;
        ++j;
        }
#endif
      }
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
    if(ac->GetActiveComponent()>=0) // one component
      {
      int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ac->GetActiveComponent()+ATTRIBUTE_OFFSET;
      double tmp=leftPoint[i]+alpha*(rightPoint[i]-leftPoint[i])-midPoint[i];
      ae=tmp*tmp;
      }
    else // module of the vector
      {
      // If x and y are two vectors, we compute: |x-y|
      // We should compute ||x|-|y|| but |x-y| is usually enough
      // and tends to produce less degenerated edges.
      // Remind that: ||x|-|y||<=|x-y|
      int i=ac->GetAttributeIndex(ac->GetActiveAttribute())+ATTRIBUTE_OFFSET;
      int j=0;
      int c=ac->GetNumberOfComponents();
      double tmp;

      ae=0;
      while(j<c)
        {
        tmp=leftPoint[i+j]+alpha*(rightPoint[i+j]-leftPoint[i+j])-midPoint[i+j];
        ae+=tmp*tmp;
        ++j;
        }
      }
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
  if(!this->DefinedByAbsolute)
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
}
