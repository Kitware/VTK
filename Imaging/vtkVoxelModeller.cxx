/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include <stdio.h>
#include "vtkVoxelModeller.h"
#include "vtkObjectFactory.h"
#include "vtkBitArray.h"

//----------------------------------------------------------------------------
vtkVoxelModeller* vtkVoxelModeller::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVoxelModeller");
  if(ret)
    {
    return (vtkVoxelModeller*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVoxelModeller;
}

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

// Specify the position in space to perform the voxelization.
void vtkVoxelModeller::SetModelBounds(float bounds[6])
{
  vtkVoxelModeller::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3],
                                   bounds[4], bounds[5]);
}

void vtkVoxelModeller::SetModelBounds(float xmin, float xmax, float ymin,
                                      float ymax, float zmin, float zmax)
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
  vtkIdType cellNum, i;
  int j, k;
  float *bounds, adjBounds[6];
  vtkCell *cell;
  float maxDistance, pcoords[3];
  vtkBitArray *newScalars;
  vtkIdType numPts, idx, numCells;
  int subId;
  int min[3], max[3];
  float x[3], distance2;
  int jkFactor;
  vtkDataSet *input=this->GetInput();
  float *weights=new float[input->GetMaxCellSize()];
  float closestPoint[3];
  float voxelHalfWidth[3], origin[3], spacing[3];
  vtkStructuredPoints *output=this->GetOutput();

//
// Initialize self; create output objects
//
  vtkDebugMacro(<< "Executing Voxel model");

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] *
    this->SampleDimensions[2];
  newScalars = vtkBitArray::New();
  newScalars->SetNumberOfTuples(numPts);
  for (i=0; i<numPts; i++)
    {
    newScalars->SetComponent(i,0,0);
    }

  output->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds(origin,spacing);
  output->SetSpacing(spacing);
  output->SetOrigin(origin);
//
// Voxel widths are 1/2 the height, width, length of a voxel
//
  for (i=0; i < 3; i++)
    {
    voxelHalfWidth[i] = spacing[i] / 2.0;
    }
//
// Traverse all cells; computing distance function on volume points.
//
  numCells = input->GetNumberOfCells();
  for (cellNum=0; cellNum < numCells; cellNum++)
    {
    cell = input->GetCell(cellNum);
    bounds = cell->GetBounds();
    for (i=0; i<3; i++)
      {
      adjBounds[2*i] = bounds[2*i] - maxDistance;
      adjBounds[2*i+1] = bounds[2*i+1] + maxDistance;
      }

    // compute dimensional bounds in data set
    for (i=0; i<3; i++)
      {
      min[i] = (int) ((float)(adjBounds[2*i] - origin[i]) / spacing[i]);
      max[i] = (int) ((float)(adjBounds[2*i+1] - origin[i]) / spacing[i]);
      if (min[i] < 0)
	{
	min[i] = 0;
	}
      if (max[i] >= this->SampleDimensions[i])
	{
	max[i] = this->SampleDimensions[i] - 1;
	}
      }

    jkFactor = this->SampleDimensions[0]*this->SampleDimensions[1];
    for (k = min[2]; k <= max[2]; k++) 
      {
      x[2] = spacing[2] * k + origin[2];
      for (j = min[1]; j <= max[1]; j++)
        {
        x[1] = spacing[1] * j + origin[1];
        for (i = min[0]; i <= max[0]; i++) 
          {
	  idx = jkFactor*k + this->SampleDimensions[0]*j + i;
	  if (!(newScalars->GetComponent(idx,0)))
	    {
	    x[0] = spacing[0] * i + origin[0];

	    if ( cell->EvaluatePosition(x, closestPoint, subId, pcoords,
                                        distance2, weights) != -1 &&
                 ((fabs(closestPoint[0] - x[0]) <= voxelHalfWidth[0]) &&
                  (fabs(closestPoint[1] - x[1]) <= voxelHalfWidth[1]) &&
                  (fabs(closestPoint[2] - x[2]) <= voxelHalfWidth[2])) )
	      {
	      newScalars->SetComponent(idx,0,1);
	      }
	    }
	  }
        }
      }
    }
  delete [] weights;
//
// Update self
//
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

// Compute the ModelBounds based on the input geometry.
float vtkVoxelModeller::ComputeModelBounds(float origin[3], float spacing[3])
{
  float *bounds, maxDist;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    adjustBounds = 1;
    bounds = (this->GetInput())->GetBounds();
    }
  else
    {
    bounds = this->ModelBounds;
    }

  for (maxDist=0.0, i=0; i<3; i++)
    {
    if ( (bounds[2*i+1] - bounds[2*i]) > maxDist )
      {
      maxDist = bounds[2*i+1] - bounds[2*i];
      }
    }
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

  // Set volume origin and data spacing
  for (i=0; i<3; i++)
    {
    origin[i] = this->ModelBounds[2*i];
    spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])/
      (this->SampleDimensions[i] - 1);
    }

  return maxDist;  
}

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

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1]
                << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] ||
       dim[1] != this->SampleDimensions[1] ||
       dim[2] != this->SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }
    for (dataDim=0, i=0; i<3 ; i++)
      {
      if (dim[i] > 1)
	{
        dataDim++;
	}
      }
    if ( dataDim  < 3 )
      {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++)
      {
      this->SampleDimensions[i] = dim[i];
      }
    this->Modified();
    }
}

void vtkVoxelModeller::Write(char *fname)
{
  FILE *fp;
  int i, j, k;
  float origin[3], spacing[3];
  
  vtkDataArray *newScalars;
  int idx;
  int bitcount;
  unsigned char uc;
  vtkStructuredPoints *output=this->GetOutput();

  vtkDebugMacro(<< "Writing Voxel model");

  // update the data
  this->Update();
  
  newScalars = output->GetPointData()->GetActiveScalars();

  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(origin,spacing);

  fp = fopen(fname,"w");
  if (!fp) 
    {
    vtkErrorMacro(<< "Couldn't open file: " << fname << endl);
    return;
    }

  fprintf(fp,"Voxel Data File\n");
  fprintf(fp,"Origin: %f %f %f\n",origin[0],origin[1],origin[2]);
  fprintf(fp,"Aspect: %f %f %f\n",spacing[0],spacing[1],spacing[2]);
  fprintf(fp,"Dimensions: %i %i %i\n",this->SampleDimensions[0],
	  this->SampleDimensions[1],this->SampleDimensions[2]);

  // write out the data
  bitcount = 0;
  idx = 0;
  uc = 0x00;

  for (k = 0; k < this->SampleDimensions[2]; k++)
    {
    for (j = 0; j < this->SampleDimensions[1]; j++)
      {
      for (i = 0; i < this->SampleDimensions[0]; i++)
	{
	if (newScalars->GetComponent(idx,0))
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
      }
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
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", "
     << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", "
     << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", "
     << this->ModelBounds[5] << ")\n";
}
