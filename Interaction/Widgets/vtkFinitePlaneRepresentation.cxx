/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFinitePlaneRepresentation.cxx

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFinitePlaneRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkDoubleArray.h"
#include "vtkFeatureEdges.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkFinitePlaneRepresentation);

//----------------------------------------------------------------------------
vtkFinitePlaneRepresentation::vtkFinitePlaneRepresentation()
{
  // The initial state
  this->InteractionState = vtkFinitePlaneRepresentation::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.;

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Origin[0] = 0.;
  this->Origin[1] = 0.;
  this->Origin[2] = 0.;

  this->Normal[0] = 0.;
  this->Normal[1] = 0.;
  this->Normal[2] = 1.;

  this->PreviousNormal[0] = 0.;
  this->PreviousNormal[1] = 0.;
  this->PreviousNormal[2] = 1.;
  this->Transform = vtkTransform::New();
  this->Transform->Identity();

  this->V1[0] = 1.;
  this->V1[1] = 0.;
  this->V1[2] = 0.;
  this->V2[0] = 0.;
  this->V2[1] = 1.;
  this->V2[2] = 0.;

  double p1[3];
  p1[0] = this->Origin[0] + this->V1[0];
  p1[1] = this->Origin[1] + this->V1[1];
  p1[2] = this->Origin[2] + this->V1[2];

  double p2[3];
  p2[0] = this->Origin[0] + this->V2[0];
  p2[1] = this->Origin[1] + this->V2[1];
  p2[2] = this->Origin[2] + this->V2[2];

  // the origin
  this->OriginGeometry = vtkSphereSource::New();
  this->OriginGeometry->SetCenter(this->Origin);
  this->OriginGeometry->Update();
  this->OriginMapper = vtkPolyDataMapper::New();
  this->OriginMapper->SetInputConnection(this->OriginGeometry->GetOutputPort());
  this->OriginActor = vtkActor::New();
  this->OriginActor->SetMapper(this->OriginMapper);

  // the X Vector
  this->V1Geometry = vtkSphereSource::New();
  this->V1Geometry->SetCenter(p1);
  this->V1Geometry->Update();
  this->V1Mapper = vtkPolyDataMapper::New();
  this->V1Mapper->SetInputConnection(this->V1Geometry->GetOutputPort());
  this->V1Actor = vtkActor::New();
  this->V1Actor->SetMapper(this->V1Mapper);

  // the Y Vector
  this->V2Geometry = vtkSphereSource::New();
  this->V2Geometry->SetCenter(p2);
  this->V2Geometry->Update();
  this->V2Mapper = vtkPolyDataMapper::New();
  this->V2Mapper->SetInputConnection(this->V2Geometry->GetOutputPort());
  this->V2Actor = vtkActor::New();
  this->V2Actor->SetMapper(this->V2Mapper);

 // Create the + plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInputConnection(this->ConeSource->GetOutputPort());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - plane normal
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInputConnection(this->LineSource2->GetOutputPort());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInputConnection(this->ConeSource2->GetOutputPort());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // The finite plane
  this->PlanePolyData  = vtkPolyData::New();

  // Construct initial points
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(4);
  this->PlanePolyData->SetPoints(points.Get());
  for (int i = 0; i < 4; i++)
  {
    points->SetPoint(i, this->Origin);
  }

  // Construct plane geometry
  vtkNew<vtkCellArray> cell;
  cell->Allocate(5);
  vtkIdType pts[4] = { 0, 1, 2, 3 };
  cell->InsertNextCell(4, pts);
  this->PlanePolyData->SetPolys(cell.Get());
  this->PlanePolyData->BuildCells();

  this->PlaneMapper = vtkPolyDataMapper::New();
  this->PlaneMapper->SetInputData(this->PlanePolyData);
  this->PlaneActor = vtkActor::New();
  this->PlaneActor->SetMapper(this->PlaneMapper);

  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetInputData(this->PlanePolyData);

  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetInputConnection(this->Edges->GetOutputPort());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = true; //control whether tubing is on
  this->DrawPlane = true; //control whether draw plane is on
  this->CurrentHandle = NULL;

  // Initial creation of the widget, serves to initialize it
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);

  this->HandlePicker->AddPickList(OriginActor);
  this->HandlePicker->AddPickList(V1Actor);
  this->HandlePicker->AddPickList(V2Actor);
  this->HandlePicker->AddPickList(LineActor);
  this->HandlePicker->AddPickList(ConeActor);
  this->HandlePicker->AddPickList(LineActor2);
  this->HandlePicker->AddPickList(ConeActor2);
  this->HandlePicker->AddPickList(PlaneActor);

  this->HandlePicker->PickFromListOn();

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->RepresentationState = vtkFinitePlaneRepresentation::Outside;

  // Pass the initial properties to the actors.
  this->LineActor->SetProperty(this->NormalProperty);
  this->ConeActor->SetProperty(this->NormalProperty);
  this->LineActor2->SetProperty(this->NormalProperty);
  this->ConeActor2->SetProperty(this->NormalProperty);
  this->PlaneActor->SetProperty(this->PlaneProperty);
  this->V1Actor->SetProperty(this->V1HandleProperty);
  this->V2Actor->SetProperty(this->V2HandleProperty);
  this->OriginActor->SetProperty(this->OriginHandleProperty);

  // Internal data memebers for performance
  this->TransformRotation = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkFinitePlaneRepresentation::~vtkFinitePlaneRepresentation()
{
  this->OriginGeometry->Delete();
  this->OriginMapper->Delete();
  this->OriginActor->Delete();

  // the X Vector
  this->V1Geometry->Delete();
  this->V1Mapper->Delete();
  this->V1Actor->Delete();

  // the Y Vector
  this->V2Geometry->Delete();
  this->V2Mapper->Delete();
  this->V2Actor->Delete();

  // The + normal cone
  this->ConeSource->Delete();
  this->ConeMapper->Delete();
  this->ConeActor->Delete();

  // The + normal line
  this->LineSource->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  // The - normal cone
  this->ConeSource2->Delete();
  this->ConeMapper2->Delete();
  this->ConeActor2->Delete();

  // The - normal line
  this->LineSource2->Delete();
  this->LineMapper2->Delete();
  this->LineActor2->Delete();

  // The finite plane
  this->PlanePolyData->Delete();
  this->PlaneMapper->Delete();
  this->PlaneActor->Delete();

  this->Edges->Delete();
  this->EdgesTuber->Delete();
  this->EdgesMapper->Delete();
  this->EdgesActor->Delete();

  this->BoundingBox->Delete();

  this->NormalProperty->Delete();
  this->SelectedNormalProperty->Delete();

  this->HandlePicker->Delete();

  this->TransformRotation->Delete();
  this->Transform->Delete();

  this->OriginHandleProperty->Delete();
  this->V1HandleProperty->Delete();
  this->V2HandleProperty->Delete();
  this->SelectedHandleProperty->Delete();
  this->PlaneProperty->Delete();
  this->SelectedPlaneProperty->Delete();
}

//----------------------------------------------------------------------
void vtkFinitePlaneRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->PlanePolyData);
}

//----------------------------------------------------------------------
void vtkFinitePlaneRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkFinitePlaneRepresentation::WidgetInteraction(double e[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Compute the two points defining the motion vector
  double pos[3];
  this->HandlePicker->GetPickPosition(pos);

  vtkInteractorObserver::ComputeWorldToDisplay(
    this->Renderer, pos[0], pos[1], pos[2], focalPoint);

  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0],
    this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkFinitePlaneRepresentation::MoveOrigin)
  {
    this->TranslateOrigin(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkFinitePlaneRepresentation::ModifyV1)
  {
    this->MovePoint1(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkFinitePlaneRepresentation::ModifyV2)
  {
    this->MovePoint2(prevPickPoint, pickPoint);
  }
   else if (this->InteractionState == vtkFinitePlaneRepresentation::Rotating)
   {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
   }
   else if (this->InteractionState == vtkFinitePlaneRepresentation::Pushing)
   {
    this->Push(prevPickPoint, pickPoint);
   }

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::Rotate(int X, int Y,
                                          double *p1, double *p2, double *vpn)
{
  double v[3];    //vector of motion
  double axis[3]; //axis of rotation

  // Mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn, v, axis);
  if (vtkMath::Normalize(axis) == 0.0)
  {
    return;
  }
  int *size = this->Renderer->GetSize();
  double l2 = (X - this->LastEventPosition[0]) *
    (X - this->LastEventPosition[0]) +
    (Y - this->LastEventPosition[1]) * (Y - this->LastEventPosition[1]);
  double theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

  //Manipulate the transform to reflect the rotation
  this->TransformRotation->Identity();
  this->TransformRotation->Translate(this->Origin);
  this->TransformRotation->RotateWXYZ(theta, axis);
  this->TransformRotation->Translate(
    -this->Origin[0], -this->Origin[1], -this->Origin[2]);

  //Set the new normal
  double nNew[3];
  this->TransformRotation->TransformNormal(this->Normal, nNew);
  this->SetNormal(nNew);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::CreateDefaultProperties()
{
  // Normal properties
  this->NormalProperty = vtkProperty::New();
  this->NormalProperty->SetColor(1.0, 1.0, 1.0);
  this->NormalProperty->SetLineWidth(2.0);

  this->SelectedNormalProperty = vtkProperty::New();
  this->SelectedNormalProperty->SetColor(1.0, 0.0, 0.0);
  this->NormalProperty->SetLineWidth(2.0);

  // Origin Handle properties
  this->OriginHandleProperty = vtkProperty::New();
  this->OriginHandleProperty->SetColor(1.0, 1.0, 1.0);

  // P1 Handle properties
  this->V1HandleProperty = vtkProperty::New();
  this->V1HandleProperty->SetColor(1.0, 0.0, 0.0);

  // P2 Handle properties
  this->V2HandleProperty = vtkProperty::New();
  this->V2HandleProperty->SetColor(0.0, 1.0, 0.0);

  this->SelectedHandleProperty = vtkProperty::New();
  this->SelectedHandleProperty->SetColor(1.0, 1.0, 0.0);

  // Plane properties
  this->PlaneProperty = vtkProperty::New();
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->PlaneProperty->SetOpacity(0.5);

  this->SelectedPlaneProperty = vtkProperty::New();
  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetColor(0.0, 1.0, 0.0);
  this->SelectedPlaneProperty->SetOpacity(0.25);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::PlaceWidget(double bnds[6])
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = ((bnds[1] - bnds[0]) * 0.5) + bnds[0];
  this->Origin[1] = ((bnds[3] - bnds[2]) * 0.5) + bnds[2];
  this->Origin[2] = ((bnds[5] - bnds[4]) * 0.5) + bnds[4];

  this->V1[0] = ((bnds[1] - bnds[0]) * 0.5);
  this->V1[1] = 0.0;
  this->V1[2] = 0.0;

  this->V2[0] = 0.0;
  this->V2[1] = ((bnds[3] - bnds[2]) * 0.5);
  this->V2[2] = 0.0;

  this->InitialLength =
    sqrt((bnds[1] - bnds[0]) * (bnds[1] - bnds[0]) +
         (bnds[3] - bnds[2]) * (bnds[3] - bnds[2]) +
         (bnds[5] - bnds[4]) * (bnds[5] - bnds[4]));

  this->ValidPick = 1; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
int vtkFinitePlaneRepresentation::ComputeInteractionState(int X, int Y,
                                                          int vtkNotUsed(modify))
{
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::Outside);
    this->InteractionState = vtkFinitePlaneRepresentation::Outside;
    return this->InteractionState;
  }

  this->SetHighlightNormal(0);
  this->SetHighlightPlane(0);
  this->SetHighlightHandle(NULL);

  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path == NULL) // Not picking this widget
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::Outside);
    this->InteractionState = vtkFinitePlaneRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = 1;
  vtkProp *prop = path->GetFirstNode()->GetViewProp();

  if (prop == this->PlaneActor)
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::Pushing);
    this->InteractionState = vtkFinitePlaneRepresentation::Pushing;
    this->SetHighlightNormal(0);
    this->SetHighlightPlane(1);
    this->SetHighlightHandle(NULL);
  }
  else if ((prop == this->ConeActor) || (prop == this->ConeActor2) ||
    (prop == this->LineActor) || (prop == this->LineActor2))
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::Rotating);
    this->InteractionState = vtkFinitePlaneRepresentation::Rotating;

    this->SetHighlightNormal(1);
    this->SetHighlightPlane(1);
    this->SetHighlightHandle(NULL);
  }
  else if (prop == this->OriginActor)
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::MoveOrigin);
    this->InteractionState = vtkFinitePlaneRepresentation::MoveOrigin;

    this->SetHighlightNormal(1);
    this->SetHighlightPlane(1);
    this->SetHighlightHandle(prop);
  }
  else if (prop == this->V1Actor)
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::ModifyV1);
    this->InteractionState = vtkFinitePlaneRepresentation::ModifyV1;

    this->SetHighlightNormal(0);
    this->SetHighlightPlane(0);
    this->SetHighlightHandle(prop);
  }
  else if (prop == this->V2Actor)
  {
    this->SetRepresentationState(vtkFinitePlaneRepresentation::ModifyV2);
    this->InteractionState = vtkFinitePlaneRepresentation::ModifyV2;

    this->SetHighlightNormal(0);
    this->SetHighlightPlane(0);
    this->SetHighlightHandle(prop);
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
double *vtkFinitePlaneRepresentation::GetBounds()
{
  this->BuildRepresentation();

  this->BoundingBox->SetBounds(this->OriginActor->GetBounds());
  this->BoundingBox->AddBounds(this->V1Actor->GetBounds());
  this->BoundingBox->AddBounds(this->V2Actor->GetBounds());
  this->BoundingBox->AddBounds(this->EdgesActor->GetBounds());
  this->BoundingBox->AddBounds(this->PlaneActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor2->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor2->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::BuildRepresentation()
{
  this->SizeHandles();

  if (this->GetMTime() < this->BuildTime &&
      this->PlanePolyData->GetMTime() < this->BuildTime)
  {
    return;
  }

  double *origin = this->GetOrigin();
  double *normal = this->GetNormal();

  // Setup the plane normal
  double d = this->PlanePolyData->GetLength() *1.2;

  double p2Line[3];
  p2Line[0] = origin[0] + 0.30 * d * normal[0];
  p2Line[1] = origin[1] + 0.30 * d * normal[1];
  p2Line[2] = origin[2] + 0.30 * d * normal[2];

  this->LineSource->SetPoint1(origin);
  this->LineSource->SetPoint2(p2Line);
  this->ConeSource->SetCenter(p2Line);
  this->ConeSource->SetDirection(normal);

  p2Line[0] = origin[0] - 0.30 * d * normal[0];
  p2Line[1] = origin[1] - 0.30 * d * normal[1];
  p2Line[2] = origin[2] - 0.30 * d * normal[2];

  this->LineSource2->SetPoint1(origin);
  this->LineSource2->SetPoint2(p2Line);
  this->ConeSource2->SetCenter(p2Line);
  this->ConeSource2->SetDirection(normal);

  // Set up the position handle
  this->OriginGeometry->SetCenter(origin);

  double vector1[3] = { this->V1[0], this->V1[1], this->V1[2] };
  this->Transform->TransformVector(vector1, vector1);

  double position[3];
  this->Transform->GetPosition(position);

  double point1[3];
  point1[0] = origin[0] + vector1[0];
  point1[1] = origin[1] + vector1[1];
  point1[2] = origin[2] + vector1[2];
  this->V1Geometry->SetCenter(point1);

  double vector2[3] = { this->V2[0], this->V2[1], this->V2[2] };
  this->Transform->TransformPoint(vector2, vector2);

  double point2[3];
  point2[0] = origin[0] + vector2[0];
  point2[1] = origin[1] + vector2[1];
  point2[2] = origin[2] + vector2[2];
  this->V2Geometry->SetCenter(point2);

  // Build Plane polydata
  vtkPoints *points = this->PlanePolyData->GetPoints();
  points->SetPoint(0,
    origin[0] - vector1[0] - vector2[0],
    origin[1] - vector1[1] - vector2[1],
    origin[2] - vector1[2] - vector2[2]);
  points->SetPoint(1,
    origin[0] - vector1[0] + vector2[0],
    origin[1] - vector1[1] + vector2[1],
    origin[2] - vector1[2] + vector2[2]);
  points->SetPoint(2,
    origin[0] + vector1[0] + vector2[0],
    origin[1] + vector1[1] + vector2[1],
    origin[2] + vector1[2] + vector2[2]);
  points->SetPoint(3,
    origin[0] + vector1[0] - vector2[0],
    origin[1] + vector1[1] - vector2[1],
    origin[2] + vector1[2] - vector2[2]);

  this->PlanePolyData->Modified();

  // Control the look of the edges
  this->EdgesMapper->SetInputConnection(this->Tubing ?
    this->EdgesTuber->GetOutputPort(): this->Edges->GetOutputPort());

  this->SizeHandles();
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->OriginActor->ReleaseGraphicsResources(w);
  this->V1Actor->ReleaseGraphicsResources(w);
  this->V2Actor->ReleaseGraphicsResources(w);
  this->PlaneActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor2->ReleaseGraphicsResources(w);
  this->LineActor2->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkFinitePlaneRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count = 0;
  this->BuildRepresentation();

  if (this->OriginActor->GetVisibility())
  {
    count += this->OriginActor->RenderOpaqueGeometry(v);
  }
  if (this->V1Actor->GetVisibility())
  {
    count += this->V1Actor->RenderOpaqueGeometry(v);
  }
  if (this->V2Actor->GetVisibility())
  {
    count += this->V2Actor->RenderOpaqueGeometry(v);
  }
  count += this->EdgesActor->RenderOpaqueGeometry(v);
  count += this->ConeActor->RenderOpaqueGeometry(v);
  count += this->LineActor->RenderOpaqueGeometry(v);
  count += this->ConeActor2->RenderOpaqueGeometry(v);
  count += this->LineActor2->RenderOpaqueGeometry(v);

  if (this->DrawPlane)
  {
    count += this->PlaneActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//----------------------------------------------------------------------------
int vtkFinitePlaneRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
  int count = 0;
  this->BuildRepresentation();

  if (this->OriginActor->GetVisibility())
  {
    count += this->OriginActor->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->V1Actor->GetVisibility())
  {
    count += this->V1Actor->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->V2Actor->GetVisibility())
  {
    count += this->V2Actor->RenderTranslucentPolygonalGeometry(v);
  }

  count += this->EdgesActor->RenderTranslucentPolygonalGeometry(v);
  count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->ConeActor2->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor2->RenderTranslucentPolygonalGeometry(v);

  if (this->DrawPlane)
  {
    count += this->PlaneActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//----------------------------------------------------------------------------
int vtkFinitePlaneRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  this->BuildRepresentation();

  if (this->OriginActor->GetVisibility())
  {
    result |= this->OriginActor->HasTranslucentPolygonalGeometry();
  }
  if (this->V1Actor->GetVisibility())
  {
    result |= this->V1Actor->HasTranslucentPolygonalGeometry();
  }
  if (this->V2Actor->GetVisibility())
  {
    result |= this->V2Actor->HasTranslucentPolygonalGeometry();
  }

  result |= this->EdgesActor->HasTranslucentPolygonalGeometry();
  result |= this->ConeActor->HasTranslucentPolygonalGeometry();
  result |= this->LineActor->HasTranslucentPolygonalGeometry();
  result |= this->ConeActor2->HasTranslucentPolygonalGeometry();
  result |= this->LineActor2->HasTranslucentPolygonalGeometry();

  if (this->DrawPlane)
  {
    result |= this->PlaneActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetHandles(bool handles)
{
  int h = handles ? 1 : 0;
  if (this->V1Actor->GetVisibility() == h)
  {
    return;
  }
  this->V1Actor->SetVisibility(h);
  this->V2Actor->SetVisibility(h);
  this->OriginActor->SetVisibility(h);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::HandlesOn()
{
  this->SetHandles(true);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::HandlesOff()
{
  this->SetHandles(false);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->GetOrigin());

  this->OriginGeometry->SetRadius(radius);
  this->V1Geometry->SetRadius(radius);
  this->V2Geometry->SetRadius(radius);

  this->ConeSource->SetHeight(radius * 2.0);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(radius * 2.0);
  this->ConeSource2->SetRadius(radius);

  this->EdgesTuber->SetRadius(radius * 0.25);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetHighlightHandle(vtkProp *prop)
{
  if (this->CurrentHandle == this->OriginActor)
  {
    this->OriginActor->SetProperty(this->OriginHandleProperty);
  }
  else if (this->CurrentHandle == this->V1Actor)
  {
    this->CurrentHandle->SetProperty(this->V1HandleProperty);
  }
  else if (this->CurrentHandle == this->V2Actor)
  {
    this->CurrentHandle->SetProperty(this->V2HandleProperty);
  }

  this->CurrentHandle = dynamic_cast<vtkActor*>(prop);

  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::RegisterPickers()
{
  this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager()->
    AddPicker(this->HandlePicker, this);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetOrigin(double x, double y, double z)
{
  double origin[3] = { x, y, z };
  this->SetOrigin(origin);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetOrigin(double x[3])
{
  if (this->Origin[0] != x[0] ||
      this->Origin[1] != x[1] ||
      this->Origin[2] != x[2])
  {
    this->Origin[0] = x[0];
    this->Origin[1] = x[1];
    this->Origin[2] = x[2];

    this->Modified();
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetV1(double x, double y)
{
  double v1[2] = { x, y };
  this->SetV1(v1);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetV1(double x[2])
{
  if (this->V1[0] != x[0] ||
      this->V1[1] != x[1])
  {
    this->V1[0] = x[0];
    this->V1[1] = x[1];

    this->Modified();
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetV2(double x, double y)
{
  double v2[2] = { x, y };
  this->SetV2(v2);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetV2(double x[2])
{
  if (this->V2[0] != x[0] ||
      this->V2[1] != x[1])
  {
    this->V2[0] = x[0];
    this->V2[1] = x[1];

    this->Modified();
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetNormal(double x, double y, double z)
{
  double n[3] = { x, y, z };
  vtkMath::Normalize(n);

  this->PreviousNormal[0] = this->Normal[0];
  this->PreviousNormal[1] = this->Normal[1];
  this->PreviousNormal[2] = this->Normal[2];

  if (n[0] != this->Normal[0] || n[1] != this->Normal[1] || n[2] != this->Normal[2])
  {
    this->Normal[0] = n[0];
    this->Normal[1] = n[1];
    this->Normal[2] = n[2];

    double RotationAxis[3];
    vtkMath::Cross(this->PreviousNormal, this->Normal, RotationAxis);
    vtkMath::Normalize(RotationAxis);
    double RotationAngle = vtkMath::DegreesFromRadians(
      acos(vtkMath::Dot(this->PreviousNormal, this->Normal)));

    this->Transform->PostMultiply();
    this->Transform->RotateWXYZ(RotationAngle,
      RotationAxis[0], RotationAxis[1], RotationAxis[2]);

    this->Modified();
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetNormal(double n[3])
{
  this->SetNormal(n[0], n[1], n[2]);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetDrawPlane(bool drawPlane)
{
  if (drawPlane == this->DrawPlane)
  {
    return;
  }

  this->DrawPlane = drawPlane;
  this->Modified();
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetHighlightPlane(int highlight)
{
  this->PlaneActor->SetProperty(
    highlight ? this->SelectedPlaneProperty : this->PlaneProperty);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetHighlightNormal(int highlight)
{
  vtkProperty* p =
    highlight ? this->SelectedNormalProperty : this->NormalProperty;
  this->LineActor->SetProperty(p);
  this->ConeActor->SetProperty(p);
  this->LineActor2->SetProperty(p);
  this->ConeActor2->SetProperty(p);
  this->OriginActor->SetProperty(p);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::SetRepresentationState(int state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  // Clamp the state
  state = (state < vtkFinitePlaneRepresentation::Outside ?
    vtkFinitePlaneRepresentation::Outside :
    (state > vtkFinitePlaneRepresentation::Pushing ?
    vtkFinitePlaneRepresentation::Pushing : state));

  this->RepresentationState = state;
  this->Modified();
}

//----------------------------------------------------------------------------
// translate origin of plane
void vtkFinitePlaneRepresentation::TranslateOrigin(double *p1, double *p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Add to the current point
  this->SetOrigin(
    this->Origin[0] + v[0],
    this->Origin[1] + v[1],
    this->Origin[2] + v[2]);
}

//----------------------------------------------------------------------------
// Move Point 1
void vtkFinitePlaneRepresentation::MovePoint1(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  vtkNew<vtkMatrix4x4> mat;
  this->Transform->GetInverse(mat.Get());

  vtkNew<vtkTransform> t;
  t->SetMatrix(mat.Get());
  t->TransformVector(v, v);

  double *v1 = this->GetV1();

  double newV1[2];
  newV1[0] = v1[0] + v[0];
  newV1[1] = v1[1] + v[1];

  this->SetV1(newV1[0], newV1[1]);
}

//----------------------------------------------------------------------------
// Modified Vector v2
void vtkFinitePlaneRepresentation::MovePoint2(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  vtkNew<vtkMatrix4x4> mat;
  this->Transform->GetInverse(mat.Get());

  vtkNew<vtkTransform> t;
  t->SetMatrix(mat.Get());
  t->TransformVector(v, v);

  double *v2 = this->GetV2();

  double newV2[2];
  newV2[0] = v2[0] + v[0];
  newV2[1] = v2[1] + v[1];

  this->SetV2(newV2[0], newV2[1]);
}

//----------------------------------------------------------------------------
// Push Face
void vtkFinitePlaneRepresentation::Push(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double distance = vtkMath::Dot(v, this->Normal);

  if (distance == 0.0)
  {
    return;
  }

  double origin[3];
  this->GetOrigin(origin);
  origin[0] += distance * this->Normal[0];
  origin[1] += distance * this->Normal[1];
  origin[2] += distance * this->Normal[2];

  this->SetOrigin(origin);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  double *bounds = this->InitialBounds;
  os << indent << "Initial Bounds: "
     << "(" << bounds[0] << ", " << bounds[1] << ") "
     << "(" << bounds[2] << ", " << bounds[3] << ") "
     << "(" << bounds[4] << ", " << bounds[5] << ")\n";

  if (this->OriginHandleProperty)
  {
    os << indent << "Origin Handle Property: " << this->OriginHandleProperty << "\n";
  }
  else
  {
    os << indent << "Origin Handle Property: (none)\n";
  }
  if (this->V1HandleProperty)
  {
    os << indent << "P1 Handle Property: " << this->V1HandleProperty << "\n";
  }
  else
  {
    os << indent << "P1 Handle Property: (none)\n";
  }
  if (this->V2HandleProperty)
  {
    os << indent << "P2 Handle Property: " << this->V2HandleProperty << "\n";
  }
  else
  {
    os << indent << "P2 Handle Property: (none)\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: "
       << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "SelectedHandle Property: (none)\n";
  }

  if (this->PlaneProperty)
  {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
  }
  else
  {
    os << indent << "Plane Property: (none)\n";
  }
  if (this->SelectedPlaneProperty)
  {
    os << indent << "Selected Plane Property: "
       << this->SelectedPlaneProperty << "\n";
  }
  else
  {
    os << indent << "Selected Plane Property: (none)\n";
  }

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << "\n";
  os << indent << "Draw Plane: " << (this->DrawPlane ? "On" : "Off") << "\n";
}
