/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSliceMapper.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxSetObjectMacro(vtkImageSliceMapper, Points, vtkPoints);

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkImageSliceMapper);

//----------------------------------------------------------------------------
vtkImageSliceMapper::vtkImageSliceMapper()
{
  this->SliceNumber = 0;
  this->SliceNumberMinValue = 0;
  this->SliceNumberMaxValue = 0;

  this->Orientation = 2;

  this->Cropping = false;

  this->CroppingRegion[0] = 0;
  this->CroppingRegion[1] = 0;
  this->CroppingRegion[2] = 0;
  this->CroppingRegion[3] = 0;
  this->CroppingRegion[4] = 0;
  this->CroppingRegion[5] = 0;

  this->Points = nullptr;
  this->ExactPixelMatch = false;
  this->PassColorData = false;

  // streaming misbehaves if there is no output port
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageSliceMapper::~vtkImageSliceMapper()
{
  if (this->Points)
  {
    this->Points->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::ReleaseGraphicsResources(vtkWindow*)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::Render(vtkRenderer*, vtkImageSlice*)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
vtkTypeBool vtkImageSliceMapper::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // compute display extent
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    int wholeExtent[6];
    int* extent = this->DataWholeExtent;

    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

    for (int k = 0; k < 6; k++)
    {
      extent[k] = wholeExtent[k];
    }

    if (this->Cropping)
    {
      for (int i = 0; i < 3; i++)
      {
        if (extent[2 * i] < this->CroppingRegion[2 * i])
        {
          extent[2 * i] = this->CroppingRegion[2 * i];
        }
        if (extent[2 * i + 1] > this->CroppingRegion[2 * i + 1])
        {
          extent[2 * i + 1] = this->CroppingRegion[2 * i + 1];
        }
      }
    }

    double* spacing = this->DataSpacing;
    double* origin = this->DataOrigin;
    double* dir = this->DataDirection;

    inInfo->Get(vtkDataObject::SPACING(), spacing);
    inInfo->Get(vtkDataObject::ORIGIN(), origin);

    vtkMatrix4x4* matrix = this->GetDataToWorldMatrix();
    if (inInfo->Has(vtkDataObject::DIRECTION()))
    {
      inInfo->Get(vtkDataObject::DIRECTION(), dir);
    }
    else
    {
      vtkMatrix3x3::Identity(dir);
    }
    // prepend indextophysical matrix
    double i2p[16];
    for (int i = 0; i < 3; ++i)
    {
      i2p[i * 4] = dir[i * 3] * spacing[0];
      i2p[i * 4 + 1] = dir[i * 3 + 1] * spacing[1];
      i2p[i * 4 + 2] = dir[i * 3 + 2] * spacing[2];
      i2p[i * 4 + 3] = origin[i];
      i2p[12 + i] = 0.0;
    }
    i2p[15] = 1.0;
    if (matrix)
    {
      vtkMatrix4x4::Multiply4x4(matrix->GetData(), i2p, i2p);
    }

    if (this->SliceFacesCamera || this->SliceAtFocalPoint)
    {
      vtkRenderer* ren = this->GetCurrentRenderer();

      if (ren)
      {
        vtkCamera* camera = ren->GetActiveCamera();

        if (this->SliceFacesCamera)
        {
          this->Orientation = this->GetOrientationFromCamera(i2p, camera);
          this->Orientation = this->Orientation % 3;
        }

        if (this->SliceAtFocalPoint)
        {
          this->SliceNumber = this->GetSliceFromCamera(i2p, camera);
        }
      }
    }

    int orientation = this->Orientation % 3;

    this->SliceNumberMinValue = wholeExtent[2 * orientation];
    this->SliceNumberMaxValue = wholeExtent[2 * orientation + 1];

    if (this->SliceNumber < extent[2 * orientation])
    {
      this->SliceNumber = extent[2 * orientation];
    }
    if (this->SliceNumber > extent[2 * orientation + 1])
    {
      this->SliceNumber = extent[2 * orientation + 1];
    }

    // the test is for an empty extent (0, -1, 0, -1, 0, -1) which
    // otherwise would be changed into (0, -1, 0, -1, -1, -1)
    if (extent[2 * orientation] <= extent[2 * orientation + 1])
    {
      extent[2 * orientation] = this->SliceNumber;
      extent[2 * orientation + 1] = this->SliceNumber;
    }

    this->DisplayExtent[0] = extent[0];
    this->DisplayExtent[1] = extent[1];
    this->DisplayExtent[2] = extent[2];
    this->DisplayExtent[3] = extent[3];
    this->DisplayExtent[4] = extent[4];
    this->DisplayExtent[5] = extent[5];

    // Create point and normal of plane
    double point[4];
    point[0] = 0.5 * (extent[0] + extent[1]);
    point[1] = 0.5 * (extent[2] + extent[3]);
    point[2] = 0.5 * (extent[4] + extent[5]);
    point[3] = 1.0;

    double normal[4];
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 0.0;
    normal[3] = -point[orientation];
    normal[orientation] = 1.0;

    // Convert point and normal to world coords
    vtkMatrix4x4::MultiplyPoint(i2p, point, point);
    point[0] /= point[3];
    point[1] /= point[3];
    point[2] /= point[3];
    vtkMatrix4x4::Invert(i2p, i2p);
    vtkMatrix4x4::Transpose(i2p, i2p);
    vtkMatrix4x4::MultiplyPoint(i2p, normal, normal);
    vtkMath::Normalize(normal);

    this->SlicePlane->SetOrigin(point);
    this->SlicePlane->SetNormal(normal);

    return 1;
  }

  // set update extent
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

    if (this->Streaming)
    {
      // only update the display extent if streaming is on
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), this->DisplayExtent, 6);
    }
    else
    {
      int ext[6];
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }

    return 1;
  }

  // just a dummy, does not do anything
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // set output extent to avoid re-execution
    output->GetInformation()->Set(vtkDataObject::DATA_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);

    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SliceNumber: " << this->SliceNumber << "\n";
  os << indent << "SliceNumberMinValue: " << this->SliceNumberMinValue << "\n";
  os << indent << "SliceNumberMaxValue: " << this->SliceNumberMaxValue << "\n";
  os << indent << "Orientation: " << this->Orientation << "\n";
  os << indent << "Cropping: " << (this->Cropping ? "On\n" : "Off\n");
  os << indent << "CroppingRegion: " << this->CroppingRegion[0] << " " << this->CroppingRegion[1]
     << " " << this->CroppingRegion[2] << " " << this->CroppingRegion[3] << " "
     << this->CroppingRegion[4] << " " << this->CroppingRegion[5] << "\n";
  os << indent << "Points: " << this->Points << "\n";
}

//----------------------------------------------------------------------------
vtkMTimeType vtkImageSliceMapper::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->SliceFacesCamera || this->SliceAtFocalPoint)
  {
    vtkImageSlice* prop = this->GetCurrentProp();
    vtkRenderer* ren = this->GetCurrentRenderer();

    if (prop && ren)
    {
      vtkCamera* camera = ren->GetActiveCamera();
      vtkMTimeType mTime2 = prop->GetMTime();
      if (mTime2 > mTime)
      {
        mTime = mTime2;
      }
      mTime2 = camera->GetMTime();
      if (mTime2 > mTime)
      {
        mTime = mTime2;
      }
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::SetSliceNumber(int i)
{
  if (i != this->SliceNumber)
  {
    this->SliceNumber = i;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetSliceNumber()
{
  return this->SliceNumber;
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetSliceNumberMinValue()
{
  this->UpdateInformation();
  return this->SliceNumberMinValue;
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetSliceNumberMaxValue()
{
  this->UpdateInformation();
  return this->SliceNumberMaxValue;
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::GetIndexBounds(double extent[6])
{
  if (!this->GetInput())
  {
    return;
  }

  this->UpdateInformation();

  extent[0] = this->DisplayExtent[0];
  extent[1] = this->DisplayExtent[1];
  extent[2] = this->DisplayExtent[2];
  extent[3] = this->DisplayExtent[3];
  extent[4] = this->DisplayExtent[4];
  extent[5] = this->DisplayExtent[5];

  int orientation = this->Orientation % 3;
  extent[2 * orientation] = this->SliceNumberMinValue;
  extent[2 * orientation + 1] = this->SliceNumberMaxValue;

  // expand by half a pixel if border is on, except in slice direction
  double border = 0.5 * (this->Border != 0);
  double borders[3];
  borders[0] = border * (orientation != 0);
  borders[1] = border * (orientation != 1);
  borders[2] = border * (orientation != 2);

  extent[0] -= borders[0];
  extent[1] += borders[0];
  extent[2] -= borders[1];
  extent[3] += borders[1];
  extent[4] -= borders[2];
  extent[5] += borders[2];
}

//----------------------------------------------------------------------------
double* vtkImageSliceMapper::GetBounds()
{
  if (!this->GetInput())
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }

  double extent[6];
  this->GetIndexBounds(extent);

  double* spacing = this->DataSpacing;
  double* origin = this->DataOrigin;
  double* direction = this->DataDirection;

  // compute bounds
  for (int k = 0; k < 2; ++k)
  {
    double kval = extent[k + 4];
    for (int j = 0; j < 2; ++j)
    {
      double jval = extent[j + 2];
      for (int i = 0; i < 2; ++i)
      {
        double ival = extent[i];
        double point[3];
        vtkImageData::TransformContinuousIndexToPhysicalPoint(
          ival, jval, kval, origin, spacing, direction, point);
        if (i + j + k == 0)
        {
          this->Bounds[0] = point[0];
          this->Bounds[1] = point[0];
          this->Bounds[2] = point[1];
          this->Bounds[3] = point[1];
          this->Bounds[4] = point[2];
          this->Bounds[5] = point[2];
        }
        else
        {
          for (int c = 0; c < 3; ++c)
          {
            this->Bounds[c * 2] = point[c] < this->Bounds[c * 2] ? point[c] : this->Bounds[c * 2];
            this->Bounds[c * 2 + 1] =
              point[c] > this->Bounds[c * 2 + 1] ? point[c] : this->Bounds[c * 2 + 1];
          }
        }
      }
    }
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetOrientationFromCamera(double const* propMatrix, vtkCamera* camera)
{
  double normal[4] = { 0, 0, -1, 0 };
  camera->GetDirectionOfProjection(normal);

  double mat[16];
  vtkMatrix4x4::Invert(propMatrix, mat);
  vtkMatrix4x4::Transpose(mat, mat);

  int maxIdx = 0;
  double maxDot = 0.0;
  for (int c = 0; c < 3; ++c)
  {
    double vec[3];
    vec[0] = mat[c];
    vec[1] = mat[c + 4];
    vec[2] = mat[c + 8];
    vtkMath::Normalize(vec);
    double dot = vtkMath::Dot(vec, normal);
    if (fabs(dot) > fabs(maxDot))
    {
      maxIdx = c;
      maxDot = dot;
    }
  }

  return maxIdx + (maxDot < 0.0 ? 3 : 0);
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetSliceFromCamera(double const* propMatrix, vtkCamera* camera)
{
  int orientation = this->Orientation;

  double p[4] = { 0, 0, 0, 1 };
  camera->GetFocalPoint(p);

  // convert world coords to data coords
  double mat[16];
  vtkMatrix4x4::Invert(propMatrix, mat);
  vtkMatrix4x4::MultiplyPoint(mat, p, p);
  double slicepos = p[orientation] / p[3];

  // round to get integer, add a tolerance to prefer rounding up
  return vtkMath::Floor(slicepos + (0.5 + 7.62939453125e-06));
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::GetSlicePlaneInDataCoords(
  vtkMatrix4x4* vtkNotUsed(propMatrix), double normal[4])
{
  int orientation = this->Orientation % 3;
  int slice = this->SliceNumber;

  normal[0] = this->DataDirection[orientation];
  normal[1] = this->DataDirection[3 + orientation];
  normal[2] = this->DataDirection[6 + orientation];
  double scale = vtkMath::Normalize(normal);

  // in this context data coordinates is physical coordinates
  // in that spacing and origin and direction are still applied
  // so it is basically index -> data (aka physical) -> world
  normal[3] = -(slice * this->DataSpacing[orientation] + this->DataOrigin[0] * normal[0] +
                this->DataOrigin[1] * normal[1] + this->DataOrigin[2] * normal[2]) /
    scale;
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::GetDimensionIndices(int orientation, int& xdim, int& ydim)
{
  orientation = orientation % 3;
  xdim = 1;
  ydim = 2;
  if (orientation != 0)
  {
    xdim = 0;
    if (orientation != 1)
    {
      ydim = 1;
    }
  }
}
