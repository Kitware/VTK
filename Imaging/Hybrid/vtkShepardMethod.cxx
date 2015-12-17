/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShepardMethod.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkShepardMethod);

//-----------------------------------------------------------------------------
// Thread the algorithm by processing each z-slice independently as each
// point is procssed. (As input points are processed, their influence is felt
// across a cuboid domain - a splat footprint. The slices that make up the
// cuboid splat are processed in parallel.) Note also that the scalar data is
// processed via templating.
template <typename TIn, typename TOut>
class vtkShepardMethodAlgorithm
{
public:
  vtkShepardMethod *ShepardMethod;
  TIn  *InScalars;
  TOut *OutScalars;
  vtkIdType Dims[3], SliceSize;
  double Origin[3], Spacing[3], Radius2;

  class Splat
    {
    public:
      vtkShepardMethodAlgorithm *Algo;
      vtkIdType XMin, XMax, YMin, YMax, ZMin, ZMax;
      Splat(vtkShepardMethodAlgorithm *algo)
        {this->Algo = algo;}
      void SetBounds(int min[3], int max[3])
        {
          this->XMin = min[0]; this->XMax = max[0];
          this->YMin = min[1]; this->YMax = max[1];
          this->ZMin = min[2]; this->ZMax = max[2];
        }
      void  operator()(vtkIdType slice, vtkIdType end)
        {
        vtkIdType i, j, jOffset, kOffset, idx;
        double cx[3], dist2;
        for ( ; slice < end; ++slice )
          {
          // Loop over all sample points in volume within footprint and
          // evaluate the splat
          cx[2] = this->Algo->Origin[2] + this->Algo->Spacing[2]*slice;
          kOffset = slice*this->Algo->SliceSize;
          for (j=this->YMin; j<=this->YMax; j++)
            {
            cx[1] = this->Algo->Origin[1] + this->Algo->Spacing[1]*j;
            jOffset = j*this->Algo->Dims[0];
            for (i=this->XMin; i<=this->XMax; i++)
              {
              cx[0] = this->Algo->Origin[0] + this->Algo->Spacing[0]*i;
              dist2 = this->Algo->Splatter->SamplePoint(cx);
              if ( dist2 <= this->Algo->Radius2 )
                {
                idx = i + jOffset + kOffset;

                }//if within splat radius
              }//i
            }//j
          }//k within splat footprint
        }
    };

};


//-----------------------------------------------------------------------------
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

  this->PowerParameter = 2.0;
}

//-----------------------------------------------------------------------------
// Compute ModelBounds from input geometry.
double vtkShepardMethod::ComputeModelBounds(double origin[3],
                                            double spacing[3])
{
  double *bounds, maxDist;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
       this->ModelBounds[2] >= this->ModelBounds[3] ||
       this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    adjustBounds = 1;
    vtkDataSet *ds = vtkDataSet::SafeDownCast(this->GetInput());
    // ds better be non null otherwise something is very wrong here
    bounds = ds->GetBounds();
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

  return maxDist;
}

//-----------------------------------------------------------------------------
int vtkShepardMethod::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->SampleDimensions[0]-1,
               0, this->SampleDimensions[1]-1,
               0, this->SampleDimensions[2]-1);

  for (i=0; i < 3; i++)
    {
    origin[i] = this->ModelBounds[2*i];
    if ( this->SampleDimensions[i] <= 1 )
      {
      ar[i] = 1;
      }
    else
      {
      ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (this->SampleDimensions[i] - 1);
      }
    }
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkShepardMethod::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  output->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  vtkIdType ptId, i;
  int j, k;
  double *px, x[3], s, *sum, spacing[3], origin[3];

  double maxDistance, distance2, inScalar;
  vtkDataArray *inScalars;
  vtkIdType numPts, numNewPts, idx;
  int min[3], max[3];
  int jkFactor;
  vtkFloatArray *newScalars =
    vtkFloatArray::SafeDownCast(output->GetPointData()->GetScalars());

  vtkDebugMacro(<< "Executing Shepard method");

  // Check input
  //
  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"Points must be defined!");
    return 1;
    }

  if ( (inScalars = input->GetPointData()->GetScalars()) == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined!");
    return 1;
    }

  newScalars->SetName(inScalars->GetName());

  // Allocate
  //
  numNewPts = this->SampleDimensions[0] * this->SampleDimensions[1]
              * this->SampleDimensions[2];

  sum = new double[numNewPts];
  for (i=0; i<numNewPts; i++)
    {
    newScalars->SetComponent(i,0,0.0);
    sum[i] = 0.0;
    }

  maxDistance = this->ComputeModelBounds(origin,spacing);
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);


  // Traverse all input points.
  // Each input point affects voxels within maxDistance.
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 1000) )
      {
      vtkDebugMacro(<<"Inserting point #" << ptId);
      this->UpdateProgress (ptId/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    px = input->GetPoint(ptId);
    inScalar = inScalars->GetComponent(ptId,0);

    for (i=0; i<3; i++) //compute dimensional bounds in data set
      {
      min[i] = static_cast<int>(
        static_cast<double>((px[i] - maxDistance) - origin[i]) / spacing[i]);
      max[i] = static_cast<int>(
        static_cast<double>((px[i] + maxDistance) - origin[i]) / spacing[i]);
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
            sum[idx] = VTK_DOUBLE_MAX;
            newScalars->SetComponent(idx,0,inScalar);
            }
          else if ( sum[idx] < VTK_DOUBLE_MAX ) //check for precise hit
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
  int numExacts = 0;
  for (ptId=0; ptId<numNewPts; ptId++)
    {
    s = newScalars->GetComponent(ptId,0);

    if ( sum[ptId] >= VTK_DOUBLE_MAX )
      {
      numExacts++; // Value previously set
      }
    else if ( sum[ptId] != 0.0 )
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

  return 1;
}

//-----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void vtkShepardMethod::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//-----------------------------------------------------------------------------
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
      vtkErrorMacro(<<"Sample dimensions must define a 3D volume!");
      return;
      }

    for ( i=0; i<3; i++)
      {
      this->SampleDimensions[i] = dim[i];
      }

    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkShepardMethod::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkShepardMethod::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: ("
     << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: ("
     << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: ("
     << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Null Value: " << this->NullValue << "\n";

  os << indent << "Power Parameter: " << this->PowerParameter << "\n";

}
