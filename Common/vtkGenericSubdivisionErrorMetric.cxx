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

vtkCxxRevisionMacro(vtkGenericSubdivisionErrorMetric,"1.3");
vtkStandardNewMacro(vtkGenericSubdivisionErrorMetric);

vtkCxxSetObjectMacro(vtkGenericSubdivisionErrorMetric, GenericCell,vtkGenericAdaptorCell);

//------------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::vtkGenericSubdivisionErrorMetric()
{
  this->Error = 1.0;  //arbitrary
  this->AttributeCollection = NULL;
  this->GenericCell = NULL;
  this->Edge1Cache = this->Edge2Cache = NULL;

}

//------------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::~vtkGenericSubdivisionErrorMetric()
{   
  if( this->Edge1Cache || this->Edge2Cache )
    {
    delete[] this->Edge1Cache;
    delete[] this->Edge2Cache;
    }
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::SetAttributeCollection(vtkGenericAttributeCollection* a)
{
  vtkSetObjectBodyMacro(AttributeCollection, vtkGenericAttributeCollection, a);

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
}


//-----------------------------------------------------------------------------
// Evaluate different type of error metric and return whether or not the edge
// needs to be split
bool vtkGenericSubdivisionErrorMetric::EvaluateEdge( double *e1, double *e2 )
{
  // We need to find a combination of the Screen Error (se), the Geometric Error
  // (ge) and the Attribute Error (ae).
  
  double ge = this->EvaluateGeometricError(e1, e2);
  double ae = this->EvaluateAttributesError(e1, e2);
  
  // This strategy gives good result but way too subdivided (Pixel Error is easy
  // to get) :
  //return ( se + ge + ae ) > this->Error ;
  
  //Here we have to decide if we really need to subdivide:
  if( ( ge + ae ) > this->Error )
    {
    // Ok the geometric or attribute error is too big to be neglected
    // Let see if by chance we are close enough (in Pixel Space), thus
    // We might not need to subdivide

    // Be carefull the order is important since EvaluateScreenError might use
    // results from EvaluateGeometricError.
    double se = this->EvaluateScreenError(e1, e2);
    if( se > this->Error )
      {
      // Ok we really need to subdivide:
      return 1;
      }
    }

  // all other cases, we don't split edge:
  return 0;
}

//-----------------------------------------------------------------------------
// EvaluateGeometricError
double vtkGenericSubdivisionErrorMetric::EvaluateGeometricError(double *e1, double *e2)
{
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }

  // The following code has been commented out since it is way too basic, and
  // error prone
#if 0
  // Evaluate the length of edge: (e1, e2):
  double x,y,z;
  int numComp = this->AttributeCollection->GetNumberOfComponents();

  x = e1output[0] - e2output[0];
  y = e1output[1] - e2output[1];
  z = e1output[2] - e2output[2];

  return x*x + y*y + z*z;
#endif

  // Those calls will evaluate workd's coordinate of edge e1 and e2.
  // And the value in Edge1Cache/ Edge1Cache can be then re-use.

  this->GenericCell->EvaluateLocation(0, e1, this->Edge1Cache);
  this->GenericCell->EvaluateLocation(0, e2, this->Edge2Cache);

  // Here we calculate difference between f(m) and value on the linear
  // approximation (m being center of edge (e1, e2) )
  
  double center[3];
  // Be carefull we re-use some precomputed code
  center[0] = (this->Edge1Cache[0] + this->Edge2Cache[0])/ 2;
  center[1] = (this->Edge1Cache[1] + this->Edge2Cache[1])/ 2;
  center[2] = (this->Edge1Cache[2] + this->Edge2Cache[2])/ 2;
  
  //Now evalute real value at center point
  double pcoord[3];
  pcoord[0] = (e1[0] + e2[0]) / 2;
  pcoord[1] = (e1[1] + e2[1]) / 2;
  pcoord[2] = (e1[2] + e2[2]) / 2;
  double real_center[3];
  
  this->GenericCell->EvaluateLocation(0, pcoord, real_center);

  double sum = 0;
  for(int i=0; i<3; i++)
    {
    sum += (center[i] - real_center[i]) * (center[i] - real_center[i]);
    }
  
  return sum;
}

//-----------------------------------------------------------------------------
// EvaluateAttributesError
double vtkGenericSubdivisionErrorMetric::EvaluateAttributesError(double *e1, double *e2)
{
  // Since VTK is now using 1D texture mapping is this really usefull to refine
  // based on attributes ?

  if( this->GenericCell->IsAttributeLinear(
        this->AttributeCollection->GetAttribute(
          this->AttributeCollection->GetActiveAttribute())))
    {
    //don't need to do anything:
    return 0;
    }

  // Evaluate the field data at point a and b:
  double *w1 = this->Edge1Cache + 3;
  double *w2 = this->Edge2Cache + 3;

  this->GenericCell->InterpolateTuple( this->AttributeCollection, e1, w1);
  this->GenericCell->InterpolateTuple( this->AttributeCollection, e2, w2);

  int numComp = this->AttributeCollection->GetNumberOfComponents();
  double w = 0;
  for(int i=0; i<numComp; i++)
    {
    w += (w1[i] - w2[i]) * (w1[i] - w2[i]);
    }

  return w;
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Error: "  << this->Error << endl;
  
  os << indent << "AttributeCollection: "  << this->AttributeCollection << endl;
  os << indent << "GenericCell: "  << this->GenericCell << endl;
}

