/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelMod.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <stdio.h>
#include "VoxelMod.hh"
#include "BScalars.hh"

// Description:
// Construct with sample dimensions=(50,50,50) and so that model bounds are
// automatically computer from input. Maximum distance is set to examine
// whole grid.
vlVoxelModeller::vlVoxelModeller()
{
  this->MaximumDistance = 1.0;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;
}

void vlVoxelModeller::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";
  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "Model Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";
}

// Description:
// Specify the position in space to perform the voxelization.
void vlVoxelModeller::SetModelBounds(float *bounds)
{
  vlVoxelModeller::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vlVoxelModeller::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
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

void vlVoxelModeller::Execute()
{
  int cellNum, i, j, k;
  float *bounds, adjBounds[6];
  vlCell *cell;
  float maxDistance, pcoords[3];
  vlBitScalars *newScalars;
  int numPts, idx;
  int subId;
  int min[3], max[3];
  float x[3], distance2;
  int jkFactor;
  float weights[MAX_CELL_SIZE];
  float closestPoint[3];
  float voxelHalfWidth[3];

  vlDebugMacro(<< "Executing Voxel model");
//
// Initialize self; create output objects
//
  this->Initialize();

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];
  newScalars = new vlBitScalars(numPts);
  for (i=0; i<numPts; i++) newScalars->SetScalar(i,0);

  this->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds();
//
// Voxel widths are 1/2 the height, width, length of a voxel
//
  for (i=0; i < 3; i++) voxelHalfWidth[i] = this->AspectRatio[i] / 2.0;
//
// Traverse all cells; computing distance function on volume points.
//
  for (cellNum=0; cellNum < this->Input->GetNumberOfCells(); cellNum++)
    {
    cell = this->Input->GetCell(cellNum);
    bounds = cell->GetBounds();
    for (i=0; i<3; i++)
      {
      adjBounds[2*i] = bounds[2*i] - maxDistance;
      adjBounds[2*i+1] = bounds[2*i+1] + maxDistance;
      }

    // compute dimensional bounds in data set
    for (i=0; i<3; i++)
      {
      min[i] = (int) ((float)(adjBounds[2*i] - this->Origin[i]) / 
                      this->AspectRatio[i]);
      max[i] = (int) ((float)(adjBounds[2*i+1] - this->Origin[i]) / 
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
	  idx = jkFactor*k + this->SampleDimensions[0]*j + i;
	  if (!(newScalars->GetScalar(idx)))
	    {
	    x[0] = this->AspectRatio[0] * i + this->Origin[0];
	    cell->EvaluatePosition(x, closestPoint, subId, pcoords, 
                                   distance2, weights);
	    if ((fabs(closestPoint[0] - x[0]) <= voxelHalfWidth[0]) &&
		(fabs(closestPoint[1] - x[1]) <= voxelHalfWidth[1]) &&
		(fabs(closestPoint[2] - x[2]) <= voxelHalfWidth[2]))
	      {
	      newScalars->SetScalar(idx,1);
	      }
	    }
	  }
        }
      }
    }
//
// Update self
//
  this->PointData.SetScalars(newScalars);

}

// Description:
// Compute ModelBounds from input geometry.
float vlVoxelModeller::ComputeModelBounds()
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
    this->AspectRatio[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])/
      (this->SampleDimensions[i] - 1);
    }

  return maxDist;  
}

// Description:
// Set the i-j-k dimensions on which to sample the distance function.
void vlVoxelModeller::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vlVoxelModeller::SetSampleDimensions(int dim[3])
{
  vlDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vlErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for (int dataDim=0, i=0; i<3 ; i++) if (dim[i] > 1) dataDim++;

    if ( dataDim  < 3 )
      {
      vlErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

void vlVoxelModeller::Write(char *fname)
{
  FILE *fp;
  int i, j, k;
  float maxDistance;
  vlBitScalars *newScalars;
  int numPts, idx;
  int bitcount;
  unsigned char uc;

  vlDebugMacro(<< "Writing Voxel model");

  // update the data
  this->Execute();
  
  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];

  newScalars = (vlBitScalars *)this->PointData.GetScalars();
  

  this->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds();

  fp = fopen(fname,"w");
  if (!fp) 
    {
    vlErrorMacro(<< "Couldn't open file: " << fname << endl);
    return;
    }

  fprintf(fp,"Voxel Data File\n");
  fprintf(fp,"Origin: %f %f %f\n",this->Origin[0],
	  this->Origin[1],this->Origin[2]);
  fprintf(fp,"Aspect: %f %f %f\n",this->AspectRatio[0],
	  this->AspectRatio[1],this->AspectRatio[2]);
  fprintf(fp,"Dimensions: %i %i %i\n",this->SampleDimensions[0],
	  this->SampleDimensions[1],this->SampleDimensions[2]);

  // write out the data
  bitcount = 0;
  idx = 0;
  uc = 0x00;

  for (k = 0; k < this->SampleDimensions[2]; k++)
    for (j = 0; j < this->SampleDimensions[1]; j++)
      for (i = 0; i < this->SampleDimensions[0]; i++)
	{
	if (newScalars->GetScalar(idx))
	  {
	  uc |= (0x80 >> bitcount);
	  }
	bitcount++;
	if (bitcount == 8)
	  {
	  fputc(uc,fp);
	  uc = 0x00;
	  bitcount = 0;
	  }
	idx++;
	}
  if (bitcount)
    {
    fputc(uc,fp);
    }

  fclose(fp);
}
