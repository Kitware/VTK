/*=========================================================================

  Program:   Visualization Library
  Module:    Shepard.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Shepard.hh"
#include "vtkMath.hh"

// Description:
// Construct with sample dimensions=(50,50,50) and so that model bounds are
// automatically computer from input. Null value for each unvisited output 
// point is 0.0. Maximum distance is 0.25.
vtkShepardMethod::vtkShepardMethod()
{
  this->MaximumDistance = 0.25;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->NullValue = 0.0;
}

// Description:
// Specify the position in space to perform the sampling.
void vtkShepardMethod::SetModelBounds(float *bounds)
{
  vtkShepardMethod::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkShepardMethod::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
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

// Description:
// Compute ModelBounds from input geometry.
float vtkShepardMethod::ComputeModelBounds()
{
  float *bounds, maxDist;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    adjustBounds = 1;
    bounds = this->Input->GetBounds();
    }
  else
    {
    bounds = this->ModelBounds;
    }

  for (maxDist=0.0, i=0; i<3; i++)
    if ( (bounds[2*i+1] - bounds[2*i]) > maxDist )
      maxDist = bounds[2*i+1] - bounds[2*i];

  maxDist *= this->MaximumDistance;

  // adjust bounds so model fits strictly inside (only if not set previously)
  if ( adjustBounds )
    {
    for (i=0; i<3; i++)
      {
      this->ModelBounds[2*i] = bounds[2*i] - maxDist;
      this->ModelBounds[2*i+1] = bounds[2*i+1] + maxDist;
      }
    }

  // Set volume origin and aspect ratio
  for (i=0; i<3; i++)
    {
    this->Origin[i] = this->ModelBounds[2*i];
    this->AspectRatio[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
                           / (this->SampleDimensions[i] - 1);
    }

  return maxDist;  
}

void vtkShepardMethod::Execute()
{
  int ptId, i, j, k;
  float *px, x[3], s, *sum;
  float maxDistance, distance2, inScalar;
  vtkScalars *inScalars;
  vtkFloatScalars *newScalars;
  int numPts, numNewPts, idx;
  int min[3], max[3];
  int jkFactor;
  vtkMath math;

  vtkDebugMacro(<< "Executing Shepard method");
  this->Initialize();
//
// Check input
//
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"Points must be defined!");
    return;
    }

  if ( (inScalars = this->Input->GetPointData()->GetScalars()) == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined!");
    return;
    }
//
// Allocate
//
  numNewPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
              * this->SampleDimensions[2];
  newScalars = new vtkFloatScalars(numNewPts);
  sum = new float[numNewPts];
  for (i=0; i<numNewPts; i++) 
    {
    newScalars->SetScalar(i,0.0);
    sum[i] = 0.0;
    }

  this->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds();
//
// Traverse all input points. Each input point affects voxels within maxDistance.
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    px = this->Input->GetPoint(ptId);
    inScalar = inScalars->GetScalar(ptId);
    
    for (i=0; i<3; i++) //compute dimensional bounds in data set
      {
      min[i] = (int) ((float)((x[i] - maxDistance) - this->Origin[i]) / 
                      this->AspectRatio[i]);
      max[i] = (int) ((float)((x[i] + maxDistance) - this->Origin[i]) / 
                      this->AspectRatio[i]);
      if (min[i] < 0) min[i] = 0;
      if (max[i] >= this->SampleDimensions[i]) max[i] = this->SampleDimensions[i] - 1;
      }

    jkFactor = this->SampleDimensions[0]*this->SampleDimensions[1];
    for (k = min[2]; k <= max[2]; k++) 
      {
      x[2] = this->AspectRatio[2] * k + this->Origin[2];
      for (j = min[1]; j <= max[1]; j++)
        {
        x[1] = this->AspectRatio[1] * j + this->Origin[1];
        for (i = min[0]; i <= max[0]; i++) 
          {
          x[0] = this->AspectRatio[0] * i + this->Origin[0];
          idx = jkFactor*k + this->SampleDimensions[0]*j + i;

          distance2 = math.Distance2BetweenPoints(x,px);

          if ( distance2 == 0.0 )
            {
            sum[idx] = LARGE_FLOAT;
            newScalars->SetScalar(idx,LARGE_FLOAT);
            }
          else
            {
            s = newScalars->GetScalar(idx);
            sum[idx] = 1.0 / distance2;
            newScalars->SetScalar(idx,s+(inScalar/distance2));
            }
          }
        }
      }
    }
//
// Run through scalars and compute final values
//
  for (ptId=0; ptId<numNewPts; ptId++)
    {
    s = newScalars->GetScalar(ptId);
    if ( sum[ptId] != 0.0 ) newScalars->SetScalar(ptId,s/sum[ptId]);
    else newScalars->SetScalar(ptId,this->NullValue);
    }
//
// Update self
//
  delete [] sum;
  this->PointData.SetScalars(newScalars);
  newScalars->Delete();
}

// Description:
// Set the i-j-k dimensions on which to sample the distance function.
void vtkShepardMethod::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkShepardMethod::SetSampleDimensions(int dim[3])
{
  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for (int dataDim=0, i=0; i<3 ; i++) if (dim[i] > 1) dataDim++;

    if ( dataDim  < 3 )
      {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

void vtkShepardMethod::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Null Value: " << this->NullValue << "\n";

}
