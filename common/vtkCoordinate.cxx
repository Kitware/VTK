/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinate.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkCoordinate.h"
#include "vtkViewport.h"

#define VTK_RINT(x) ((x > 0.0) ? (int)(x + 0.5) : (int)(x - 0.5))

// Description:
// Creates an Coordinate with the following defaults: 
// value of  0, 0, 0 in world  coordinates
vtkCoordinate::vtkCoordinate()
{
  this->CoordinateSystem = VTK_WORLD;
  this->Value[0] = 0.0;
  this->Value[1] = 0.0;
  this->Value[2] = 0.0;
  this->Viewport = NULL;
  this->ReferenceCoordinate = NULL;
  this->Computing = 0;
}

// Description:
// Destroy a Coordinate.  
vtkCoordinate::~vtkCoordinate()
{
  this->SetReferenceCoordinate(NULL);
}

void vtkCoordinate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkReferenceCount::PrintSelf(os,indent);

  char posString[64];
  switch (this->CoordinateSystem)
    {
    case VTK_DISPLAY:
      strcpy(posString, "Display"); break;
    case VTK_NORMALIZED_DISPLAY:
      strcpy(posString, "Normalized Display"); break;
    case VTK_VIEWPORT:
      strcpy(posString, "Viewport"); break;
    case VTK_NORMALIZED_VIEWPORT:
      strcpy(posString, "Normalized Viewport"); break;
    case VTK_VIEW:
      strcpy(posString, "View"); break;
    case VTK_WORLD:
      strcpy(posString, "World"); break;
    default:
	strcpy(posString, "UNKNOWN!"); break;
    }

  os << indent << "Coordinate System: " << posString << "\n";
  os << indent << "Value: (" << this->Value[0] << "," 
     << this->Value[1] << "," << this->Value[2] << ")\n";
  os << indent << "ReferenceCoordinate: " << this->ReferenceCoordinate << "\n";
  os << indent << "Viewport: " << this->Viewport << "\n";
}

float *vtkCoordinate::GetComputedWorldValue(vtkViewport* viewport)
{
  float *val = this->ComputedWorldValue;
  
  // prevent infinite loops
  if (this->Computing)
    {
    return val;
    }
  this->Computing = 1;

  val[0] = this->Value[0];
  val[1] = this->Value[1];
  val[2] = this->Value[2];

  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }
  
  // if viewport is NULL then we can only do minimal calculations
  if (!viewport)
    {
    if (this->CoordinateSystem == VTK_WORLD)
      {
      if (this->ReferenceCoordinate)
	{
	float *RefValue;
	
	RefValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
	val[0] += RefValue[0];
	val[1] += RefValue[1];
	val[2] += RefValue[2];
	}
      this->Computing = 0;
      }
    else
      {
      vtkErrorMacro("Attempt to compute world coordinates from another coordinate system without a viewport");
      }
    return val;
    }
  
  if (this->ReferenceCoordinate && this->CoordinateSystem != VTK_WORLD)
    {
    float RefValue[3];
    int *ival;
    
    ival = this->ReferenceCoordinate->GetComputedDisplayValue(viewport);
    RefValue[0] = (float)(ival[0]);
    RefValue[1] = (float)(ival[1]);
    RefValue[2] = (float)(ival[2]);
    
    // convert to current coordinate system
    switch (this->CoordinateSystem)
      {
      case VTK_NORMALIZED_DISPLAY:
	viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
      case VTK_VIEWPORT:
	viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
      case VTK_NORMALIZED_VIEWPORT:
	viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
	viewport->ViewportToNormalizedViewport(RefValue[0],RefValue[1]);
      case VTK_VIEW:
	viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
	viewport->ViewportToNormalizedViewport(RefValue[0],RefValue[1]);
	viewport->NormalizedViewportToView(RefValue[0],RefValue[1],RefValue[2]);
      }
    
    // add to current value
    val[0] += RefValue[0];
    val[1] += RefValue[1];
    val[2] += RefValue[2];
    }
  
  // compute our WC
  switch (this->CoordinateSystem)
    {
    case VTK_DISPLAY:
      viewport->DisplayToNormalizedDisplay(val[0],val[1]);
    case VTK_NORMALIZED_DISPLAY:
      viewport->NormalizedDisplayToViewport(val[0],val[1]);
    case VTK_VIEWPORT:
      viewport->ViewportToNormalizedViewport(val[0],val[1]);
    case VTK_NORMALIZED_VIEWPORT:
      viewport->NormalizedViewportToView(val[0],val[1],val[2]);
    case VTK_VIEW:
      viewport->ViewToWorld(val[0],val[1],val[2]);
    }
  
  if (this->ReferenceCoordinate && this->CoordinateSystem == VTK_WORLD)
    {
    float *RefValue;
    
    RefValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
    val[0] += RefValue[0];
    val[1] += RefValue[1];
    val[2] += RefValue[2];
    }
  
  this->Computing = 0;
  vtkDebugMacro("Returning WorldValue of : " << 
		this->ComputedWorldValue[0] << " , " << 
		this->ComputedWorldValue[1] << " , " << 
		this->ComputedWorldValue[2]);
  return val;
}



int *vtkCoordinate::GetComputedViewportValue(vtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }

  int *d = this->GetComputedDisplayValue(viewport);

  if (!viewport)
    {
    vtkDebugMacro("Attempt to convert to compute viewport coordinates without a viewport, results may not be valid");
    return this->ComputedDisplayValue;
    }

  float f[2];
  
  f[0] = (float)d[0];
  f[1] = (float)d[1];
  
  viewport->DisplayToNormalizedDisplay(f[0],f[1]);
  viewport->NormalizedDisplayToViewport(f[0],f[1]);
  
  this->ComputedViewportValue[0] = (int)VTK_RINT(f[0]);
  this->ComputedViewportValue[1] = (int)VTK_RINT(f[1]);

  return this->ComputedViewportValue;
}

int *vtkCoordinate::GetComputedLocalDisplayValue(vtkViewport* viewport)
{
  float a[2];

  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }
  this->GetComputedDisplayValue(viewport);

  if (!viewport)
    {
    vtkErrorMacro("Attempt to convert to local display coordinates without a viewport");
    return this->ComputedDisplayValue;
    }
    
  a[0] = (float)this->ComputedDisplayValue[0];
  a[1] = (float)this->ComputedDisplayValue[1];
  
  viewport->DisplayToLocalDisplay(a[0],a[1]);

  this->ComputedDisplayValue[0] = (int)VTK_RINT(a[0]);
  this->ComputedDisplayValue[1] = (int)VTK_RINT(a[1]);

  vtkDebugMacro("Returning LocalDisplayValue of : " << 
		this->ComputedDisplayValue[0] << " , " << 
		this->ComputedDisplayValue[1]);
  
  return this->ComputedDisplayValue;
}

int *vtkCoordinate::GetComputedDisplayValue(vtkViewport* viewport)
{
  float val[3];

  // prevent infinite loops
  if (this->Computing)
    {
    return this->ComputedDisplayValue;
    }
  this->Computing = 1;

  val[0] = this->Value[0];
  val[1] = this->Value[1];
  val[2] = this->Value[2];

  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }

  // if viewport is NULL, there is very little we can do
  if (viewport == NULL)
    {
    // for DISPLAY and VIEWPORT just use the value
    if (this->CoordinateSystem == VTK_DISPLAY ||
	this->CoordinateSystem == VTK_VIEWPORT)
      {
      this->ComputedDisplayValue[0] = (int)VTK_RINT(val[0]);
      this->ComputedDisplayValue[1] = (int)VTK_RINT(val[1]);
      if (this->ReferenceCoordinate)
	{
	int *RefValue;
	
	RefValue = this->ReferenceCoordinate->GetComputedDisplayValue(viewport);
	this->ComputedDisplayValue[0] += RefValue[0];
	this->ComputedDisplayValue[1] += RefValue[1];
	}
      }
    else
      {
      vtkErrorMacro("Request for coordinate transformation without required viewport");
      }
    return this->ComputedDisplayValue;
    }
  
  // compute our WC
  switch (this->CoordinateSystem)
    {
    case VTK_WORLD:
      if (this->ReferenceCoordinate)
	{
	float *RefValue;
	
	RefValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
	val[0] += RefValue[0];
	val[1] += RefValue[1];
	val[2] += RefValue[2];
	}
      viewport->WorldToView(val[0],val[1],val[2]);
    case VTK_VIEW:
      viewport->ViewToNormalizedViewport(val[0],val[1],val[2]);
    case VTK_NORMALIZED_VIEWPORT:
      viewport->NormalizedViewportToViewport(val[0],val[1]);
    case VTK_VIEWPORT:
      viewport->ViewportToNormalizedDisplay(val[0],val[1]);
    case VTK_NORMALIZED_DISPLAY:
      viewport->NormalizedDisplayToDisplay(val[0],val[1]);
    }
  
  this->ComputedDisplayValue[0] = (int)VTK_RINT(val[0]);
  this->ComputedDisplayValue[1] = (int)VTK_RINT(val[1]);
  
  // if we have a reference coordinate then get that first
  if (this->ReferenceCoordinate && this->CoordinateSystem != VTK_WORLD)
    {
    int *RefValue;
    
    RefValue = this->ReferenceCoordinate->GetComputedDisplayValue(viewport);
    this->ComputedDisplayValue[0] += RefValue[0];
    this->ComputedDisplayValue[1] += RefValue[1];
    }
  
  this->Computing = 0;
  vtkDebugMacro("Returning DisplayValue of : " << 
		this->ComputedDisplayValue[0] << " , " << 
		this->ComputedDisplayValue[1]);
  return this->ComputedDisplayValue;
}





float *vtkCoordinate::GetComputedValue(vtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }
  
  switch (this->CoordinateSystem)
    {
    case VTK_WORLD:
      return this->GetComputedWorldValue(viewport);
    case VTK_VIEW:
    case VTK_NORMALIZED_VIEWPORT:
    case VTK_VIEWPORT:
      {
      // result stored in computed world value due to float
      // but is really a viewport value
      int *v = this->GetComputedViewportValue(viewport);
      this->ComputedWorldValue[0] = v[0];
      this->ComputedWorldValue[1] = v[1];
      break;
      }
    case VTK_NORMALIZED_DISPLAY:
    case VTK_DISPLAY:
      {
      // result stored in computed world value due to float
      // but is really a display value
      int *d = this->GetComputedDisplayValue(viewport);
      this->ComputedWorldValue[0] = d[0];
      this->ComputedWorldValue[1] = d[1];
      break;
      }
    }

  return this->ComputedWorldValue;
}

