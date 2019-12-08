/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightActor.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCameraActor.h"
#include "vtkConeSource.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkLightActor);
vtkCxxSetObjectMacro(vtkLightActor, Light, vtkLight);

// ----------------------------------------------------------------------------
vtkLightActor::vtkLightActor()
{
  this->Light = nullptr;
  this->ClippingRange[0] = 0.5;
  this->ClippingRange[1] = 10.0;

  this->ConeSource = nullptr;
  this->ConeMapper = nullptr;
  this->ConeActor = nullptr;
  this->CameraLight = nullptr;
  this->FrustumActor = nullptr;

  this->BoundingBox = new vtkBoundingBox;
}

// ----------------------------------------------------------------------------
vtkLightActor::~vtkLightActor()
{
  this->SetLight(nullptr);
  if (this->ConeActor != nullptr)
  {
    this->ConeActor->Delete();
  }

  if (this->ConeMapper != nullptr)
  {
    this->ConeMapper->Delete();
  }

  if (this->FrustumActor != nullptr)
  {
    this->FrustumActor->Delete();
  }
  if (this->ConeSource != nullptr)
  {
    this->ConeSource->Delete();
  }
  if (this->CameraLight != nullptr)
  {
    this->CameraLight->Delete();
  }
  delete this->BoundingBox;
}

// ----------------------------------------------------------------------------
// Description:
// Set/Get the location of the near and far clipping planes along the
// direction of projection.  Both of these values must be positive.
// Initial values are  (0.5,11.0)
void vtkLightActor::SetClippingRange(double dNear, double dFar)
{
  this->ClippingRange[0] = dNear;
  this->ClippingRange[1] = dFar;
}

// ----------------------------------------------------------------------------
void vtkLightActor::SetClippingRange(const double a[2])
{
  this->SetClippingRange(a[0], a[1]);
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int vtkLightActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->UpdateViewProps();

  int result = 0;

  if (this->ConeActor != nullptr && this->ConeActor->GetMapper() != nullptr)
  {
    result = this->ConeActor->RenderOpaqueGeometry(viewport);
    result += this->FrustumActor->RenderOpaqueGeometry(viewport);
  }

  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
vtkTypeBool vtkLightActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkLightActor::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->ConeActor != nullptr)
  {
    this->ConeActor->ReleaseGraphicsResources(window);
    this->FrustumActor->ReleaseGraphicsResources(window);
  }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkLightActor::GetBounds()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;

  this->UpdateViewProps();

  this->BoundingBox->Reset();

  if (this->ConeActor != nullptr)
  {
    if (this->ConeActor->GetUseBounds())
    {
      this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
    }
    if (this->FrustumActor->GetUseBounds())
    {
      this->BoundingBox->AddBounds(this->FrustumActor->GetBounds());
    }
  }

  int i = 0;
  while (i < 6)
  {
    this->Bounds[i] = this->BoundingBox->GetBound(i);
    ++i;
  }
  if (this->Bounds[0] == VTK_DOUBLE_MAX)
  {
    // we cannot initialize the Bounds the same way vtkBoundingBox does because
    // vtkProp3D::GetLength() does not check if the Bounds are initialized or
    // not and makes a call to sqrt(). This call to sqrt with invalid values
    // would raise a floating-point overflow exception (notably on BCC).
    // As vtkMath::UninitializeBounds initialized finite unvalid bounds, it
    // passes silently and GetLength() returns 0.
    vtkMath::UninitializeBounds(this->Bounds);
  }

  return this->Bounds;
}

//-------------------------------------------------------------------------
vtkMTimeType vtkLightActor::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Light != nullptr)
  {
    vtkMTimeType time;
    time = this->Light->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }
  return mTime;
}

// ----------------------------------------------------------------------------
void vtkLightActor::UpdateViewProps()
{
  if (this->Light == nullptr)
  {
    vtkDebugMacro(<< "no light.");
    return;
  }
  double angle = this->Light->GetConeAngle();

  if (this->Light->GetPositional() && angle < 90.0)
  {
    if (this->ConeSource == nullptr)
    {
      this->ConeSource = vtkConeSource::New();
    }

    this->ConeSource->SetResolution(24);
    double* pos = this->Light->GetPosition();
    double* f = this->Light->GetFocalPoint();

    double direction[3];
    int i = 0;
    while (i < 3)
    {
      direction[i] = pos[i] - f[i];
      ++i;
    }
    double height = 1.0;
    double center[3]; //=pos

    double n = vtkMath::Norm(direction);

    // cone center is the middle of its axis, not the center of the base...
    i = 0;
    while (i < 3)
    {
      center[i] = pos[i] - 0.5 * height / n * direction[i];
      ++i;
    }
    this->ConeSource->SetCenter(center);
    this->ConeSource->SetDirection(direction);
    this->ConeSource->SetHeight(height);
    this->ConeSource->SetAngle(angle);

    if (this->ConeMapper == nullptr)
    {
      this->ConeMapper = vtkPolyDataMapper::New();
      this->ConeMapper->SetInputConnection(this->ConeSource->GetOutputPort());
      this->ConeMapper->SetScalarVisibility(0);
    }

    if (this->ConeActor == nullptr)
    {
      this->ConeActor = vtkActor::New();
      this->ConeActor->SetMapper(this->ConeMapper);
    }

    this->ConeActor->SetVisibility(this->Light->GetSwitch());

    vtkProperty* p = this->ConeActor->GetProperty();
    p->SetLighting(false);
    p->SetColor(this->Light->GetDiffuseColor());
    p->SetRepresentationToWireframe();

    if (this->CameraLight == nullptr)
    {
      this->CameraLight = vtkCamera::New();
    }

    this->CameraLight->SetPosition(this->Light->GetPosition());
    this->CameraLight->SetFocalPoint(this->Light->GetFocalPoint());
    this->CameraLight->SetViewUp(0.0, 1.0, 0.0);
    // view angle is an aperture, but cone (or light) angle is between
    // the axis of the cone and a ray along the edge of the cone.
    this->CameraLight->SetViewAngle(angle * 2.0);
    // initial clip=(0.1,1000). near>0, far>near);
    this->CameraLight->SetClippingRange(this->ClippingRange);

    if (this->FrustumActor == nullptr)
    {
      this->FrustumActor = vtkCameraActor::New();
    }
    this->FrustumActor->SetCamera(this->CameraLight);
    this->FrustumActor->SetWidthByHeightRatio(1.0); // camera light is square
    this->FrustumActor->SetUseBounds(false);
  }
  else
  {
    if (this->ConeActor)
    {
      this->ConeActor->SetMapper(nullptr);
    }
    if (this->FrustumActor)
    {
      this->FrustumActor->SetCamera(nullptr);
    }
    vtkErrorMacro(<< "not a spotlight.");
    return;
  }
}

//-------------------------------------------------------------------------
void vtkLightActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Light: ";
  if (this->Light == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->Light->PrintSelf(os, indent);
  }

  os << indent << "ClippingRange: " << this->ClippingRange[0] << "," << this->ClippingRange[1]
     << endl;
}
