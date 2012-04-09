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
#include "vtkPolyDataMapper.h"
#include "vtkFrustumSource.h"
#include "vtkCamera.h"
#include "vtkPlanes.h"
#include "vtkProperty.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkCameraActor);
vtkCxxSetObjectMacro(vtkCameraActor, Camera, vtkCamera);

// ----------------------------------------------------------------------------
vtkCameraActor::vtkCameraActor()
{
  this->Camera=0;
  this->WidthByHeightRatio=1.0;
  this->FrustumSource=0;
  this->FrustumMapper=0;
  this->FrustumActor=0;
}

// ----------------------------------------------------------------------------
vtkCameraActor::~vtkCameraActor()
{
  this->SetCamera(0);
  
  if(this->FrustumActor!=0)
    {
    this->FrustumActor->Delete();
    }
  
   if(this->FrustumMapper!=0)
    {
    this->FrustumMapper->Delete();
    }
  if(this->FrustumSource!=0)
    {
    this->FrustumSource->Delete();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int vtkCameraActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->UpdateViewProps();

  int result=0;
  if(this->FrustumActor!=0 && this->FrustumActor->GetMapper()!=0)
    {
    result=this->FrustumActor->RenderOpaqueGeometry(viewport);
    }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
int vtkCameraActor::HasTranslucentPolygonalGeometry()
{
  return false;
}


//-----------------------------------------------------------------------------
void vtkCameraActor::ReleaseGraphicsResources(vtkWindow *window)
{
  if(this->FrustumActor!=0)
    {
    this->FrustumActor->ReleaseGraphicsResources(window);
    }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkCameraActor::GetBounds()
{
  // we cannot initialize the Bounds the same way vtkBoundingBox does because
  // vtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As vtkMath::UninitializeBounds initialized finite unvalid bounds, it
  // passes silently and GetLength() returns 0.
  vtkMath::UninitializeBounds(this->Bounds);
  
  this->UpdateViewProps();
  if(this->FrustumActor!=0 && this->FrustumActor->GetUseBounds())
    {
    this->FrustumActor->GetBounds(this->Bounds);
    }
  return this->Bounds;
}

//-------------------------------------------------------------------------
unsigned long int vtkCameraActor::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  if(this->Camera!=0)
    {
    unsigned long time;
    time = this->Camera->GetMTime();
    if(time>mTime)
      {
      mTime=time;
      }
    }
  return mTime;
}

// ----------------------------------------------------------------------------
// Description:
// Get property of the internal actor.
vtkProperty *vtkCameraActor::GetProperty()
{
  if(this->FrustumActor==0)
    {
    this->FrustumActor=vtkActor::New();
    }
  
  return this->FrustumActor->GetProperty();
}
  
// ----------------------------------------------------------------------------
// Description:
// Set property of the internal actor.
void vtkCameraActor::SetProperty(vtkProperty *p)
{
  if(this->FrustumActor==0)
    {
    this->FrustumActor=vtkActor::New();
    }
  
  this->FrustumActor->SetProperty(p);
}

// ----------------------------------------------------------------------------
void vtkCameraActor::UpdateViewProps()
{
  if(this->Camera==0)
    {
    vtkDebugMacro(<< "no camera to represent.");
    return;
    }
 
  vtkPlanes *planes=0;
  if(this->FrustumSource==0)
    {
    this->FrustumSource=vtkFrustumSource::New();
    planes=vtkPlanes::New();
    this->FrustumSource->SetPlanes(planes);
    planes->Delete();
    }
  else
    {
    planes=this->FrustumSource->GetPlanes();
    }
  
  double coefs[24];
  this->Camera->GetFrustumPlanes(this->WidthByHeightRatio,coefs);
  planes->SetFrustumPlanes(coefs);
  
  this->FrustumSource->SetShowLines(false);
  
  if(this->FrustumMapper==0)
    {
    this->FrustumMapper=vtkPolyDataMapper::New();
    }
  
  this->FrustumMapper->SetInputConnection(
    this->FrustumSource->GetOutputPort());
  
  if(this->FrustumActor==0)
    {
    this->FrustumActor=vtkActor::New();
    }
 
  this->FrustumActor->SetMapper(this->FrustumMapper);
  
  vtkProperty *p=this->FrustumActor->GetProperty();
  p->SetRepresentationToWireframe();
  this->FrustumActor->SetVisibility(1);
}

//-------------------------------------------------------------------------
void vtkCameraActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Camera: ";
  if(this->Camera==0)
    {
    os << "(none)" << endl; 
    }
  else
    {
    this->Camera->PrintSelf(os,indent);
    }
  
  
  os << indent << "WidthByHeightRatio: " << this->WidthByHeightRatio <<  endl;
}
