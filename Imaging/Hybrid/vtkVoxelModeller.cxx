/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoxelModeller.h"

#include "vtkBitArray.h"
#include "vtkCell.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

#include <math.h>

vtkStandardNewMacro(vtkVoxelModeller);

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

  this->ScalarType = VTK_BIT;
  this->ForegroundValue = 1.0;
  this->BackgroundValue = 0.0;
}

// Specify the position in space to perform the voxelization.
void vtkVoxelModeller::SetModelBounds(const double bounds[6])
{
  vtkVoxelModeller::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3],
                                   bounds[4], bounds[5]);
}

void vtkVoxelModeller::SetModelBounds(double xmin, double xmax, double ymin,
                                      double ymax, double zmin, double zmax)
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

int vtkVoxelModeller::RequestInformation (
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

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->ScalarType, 1);
  return 1;
}

int vtkVoxelModeller::RequestData(
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
  output->SetExtent(outInfo->Get(
                      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  vtkIdType cellNum, i;
  int j, k;
  double *bounds, adjBounds[6];
  vtkCell *cell;
  double maxDistance, pcoords[3];
  vtkIdType numPts, idx, numCells;
  int subId;
  int min[3], max[3];
  double x[3], distance2;
  int jkFactor;
  double *weights=new double[input->GetMaxCellSize()];
  double closestPoint[3];
  double voxelHalfWidth[3], origin[3], spacing[3];
  vtkDataArray *newScalars = output->GetPointData()->GetScalars();

  //
  // Initialize self; create output objects
  //
  vtkDebugMacro(<< "Executing Voxel model");

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] *
    this->SampleDimensions[2];
  for (i=0; i<numPts; i++)
    {
    newScalars->SetComponent(i,0,this->BackgroundValue);
    }

  maxDistance = this->ComputeModelBounds(origin,spacing);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
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
      min[i] = static_cast<int>(
        static_cast<double>(adjBounds[2*i] - origin[i]) / spacing[i]);
      max[i] = static_cast<int>(
        static_cast<double>(adjBounds[2*i+1] - origin[i]) / spacing[i]);
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
              newScalars->SetComponent(idx,0,this->ForegroundValue);
              }
            }
          }
        }
      }
    }
  delete [] weights;

  return 1;
}

// Compute the ModelBounds based on the input geometry.
double vtkVoxelModeller::ComputeModelBounds(double origin[3],
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

int vtkVoxelModeller::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkVoxelModeller::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
  os << indent << "ScalarType: " << this->ScalarType << endl;
  os << indent << "ForegroundValue: " << this->ForegroundValue << endl;
  os << indent << "BackgroundValue: " << this->BackgroundValue << endl;
}
