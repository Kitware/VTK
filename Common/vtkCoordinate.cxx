/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinate.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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

#include "vtkCoordinate.h"
#include "vtkViewport.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkCoordinate* vtkCoordinate::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCoordinate");
  if(ret)
    {
    return (vtkCoordinate*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCoordinate;
}



#define VTK_RINT(x) ((x > 0.0) ? (int)(x + 0.5) : (int)(x - 0.5))

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

// Destroy a Coordinate.
vtkCoordinate::~vtkCoordinate()
{
  // To get rid of references (Refence counting).
  this->SetReferenceCoordinate(NULL);
  this->SetViewport(NULL);
}

const char *vtkCoordinate::GetCoordinateSystemAsString()
{
  switch (this->CoordinateSystem)
    {
    case VTK_DISPLAY:
      return "Display";
    case VTK_NORMALIZED_DISPLAY:
      return "Normalized Display";
    case VTK_VIEWPORT:
      return "Viewport";
    case VTK_NORMALIZED_VIEWPORT:
      return "Normalized Viewport";
    case VTK_VIEW:
      return "View";
    case VTK_WORLD:
      return "World";
    case VTK_USERDEFINED:
      return "User Defined";
    default:
	return "UNKNOWN!";
    }
}

void vtkCoordinate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);


  os << indent << "Coordinate System: " << this->GetCoordinateSystemAsString() << "\n";
  os << indent << "Value: (" << this->Value[0] << ","
     << this->Value[1] << "," << this->Value[2] << ")\n";
  if (this->ReferenceCoordinate)
    {
    os << indent << "ReferenceCoordinate: " << this->ReferenceCoordinate << "\n";
    }
  else
    {
    os << indent << "ReferenceCoordinate: (none)\n";
    }
  if (this->Viewport)
    {
    os << indent << "Viewport: " << this->Viewport << "\n";
    }
  else
    {
    os << indent << "Viewport: (none)\n";
    }
}


void vtkCoordinate::SetViewport(vtkViewport *viewport)
{
  if (this->Viewport != viewport)
    {
    if (this->Viewport != NULL) 
      {
      this->Viewport->UnRegister(this);
      }
    this->Viewport = viewport;
    if (this->Viewport != NULL)
      {
      this->Viewport->Register(this);
      }
    this->Modified();
    }
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
    float *fval;

    fval = this->ReferenceCoordinate->GetComputedFloatDisplayValue(viewport);
    RefValue[0] = fval[0];
    RefValue[1] = fval[1];
    RefValue[2] = 0.0;

    // convert to current coordinate system
    switch (this->CoordinateSystem)
      {
      case VTK_NORMALIZED_DISPLAY:
	      viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	      break;
      case VTK_VIEWPORT:
	      viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	      viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
	      break;
      case VTK_NORMALIZED_VIEWPORT:
	      viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	      viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
	      viewport->ViewportToNormalizedViewport(RefValue[0],RefValue[1]);
	      break;
      case VTK_VIEW:
	      viewport->DisplayToNormalizedDisplay(RefValue[0],RefValue[1]);
	      viewport->NormalizedDisplayToViewport(RefValue[0],RefValue[1]);
	      viewport->ViewportToNormalizedViewport(RefValue[0],RefValue[1]);
	      viewport->NormalizedViewportToView(RefValue[0],RefValue[1],RefValue[2]);
	      break;
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


float *vtkCoordinate::GetComputedFloatViewportValue(vtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }

  float *d = this->GetComputedFloatDisplayValue(viewport);

  if (!viewport)
    {
    vtkDebugMacro("Attempt to convert to compute viewport coordinates without a viewport, results may not be valid");
    return d;
    }

  float f[2];
  f[0] = d[0];
  f[1] = d[1];

  viewport->DisplayToNormalizedDisplay(f[0],f[1]);
  viewport->NormalizedDisplayToViewport(f[0],f[1]);

  this->ComputedFloatViewportValue[0] = f[0];
  this->ComputedFloatViewportValue[1] = f[1];

  return this->ComputedFloatViewportValue;
}

int *vtkCoordinate::GetComputedViewportValue(vtkViewport* viewport)
{
  float *f = this->GetComputedFloatViewportValue(viewport);

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

float *vtkCoordinate::GetComputedFloatDisplayValue(vtkViewport* viewport)
{
  float val[3];

  // prevent infinite loops
  if (this->Computing)
    {
    return this->ComputedFloatDisplayValue;
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
    if (this->CoordinateSystem == VTK_DISPLAY)
      {
      this->ComputedFloatDisplayValue[0] = val[0];
      this->ComputedFloatDisplayValue[1] = val[1];
      if (this->ReferenceCoordinate)
	      {
	      float *RefValue;
	      RefValue = this->ReferenceCoordinate->GetComputedFloatDisplayValue(viewport);
	      this->ComputedFloatDisplayValue[0] += RefValue[0];
	      this->ComputedFloatDisplayValue[1] += RefValue[1];
	      }
      }
    else
      {
      vtkErrorMacro("Request for coordinate transformation without required viewport");
      }
    return this->ComputedFloatDisplayValue;
    }

  // compute our DC
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
      if ((this->CoordinateSystem == VTK_NORMALIZED_VIEWPORT ||
	      this->CoordinateSystem == VTK_VIEWPORT) &&
	      this->ReferenceCoordinate)
	      {
	      float *RefValue;
        RefValue =
	        this->ReferenceCoordinate->GetComputedFloatViewportValue(viewport);
	      val[0] += RefValue[0];
	      val[1] += RefValue[1];
	      }
      viewport->ViewportToNormalizedDisplay(val[0],val[1]);
    case VTK_NORMALIZED_DISPLAY:
      viewport->NormalizedDisplayToDisplay(val[0],val[1]);
      break; // do not remove this break statement!
    case VTK_USERDEFINED:
        this->GetComputedUserDefinedValue(viewport);
	      val[0] = this->ComputedUserDefinedValue[0];
	      val[1] = this->ComputedUserDefinedValue[1];
	      val[2] = this->ComputedUserDefinedValue[2];
        break;
    }

  // if we have a reference coordinate and we haven't handled it yet
  if (this->ReferenceCoordinate &&
      (this->CoordinateSystem == VTK_DISPLAY ||
       this->CoordinateSystem == VTK_NORMALIZED_DISPLAY))
    {
    float *RefValue;
    RefValue =
      this->ReferenceCoordinate->GetComputedFloatDisplayValue(viewport);
    val[0] += RefValue[0];
    val[1] += RefValue[1];
    }
  this->ComputedFloatDisplayValue[0] = val[0];
  this->ComputedFloatDisplayValue[1] = val[1];

  this->Computing = 0;
  return this->ComputedFloatDisplayValue;
}


int *vtkCoordinate::GetComputedDisplayValue(vtkViewport* viewport)
{
  float *val = this->GetComputedFloatDisplayValue(viewport);

  this->ComputedDisplayValue[0] = (int)(val[0]);
  this->ComputedDisplayValue[1] = (int)(val[1]);

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

