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

#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPlane.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxSetObjectMacro(vtkImageSliceMapper, Points, vtkPoints);

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkImageSliceMapper)

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

  this->Points = NULL;
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
void vtkImageSliceMapper::ReleaseGraphicsResources(vtkWindow *)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::Render(vtkRenderer *, vtkImageSlice *)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // compute display extent
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    int wholeExtent[6];
    int *extent = this->DataWholeExtent;
    double *spacing = this->DataSpacing;
    double *origin = this->DataOrigin;

    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    inInfo->Get(vtkDataObject::SPACING(), spacing);
    inInfo->Get(vtkDataObject::ORIGIN(), origin);

    vtkMatrix4x4 *matrix = this->GetDataToWorldMatrix();

    for (int k = 0; k < 6; k++)
    {
      extent[k] = wholeExtent[k];
    }

    if (this->Cropping)
    {
      for (int i = 0; i < 3; i++)
      {
        if (extent[2*i] < this->CroppingRegion[2*i])
        {
          extent[2*i] = this->CroppingRegion[2*i];
        }
        if (extent[2*i+1] > this->CroppingRegion[2*i+1])
        {
          extent[2*i+1] = this->CroppingRegion[2*i+1];
        }
      }
    }

    if (this->SliceFacesCamera || this->SliceAtFocalPoint)
    {
      vtkRenderer *ren = this->GetCurrentRenderer();

      if (matrix && ren)
      {
        vtkCamera *camera = ren->GetActiveCamera();

        if (this->SliceFacesCamera)
        {
          this->Orientation = this->GetOrientationFromCamera(matrix, camera);
          this->Orientation = this->Orientation % 3;
        }

        if (this->SliceAtFocalPoint)
        {
          this->SliceNumber = this->GetSliceFromCamera(matrix, camera);
        }
      }
    }

    int orientation = this->Orientation % 3;

    this->SliceNumberMinValue = wholeExtent[2*orientation];
    this->SliceNumberMaxValue = wholeExtent[2*orientation + 1];

    if (this->SliceNumber < extent[2*orientation])
    {
      this->SliceNumber = extent[2*orientation];
    }
    if (this->SliceNumber > extent[2*orientation + 1])
    {
      this->SliceNumber = extent[2*orientation + 1];
    }

    // the test is for an empty extent (0, -1, 0, -1, 0, -1) which
    // otherwise would be changed into (0, -1, 0, -1, -1, -1)
    if (extent[2*orientation] <= extent[2*orientation + 1])
    {
      extent[2*orientation] = this->SliceNumber;
      extent[2*orientation + 1] = this->SliceNumber;
    }

    this->DisplayExtent[0] = extent[0];
    this->DisplayExtent[1] = extent[1];
    this->DisplayExtent[2] = extent[2];
    this->DisplayExtent[3] = extent[3];
    this->DisplayExtent[4] = extent[4];
    this->DisplayExtent[5] = extent[5];

    // Create point and normal of plane
    double point[4];
    point[0] = 0.5*(extent[0] + extent[1])*spacing[0] + origin[0];
    point[1] = 0.5*(extent[2] + extent[3])*spacing[1] + origin[1];
    point[2] = 0.5*(extent[4] + extent[5])*spacing[2] + origin[2];
    point[3] = 1.0;

    double normal[4];
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 0.0;
    normal[3] = -point[orientation];
    normal[orientation] = 1.0;

    if (matrix)
    {
      // Convert point and normal to world coords
      double mat[16];
      vtkMatrix4x4::DeepCopy(mat, matrix);
      vtkMatrix4x4::MultiplyPoint(mat, point, point);
      point[0] /= point[3];
      point[1] /= point[3];
      point[2] /= point[3];
      vtkMatrix4x4::Invert(mat, mat);
      vtkMatrix4x4::Transpose(mat, mat);
      vtkMatrix4x4::MultiplyPoint(mat, normal, normal);
      vtkMath::Normalize(normal);
    }

    this->SlicePlane->SetOrigin(point);
    this->SlicePlane->SetNormal(normal);

    return 1;
  }

  // set update extent
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

    if (this->Streaming)
    {
      // only update the display extent if streaming is on
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        this->DisplayExtent, 6);
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
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkImageData *output = vtkImageData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SliceNumber: " << this->SliceNumber << "\n";
  os << indent << "SliceNumberMinValue: "
     << this->SliceNumberMinValue << "\n";
  os << indent << "SliceNumberMaxValue: "
     << this->SliceNumberMaxValue<< "\n";
  os << indent << "Orientation: " << this->Orientation << "\n";
  os << indent << "Cropping: "
     << ( this->Cropping ? "On\n" : "Off\n" );
  os << indent << "CroppingRegion: "
     << this->CroppingRegion[0] << " " << this->CroppingRegion[1] << " "
     << this->CroppingRegion[2] << " " << this->CroppingRegion[3] << " "
     << this->CroppingRegion[4] << " " << this->CroppingRegion[5] << "\n";
  os << indent << "Points: " << this->Points << "\n";
}

//----------------------------------------------------------------------------
vtkMTimeType vtkImageSliceMapper::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->SliceFacesCamera || this->SliceAtFocalPoint)
  {
    vtkImageSlice *prop = this->GetCurrentProp();
    vtkRenderer *ren = this->GetCurrentRenderer();

    if (prop && ren)
    {
      vtkCamera *camera = ren->GetActiveCamera();
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
double *vtkImageSliceMapper::GetBounds()
{
  if (!this->GetInput())
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }

  this->UpdateInformation();

  int extent[6];
  extent[0] = this->DisplayExtent[0];
  extent[1] = this->DisplayExtent[1];
  extent[2] = this->DisplayExtent[2];
  extent[3] = this->DisplayExtent[3];
  extent[4] = this->DisplayExtent[4];
  extent[5] = this->DisplayExtent[5];

  int orientation = this->Orientation % 3;
  extent[2*orientation] = this->SliceNumberMinValue;
  extent[2*orientation + 1] = this->SliceNumberMaxValue;

  double *spacing = this->DataSpacing;
  double *origin = this->DataOrigin;

  // expand by half a pixel if border is on, except in slice direction
  double border = 0.5*(this->Border != 0);
  double borderX = border*(orientation != 0);
  double borderY = border*(orientation != 1);
  double borderZ = border*(orientation != 2);

  // swap the extent if the spacing is negative
  int swapX = (spacing[0] < 0);
  int swapY = (spacing[1] < 0);
  int swapZ = (spacing[2] < 0);

  this->Bounds[0+swapX] = origin[0] + (extent[0] - borderX) * spacing[0];
  this->Bounds[2+swapY] = origin[1] + (extent[2] - borderY) * spacing[1];
  this->Bounds[4+swapZ] = origin[2] + (extent[4] - borderZ) * spacing[2];

  this->Bounds[1-swapX] = origin[0] + (extent[1] + borderX) * spacing[0];
  this->Bounds[3-swapY] = origin[1] + (extent[3] + borderY) * spacing[1];
  this->Bounds[5-swapZ] = origin[2] + (extent[5] + borderZ) * spacing[2];

  return this->Bounds;
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetOrientationFromCamera(
  vtkMatrix4x4 *propMatrix, vtkCamera *camera)
{
  int orientation = 2;
  double normal[4] = { 0, 0, -1, 0 };
  double maxval = 0;
  double mat[16];

  camera->GetDirectionOfProjection(normal);
  vtkMatrix4x4::Transpose(*propMatrix->Element, mat);
  vtkMatrix4x4::MultiplyPoint(mat, normal, normal);

  for (int i = 2; i >= 0; --i)
  {
    int j = 0;
    double a = normal[i];
    if (a < 0)
    {
      a = -a;
      j = 3;
    }
    if (a > maxval)
    {
      orientation = i + j;
      maxval = a;
    }
  }

  return orientation;
}

//----------------------------------------------------------------------------
int vtkImageSliceMapper::GetSliceFromCamera(
  vtkMatrix4x4 *propMatrix, vtkCamera *camera)
{
  int orientation = this->Orientation;

  double p[4] = { 0, 0, 0, 1 };
  camera->GetFocalPoint(p);

  // convert world coords to data coords
  double mat[16];
  vtkMatrix4x4::Invert(*propMatrix->Element, mat);
  vtkMatrix4x4::MultiplyPoint(mat, p, p);
  double slicepos = p[orientation]/p[3];

  // adjust for origin/spacing
  slicepos -= this->DataOrigin[orientation];
  slicepos /= this->DataSpacing[orientation];

  // round to get integer, add a tolerance to prefer rounding up
  return vtkMath::Floor(slicepos + (0.5 + 7.62939453125e-06));
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::GetSlicePlaneInDataCoords(
  vtkMatrix4x4 *vtkNotUsed(propMatrix), double normal[4])
{
  int orientation = this->Orientation % 3;
  int slice = this->SliceNumber;

  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 0.0;
  normal[3] = -(slice*this->DataSpacing[orientation] +
                this->DataOrigin[orientation]);
  normal[orientation] = 1.0;
}

//----------------------------------------------------------------------------
void vtkImageSliceMapper::GetDimensionIndices(
  int orientation, int &xdim, int &ydim)
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
