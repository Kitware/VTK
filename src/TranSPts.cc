/*=========================================================================

  Program:   Visualization Library
  Module:    TranSPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TranSPts.hh"

// Description:
// Construct object to use input dimensions as sample dimensions,
// and to conpute bounds automatically from input. Fill value is set
// to large positive integer.
vlTransformStructuredPoints::vlTransformStructuredPoints()
{
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 0; // use input dimensions by default
  this->SampleDimensions[1] = 0;
  this->SampleDimensions[2] = 0;

  this->FillValue = LARGE_FLOAT;

  this->Transform = NULL;
}

void vlTransformStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlTransformStructuredPoints::GetClassName()))
    {
    vlStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);

    os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
                 << this->SampleDimensions[1] << ", "
                 << this->SampleDimensions[2] << ")\n";
    os << indent << "ModelBounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

    os << indent << "Fill Value:" << this->FillValue << "\n";

    os << indent << "Transform:" << this->Transform << "\n";
    }
}

// Description:
// Define pre-transformed size of structured point set.
void vlTransformStructuredPoints::SetModelBounds(float *bounds)
{
  vlTransformStructuredPoints::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vlTransformStructuredPoints::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
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

void vlTransformStructuredPoints::Execute()
{
  int i, numPts, numOutPts;
  int *dimIn;
  float *originIn, *aspectIn, ar[3];
  vlPointData *pd;
  vlScalars *inScalars, *outScalars;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;

  vlDebugMacro(<<"Transforming points");
  this->Initialize();

  // make sure there is input
  pd = this->Input->GetPointData();
  inScalars = pd->GetScalars();
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 ||
  inScalars == NULL )
    {
    vlErrorMacro(<<"No data to transform!");
    return;
    }

  // Get origin, aspect ratio and dimensions from input
  dimIn = input->GetDimensions();
  originIn = input->GetOrigin();
  aspectIn = input->GetAspectRatio();

  // if dimensions are not specified, use input's dimensions
  if (this->SampleDimensions[0] <= 1 || this->SampleDimensions[1] <= 1 || 
  this->SampleDimensions[2] <= 1)
    {
    this->SetDimensions(dimIn);
    }
	
  // otherwise use the specified dimensions
  else 
    {
    this->SetDimensions(this->SampleDimensions);
    }

  // if bounds are not specified, use input's aspect ratio and origin
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5])
    {
    this->SetAspectRatio(aspectIn);
    this->SetOrigin(originIn);
    }
  // otherwise, calculate them from bounds
  else 
    {
    this->SetOrigin(this->ModelBounds[0], this->ModelBounds[1], 
                    this->ModelBounds[2]);
    for (i=0; i<3; i++) 
      ar[i] = (this->ModelBounds[2*i+1]-this->ModelBounds[2*i]) /
              (this->Dimensions[i] - 1);

    this->SetAspectRatio(ar);
    }

  // Allocate data.  Scalar type is same as input.
  numOutPts = this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2];
  outScalars = inScalars->MakeObject(numOutPts);
  for (i = 0; i < numOutPts; i++) outScalars->SetScalar(i,this->FillValue);

  // Loop over all output voxels, transforming and then resampling from input.
  // Need to get inverse transformation matrix to perform transformation.






  // Update ourselves
  this->PointData.SetScalars(outScalars);

}


unsigned long int vlTransformStructuredPoints::GetMTime()
{
  unsigned long int mtime = 0;

  if ( this->Transform ) mtime = this->Transform->GetMTime();
  if (mtime > this->MTime) return mtime;
  return this->MTime;
}


