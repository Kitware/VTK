/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightRepresentation.h"

#include "vtkActor.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkLightRepresentation);

//----------------------------------------------------------------------
vtkLightRepresentation::vtkLightRepresentation()
{
  // Initialize state
  this->InteractionState = vtkLightRepresentation::Outside;
  this->HandleSize = 10.0;
  this->InitialLength = 1;
  this->ValidPick = 1;

  // Set up the initial properties
  this->Property->SetAmbient(1.0);
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetRepresentationToWireframe();

  // Represent the sphere
  this->Sphere->LatLongTessellationOn();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper->SetInputConnection(this->Sphere->GetOutputPort());
  this->SphereActor->SetMapper(this->SphereMapper);
  this->SphereActor->SetProperty(this->Property);

  // Sphere picking
  this->SpherePicker->PickFromListOn();
  this->SpherePicker->AddPickList(this->SphereActor);
  this->SpherePicker->SetTolerance(0.01); // need some fluff

  // Represent the Cone
  this->ConeMapper->SetInputConnection(this->Cone->GetOutputPort());
  this->ConeActor->SetMapper(this->ConeMapper);
  this->ConeActor->SetProperty(this->Property);

  // Cone picking
  this->ConePicker->PickFromListOn();
  this->ConePicker->AddPickList(this->ConeActor);
  this->ConePicker->SetTolerance(0.01); // need some fluff

  // Represent the Line
  this->LineMapper->SetInputConnection(this->Line->GetOutputPort());
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->Property);

  // Line picking
  this->LinePicker->PickFromListOn();
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->SetTolerance(0.01); // need some fluff

  // Update the representation sources
  this->UpdateSources();
}

//----------------------------------------------------------------------
vtkLightRepresentation::~vtkLightRepresentation()
{
  // Needed in order to be able to forward declare the classes in vtkNew
}

//----------------------------------------------------------------------
void vtkLightRepresentation::SetLightPosition(double x[3])
{
  if (this->LightPosition[0] != x[0] || this->LightPosition[1] != x[1] ||
    this->LightPosition[2] != x[2])
  {
    this->LightPosition[0] = x[0];
    this->LightPosition[1] = x[1];
    this->LightPosition[2] = x[2];
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkLightRepresentation::SetFocalPoint(double x[3])
{
  if (this->FocalPoint[0] != x[0] || this->FocalPoint[1] != x[1] || this->FocalPoint[2] != x[2])
  {
    this->FocalPoint[0] = x[0];
    this->FocalPoint[1] = x[1];
    this->FocalPoint[2] = x[2];
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkLightRepresentation::SetConeAngle(double angle)
{
  // Clamp between 0 and 89.98 because of
  // https://gitlab.kitware.com/paraview/paraview/issues/19223
  angle = vtkMath::ClampValue(angle, 0.0, 89.98);
  if (this->ConeAngle != angle)
  {
    this->ConeAngle = angle;
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkLightRepresentation::SetLightColor(double* color)
{
  this->Property->SetColor(color);
}

//----------------------------------------------------------------------
double* vtkLightRepresentation::GetLightColor()
{
  return this->Property->GetColor();
}

//----------------------------------------------------------------------
void vtkLightRepresentation::UpdateSources()
{
  this->Sphere->SetCenter(this->LightPosition);
  this->Line->SetPoint1(this->LightPosition);
  this->Line->SetPoint2(this->FocalPoint);

  double vec[3];
  vtkMath::Subtract(this->LightPosition, this->FocalPoint, vec);
  double center[3];
  vtkMath::Add(this->LightPosition, this->FocalPoint, center);
  vtkMath::MultiplyScalar(center, 0.5);
  double height = vtkMath::Norm(vec);

  this->Cone->SetCenter(center);
  this->Cone->SetHeight(height);
  this->Cone->SetDirection(vec);
  this->Cone->SetRadius(std::tan(vtkMath::Pi() * this->ConeAngle / 180) * height);

  this->Sphere->Update();
  this->Line->Update();
  this->Cone->Update();
  this->SizeHandles();
}

//----------------------------------------------------------------------
double* vtkLightRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->SphereActor->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());
  if (this->Positional)
  {
    this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
  }
  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------
void vtkLightRepresentation::StartWidgetInteraction(double eventPosition[2])
{
  // Store the start position
  this->StartEventPosition[0] = eventPosition[0];
  this->StartEventPosition[1] = eventPosition[1];
  this->StartEventPosition[2] = 0.0;

  // Store the last position
  this->LastEventPosition[0] = eventPosition[0];
  this->LastEventPosition[1] = eventPosition[1];
  this->LastEventPosition[2] = 0.0;

  // Initialize scaling distance
  this->LastScalingDistance2 = -1;
}

//----------------------------------------------------------------------
void vtkLightRepresentation::WidgetInteraction(double eventPosition[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  double lookPoint[4], pickPoint[4];
  double vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  double pos[3];
  this->LastPicker->GetPickPosition(pos);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], lookPoint);
  double z = lookPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPosition[0], eventPosition[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkLightRepresentation::MovingLight)
  {
    this->SetLightPosition(pickPoint);
  }
  else if (this->InteractionState == vtkLightRepresentation::MovingFocalPoint ||
    this->InteractionState == vtkLightRepresentation::MovingPositionalFocalPoint)
  {
    this->SetFocalPoint(pickPoint);
  }
  else if (this->InteractionState == vtkLightRepresentation::ScalingConeAngle)
  {
    // Recover last picked point
    double lastPickPoint[4];
    vtkInteractorObserver::ComputeDisplayToWorld(
      this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, lastPickPoint);

    // Scale the cone angle
    this->ScaleConeAngle(pickPoint, lastPickPoint);
  }

  // Store the start position
  this->LastEventPosition[0] = eventPosition[0];
  this->LastEventPosition[1] = eventPosition[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
int vtkLightRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // Picked point is not inside
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    this->InteractionState = vtkLightRepresentation::Outside;
    return this->InteractionState;
  }

  // Check if sphere is picked
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->SpherePicker);
  if (path != nullptr)
  {
    this->LastPicker = this->SpherePicker;
    this->InteractionState = vtkLightRepresentation::MovingLight;
    return this->InteractionState;
  }

  if (this->Positional)
  {
    // Check if cone is picked
    path = this->GetAssemblyPath(X, Y, 0., this->ConePicker);
    if (path != nullptr)
    {
      this->LastPicker = this->ConePicker;
      this->InteractionState = vtkLightRepresentation::MovingPositionalFocalPoint;
      return this->InteractionState;
    }
  }
  else
  {
    // Check if line is picked
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);
    if (path != nullptr)
    {
      this->LastPicker = this->LinePicker;
      this->InteractionState = vtkLightRepresentation::MovingFocalPoint;
      return this->InteractionState;
    }
  }

  this->InteractionState = vtkLightRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkLightRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer &&
      ((this->Renderer->GetVTKWindow() &&
         this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) ||
        (this->Renderer->GetActiveCamera() &&
          this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime))))
  {
    // resize the handles
    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkLightRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->SphereActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkLightRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->SphereActor->RenderOpaqueGeometry(v);
  count += this->LineActor->RenderOpaqueGeometry(v);
  if (this->Positional)
  {
    count += this->ConeActor->RenderOpaqueGeometry(v);
  }
  return count;
}

//----------------------------------------------------------------------
int vtkLightRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
  if (this->Positional)
  {
    count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
  }
  return count;
}

//----------------------------------------------------------------------
void vtkLightRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "LightPosition: " << this->LightPosition[0] << " " << this->LightPosition[1]
     << " " << this->LightPosition[2] << endl;
  os << indent << "FocalPoint: " << this->FocalPoint[0] << " " << this->FocalPoint[1] << " "
     << this->FocalPoint[2] << endl;
  os << indent << "ConeAngle: " << this->ConeAngle << endl;
  os << indent << "Positional: " << this->Positional << endl;

  os << indent << "Property: ";
  this->Property->PrintSelf(os, indent.GetNextIndent());

  os << indent << "BoundingBox: ";
  this->BoundingBox->PrintSelf(os, indent.GetNextIndent());

  os << indent << "LastScalingDistance2: " << this->LastScalingDistance2 << endl;
  os << indent << "LastEventPosition: " << this->LastEventPosition[0] << " "
     << this->LastEventPosition[1] << " " << this->LastEventPosition[2] << endl;

  os << indent << "Sphere: ";
  this->Sphere->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SphereActor: ";
  this->SphereActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SphereMapper: ";
  this->SphereMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SpherePicker: ";
  this->SpherePicker->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Line: ";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LineActor: ";
  this->LineActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LineMapper: ";
  this->LineMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LinePicker: ";
  this->LinePicker->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Cone: ";
  this->Cone->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConeActor: ";
  this->ConeActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConeMapper: ";
  this->ConeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConePicker: ";
  this->ConePicker->PrintSelf(os, indent.GetNextIndent());

  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkLightRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->Sphere->GetOutput()->GetCenter());
  this->Sphere->SetRadius(radius);
}

//----------------------------------------------------------------------------
void vtkLightRepresentation::ScaleConeAngle(double* pickPoint, double* lastPickPoint)
{
  double vecOrig[3];
  double vecCur[3];
  double vecPrev[3];
  double project[3];

  // Compute the squated distance from the picked point
  vtkMath::Subtract(this->FocalPoint, this->LightPosition, vecOrig);
  vtkMath::Subtract(pickPoint, this->LightPosition, vecCur);
  vtkMath::Subtract(lastPickPoint, this->LightPosition, vecPrev);
  vtkMath::ProjectVector(vecCur, vecOrig, project);
  double distance2 = vtkMath::Distance2BetweenPoints(pickPoint, project);

  // If a squared distance has been computed before, the angle has changed
  if (this->LastScalingDistance2 != -1)
  {
    // Compute the direction of the angle change
    double factor = this->LastScalingDistance2 < distance2 ? 1 : -1;

    // Compute the difference of the change
    double deltaAngle =
      factor * 180 * vtkMath::AngleBetweenVectors(vecCur, vecPrev) / vtkMath::Pi();

    // Add it to the current angle
    this->SetConeAngle(this->ConeAngle + deltaAngle);
  }

  // Store the last scaling squared distance
  this->LastScalingDistance2 = distance2;
}
