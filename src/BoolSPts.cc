/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoolSPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BoolSPts.hh"

// Description:
// Construct with sample resolution of (50,50,50) and automatic 
// computation of sample bounds. Initial boolean operation is union.
vtkBooleanStructuredPoints::vtkBooleanStructuredPoints()
{
  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->OperationType = UNION_OPERATOR;
  // this->Operator = this->Union;
}

vtkBooleanStructuredPoints::~vtkBooleanStructuredPoints()
{
}

unsigned long int vtkBooleanStructuredPoints::GetMTime()
{
  unsigned long dtime = this->vtkStructuredPoints::GetMTime();
  unsigned long ftime = this->vtkFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

// Description:
// Add another structured point set to the list of objects to boolean.
void vtkBooleanStructuredPoints::AddInput(vtkStructuredPoints *sp)
{
  if ( ! this->InputList.IsItemPresent(sp) )
    {
    this->Modified();
    this->InputList.AddItem(sp);
    }
}

// Description:
// Remove an object from the list of objects to boolean.
void vtkBooleanStructuredPoints::RemoveInput(vtkStructuredPoints *sp)
{
  if ( this->InputList.IsItemPresent(sp) )
    {
    this->Modified();
    this->InputList.RemoveItem(sp);
    }
}

void vtkBooleanStructuredPoints::Update()
{
  unsigned long int mtime, dsMtime;
  vtkDataSet *ds;

  // make sure input is available
  if ( this->InputList.GetNumberOfItems() < 1 ) return;

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if (mtime > this->GetMTime() || this->GetMTime() > this->ExecuteTime ||
  this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  for (this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
    if ( ds->ShouldIReleaseData() ) ds->ReleaseData();
}

// Initialize object prior to performing Boolean operations
void vtkBooleanStructuredPoints::InitializeBoolean()
{
  vtkScalars *inScalars=NULL;
  vtkScalars *newScalars;
  vtkStructuredPoints *sp;
  float *bounds;
  int numPts;
  int i, j;

  this->Initialize();
  this->SetDimensions(this->SampleDimensions);
  numPts = this->GetNumberOfPoints();

  // If ModelBounds unset, use input, else punt
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    if ( this->InputList.GetNumberOfItems() > 0 )
      {
       this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = LARGE_FLOAT;
       this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = -LARGE_FLOAT;
      for ( this->InputList.InitTraversal(); sp = this->InputList.GetNextItem(); )
        {
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
  if ( this->InputList.GetNumberOfItems() > 0 )
    {
    this->InputList.InitTraversal(); sp = this->InputList.GetNextItem();
    inScalars = sp->GetPointData()->GetScalars();
    }

  if ( inScalars != NULL )
    {
    newScalars = inScalars->MakeObject(numPts); //copy
    }
  else
    {
    newScalars = new vtkFloatScalars(numPts);
    }
  this->PointData.SetScalars(newScalars);
  newScalars->Delete();

}

// Perform Boolean operations on input volumes
void vtkBooleanStructuredPoints::Execute()
{
  vtkStructuredPoints *sp;

  this->InitializeBoolean();

  for ( this->InputList.InitTraversal(); sp = this->InputList.GetNextItem(); )
    {
    this->Append(sp);
    }
}

// Description:
// Perform Boolean operations by appending to current output data.
void vtkBooleanStructuredPoints::Append(vtkStructuredPoints *sp)
{
  vtkScalars *currentScalars, *inScalars;
  float *in_bounds;
  float *dest_bounds;
  int i,j,k;
  float *in_aspect;
  float in_x,in_y,in_z;
  int in_i,in_j,in_k;
  int in_kval,in_jval;
  int dest_kval,dest_jval;
  int *in_dimensions;

  if ( (currentScalars = this->PointData.GetScalars()) == NULL )
    {
    this->InitializeBoolean();
    currentScalars = this->PointData.GetScalars();
    }

  inScalars = sp->GetPointData()->GetScalars();

  in_bounds = sp->GetBounds();
  dest_bounds = this->GetModelBounds();
  in_aspect = sp->GetAspectRatio();
  in_dimensions = sp->GetDimensions();

  // now perform operation on data
  switch (this->OperationType)
    {
    case UNION_OPERATOR :
      {
      // for each cell
      for (k = 0; k < this->SampleDimensions[2]; k++)
	{
	in_z = dest_bounds[4] + k*this->AspectRatio[2];
	in_k = int ((float)(in_z - in_bounds[4])/in_aspect[2]);
	if ((in_k >= 0)&&(in_k < in_dimensions[2]))
	  {
	  in_kval = in_k*in_dimensions[0]*in_dimensions[1];
	  dest_kval = k*this->SampleDimensions[0]*this->SampleDimensions[1];
	  for (j = 0; j < this->SampleDimensions[1]; j++)
	    {
	    in_y = dest_bounds[2] + j*this->AspectRatio[1];
	    in_j = (int) ((float)(in_y - in_bounds[2])/in_aspect[1]);
	    if ((in_j >= 0)&&(in_j < in_dimensions[1]))
	      {
	      in_jval = in_j*in_dimensions[0];
	      dest_jval = j*this->SampleDimensions[0];
	      for (i = 0; i < this->SampleDimensions[0]; i++)
		{
		in_x = dest_bounds[0] + i*this->AspectRatio[0];
		in_i = (int) ((float)(in_x - in_bounds[0])/in_aspect[0]);
		if ((in_i >= 0)&&(in_i < in_dimensions[0]))
		  {
		  if (inScalars->GetScalar(in_kval+in_jval+in_i))
		    {
		    currentScalars->SetScalar(dest_kval + dest_jval+i,1);
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
      break;
    }
}

// Description:
// Set the i-j-k dimensions on which to perform boolean operation.
void vtkBooleanStructuredPoints::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkBooleanStructuredPoints::SetSampleDimensions(int dim[3])
{
  int i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<0 || dim[1]<0 || dim[2]<0 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

// Description:
// Set the size of the volume oon which to perform the sampling.
void vtkBooleanStructuredPoints::SetModelBounds(float *bounds)
{
  vtkBooleanStructuredPoints::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkBooleanStructuredPoints::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
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


int vtkBooleanStructuredPoints::GetDataReleased()
{
  return this->DataReleased;
}

void vtkBooleanStructuredPoints::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkBooleanStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPoints::PrintSelf(os,indent);
  vtkFilter::_PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}

