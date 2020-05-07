/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeMapper.h"

#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkRectilinearGrid.h"

// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  int i;

  this->BlendMode = vtkVolumeMapper::COMPOSITE_BLEND;
  this->AverageIPScalarRange[0] = VTK_FLOAT_MIN;
  this->AverageIPScalarRange[1] = VTK_FLOAT_MAX;

  this->Cropping = 0;
  for (i = 0; i < 3; i++)
  {
    this->CroppingRegionPlanes[2 * i] = 0;
    this->CroppingRegionPlanes[2 * i + 1] = 1;
    this->VoxelCroppingRegionPlanes[2 * i] = 0;
    this->VoxelCroppingRegionPlanes[2 * i + 1] = 1;
  }
  this->CroppingRegionFlags = VTK_CROP_SUBVOLUME;
}

vtkVolumeMapper::~vtkVolumeMapper() = default;

void vtkVolumeMapper::ConvertCroppingRegionPlanesToVoxels()
{
  vtkDataSet* input = this->GetInput();
  const double* bds = this->GetInput()->GetBounds();
  double physicalPt[3], ijk[3];
  int dims[3];
  vtkImageData* imageData = vtkImageData::SafeDownCast(input);
  vtkRectilinearGrid* rectGrid = vtkRectilinearGrid::SafeDownCast(input);
  if (imageData)
  {
    imageData->GetDimensions(dims);
  }
  else if (rectGrid)
  {
    rectGrid->GetDimensions(dims);
  }
  else
  {
    return;
  }
  for (int i = 0; i < 6; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      physicalPt[j] = bds[2 * j];
    }
    physicalPt[i / 2] = this->CroppingRegionPlanes[i];
    if (imageData)
    {
      imageData->TransformPhysicalPointToContinuousIndex(physicalPt, ijk);
      ijk[i / 2] = ijk[i / 2] < 0 ? 0 : ijk[i / 2];
      ijk[i / 2] = ijk[i / 2] > dims[i / 2] - 1 ? dims[i / 2] - 1 : ijk[i / 2];
    }
    else if (rectGrid)
    {
      int ijkI[3];
      double pCoords[3];
      if (!rectGrid->ComputeStructuredCoordinates(physicalPt, ijkI, pCoords))
      {
        if (physicalPt[i / 2] < bds[i / 2])
        {
          ijk[i / 2] = 0;
        }
        else
        {
          ijk[i / 2] = dims[i / 2] - 1;
        }
      }
      else
      {
        ijk[i / 2] = static_cast<double>(ijkI[i / 2]);
      }
    }
    this->VoxelCroppingRegionPlanes[i] = ijk[i / 2];
  }
}

void vtkVolumeMapper::SetInputData(vtkDataSet* genericInput)
{
  if (vtkImageData* imageData = vtkImageData::SafeDownCast(genericInput))
  {
    this->SetInputData(imageData);
  }
  else if (vtkRectilinearGrid* rectGrid = vtkRectilinearGrid::SafeDownCast(genericInput))
  {
    this->SetInputData(rectGrid);
  }
  else
  {
    vtkErrorMacro("The SetInput method of this mapper requires either"
      << " a vtkImageData or a vtkRectilinearGrid as input");
  }
}

void vtkVolumeMapper::SetInputData(vtkImageData* input)
{
  this->SetInputDataInternal(0, input);
}

void vtkVolumeMapper::SetInputData(vtkRectilinearGrid* input)
{
  this->SetInputDataInternal(0, input);
}

vtkDataSet* vtkVolumeMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

vtkDataSet* vtkVolumeMapper::GetInput(const int port)
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(port, 0));
}

// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cropping: " << (this->Cropping ? "On\n" : "Off\n");

  os << indent << "Cropping Region Planes: " << endl
     << indent << "  In X: " << this->CroppingRegionPlanes[0] << " to "
     << this->CroppingRegionPlanes[1] << endl
     << indent << "  In Y: " << this->CroppingRegionPlanes[2] << " to "
     << this->CroppingRegionPlanes[3] << endl
     << indent << "  In Z: " << this->CroppingRegionPlanes[4] << " to "
     << this->CroppingRegionPlanes[5] << endl;

  os << indent << "Cropping Region Flags: " << this->CroppingRegionFlags << endl;

  os << indent << "BlendMode: " << this->BlendMode << endl;

  // Don't print this->VoxelCroppingRegionPlanes
}

//------------------------------------------------------------------------------
int vtkVolumeMapper::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
double vtkVolumeMapper::SpacingAdjustedSampleDistance(double inputSpacing[3], int inputExtent[6])
{
  // compute 1/2 the average spacing
  double dist = (inputSpacing[0] + inputSpacing[1] + inputSpacing[2]) / 6.0;
  double avgNumVoxels =
    pow(static_cast<double>((inputExtent[1] - inputExtent[0]) * (inputExtent[3] - inputExtent[2]) *
          (inputExtent[5] - inputExtent[4])),
      static_cast<double>(0.333));

  if (avgNumVoxels < 100)
  {
    dist *= 0.01 + (1 - 0.01) * avgNumVoxels / 100;
  }

  return dist;
}
