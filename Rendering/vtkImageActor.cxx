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

#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkImageActor);

//----------------------------------------------------------------------------
vtkImageActor* vtkImageActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkImageActor");
  return static_cast<vtkImageActor *>(ret);
}

vtkImageActor::vtkImageActor()
{
  this->Input = NULL;
  this->Interpolate = 1;
  this->Opacity = 1.0;
  this->DisplayExtent[0] = -1;
  this->DisplayExtent[1] = 0;
  this->DisplayExtent[2] = 0;
  this->DisplayExtent[3] = 0;
  this->DisplayExtent[4] = 0;
  this->DisplayExtent[5] = 0;  
  this->ComputedDisplayExtent[0] = 0;
  this->ComputedDisplayExtent[1] = 0;
  this->ComputedDisplayExtent[2] = 0;
  this->ComputedDisplayExtent[3] = 0;
  this->ComputedDisplayExtent[4] = 0;
  this->ComputedDisplayExtent[5] = 0;

  vtkMath::UninitializeBounds(this->DisplayBounds);
}

vtkImageActor::~vtkImageActor()
{
  if (this->Input)
    {
    this->GetInput()->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumber()
{
  // find the first axis with a one pixel extent and return
  // its value

  if (this->ComputedDisplayExtent[0] == this->ComputedDisplayExtent[1])
    {
    return this->ComputedDisplayExtent[0];
    }
  if (this->ComputedDisplayExtent[2] == this->ComputedDisplayExtent[3])
    {
    return this->ComputedDisplayExtent[2];
    }
  return this->ComputedDisplayExtent[4];
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMax()
{
  if (!this->GetInput())
    {
    return 0;
    }

  this->GetInput()->UpdateInformation();
  int *wextent = this->GetInput()->GetWholeExtent();

  // find the first axis with a one pixel extent and return
  // its value
  if (this->ComputedDisplayExtent[0] == this->ComputedDisplayExtent[1])
    {
    return wextent[1];
    }
  if (this->ComputedDisplayExtent[2] == this->ComputedDisplayExtent[3])
    {
    return wextent[3];
    }
  return wextent[5];
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMin()
{
  if (!this->GetInput())
    {
    return 0;
    }

  this->GetInput()->UpdateInformation();
  int *wextent = this->GetInput()->GetWholeExtent();

  // find the first axis with a one pixel extent and return
  // its value
  if (this->ComputedDisplayExtent[0] == this->ComputedDisplayExtent[1])
    {
    return wextent[0];
    }
  if (this->ComputedDisplayExtent[2] == this->ComputedDisplayExtent[3])
    {
    return wextent[2];
    }
  return wextent[4];
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
    for (idx = 0; idx < 6; ++idx)
      {
      this->ComputedDisplayExtent[idx] = extent[idx];
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
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->DisplayExtent[idx];
    }
}

//-----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int vtkImageActor::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageActor::RenderTranslucentPolygonalGeometry");

  vtkImageData *input = this->GetInput();
  if (!input)
    {
    return 0;
    }

  // render the texture map
  if ( input->GetScalarType() == VTK_UNSIGNED_CHAR )
    {
    if (!(this->Opacity >= 1.0 && input->GetNumberOfScalarComponents() % 2))
      {
      this->Render(vtkRenderer::SafeDownCast(viewport));
      return 1;
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkImageActor::HasTranslucentPolygonalGeometry()
{
  vtkImageData *input = this->GetInput();
  if (!input)
    {
    return 0;
    }

  // render the texture map
  if ( input->GetScalarType() == VTK_UNSIGNED_CHAR )
    {
    if (!(this->Opacity >= 1.0 && input->GetNumberOfScalarComponents() % 2))
      {
      return 1;
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkImageActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageActor::RenderOpaqueGeometry");

  vtkImageData *input = this->GetInput();
  if (!input)
    {
    return 0;
    }
  // make sure the data is available
  input->UpdateInformation();

  // if the display extent has not been set, then compute one
  int *wExtent = input->GetWholeExtent();
  if (this->DisplayExtent[0] == -1)
    {
    this->ComputedDisplayExtent[0] = wExtent[0];
    this->ComputedDisplayExtent[1] = wExtent[1];
    this->ComputedDisplayExtent[2] = wExtent[2];
    this->ComputedDisplayExtent[3] = wExtent[3];
    this->ComputedDisplayExtent[4] = wExtent[4];
    this->ComputedDisplayExtent[5] = wExtent[4];
    }
  input->SetUpdateExtent(this->ComputedDisplayExtent);
  input->PropagateUpdateExtent();
  input->UpdateData();

  // render the texture map
  if ( input->GetScalarType() == VTK_UNSIGNED_CHAR )
    {
    if (this->Opacity >= 1.0 && input->GetNumberOfScalarComponents() % 2)
      {
      this->Render(vtkRenderer::SafeDownCast(viewport));
      return 1;
      }
    }
  else
    {
    vtkErrorMacro(<<"This filter requires unsigned char scalars as input");
    }

  return 0;
}

// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImageActor::GetDisplayBounds()
{
  if (!this->Input)
    {
    return this->DisplayBounds;
    }
  this->Input->UpdateInformation();
  double *spacing = this->Input->GetSpacing();
  double *origin = this->Input->GetOrigin();

  // if the display extent has not been set, then compute one
  int *wExtent = this->Input->GetWholeExtent();
  if (this->DisplayExtent[0] == -1)
    {
    this->ComputedDisplayExtent[0] = wExtent[0];
    this->ComputedDisplayExtent[1] = wExtent[1];
    this->ComputedDisplayExtent[2] = wExtent[2];
    this->ComputedDisplayExtent[3] = wExtent[3];
    this->ComputedDisplayExtent[4] = wExtent[4];
    this->ComputedDisplayExtent[5] = wExtent[4];
    }
  if (spacing[0] >= 0)
    {
    this->DisplayBounds[0] =
      this->ComputedDisplayExtent[0]*spacing[0] + origin[0];
    this->DisplayBounds[1] =
      this->ComputedDisplayExtent[1]*spacing[0] + origin[0];
    }
  else
    {
    this->DisplayBounds[0] =
      this->ComputedDisplayExtent[1]*spacing[0] + origin[0];
    this->DisplayBounds[1] =
      this->ComputedDisplayExtent[0]*spacing[0] + origin[0];
    }
  if (spacing[1] >= 0)
    {
    this->DisplayBounds[2] =
      this->ComputedDisplayExtent[2]*spacing[1] + origin[1];
    this->DisplayBounds[3] =
      this->ComputedDisplayExtent[3]*spacing[1] + origin[1];
    }
  else
    {
    this->DisplayBounds[2] =
      this->ComputedDisplayExtent[3]*spacing[1] + origin[1];
    this->DisplayBounds[3] =
      this->ComputedDisplayExtent[2]*spacing[1] + origin[1];
    }
  if (spacing[2] >= 0)
    {
    this->DisplayBounds[4] =
      this->ComputedDisplayExtent[4]*spacing[2] + origin[2];
    this->DisplayBounds[5] =
      this->ComputedDisplayExtent[5]*spacing[2] + origin[2];
    }
  else
    {
    this->DisplayBounds[4] =
      this->ComputedDisplayExtent[5]*spacing[2] + origin[2];
    this->DisplayBounds[5] =
      this->ComputedDisplayExtent[4]*spacing[2] + origin[2];
    }
  
  return this->DisplayBounds;
}

// Get the bounds for the displayed data as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkImageActor::GetDisplayBounds(double bounds[6])
{
  this->GetDisplayBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->DisplayBounds[i];
    }
}

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
  
  // save the old transform
  this->Transform->Push();
  this->Transform->SetMatrix(this->GetMatrix());

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++) 
    {
    this->Transform->TransformPoint(fptr,fptr);
    fptr += 3;
    }
  
  this->Transform->Pop();  
  
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

void vtkImageActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
  os << indent << "Opacity: " << this->Opacity << "\n";

  int idx;  
  os << indent << "DisplayExtent: (" << this->DisplayExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DisplayExtent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMin()
{
  int *extent;
  
  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInput()->UpdateInformation();
  extent = this->GetInput()->GetWholeExtent();
  return extent[4];
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMax()
{
  int *extent;
  
  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInput()->UpdateInformation();
  extent = this->GetInput()->GetWholeExtent();
  return extent[5];
}

void vtkImageActor::SetInput(vtkImageData *args)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this         
  << "): setting Input to " << args );     
  if (this->Input != args)                                       
    {                                                           
    if (this->Input != NULL) 
      { 
      this->Input->UnRegister(this); 
      }   
    this->Input = args;                                          
    if (this->Input != NULL) 
      { 
      this->Input->Register(this); 
      }     
    this->Modified();                                           
    }                                                           
}

