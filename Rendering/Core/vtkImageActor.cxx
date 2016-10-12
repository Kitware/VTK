/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageActor.h"

#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkRenderer.h"
#include "vtkImageProperty.h"
#include "vtkImageSliceMapper.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageActor);

//----------------------------------------------------------------------------
vtkImageActor::vtkImageActor()
{
  this->DisplayExtent[0] = 0;
  this->DisplayExtent[1] = -1;
  this->DisplayExtent[2] = 0;
  this->DisplayExtent[3] = -1;
  this->DisplayExtent[4] = 0;
  this->DisplayExtent[5] = -1;

  vtkMath::UninitializeBounds(this->DisplayBounds);

  this->Property = vtkImageProperty::New();
  this->Property->SetInterpolationTypeToLinear();
  this->Property->SetAmbient(1.0);
  this->Property->SetDiffuse(0.0);

  vtkImageSliceMapper *mapper = vtkImageSliceMapper::New();
  this->Mapper = mapper;
  mapper->BorderOff();
  mapper->SliceAtFocalPointOff();
  mapper->SliceFacesCameraOff();
  mapper->SetOrientationToZ();
  // For backwards compabilitity, make Streaming the default behavior
  mapper->StreamingOn();

  // The result of HasTranslucentPolygonalGeometry is cached
  this->TranslucentCachedResult = false;
  this->ForceOpaque = false;
}

//----------------------------------------------------------------------------
vtkImageActor::~vtkImageActor()
{
  if (this->Property)
  {
    this->Property->Delete();
    this->Property = NULL;
  }
  if (this->Mapper)
  {
    this->Mapper->Delete();
    this->Mapper = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkImageActor::SetInputData(vtkImageData *input)
{
  if (this->Mapper && input != this->Mapper->GetInput())
  {
    this->Mapper->SetInputData(input);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkAlgorithm *vtkImageActor::GetInputAlgorithm()
{
  if (!this->Mapper)
  {
    return 0;
  }

  return this->Mapper->GetInputAlgorithm();
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageActor::GetInput()
{
  if (!this->Mapper)
  {
    return 0;
  }

  return this->Mapper->GetInput();
}

//----------------------------------------------------------------------------
void vtkImageActor::SetInterpolate(int i)
{
  if (this->Property)
  {
    if (i)
    {
      if (this->Property->GetInterpolationType() != VTK_LINEAR_INTERPOLATION)
      {
        this->Property->SetInterpolationTypeToLinear();
        this->Modified();
      }
    }
    else
    {
      if (this->Property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION)
      {
        this->Property->SetInterpolationTypeToNearest();
        this->Modified();
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkImageActor::GetInterpolate()
{
  if (this->Property &&
      this->Property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION)
  {
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkImageActor::SetOpacity(double o)
{
  if (this->Property && this->Property->GetOpacity() != o)
  {
    this->Property->SetOpacity(o);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkImageActor::GetOpacity()
{
  if (this->Property)
  {
    return this->Property->GetOpacity();
  }

  return 1.0;
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumber()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
  {
    return 0;
  }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)->GetSliceNumber();
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMax()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
  {
    return 0;
  }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)
    ->GetSliceNumberMaxValue();
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMin()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
  {
    return 0;
  }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)
    ->GetSliceNumberMinValue();
}

//----------------------------------------------------------------------------
void vtkImageActor::SetDisplayExtent(int extent[6])
{
  int idx, modified = 0;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->DisplayExtent[idx] != extent[idx])
    {
      this->DisplayExtent[idx] = extent[idx];
      modified = 1;
    }
  }

  if (modified)
  {
    if (this->Mapper && this->Mapper->IsA("vtkImageSliceMapper"))
    {
      if (this->DisplayExtent[0] <= this->DisplayExtent[1])
      {
        static_cast<vtkImageSliceMapper *>(this->Mapper)->CroppingOn();
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetCroppingRegion(this->DisplayExtent);
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetOrientation(this->GetOrientationFromExtent(this->DisplayExtent));
      }
      else
      {
        static_cast<vtkImageSliceMapper *>(this->Mapper)->CroppingOff();
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetOrientationToZ();
      }
    }
    this->Modified();
  }
}
//----------------------------------------------------------------------------
void vtkImageActor::SetDisplayExtent(int minX, int maxX,
                                     int minY, int maxY,
                                     int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetDisplayExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageActor::GetDisplayExtent(int extent[6])
{
  for (int idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->DisplayExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImageActor::GetDisplayBounds()
{
  vtkAlgorithm* inputAlg = NULL;

  if (this->Mapper && this->Mapper->GetNumberOfInputConnections(0) > 0)
  {
    inputAlg = this->Mapper->GetInputAlgorithm();
  }

  if (!inputAlg)
  {
    return this->DisplayBounds;
  }

  inputAlg->UpdateInformation();
  int extent[6];
  vtkInformation* inputInfo =
    this->Mapper->GetInputInformation();
  inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  double spacing[3] = {1, 1, 1};
  if (inputInfo->Has(vtkDataObject::SPACING()))
  {
    inputInfo->Get(vtkDataObject::SPACING(), spacing);
  }
  double origin[3] = {0, 0, 0};
  if (inputInfo->Has(vtkDataObject::ORIGIN()))
  {
    inputInfo->Get(vtkDataObject::ORIGIN(), origin);
  }

  // if the display extent has not been set, use first slice
  extent[5] = extent[4];

  if (this->DisplayExtent[0] <= this->DisplayExtent[1])
  {
    extent[0] = this->DisplayExtent[0];
    extent[1] = this->DisplayExtent[1];
    extent[2] = this->DisplayExtent[2];
    extent[3] = this->DisplayExtent[3];
    extent[4] = this->DisplayExtent[4];
    extent[5] = this->DisplayExtent[5];
  }

  if (spacing[0] >= 0)
  {
    this->DisplayBounds[0] = extent[0]*spacing[0] + origin[0];
    this->DisplayBounds[1] = extent[1]*spacing[0] + origin[0];
  }
  else
  {
    this->DisplayBounds[0] = extent[1]*spacing[0] + origin[0];
    this->DisplayBounds[1] = extent[0]*spacing[0] + origin[0];
  }
  if (spacing[1] >= 0)
  {
    this->DisplayBounds[2] = extent[2]*spacing[1] + origin[1];
    this->DisplayBounds[3] = extent[3]*spacing[1] + origin[1];
  }
  else
  {
    this->DisplayBounds[2] = extent[3]*spacing[1] + origin[1];
    this->DisplayBounds[3] = extent[2]*spacing[1] + origin[1];
  }
  if (spacing[2] >= 0)
  {
    this->DisplayBounds[4] = extent[4]*spacing[2] + origin[2];
    this->DisplayBounds[5] = extent[5]*spacing[2] + origin[2];
  }
  else
  {
    this->DisplayBounds[4] = extent[5]*spacing[2] + origin[2];
    this->DisplayBounds[5] = extent[4]*spacing[2] + origin[2];
  }

  return this->DisplayBounds;
}

//----------------------------------------------------------------------------
// Get the bounds for the displayed data as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkImageActor::GetDisplayBounds(double bounds[6])
{
  this->GetDisplayBounds();
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->DisplayBounds[i];
  }
}

//----------------------------------------------------------------------------
// Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImageActor::GetBounds()
{
  int i,n;
  double *bounds, bbox[24], *fptr;

  bounds = this->GetDisplayBounds();
  // Check for the special case when the data bounds are unknown
  if (!bounds)
  {
    return bounds;
  }

  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

  // make sure matrix (transform) is up-to-date
  this->ComputeMatrix();

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++)
  {
    double homogeneousPt[4] = {fptr[0], fptr[1], fptr[2], 1.0};
    this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
    fptr[0] = homogeneousPt[0] / homogeneousPt[3];
    fptr[1] = homogeneousPt[1] / homogeneousPt[3];
    fptr[2] = homogeneousPt[2] / homogeneousPt[3];
    fptr += 3;
  }

  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  for (i = 0; i < 8; i++)
  {
    for (n = 0; n < 3; n++)
    {
      if (bbox[i*3+n] < this->Bounds[n*2])
      {
        this->Bounds[n*2] = bbox[i*3+n];
      }
      if (bbox[i*3+n] > this->Bounds[n*2+1])
      {
        this->Bounds[n*2+1] = bbox[i*3+n];
      }
    }
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
int vtkImageActor::GetOrientationFromExtent(const int extent[6])
{
  int orientation = 2;

  if (extent[4] == extent[5])
  {
    orientation = 2;
  }
  else if (extent[2] == extent[3])
  {
    orientation = 1;
  }
  else if (extent[0] == extent[1])
  {
    orientation = 0;
  }

  return orientation;
}

//----------------------------------------------------------------------------
void vtkImageActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ForceOpaque: "
     << (this->ForceOpaque ? "On\n" : "Off\n");

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Interpolate: " << (this->GetInterpolate() ? "On\n" : "Off\n");
  os << indent << "Opacity: " << this->GetOpacity() << "\n";

  os << indent << "DisplayExtent: (" << this->DisplayExtent[0];
  for (int idx = 1; idx < 6; ++idx)
  {
    os << ", " << this->DisplayExtent[idx];
  }
  os << ")\n";
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMin()
{
  int *extent;

  if ( ! this->GetInputAlgorithm())
  {
    return 0;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->Mapper->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[4];
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMax()
{
  int *extent;

  if ( ! this->GetInputAlgorithm())
  {
    return 0;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->Mapper->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[5];
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkImageActor::HasTranslucentPolygonalGeometry()
{
  if (this->ForceOpaque)
  {
    return 0;
  }

  if (this->ForceTranslucent)
  {
    return 1;
  }

  // Always consider translucent if opacity is less than unity
  if (this->GetOpacity() < 1.0)
  {
    return 1;
  }

  // Otherwise check the scalar information, and if the image has
  // color scalars (i.e. type is unsigned char) and if it has an
  // alpha channel (4-component RGBA, or 2-component Luminance + Alpha),
  // then we "guess" that it is meant to be translucent.  This is for
  // backwards compatibility, note that the newer vtkImageSlice class
  // does not do this check.

  if (!this->Mapper || this->Mapper->GetNumberOfInputConnections(0) == 0)
  {
    return 0;
  }

  vtkAlgorithm *inputAlg = this->Mapper->GetInputAlgorithm();
  if (!inputAlg)
  {
    return 0;
  }

  // This MTime check is the same as done in vtkTexture
  if (this->GetMTime() < this->TranslucentComputationTime)
  {
    vtkImageData *input = this->GetInput();
    if (input == NULL ||
        input->GetMTime() <= this->TranslucentComputationTime)
    {
      return this->TranslucentCachedResult;
    }
  }

  int scalarType = VTK_VOID;
  int numComp = 1;

  vtkInformation *inputInfo = this->Mapper->GetInputInformation();
  inputAlg->UpdateInformation();

  // Get the information for the image scalars
  vtkInformation *scalarInfo =
    vtkDataObject::GetActiveFieldInformation(
       inputInfo,
       vtkDataObject::FIELD_ASSOCIATION_POINTS,
       vtkDataSetAttributes::SCALARS);

  if (scalarInfo)
  {
    if (scalarInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()))
    {
      scalarType = scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
    }
    if (scalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      numComp = scalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    }
  }

  this->TranslucentCachedResult = (scalarType == VTK_UNSIGNED_CHAR &&
                                   numComp % 2 == 0);
  this->TranslucentComputationTime.Modified();

  return this->TranslucentCachedResult;
}
