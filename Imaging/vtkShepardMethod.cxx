/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Paul A, Hsieh for bug fixes


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
#include "vtkShepardMethod.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//-------------------------------------------------------------------------
vtkShepardMethod* vtkShepardMethod::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkShepardMethod");
  if(ret)
    {
    return (vtkShepardMethod*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkShepardMethod;
}

// Construct with sample dimensions=(50,50,50) and so that model bounds are
// automatically computed from input. Null value for each unvisited output 
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

// Compute ModelBounds from input geometry.
float vtkShepardMethod::ComputeModelBounds(float origin[3], float spacing[3])
{
  float *bounds, maxDist;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    adjustBounds = 1;
    bounds = this->GetInput()->GetBounds();
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
    spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
            / (this->SampleDimensions[i] - 1);
    }

  this->GetOutput()->SetOrigin(origin);
  this->GetOutput()->SetSpacing(spacing);

  return maxDist;  
}

void vtkShepardMethod::Execute()
{
  vtkIdType ptId, i;
  int j, k;
  float *px, x[3], s, *sum, spacing[3], origin[3];
  
  float maxDistance, distance2, inScalar;
  vtkDataArray *inScalars;
  vtkFloatArray *newScalars;
  vtkIdType numPts, numNewPts, idx;
  int min[3], max[3];
  int jkFactor;
  vtkDataSet *input = this->GetInput();
  vtkStructuredPoints *output = this->GetOutput();

  vtkDebugMacro(<< "Executing Shepard method");

  // Check input
  //
  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"Points must be defined!");
    return;
    }

  if ( (inScalars = input->GetPointData()->GetScalars()) == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined!");
    return;
    }

  // Allocate
  //
  numNewPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
              * this->SampleDimensions[2];

  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numNewPts);

  sum = new float[numNewPts];
  for (i=0; i<numNewPts; i++) 
    {
    newScalars->SetComponent(i,0,0.0);
    sum[i] = 0.0;
    }

  output->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds(origin,spacing);

  // Traverse all input points. 
  // Each input point affects voxels within maxDistance.
  //
  int abortExecute=0;
  for (ptId=0; ptId < numPts && !abortExecute; ptId++)
    {
    if ( ! (ptId % 1000) )
      {
      vtkDebugMacro(<<"Inserting point #" << ptId);
      this->UpdateProgress (ptId/numPts);
      if (this->GetAbortExecute())
        {
        abortExecute = 1;
        break;
        }
      }

    px = input->GetPoint(ptId);
    inScalar = inScalars->GetComponent(ptId,0);
    
    for (i=0; i<3; i++) //compute dimensional bounds in data set
      {
      float amin = (float)((px[i] - maxDistance) - origin[i]) / spacing[i];
      float amax = (float)((px[i] + maxDistance) - origin[i]) / spacing[i];
      min[i] = (int) amin;
      max[i] = (int) amax;
      
      if (min[i] < amin)
        {
        min[i]++; // round upward to nearest integer to get min[i]
        }
      if (max[i] > amax)
        {
        max[i]--; // round downward to nearest integer to get max[i]
        }

      if (min[i] < 0)
        {
        min[i] = 0; // valid range check
        }
      if (max[i] >= this->SampleDimensions[i]) 
        {
        max[i] = this->SampleDimensions[i] - 1;
        }
      }

    for (i=0; i<3; i++) //compute dimensional bounds in data set
      {
      min[i] = (int) ((float)((px[i] - maxDistance) - origin[i]) / spacing[i]);
      max[i] = (int) ((float)((px[i] + maxDistance) - origin[i]) / spacing[i]);
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
          x[0] = spacing[0] * i + origin[0];
          idx = jkFactor*k + this->SampleDimensions[0]*j + i;

          distance2 = vtkMath::Distance2BetweenPoints(x,px);

          if ( distance2 == 0.0 )
            {
            sum[idx] = VTK_LARGE_FLOAT;
            newScalars->SetComponent(idx,0,VTK_LARGE_FLOAT);
            }
          else
            {
            s = newScalars->GetComponent(idx,0);
            sum[idx] += 1.0 / distance2;
            newScalars->SetComponent(idx,0,s+(inScalar/distance2));
            }
          }
        }
      }
    }

  // Run through scalars and compute final values
  //
  for (ptId=0; ptId<numNewPts; ptId++)
    {
    s = newScalars->GetComponent(ptId,0);
    if ( sum[ptId] != 0.0 )
      {
      newScalars->SetComponent(ptId,0,s/sum[ptId]);
      }
    else
      {
      newScalars->SetComponent(ptId,0,this->NullValue);
      }
    }

  // Update self
  //
  delete [] sum;
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

// Set the i-j-k dimensions on which to sample the distance function.
void vtkShepardMethod::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

// Set the i-j-k dimensions on which to sample the distance function.
void vtkShepardMethod::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," 
                << dim[1] << "," << dim[2] << ")");

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
