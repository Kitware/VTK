/*=========================================================================

  Program:   Visualization Library
  Module:    SampleF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "SampleF.hh"
#include "FScalars.hh"

vlSampleFunction::vlSampleFunction()
{
  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 1;
  this->CapValue = LARGE_FLOAT;

  this->ImplicitFunction = NULL;
}

void vlSampleFunction::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSampleFunction::GetClassName()))
    {
    vlDataSetToStructuredPointsFilter::PrintSelf(os,indent);

    os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
                 << this->SampleDimensions[1] << ", "
                 << this->SampleDimensions[2] << ")\n";
    os << indent << "ModelBounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";
    }
}

void vlSampleFunction::SetModelBounds(float *bounds)
{
  vlSampleFunction::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vlSampleFunction::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    float length;

    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;

    this->Origin[0] = xmin;
    this->Origin[1] = ymin;
    this->Origin[2] = zmin;

    if ( (length = xmax - xmin) == 0.0 ) length = 1.0;
    this->AspectRatio[0] = 1.0;
    this->AspectRatio[1] = (ymax - ymin) / length;
    this->AspectRatio[2] = (zmax - zmin) / length;
    }
}

void vlSampleFunction::Execute()
{
  int ptId;
  vlFloatScalars *newScalars;
  int numPts;
  float p[3], s;

  vlDebugMacro(<< "Sampling implicit function");
//
// Initialize self; create output objects
//
  this->Initialize();

  if ( !this->ImplicitFunction )
    {
    vlErrorMacro(<<"No implicit function specified");
    return;
    }

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  newScalars = new vlFloatScalars(numPts);

  this->SetDimensions(this->GetSampleDimensions());
//
// Traverse all points evaluating implicit function at each point
//
  for (ptId=0; ptId < numPts; ptId++ )
    {
    s = this->ImplicitFunction->Evaluate(p[0], p[1], p[2]);
    newScalars->SetScalar(ptId,s);
    }
//
// If capping is turned on, set the distances of the outside of the volume
// to the CapValue.
//
  if ( this->Capping )
    {
    this->Cap(newScalars);
    }
//
// Update self
//
  this->PointData.SetScalars(newScalars);

}


void vlSampleFunction::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vlSampleFunction::SetSampleDimensions(int dim[3])
{
  vlDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    for ( int i=0; i<3; i++) this->SampleDimensions[i] = dim[i];
    this->Modified();
    }
}

void vlSampleFunction::Cap(vlFloatScalars *s)
{
  int i,j,k;
  int idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  k = 0;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+j*this->SampleDimensions[1], this->CapValue);

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+j*this->SampleDimensions[1], this->CapValue);

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(j*this->SampleDimensions[0]+k*d01, this->CapValue);

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(i+j*this->SampleDimensions[0]+k*d01, this->CapValue);

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+k*d01, this->CapValue);

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+k*d01, this->CapValue);

}


