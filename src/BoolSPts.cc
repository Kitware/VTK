/*=========================================================================

  Program:   Visualization Library
  Module:    BoolSPts.cc
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
#include "BoolSPts.hh"

vlBooleanStructuredPoints::vlBooleanStructuredPoints()
{
  this->OperationType = UNION_OPERATOR;
  this->Operator = this->Union;
}

vlBooleanStructuredPoints::~vlBooleanStructuredPoints()
{
  vlStructuredPoints *ds;

  for ( int i=0; i < this->Input.GetNumberOfItems(); i++ )
    {
    ds = this->Input.GetItem(i+1);
    ds->UnRegister(this);
    }
}

void vlBooleanStructuredPoints::AddInput(vlStructuredPoints *sp)
{
  if ( ! this->Input.IsItemPresent(sp) )
    {
    this->Modified();
    sp->Register(this);
    this->Input.AddItem(sp);
    }
}

void vlBooleanStructuredPoints::RemoveInput(vlStructuredPoints *sp)
{
  if ( this->Input.IsItemPresent(sp) )
    {
    this->Modified();
    sp->UnRegister(this);
    this->Input.RemoveItem(sp);
    }
}

void vlBooleanStructuredPoints::Update()
{
  unsigned long int mtime, ds_mtime;
  int i;
  vlDataSet *ds;

  // make sure input is available
  if ( this->Input.GetNumberOfItems() < 1 ) return;

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, i=0; i < this->Input.GetNumberOfItems(); i++)
    {
    ds = this->Input.GetItem(i+1);
    ds_mtime = ds->GetMTime();
    if ( ds_mtime > mtime ) mtime = ds_mtime;
    ds->Update();
    }
  this->Updating = 0;

  if (mtime > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}

// Initialize object prior to performing Boolean operations
void vlBooleanStructuredPoints::InitializeBoolean()
{
  vlScalars *inScalars=NULL;
  vlScalars *newScalars;
  vlStructuredPoints *sp;
  float *bounds;
  int numPts;
  int i, j;

  this->Initialize();
  this->SetDimensions(this->SampleDimensions);
  numPts = this->GetNumberOfPoints();

  // If ModelBounds unset, use input, else punt
  if ( this->ModelBounds[0] <= this->ModelBounds[1] ||
  this->ModelBounds[2] <= this->ModelBounds[3] ||
  this->ModelBounds[4] <= this->ModelBounds[5] )
    {
    if ( this->Input.GetNumberOfItems() > 0 )
      {
       this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = LARGE_FLOAT;
       this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = -LARGE_FLOAT;
      for ( i=1; i <= this->Input.GetNumberOfItems(); i++ )
        {
        sp = this->Input.GetItem(i);
        bounds = sp->GetBounds();
        for (j=0; j < 3; j++)
          {
          if ( bounds[2*j] < this->ModelBounds[2*j] )
            this->ModelBounds[2*j] = bounds[2*j];

          if ( bounds[2*j+1] > this->ModelBounds[2*j+1] )
            this->ModelBounds[2*j+1] = bounds[2*j+1];
          }
        }
      } 
    else
      {
      this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = 0.0;
      this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = 1000.0;
      }
    }

  // Update origin and aspect ratio
  for (i=0; i<3; i++)
    {
    this->Origin[i] = this->ModelBounds[2*i];
    this->AspectRatio[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
                           / (this->SampleDimensions[i] - 1);
    }

  // Create output scalar (same type as input)
  if ( this->Input.GetNumberOfItems() > 0 )
    {
    sp = this->Input.GetItem(1);
    inScalars = sp->GetPointData()->GetScalars();
    }

  if ( inScalars != NULL )
    {
    newScalars = inScalars->MakeObject(numPts);
    }
  else
    {
    newScalars = new vlFloatScalars(numPts);
    }

  this->PointData.SetScalars(newScalars);
}

// Perform Boolean operations on input volumes
void vlBooleanStructuredPoints::Execute()
{
  vlStructuredPoints *sp;
  int i;

  this->InitializeBoolean();

  if ( this->Input.GetNumberOfItems() > 0 )
    {
    for ( i=1; i <= this->Input.GetNumberOfItems(); i++ )
      {
      sp = this->Input.GetItem(i);
      this->Append(sp);
      }
    }
}

// Perform Boolean operations by appending to current data
void vlBooleanStructuredPoints::Append(vlStructuredPoints *sp)
{
  vlScalars *currentScalars, *inScalars;

  if ( (currentScalars = this->PointData.GetScalars()) == NULL )
    {
    this->InitializeBoolean();
    currentScalars = this->PointData.GetScalars();
    }

  inScalars = sp->GetPointData()->GetScalars();

  // now perform operation on data

}

void vlBooleanStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlBooleanStructuredPoints::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlStructuredPoints::PrintSelf(os,indent);
    vlFilter::PrintSelf(os,indent);

    os << indent << "Input DataSets:\n";
    this->Input.PrintSelf(os,indent.GetNextIndent());

    this->PrintWatchOff(); // stop worrying about it now
    }
}

void vlBooleanStructuredPoints::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vlBooleanStructuredPoints::SetSampleDimensions(int dim[3])
{
  int i;

  vlDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<0 || dim[1]<0 || dim[2]<0 )
      {
      vlErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

void vlBooleanStructuredPoints::SetModelBounds(float *bounds)
{
  vlBooleanStructuredPoints::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vlBooleanStructuredPoints::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
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

