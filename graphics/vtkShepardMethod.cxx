/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.cxx
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
#include "vtkShepardMethod.h"
#include "vtkMath.h"

// Description:
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

void vtkShepardMethod::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->SetModelBounds(bounds);
}

// Description:
// Compute ModelBounds from input geometry.
float vtkShepardMethod::ComputeModelBounds(float origin[3], float ar[3])
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
    ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
            / (this->SampleDimensions[i] - 1);
    }

  ((vtkStructuredPoints *)this->Output)->SetOrigin(origin);
  ((vtkStructuredPoints *)this->Output)->SetAspectRatio(ar);

  return maxDist;  
}

void vtkShepardMethod::Execute()
{
  int ptId, i, j, k;
  float *px, x[3], s, *sum, ar[3], origin[3];
  
  float maxDistance, distance2, inScalar;
  vtkScalars *inScalars;
  vtkFloatScalars *newScalars;
  int numPts, numNewPts, idx;
  int min[3], max[3];
  int jkFactor;
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

  vtkDebugMacro(<< "Executing Shepard method");
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
  newScalars->SetNumberOfScalars(numNewPts);

  sum = new float[numNewPts];
  for (i=0; i<numNewPts; i++) 
    {
    newScalars->SetScalar(i,0.0);
    sum[i] = 0.0;
    }

  output->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds(origin,ar);
//
// Traverse all input points. Each input point affects voxels within maxDistance.
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    px = this->Input->GetPoint(ptId);
    inScalar = inScalars->GetScalar(ptId);
    
    for (i=0; i<3; i++) //compute dimensional bounds in data set
      {
      min[i] = (int) ((float)((px[i] - maxDistance) - origin[i]) / ar[i]);
      max[i] = (int) ((float)((px[i] + maxDistance) - origin[i]) / ar[i]);
      if (min[i] < 0) min[i] = 0;
      if (max[i] >= this->SampleDimensions[i]) 
	max[i] = this->SampleDimensions[i] - 1;
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
          x[0] = ar[0] * i + origin[0];
          idx = jkFactor*k + this->SampleDimensions[0]*j + i;

          distance2 = vtkMath::Distance2BetweenPoints(x,px);

          if ( distance2 == 0.0 )
            {
            sum[idx] = VTK_LARGE_FLOAT;
            newScalars->SetScalar(idx,VTK_LARGE_FLOAT);
            }
          else
            {
            s = newScalars->GetScalar(idx);
            sum[idx] += 1.0 / distance2;
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
  output->GetPointData()->SetScalars(newScalars);
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

// Description:
// Set the i-j-k dimensions on which to sample the distance function.
void vtkShepardMethod::SetSampleDimensions(int dim[3])
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

void vtkShepardMethod::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetFilter::PrintSelf(os,indent);

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
