/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaneWidget.h"

#include "vtkAssemblyNode.h"
#include "vtkCallbackCommand.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPlanes.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"

vtkCxxRevisionMacro(vtkPlaneWidget, "1.12");
vtkStandardNewMacro(vtkPlaneWidget);

vtkCxxSetObjectMacro(vtkPlaneWidget,PlaneProperty,vtkProperty);

vtkPlaneWidget::vtkPlaneWidget()
{
  this->State = vtkPlaneWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkPlaneWidget::ProcessEvents);
  
  this->NormalToXAxis = 0;
  this->NormalToYAxis = 0;
  this->NormalToZAxis = 0;
  this->Representation = VTK_PLANE_WIREFRAME;

  //Build the representation of the widget
  int i;
  // Represent the plane
  this->PlaneSource = vtkPlaneSource::New();
  this->PlaneSource->SetXResolution(4);
  this->PlaneSource->SetYResolution(4);
  this->PlaneOutline = vtkPolyData::New();
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(4);
  vtkCellArray *outline = vtkCellArray::New();
  outline->InsertNextCell(4);
  outline->InsertCellPoint(0);
  outline->InsertCellPoint(1);
  outline->InsertCellPoint(2);
  outline->InsertCellPoint(3);
  this->PlaneOutline->SetPoints(pts);
  pts->Delete();
  this->PlaneOutline->SetPolys(outline);
  outline->Delete();
  this->PlaneMapper = vtkPolyDataMapper::New();
  this->PlaneMapper->SetInput(this->PlaneSource->GetOutput());
  this->PlaneActor = vtkActor::New();
  this->PlaneActor->SetMapper(this->PlaneMapper);

  // Create the handles
  this->Handle = new vtkActor* [4];
  this->HandleMapper = new vtkPolyDataMapper* [4];
  this->HandleGeometry = new vtkSphereSource* [4];
  for (i=0; i<4; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }
  
  // Create the plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInput(this->ConeSource->GetOutput());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  this->Transform = vtkTransform::New();

  // Define the point coordinates
  float bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);
  for (i=0; i<4; i++)
    {
    this->HandlePicker->AddPickList(this->Handle[i]);
    }
  this->HandlePicker->PickFromListOn();

  this->PlanePicker = vtkCellPicker::New();
  this->PlanePicker->SetTolerance(0.005); //need some fluff
  this->PlanePicker->AddPickList(this->PlaneActor);
  this->PlanePicker->AddPickList(this->ConeActor);
  this->PlanePicker->AddPickList(this->LineActor);
  this->PlanePicker->PickFromListOn();
  
  this->CurrentHandle = NULL;

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->PlaneProperty = NULL;
  this->SelectedPlaneProperty = NULL;
  this->CreateDefaultProperties();
  
  this->SelectRepresentation();
}

vtkPlaneWidget::~vtkPlaneWidget()
{
  this->PlaneActor->Delete();
  this->PlaneMapper->Delete();
  this->PlaneSource->Delete();
  this->PlaneOutline->Delete();

  for (int i=0; i<4; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;
  
  this->ConeActor->Delete();
  this->ConeMapper->Delete();
  this->ConeSource->Delete();

  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineSource->Delete();

  this->HandlePicker->Delete();
  this->PlanePicker->Delete();

  if ( this->HandleProperty )
    {
    this->HandleProperty->Delete();
    }
  if ( this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty->Delete();
    }
  if ( this->PlaneProperty )
    {
    this->PlaneProperty->Delete();
    }
  if ( this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty->Delete();
    }
  
  this->Transform->Delete();
}

void vtkPlaneWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //------------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling plane widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->CurrentRenderer = this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],this->Interactor->GetLastEventPosition()[1]);
    if (this->CurrentRenderer == NULL)
      {
      return;
      }

    this->Enabled = 1;

    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, 
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);

    // Add the plane
    this->CurrentRenderer->AddActor(this->PlaneActor);
    this->PlaneActor->SetProperty(this->PlaneProperty);

    // turn on the handles
    for (int j=0; j<4; j++)
      {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
      }

    // add the normal vector
    this->CurrentRenderer->AddActor(this->LineActor);
    this->LineActor->SetProperty(this->HandleProperty);
    this->CurrentRenderer->AddActor(this->ConeActor);
    this->ConeActor->SetProperty(this->HandleProperty);

    this->SelectRepresentation();
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling plane widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the plane
    this->CurrentRenderer->RemoveActor(this->PlaneActor);

    // turn off the handles
    for (int i=0; i<4; i++)
      {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
      }

    // turn off the normal vector
    this->CurrentRenderer->RemoveActor(this->LineActor);
    this->CurrentRenderer->RemoveActor(this->ConeActor);

    this->CurrentHandle = NULL;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkPlaneWidget::ProcessEvents(vtkObject* object, unsigned long event,
                                       void* clientdata, void* vtkNotUsed(calldata))
{
  vtkPlaneWidget* self = reinterpret_cast<vtkPlaneWidget *>( clientdata );
  vtkRenderWindowInteractor* rwi = static_cast<vtkRenderWindowInteractor *>( object );
  int* XY = rwi->GetEventPosition();

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    }
}

void vtkPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->HandleProperty )
    {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
  else
    {
    os << indent << "Handle Property: (none)\n";
    }
  if ( this->SelectedHandleProperty )
    {
    os << indent << "Selected Handle Property: " 
       << this->SelectedHandleProperty << "\n";
    }
  else
    {
    os << indent << "SelectedHandle Property: (none)\n";
    }

  if ( this->PlaneProperty )
    {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
    }
  else
    {
    os << indent << "Plane Property: (none)\n";
    }
  if ( this->SelectedPlaneProperty )
    {
    os << indent << "Selected Plane Property: " 
       << this->SelectedPlaneProperty << "\n";
    }
  else
    {
    os << indent << "Selected Plane Property: (none)\n";
    }

  os << indent << "Plane Representation: ";
  if ( this->Representation == VTK_PLANE_WIREFRAME )
    {
    os << "Wireframe\n";
    }
  else if ( this->Representation == VTK_PLANE_SURFACE )
    {
    os << "Surface\n";
    }
  else //( this->Representation == VTK_PLANE_OUTLINE )
    {
    os << "Outline\n";
    }

  os << indent << "Normal To X Axis: " 
     << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: " 
     << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: " 
     << (this->NormalToZAxis ? "On" : "Off") << "\n";

  int res = this->PlaneSource->GetXResolution();
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  os << indent << "Resolution: " << res << "\n";
  os << indent << "Origin: (" << o[0] << ", "
                              << o[1] << ", "
                              << o[2] << ")\n";
  os << indent << "Point 1: (" << pt1[0] << ", "
                               << pt1[1] << ", "
                               << pt1[2] << ")\n";
  os << indent << "Point 2: (" << pt2[0] << ", "
                               << pt2[1] << ", "
                               << pt2[2] << ")\n";
}

void vtkPlaneWidget::PositionHandles()
{
  //int res = this->PlaneSource->GetXResolution();
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  this->HandleGeometry[0]->SetCenter(o);
  this->HandleGeometry[1]->SetCenter(pt1);
  this->HandleGeometry[2]->SetCenter(pt2);

  float x[3];
  x[0] = o[0] + (pt1[0]-o[0]) + (pt2[0]-o[0]);
  x[1] = o[1] + (pt1[1]-o[1]) + (pt2[1]-o[1]);
  x[2] = o[2] + (pt1[2]-o[2]) + (pt2[2]-o[2]);
  this->HandleGeometry[3]->SetCenter(x); //far corner

  // set up the outline
  if ( this->Representation == VTK_PLANE_OUTLINE )
    {
    this->PlaneOutline->GetPoints()->SetPoint(0,o);
    this->PlaneOutline->GetPoints()->SetPoint(1,pt1);
    this->PlaneOutline->GetPoints()->SetPoint(2,x);
    this->PlaneOutline->GetPoints()->SetPoint(3,pt2);
    this->PlaneOutline->Modified();
    }
  this->SelectRepresentation();

  // Create the normal vector
  float center[3];
  this->PlaneSource->GetCenter(center);
  this->LineSource->SetPoint1(center);
  float p2[3];
  this->PlaneSource->GetNormal(this->Normal);
  vtkMath::Normalize(this->Normal);
  float d = sqrt( vtkMath::Distance2BetweenPoints(
    this->PlaneSource->GetPoint1(),this->PlaneSource->GetPoint2()) );
  p2[0] = center[0] + 0.35 * d * this->Normal[0];
  p2[1] = center[1] + 0.35 * d * this->Normal[1];
  p2[2] = center[2] + 0.35 * d * this->Normal[2];
  this->LineSource->SetPoint2(p2);
  this->ConeSource->SetCenter(p2);
  this->ConeSource->SetDirection(this->Normal);
}

int vtkPlaneWidget::HighlightHandle(vtkProp *prop)
{
  // first unhighlight anything picked
  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->HandleProperty);
    }

  this->CurrentHandle = (vtkActor *)prop;

  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    for (int i=0; i<4; i++) //find handle
      {
      if ( this->CurrentHandle == this->Handle[i] )
        {
        return i;
        }
      }
    }
  
  return -1;
}

void vtkPlaneWidget::HighlightNormal(int highlight)
{
  if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedHandleProperty);
    this->ConeActor->SetProperty(this->SelectedHandleProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->HandleProperty);
    this->ConeActor->SetProperty(this->HandleProperty);
    }
}

void vtkPlaneWidget::HighlightPlane(int highlight)
{
  if ( highlight )
    {
    this->PlaneActor->SetProperty(this->SelectedPlaneProperty);
    }
  else
    {
    this->PlaneActor->SetProperty(this->PlaneProperty);
    }
}

void vtkPlaneWidget::OnLeftButtonDown (int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift), int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkPlaneWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->HighlightHandle(path->GetFirstNode()->GetProp());
    }
  else
    {
    this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->PlanePicker->GetPath();
    if ( path != NULL )
      {
      vtkProp *prop = path->GetFirstNode()->GetProp();
      if ( prop == this->ConeActor || prop == this->LineActor )
        {
        this->HighlightNormal(1);
        this->State = vtkPlaneWidget::Rotating;
        }
      else
        {
        this->HighlightPlane(1);
        }
      }
    else
      {
      this->HighlightHandle(NULL);
      this->State = vtkPlaneWidget::Outside;
      return;
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnLeftButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift), 
                                     int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkPlaneWidget::Start;
  this->HighlightHandle(NULL);
  this->HighlightPlane(0);
  this->HighlightNormal(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnMiddleButtonDown (int vtkNotUsed(ctrl), 
                                         int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkPlaneWidget::Pushing;

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->PlanePicker->GetPath();
    }
  
  if ( path == NULL ) //nothing picked
    {
    this->State = vtkPlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);
  this->HighlightNormal(1);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnMiddleButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift), 
                                       int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkPlaneWidget::Start;
  this->HighlightPlane(0);
  this->HighlightNormal(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnRightButtonDown (int vtkNotUsed(ctrl), 
                                        int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkPlaneWidget::Scaling;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->PlanePicker->GetPath();
    if ( path == NULL )
      {
      this->State = vtkPlaneWidget::Outside;
      this->HighlightPlane(0);
      return;
      }
    else
      {
      this->HighlightPlane(1);
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnRightButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift), 
                                      int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkPlaneWidget::Start;
  this->HighlightPlane(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPlaneWidget::OnMouseMove (int vtkNotUsed(ctrl), 
                                  int vtkNotUsed(shift), int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkPlaneWidget::Outside || 
       this->State == vtkPlaneWidget::Start )
    {
    return;
    }
  
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkRenderer *renderer = this->Interactor->FindPokedRenderer(X,Y);
  vtkCamera *camera = renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),double(this->Interactor->GetLastEventPosition()[1]),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkPlaneWidget::Moving )
    {
    // Okay to process
    if ( this->CurrentHandle )
      {
      if ( this->CurrentHandle == this->Handle[0] )
        {
        this->MoveOrigin(prevPickPoint, pickPoint);
        }
      else if ( this->CurrentHandle == this->Handle[1] )
        {
        this->MovePoint1(prevPickPoint, pickPoint);
        }
      else if ( this->CurrentHandle == this->Handle[2] )
        {
        this->MovePoint2(prevPickPoint, pickPoint);
        }
      else if ( this->CurrentHandle == this->Handle[3] )
        {
        this->MovePoint3(prevPickPoint, pickPoint);
        }
      }
    else //must be moving the plane
      {
      this->Translate(prevPickPoint, pickPoint);
      }
    }
  else if ( this->State == vtkPlaneWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }
  else if ( this->State == vtkPlaneWidget::Pushing )
    {
    this->Push(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkPlaneWidget::Rotating )
    {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
}

void vtkPlaneWidget::MoveOrigin(double *p1, double *p2)
{
  //Get the plane definition
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  //Get the vector of motion
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // The point opposite the origin (pt3) stays fixed
  float pt3[3];
  pt3[0] = o[0] + (pt1[0] - o[0]) + (pt2[0] - o[0]);
  pt3[1] = o[1] + (pt1[1] - o[1]) + (pt2[1] - o[1]);
  pt3[2] = o[2] + (pt1[2] - o[2]) + (pt2[2] - o[2]);

  // Define vectors from point pt3
  float p13[3], p23[3];
  p13[0] = pt1[0] - pt3[0];
  p13[1] = pt1[1] - pt3[1];
  p13[2] = pt1[2] - pt3[2];
  p23[0] = pt2[0] - pt3[0];
  p23[1] = pt2[1] - pt3[1];
  p23[2] = pt2[2] - pt3[2];

  float vN = vtkMath::Norm(v);
  float n13 = vtkMath::Norm(p13);
  float n23 = vtkMath::Norm(p23);

  // Project v onto these vector to determine the amount of motion
  // Scale it by the relative size of the motion to the vector length
  float d1 = (vN/n13) * vtkMath::Dot(v,p13) / (vN*n13);
  float d2 = (vN/n23) * vtkMath::Dot(v,p23) / (vN*n23);

  float point1[3], point2[3], origin[3];
  for (int i=0; i<3; i++)
    {
    point1[i] = pt3[i] + (1.0+d1)*p13[i];
    point2[i] = pt3[i] + (1.0+d2)*p23[i];
    origin[i] = pt3[i] + (1.0+d1)*p13[i] + (1.0+d2)*p23[i];
    }
  
  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->SetPoint2(point2);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::MovePoint1(double *p1, double *p2)
{
  //Get the plane definition
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  //Get the vector of motion
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Need the point opposite the origin (pt3) 
  float pt3[3];
  pt3[0] = o[0] + (pt1[0] - o[0]) + (pt2[0] - o[0]);
  pt3[1] = o[1] + (pt1[1] - o[1]) + (pt2[1] - o[1]);
  pt3[2] = o[2] + (pt1[2] - o[2]) + (pt2[2] - o[2]);

  // Define vectors from point pt2
  float p32[3], p02[3];
  p02[0] = o[0] - pt2[0];
  p02[1] = o[1] - pt2[1];
  p02[2] = o[2] - pt2[2];
  p32[0] = pt3[0] - pt2[0];
  p32[1] = pt3[1] - pt2[1];
  p32[2] = pt3[2] - pt2[2];

  float vN = vtkMath::Norm(v);
  float n02 = vtkMath::Norm(p02);
  float n32 = vtkMath::Norm(p32);

  // Project v onto these vector to determine the amount of motion
  // Scale it by the relative size of the motion to the vector length
  float d1 = (vN/n02) * vtkMath::Dot(v,p02) / (vN*n02);
  float d2 = (vN/n32) * vtkMath::Dot(v,p32) / (vN*n32);

  float point1[3], origin[3];
  for (int i=0; i<3; i++)
    {
    origin[i] = pt2[i] + (1.0+d1)*p02[i];
    point1[i] = pt2[i] + (1.0+d1)*p02[i] + (1.0+d2)*p32[i];
    }
  
  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::MovePoint2(double *p1, double *p2)
{
  //Get the plane definition
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  //Get the vector of motion
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // The point opposite point2 (pt1) stays fixed
  float pt3[3];
  pt3[0] = o[0] + (pt1[0] - o[0]) + (pt2[0] - o[0]);
  pt3[1] = o[1] + (pt1[1] - o[1]) + (pt2[1] - o[1]);
  pt3[2] = o[2] + (pt1[2] - o[2]) + (pt2[2] - o[2]);

  // Define vectors from point pt1
  float p01[3], p31[3];
  p31[0] = pt3[0] - pt1[0];
  p31[1] = pt3[1] - pt1[1];
  p31[2] = pt3[2] - pt1[2];
  p01[0] = o[0] - pt1[0];
  p01[1] = o[1] - pt1[1];
  p01[2] = o[2] - pt1[2];

  float vN = vtkMath::Norm(v);
  float n31 = vtkMath::Norm(p31);
  float n01 = vtkMath::Norm(p01);

  // Project v onto these vector to determine the amount of motion
  // Scale it by the relative size of the motion to the vector length
  float d1 = (vN/n31) * vtkMath::Dot(v,p31) / (vN*n31);
  float d2 = (vN/n01) * vtkMath::Dot(v,p01) / (vN*n01);

  float point2[3], origin[3];
  for (int i=0; i<3; i++)
    {
    point2[i] = pt1[i] + (1.0+d1)*p31[i] + (1.0+d2)*p01[i];
    origin[i] = pt1[i] + (1.0+d2)*p01[i];
    }
  
  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint2(point2);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::MovePoint3(double *p1, double *p2)
{
  //Get the plane definition
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  //Get the vector of motion
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Define vectors from point pt3
  float p10[3], p20[3];
  p10[0] = pt1[0] - o[0];
  p10[1] = pt1[1] - o[1];
  p10[2] = pt1[2] - o[2];
  p20[0] = pt2[0] - o[0];
  p20[1] = pt2[1] - o[1];
  p20[2] = pt2[2] - o[2];

  float vN = vtkMath::Norm(v);
  float n10 = vtkMath::Norm(p10);
  float n20 = vtkMath::Norm(p20);

  // Project v onto these vector to determine the amount of motion
  // Scale it by the relative size of the motion to the vector length
  float d1 = (vN/n10) * vtkMath::Dot(v,p10) / (vN*n10);
  float d2 = (vN/n20) * vtkMath::Dot(v,p20) / (vN*n20);

  float point1[3], point2[3];
  for (int i=0; i<3; i++)
    {
    point1[i] = o[i] + (1.0+d1)*p10[i];
    point2[i] = o[i] + (1.0+d2)*p20[i];
    }
  
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->SetPoint2(point2);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();
  float *center = this->PlaneSource->GetCenter();

  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
    {
    return;
    }
  int *size = this->CurrentRenderer->GetSize();
  double l2 = (X-this->Interactor->GetLastEventPosition()[0])*(X-this->Interactor->GetLastEventPosition()[0]) + (Y-this->Interactor->GetLastEventPosition()[1])*(Y-this->Interactor->GetLastEventPosition()[1]);
  theta = 360.0 * sqrt(l2/((double)size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0],center[1],center[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-center[0],-center[1],-center[2]);

  //Set the corners
  float oNew[3], pt1New[3], pt2New[3];
  this->Transform->TransformPoint(o,oNew);
  this->Transform->TransformPoint(pt1,pt1New);
  this->Transform->TransformPoint(pt2,pt2New);

  this->PlaneSource->SetOrigin(oNew);
  this->PlaneSource->SetPoint1(pt1New);
  this->PlaneSource->SetPoint2(pt2New);
  this->PlaneSource->Update();

  this->PositionHandles();
}

// Loop through all points and translate them
void vtkPlaneWidget::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->PlaneSource->GetXResolution();
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  float origin[3], point1[3], point2[3];
  for (int i=0; i<3; i++)
    {
    origin[i] = o[i] + v[i];
    point1[i] = pt1[i] + v[i];
    point2[i] = pt2[i] + v[i];
    }
  
  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->SetPoint2(point2);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->PlaneSource->GetXResolution();
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  float center[3];
  center[0] = o[0] + (pt1[0]-o[0])/2.0 + (pt2[0]-o[0])/2.0;
  center[1] = o[1] + (pt1[1]-o[1])/2.0 + (pt2[1]-o[1])/2.0;
  center[2] = o[2] + (pt1[2]-o[2])/2.0 + (pt2[2]-o[2])/2.0;

  // Compute the scale factor
  float sf = vtkMath::Norm(v) / sqrt(vtkMath::Distance2BetweenPoints(pt1,pt2));
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  // Move the corner points
  float origin[3], point1[3], point2[3];
  for (int i=0; i<3; i++)
    {
    origin[i] = sf * (o[i] - center[i]) + center[i];
    point1[i] = sf * (pt1[i] - center[i]) + center[i];
    point2[i] = sf * (pt2[i] - center[i]) + center[i];
    }

  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->SetPoint2(point2);
  this->PlaneSource->Update();

  this->PositionHandles();
}

void vtkPlaneWidget::Push(double *p1, double *p2)
{
  //Get the motion vector
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  this->PlaneSource->Push( vtkMath::Dot(v,this->Normal) );
  this->PlaneSource->Update();
  this->PositionHandles();
}

void vtkPlaneWidget::CreateDefaultProperties()
{
  if ( ! this->HandleProperty )
    {
    this->HandleProperty = vtkProperty::New();
    this->HandleProperty->SetColor(1,1,1);
    }
  if ( ! this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty = vtkProperty::New();
    this->SelectedHandleProperty->SetColor(1,0,0);
    }
  
  if ( ! this->PlaneProperty )
    {
    this->PlaneProperty = vtkProperty::New();
    this->SelectRepresentation();
    this->PlaneProperty->SetAmbient(1.0);
    this->PlaneProperty->SetAmbientColor(1.0,1.0,1.0);
    }
  if ( ! this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty = vtkProperty::New();
    this->SelectRepresentation();
    this->SelectedPlaneProperty->SetAmbient(1.0);
    this->SelectedPlaneProperty->SetAmbientColor(0.0,1.0,0.0);
    }
}

void vtkPlaneWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  
  if ( this->NormalToYAxis )
    {
    this->PlaneSource->SetOrigin(bounds[0],center[1],bounds[4]);
    this->PlaneSource->SetPoint1(bounds[1],center[1],bounds[4]);
    this->PlaneSource->SetPoint2(bounds[0],center[1],bounds[5]);
    }
  else if ( this->NormalToZAxis )
    {
    this->PlaneSource->SetOrigin(bounds[0],bounds[2],center[2]);
    this->PlaneSource->SetPoint1(bounds[1],bounds[2],center[2]);
    this->PlaneSource->SetPoint2(bounds[0],bounds[3],center[2]);
    }
  else //default or x-normal
    {
    this->PlaneSource->SetOrigin(center[0],bounds[2],bounds[4]);
    this->PlaneSource->SetPoint1(center[0],bounds[3],bounds[4]);
    this->PlaneSource->SetPoint2(center[0],bounds[2],bounds[5]);
    }
  this->PlaneSource->Update();

  // Position the handles at the end of the planes
  this->PositionHandles();

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // Set the radius on the sphere handles
  for(i=0; i<4; i++)
    {
    this->HandleGeometry[i]->SetRadius(0.025*this->InitialLength);
    }

  // Set the height and radius of the cone
  this->ConeSource->SetHeight(0.060*this->InitialLength);
  this->ConeSource->SetRadius(0.025*this->InitialLength);  
}

void vtkPlaneWidget::SelectRepresentation()
{
  if ( ! this->CurrentRenderer )
    {
    return;
    }

  if ( this->Representation == VTK_PLANE_OFF )
    {
    this->CurrentRenderer->RemoveActor(this->PlaneActor);
    }
  else if ( this->Representation == VTK_PLANE_OUTLINE )
    {
    this->CurrentRenderer->RemoveActor(this->PlaneActor);
    this->CurrentRenderer->AddActor(this->PlaneActor);
    this->PlaneMapper->SetInput( this->PlaneOutline );
        this->PlaneActor->GetProperty()->SetRepresentationToWireframe();
    }
  else if ( this->Representation == VTK_PLANE_SURFACE )
    {
    this->CurrentRenderer->RemoveActor(this->PlaneActor);
    this->CurrentRenderer->AddActor(this->PlaneActor);
    this->PlaneMapper->SetInput( this->PlaneSource->GetOutput() );
    this->PlaneActor->GetProperty()->SetRepresentationToSurface();
    }
  else //( this->Representation == VTK_PLANE_WIREFRAME )
    {
    this->CurrentRenderer->RemoveActor(this->PlaneActor);
    this->CurrentRenderer->AddActor(this->PlaneActor);
    this->PlaneMapper->SetInput( this->PlaneSource->GetOutput() );
    this->PlaneActor->GetProperty()->SetRepresentationToWireframe();
    }
}

// Description:
// Set/Get the resolution (number of subdivisions) of the plane.
void vtkPlaneWidget::SetResolution(int r)
{
  this->PlaneSource->SetXResolution(r); 
  this->PlaneSource->SetYResolution(r); 
}

int vtkPlaneWidget::GetResolution()
{ 
  return this->PlaneSource->GetXResolution(); 
}

// Description:
// Set/Get the origin of the plane.
void vtkPlaneWidget::SetOrigin(float x, float y, float z) 
{
  this->PlaneSource->SetOrigin(x,y,z);
}

void vtkPlaneWidget::SetOrigin(float x[3]) 
{
  this->PlaneSource->SetOrigin(x);
}

float* vtkPlaneWidget::GetOrigin() 
{
  return this->PlaneSource->GetOrigin();
}

void vtkPlaneWidget::GetOrigin(float xyz[3]) 
{
  this->PlaneSource->GetOrigin(xyz);
}

// Description:
// Set/Get the position of the point defining the first axis of the plane.
void vtkPlaneWidget::SetPoint1(float x, float y, float z) 
{
  this->PlaneSource->SetPoint1(x,y,z);
}

void vtkPlaneWidget::SetPoint1(float x[3]) 
{
  this->PlaneSource->SetPoint1(x);
}

float* vtkPlaneWidget::GetPoint1() 
{
  return this->PlaneSource->GetPoint1();
}

void vtkPlaneWidget::GetPoint1(float xyz[3]) 
{
  this->PlaneSource->GetPoint1(xyz);
}

// Description:
// Set/Get the position of the point defining the second axis of the plane.
void vtkPlaneWidget::SetPoint2(float x, float y, float z) 
{
  this->PlaneSource->SetPoint2(x,y,z);
}

void vtkPlaneWidget::SetPoint2(float x[3]) 
{
  this->PlaneSource->SetPoint2(x);
}

float* vtkPlaneWidget::GetPoint2() 
{
  return this->PlaneSource->GetPoint2();
}

void vtkPlaneWidget::GetPoint2(float xyz[3]) 
{
  this->PlaneSource->GetPoint2(xyz);
}

// Description:
// Get the center of the plane.
float* vtkPlaneWidget::GetCenter() 
{
  return this->PlaneSource->GetCenter();
}

void vtkPlaneWidget::GetCenter(float xyz[3]) 
{
  this->PlaneSource->GetCenter(xyz);
}

// Description:
// Get the normal to the plane.
float* vtkPlaneWidget::GetNormal() 
{
  return this->PlaneSource->GetNormal();
}

void vtkPlaneWidget::GetNormal(float xyz[3]) 
{
  this->PlaneSource->GetNormal(xyz);
}

void vtkPlaneWidget::GetPolyData(vtkPolyData *pd)
{ 
  pd->ShallowCopy(this->PlaneSource->GetOutput()); 
}


void vtkPlaneWidget::RealiseGeometry(void)
{
  this->PlaneSource->Update();
  this->PositionHandles();
}
