/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include <stdio.h>
#include "vtkVoxelModeller.h"
#include "vtkBitScalars.h"

// Description:
// Construct an instance of vtkVoxelModeller with its sample dimensions
// set to (50,50,50), and so that the model bounds are
// automatically computed from its input. The maximum distance is set to 
// examine the whole grid. This could be made much faster, and probably
// will be in the future.
vtkVoxelModeller::vtkVoxelModeller()
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

// Description:
// Specify the position in space to perform the voxelization.
void vtkVoxelModeller::SetModelBounds(float *bounds)
{
  vtkVoxelModeller::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkVoxelModeller::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;
    }
}

void vtkVoxelModeller::Execute()
{
  int cellNum, i, j, k;
  float *bounds, adjBounds[6];
  vtkCell *cell;
  float maxDistance, pcoords[3];
  vtkBitScalars *newScalars;
  int numPts, idx;
  int subId;
  int min[3], max[3];
  float x[3], distance2;
  int jkFactor;
  float *weights=new float[this->Input->GetMaxCellSize()];
  float closestPoint[3];
  float voxelHalfWidth[3], origin[3], ar[3];
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;
//
// Initialize self; create output objects
//
  vtkDebugMacro(<< "Executing Voxel model");

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];
  newScalars = new vtkBitScalars(numPts);
  for (i=0; i<numPts; i++) newScalars->SetScalar(i,0);

  output->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds(origin,ar);
//
// Voxel widths are 1/2 the height, width, length of a voxel
//
  for (i=0; i < 3; i++) voxelHalfWidth[i] = ar[i] / 2.0;
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
      min[i] = (int) ((float)(adjBounds[2*i] - origin[i]) / 
                      ar[i]);
      max[i] = (int) ((float)(adjBounds[2*i+1] - origin[i]) / 
                      ar[i]);
      if (min[i] < 0) min[i] = 0;
      if (max[i] >= this->SampleDimensions[i]) max[i] = this->SampleDimensions[i] - 1;
      }

    jkFactor = this->SampleDimensions[0]*this->SampleDimensions[1];
    for (k = min[2]; k <= max[2]; k++) 
      {
      x[2] = ar[2] * k + origin[2];
      for (j = min[1]; j <= max[1]; j++)
        {
        x[1] = ar[1] * j + origin[1];
        for (i = min[0]; i <= max[0]; i++) 
          {
	  idx = jkFactor*k + this->SampleDimensions[0]*j + i;
	  if (!(newScalars->GetScalar(idx)))
	    {
	    x[0] = ar[0] * i + origin[0];

	    if ( cell->EvaluatePosition(x, closestPoint, subId, pcoords, distance2, weights) != -1 &&
	    ((fabs(closestPoint[0] - x[0]) <= voxelHalfWidth[0]) &&
            (fabs(closestPoint[1] - x[1]) <= voxelHalfWidth[1]) &&
	    (fabs(closestPoint[2] - x[2]) <= voxelHalfWidth[2])) )
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
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

// Description:
// Compute the ModelBounds based on the input geometry.
float vtkVoxelModeller::ComputeModelBounds(float origin[3], float ar[3])
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
    origin[i] = this->ModelBounds[2*i];
    ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])/
      (this->SampleDimensions[i] - 1);
    }

  return maxDist;  
}

// Description:
// Set the i-j-k dimensions on which to sample the distance function.
void vtkVoxelModeller::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkVoxelModeller::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for (dataDim=0, i=0; i<3 ; i++) if (dim[i] > 1) dataDim++;

    if ( dataDim  < 3 )
      {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

void vtkVoxelModeller::Write(char *fname)
{
  FILE *fp;
  int i, j, k;
  float maxDistance, origin[3], ar[3];
  
  vtkBitScalars *newScalars;
  int numPts, idx;
  int bitcount;
  unsigned char uc;

  vtkDebugMacro(<< "Writing Voxel model");

  // update the data
  this->Update();
  
  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];

  newScalars = (vtkBitScalars *)this->Output->GetPointData()->GetScalars();

  ((vtkStructuredPoints *)(this->Output))->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds(origin,ar);

  fp = fopen(fname,"w");
  if (!fp) 
    {
    vtkErrorMacro(<< "Couldn't open file: " << fname << endl);
    return;
    }

  fprintf(fp,"Voxel Data File\n");
  fprintf(fp,"Origin: %f %f %f\n",origin[0],origin[1],origin[2]);
  fprintf(fp,"Aspect: %f %f %f\n",ar[0],ar[1],ar[2]);
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

void vtkVoxelModeller::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";
  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "Model Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";
}

