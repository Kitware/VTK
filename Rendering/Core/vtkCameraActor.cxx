/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCameraActor.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFrustumSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkCameraActor);
vtkCxxSetObjectMacro(vtkCameraActor, Camera, vtkCamera);

// ----------------------------------------------------------------------------
vtkCameraActor::vtkCameraActor()
{
  this->Camera = nullptr;
  this->WidthByHeightRatio = 1.0;
  this->FrustumSource = nullptr;
  this->FrustumMapper = nullptr;
  this->FrustumActor = nullptr;
}

// ----------------------------------------------------------------------------
vtkCameraActor::~vtkCameraActor()
{
  this->SetCamera(nullptr);

  if (this->FrustumActor != nullptr)
  {
    this->FrustumActor->Delete();
  }

  if (this->FrustumMapper != nullptr)
  {
    this->FrustumMapper->Delete();
  }
  if (this->FrustumSource != nullptr)
  {
    this->FrustumSource->Delete();
  }
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int vtkCameraActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->UpdateViewProps();

  int result = 0;
  if (this->FrustumActor != nullptr && this->FrustumActor->GetMapper() != nullptr)
  {
    result = this->FrustumActor->RenderOpaqueGeometry(viewport);
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
vtkTypeBool vtkCameraActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkCameraActor::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->FrustumActor != nullptr)
  {
    this->FrustumActor->ReleaseGraphicsResources(window);
  }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkCameraActor::GetBounds()
{
  // we cannot initialize the Bounds the same way vtkBoundingBox does because
  // vtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As vtkMath::UninitializeBounds initialized finite unvalid bounds, it
  // passes silently and GetLength() returns 0.
  vtkMath::UninitializeBounds(this->Bounds);

  this->UpdateViewProps();
  if (this->FrustumActor != nullptr && this->FrustumActor->GetUseBounds())
  {
    this->FrustumActor->GetBounds(this->Bounds);
  }
  return this->Bounds;
}

//-------------------------------------------------------------------------
vtkMTimeType vtkCameraActor::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Camera != nullptr)
  {
    vtkMTimeType time;
    time = this->Camera->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }
  return mTime;
}

// ----------------------------------------------------------------------------
// Description:
// Get property of the internal actor.
vtkProperty* vtkCameraActor::GetProperty()
{
  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = vtkActor::New();
  }

  return this->FrustumActor->GetProperty();
}

// ----------------------------------------------------------------------------
// Description:
// Set property of the internal actor.
void vtkCameraActor::SetProperty(vtkProperty* p)
{
  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = vtkActor::New();
  }

  this->FrustumActor->SetProperty(p);
}

// ----------------------------------------------------------------------------
void vtkCameraActor::UpdateViewProps()
{
  if (this->Camera == nullptr)
  {
    vtkDebugMacro(<< "no camera to represent.");
    return;
  }

  vtkPlanes* planes = nullptr;
  if (this->FrustumSource == nullptr)
  {
    this->FrustumSource = vtkFrustumSource::New();
    planes = vtkPlanes::New();
    this->FrustumSource->SetPlanes(planes);
    planes->Delete();
  }
  else
  {
    planes = this->FrustumSource->GetPlanes();
  }

  double coefs[24];
  this->Camera->GetFrustumPlanes(this->WidthByHeightRatio, coefs);
  planes->SetFrustumPlanes(coefs);

  this->FrustumSource->SetShowLines(false);

  if (this->FrustumMapper == nullptr)
  {
    this->FrustumMapper = vtkPolyDataMapper::New();
  }

  this->FrustumMapper->SetInputConnection(this->FrustumSource->GetOutputPort());

  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = vtkActor::New();
  }

  this->FrustumActor->SetMapper(this->FrustumMapper);

  vtkProperty* p = this->FrustumActor->GetProperty();
  p->SetRepresentationToWireframe();
  this->FrustumActor->SetVisibility(1);
}

//-------------------------------------------------------------------------
void vtkCameraActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Camera: ";
  if (this->Camera == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->Camera->PrintSelf(os, indent);
  }

  os << indent << "WidthByHeightRatio: " << this->WidthByHeightRatio << endl;
}
