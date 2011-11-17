/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoordinate.h"
#include "vtkViewport.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCoordinate);

vtkCxxSetObjectMacro(vtkCoordinate,ReferenceCoordinate,vtkCoordinate);

#define VTK_RINT(x) ((x > 0.0) ? static_cast<int>(x + 0.5) : static_cast<int>(x - 0.5))

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Destroy a Coordinate.
vtkCoordinate::~vtkCoordinate()
{
  // To get rid of references (Reference counting).
  this->SetReferenceCoordinate(NULL);
}

//----------------------------------------------------------------------------
// Set the viewport. This is a raw pointer, not a weak pointer or a reference
// counted object to avoid cycle reference loop between rendering classes
// and filter classes.
void vtkCoordinate::SetViewport(vtkViewport *viewport)
{
  if(this->Viewport!=viewport)
    {
    this->Viewport=viewport;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkCoordinate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Coordinate System: " << 
    this->GetCoordinateSystemAsString() << "\n";
  os << indent << "Value: (" << this->Value[0] << ","
     << this->Value[1] << "," << this->Value[2] << ")\n";
  if (this->ReferenceCoordinate)
    {
    os << indent << "ReferenceCoordinate: " << 
      this->ReferenceCoordinate << "\n";
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

//----------------------------------------------------------------------------
double *vtkCoordinate::GetComputedWorldValue(vtkViewport* viewport)
{
  double *val = this->ComputedWorldValue;
  
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
        double *refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
        val[2] += refValue[2];
        }
      this->Computing = 0;
      }
    else
      {
      vtkErrorMacro(
        "Attempt to compute world coordinates from another coordinate system without a viewport");
      }
    return val;
    }

  if (this->ReferenceCoordinate && this->CoordinateSystem != VTK_WORLD)
    {
    double refValue[3];
    double *fval;

    fval = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
    refValue[0] = fval[0];
    refValue[1] = fval[1];
    refValue[2] = 0.0;

    // convert to current coordinate system
    switch (this->CoordinateSystem)
      {
      case VTK_NORMALIZED_DISPLAY:
        viewport->DisplayToNormalizedDisplay(refValue[0],refValue[1]);
        break;
      case VTK_VIEWPORT:
        viewport->DisplayToNormalizedDisplay(refValue[0],refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0],refValue[1]);
        break;
      case VTK_NORMALIZED_VIEWPORT:
        viewport->DisplayToNormalizedDisplay(refValue[0],refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0],refValue[1]);
        viewport->ViewportToNormalizedViewport(refValue[0],refValue[1]);
        break;
      case VTK_VIEW:
        viewport->DisplayToNormalizedDisplay(refValue[0],refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0],refValue[1]);
        viewport->ViewportToNormalizedViewport(refValue[0],refValue[1]);
        viewport->NormalizedViewportToView(refValue[0],
                                           refValue[1],
                                           refValue[2]);
        break;
      }

    // add to current value
    val[0] += refValue[0];
    val[1] += refValue[1];
    val[2] += refValue[2];
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
    double *refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
    val[0] += refValue[0];
    val[1] += refValue[1];
    val[2] += refValue[2];
    }

  this->Computing = 0;
  vtkDebugMacro("Returning WorldValue of : " <<
                this->ComputedWorldValue[0] << " , " <<
                this->ComputedWorldValue[1] << " , " <<
                this->ComputedWorldValue[2]);
  return val;
}


//----------------------------------------------------------------------------
double *vtkCoordinate::GetComputedDoubleViewportValue(vtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
    {
    viewport = this->Viewport;
    }

  double *d = this->GetComputedDoubleDisplayValue(viewport);

  if (!viewport)
    {
    vtkDebugMacro(
      "Attempt to convert to compute viewport coordinates without a viewport, results may not be valid");
    return d;
    }

  double f[2];
  f[0] = d[0];
  f[1] = d[1];

  viewport->DisplayToNormalizedDisplay(f[0],f[1]);
  viewport->NormalizedDisplayToViewport(f[0],f[1]);

  this->ComputedDoubleViewportValue[0] = f[0];
  this->ComputedDoubleViewportValue[1] = f[1];

  return this->ComputedDoubleViewportValue;
}

//----------------------------------------------------------------------------
int *vtkCoordinate::GetComputedViewportValue(vtkViewport* viewport)
{
  double *f = this->GetComputedDoubleViewportValue(viewport);

  this->ComputedViewportValue[0] = static_cast<int>(VTK_RINT(f[0]));
  this->ComputedViewportValue[1] = static_cast<int>(VTK_RINT(f[1]));

  return this->ComputedViewportValue;
}

//----------------------------------------------------------------------------
int *vtkCoordinate::GetComputedLocalDisplayValue(vtkViewport* viewport)
{
  double a[2];

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

  a[0] = static_cast<double>(this->ComputedDisplayValue[0]);
  a[1] = static_cast<double>(this->ComputedDisplayValue[1]);

  viewport->DisplayToLocalDisplay(a[0],a[1]);

this->ComputedDisplayValue[0] = static_cast<int>(VTK_RINT(a[0]));
this->ComputedDisplayValue[1] = static_cast<int>(VTK_RINT(a[1]));

  vtkDebugMacro("Returning LocalDisplayValue of : " <<
                this->ComputedDisplayValue[0] << " , " <<
                this->ComputedDisplayValue[1]);

  return this->ComputedDisplayValue;
}

//----------------------------------------------------------------------------
double *vtkCoordinate::GetComputedDoubleDisplayValue(vtkViewport* viewport)
{
  double val[3];

  // prevent infinite loops
  if (this->Computing)
    {
    return this->ComputedDoubleDisplayValue;
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
      this->ComputedDoubleDisplayValue[0] = val[0];
      this->ComputedDoubleDisplayValue[1] = val[1];
      if (this->ReferenceCoordinate)
        {
        double *refValue = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
        this->ComputedDoubleDisplayValue[0] += refValue[0];
        this->ComputedDoubleDisplayValue[1] += refValue[1];
        }
      }
    else
      {
      this->ComputedDoubleDisplayValue[0] = static_cast<double>(VTK_LARGE_INTEGER);
      this->ComputedDoubleDisplayValue[1] = static_cast<double>(VTK_LARGE_INTEGER);

      vtkErrorMacro("Request for coordinate transformation without required viewport");
      }
    return this->ComputedDoubleDisplayValue;
    }

  // compute our DC
  switch (this->CoordinateSystem)
    {
    case VTK_WORLD:
      if (this->ReferenceCoordinate)
        {
        double *refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
        val[2] += refValue[2];
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
        double *refValue = this->ReferenceCoordinate->GetComputedDoubleViewportValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
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
    double *refValue = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
    val[0] += refValue[0];
    val[1] += refValue[1];
    }
  this->ComputedDoubleDisplayValue[0] = val[0];
  this->ComputedDoubleDisplayValue[1] = val[1];

  this->Computing = 0;
  return this->ComputedDoubleDisplayValue;
}


//----------------------------------------------------------------------------
int *vtkCoordinate::GetComputedDisplayValue(vtkViewport* viewport)
{
  double *val = this->GetComputedDoubleDisplayValue(viewport);

  this->ComputedDisplayValue[0] = static_cast<int>(val[0]);
  this->ComputedDisplayValue[1] = static_cast<int>(val[1]);

  vtkDebugMacro("Returning DisplayValue of : " <<
                this->ComputedDisplayValue[0] << " , " <<
                this->ComputedDisplayValue[1]);
  return this->ComputedDisplayValue;
}

//----------------------------------------------------------------------------
double *vtkCoordinate::GetComputedValue(vtkViewport* viewport)
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
      // result stored in computed world value due to double
      // but is really a viewport value
      int *v = this->GetComputedViewportValue(viewport);
      this->ComputedWorldValue[0] = v[0];
      this->ComputedWorldValue[1] = v[1];
      break;
      }
    case VTK_NORMALIZED_DISPLAY:
    case VTK_DISPLAY:
      {
      // result stored in computed world value due to double
      // but is really a display value
      int *d = this->GetComputedDisplayValue(viewport);
      this->ComputedWorldValue[0] = d[0];
      this->ComputedWorldValue[1] = d[1];
      break;
      }
    }

  return this->ComputedWorldValue;
}

