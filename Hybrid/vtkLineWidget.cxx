/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineWidget.cxx
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
#include "vtkLineWidget.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkFloatArray.h"
#include "vtkCellPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkDoubleArray.h"
#include "vtkPlanes.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkLineWidget, "1.11");
vtkStandardNewMacro(vtkLineWidget);

vtkLineWidget::vtkLineWidget()
{
  this->State = vtkLineWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkLineWidget::ProcessEvents);
  
  this->AlignWithXAxis = 0;
  this->AlignWithYAxis = 0;
  this->AlignWithZAxis = 0;

  //Build the representation of the widget
  int i;
  // Represent the line
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(5);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // Create the handles
  this->Handle = new vtkActor* [2];
  this->HandleMapper = new vtkPolyDataMapper* [2];
  this->HandleGeometry = new vtkSphereSource* [2];
  for (i=0; i<2; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }
  
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
  for (i=0; i<2; i++)
    {
    this->HandlePicker->AddPickList(this->Handle[i]);
    }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = vtkCellPicker::New();
  this->LinePicker->SetTolerance(0.005); //need some fluff
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();
  
  this->CurrentHandle = NULL;

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->LineProperty = NULL;
  this->SelectedLineProperty = NULL;
  this->CreateDefaultProperties();
}

vtkLineWidget::~vtkLineWidget()
{
  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineSource->Delete();

  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;
  
  this->HandlePicker->Delete();
  this->LinePicker->Delete();

  if ( this->HandleProperty )
    {
    this->HandleProperty->Delete();
    }
  if ( this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty->Delete();
    }
  if ( this->LineProperty )
    {
    this->LineProperty->Delete();
    }
  if ( this->SelectedLineProperty )
    {
    this->SelectedLineProperty->Delete();
    }
}

void vtkLineWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //------------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling line widget");

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
    i->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);

    // Add the line
    this->CurrentRenderer->AddActor(this->LineActor);
    this->LineActor->SetProperty(this->LineProperty);

    // turn on the handles
    for (int j=0; j<2; j++)
      {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
      }

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling line widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the line
    this->CurrentRenderer->RemoveActor(this->LineActor);

    // turn off the handles
    for (int i=0; i<2; i++)
      {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
      }

    this->CurrentHandle = NULL;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkLineWidget::ProcessEvents(vtkObject* object, unsigned long event,
                                  void* clientdata, void* vtkNotUsed(calldata))
{
  vtkLineWidget* self = reinterpret_cast<vtkLineWidget *>( clientdata );
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

void vtkLineWidget::PrintSelf(ostream& os, vtkIndent indent)
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

  if ( this->LineProperty )
    {
    os << indent << "Line Property: " << this->LineProperty << "\n";
    }
  else
    {
    os << indent << "Line Property: (none)\n";
    }
  if ( this->SelectedLineProperty )
    {
    os << indent << "Selected Line Property: " 
       << this->SelectedLineProperty << "\n";
    }
  else
    {
    os << indent << "Selected Line Property: (none)\n";
    }

  os << indent << "Align With X Axis: " 
     << (this->AlignWithXAxis ? "On" : "Off") << "\n";
  os << indent << "Align With Y Axis: " 
     << (this->AlignWithYAxis ? "On" : "Off") << "\n";
  os << indent << "Align With Z Axis: " 
     << (this->AlignWithZAxis ? "On" : "Off") << "\n";

  int res = this->LineSource->GetResolution();
  float *pt1 = this->LineSource->GetPoint1();
  float *pt2 = this->LineSource->GetPoint2();

  os << indent << "Resolution: " << res << "\n";
  os << indent << "Point 1: (" << pt1[0] << ", "
                               << pt1[1] << ", "
                               << pt1[2] << ")\n";
  os << indent << "Point 2: (" << pt2[0] << ", "
                               << pt2[1] << ", "
                               << pt2[2] << ")\n";
}

void vtkLineWidget::PositionHandles()
{
  //int res = this->LineSource->GetResolution();
  float *pt1 = this->LineSource->GetPoint1();
  float *pt2 = this->LineSource->GetPoint2();

  this->HandleGeometry[0]->SetCenter(pt1);
  this->HandleGeometry[1]->SetCenter(pt2);
}

int vtkLineWidget::HighlightHandle(vtkProp *prop)
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
    for (int i=0; i<2; i++) //find handle
      {
      if ( this->CurrentHandle == this->Handle[i] )
        {
        return i;
        }
      }
    }
  
  return -1;
}

void vtkLineWidget::HighlightLine(int highlight)
{
  if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedLineProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->LineProperty);
    }
}

void vtkLineWidget::OnLeftButtonDown (int vtkNotUsed(ctrl), 
                                      int vtkNotUsed(shift), 
                                      int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkLineWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
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
    this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->LinePicker->GetPath();
    if ( path != NULL )
      {
      this->HighlightLine(1);
      }
    else
      {
      this->HighlightHandle(NULL);
      this->State = vtkLineWidget::Outside;
      return;
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::OnMouseMove (int vtkNotUsed(ctrl), 
                                 int vtkNotUsed(shift), int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkLineWidget::Outside || 
       this->State == vtkLineWidget::Start )
    {
    return;
    }
  
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

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
  if ( this->State == vtkLineWidget::Moving )
    {
    // Okay to process
    if ( this->CurrentHandle && !this->AlignWithXAxis && 
         !this->AlignWithYAxis && !this->AlignWithZAxis )
      {
      if ( this->CurrentHandle == this->Handle[0] )
        {
        this->MovePoint1(prevPickPoint, pickPoint);
        }
      else if ( this->CurrentHandle == this->Handle[1] )
        {
        this->MovePoint2(prevPickPoint, pickPoint);
        }
      }
    else //must be moving the line
      {
      this->Translate(prevPickPoint, pickPoint);
      }
    }
  else if ( this->State == vtkLineWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
}

void vtkLineWidget::OnLeftButtonUp (int vtkNotUsed(ctrl), 
                                    int vtkNotUsed(shift), 
                                    int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkLineWidget::Outside )
    {
    return;
    }

  this->State = vtkLineWidget::Start;
  this->HighlightHandle(NULL);
  this->HighlightLine(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::OnMiddleButtonDown (int vtkNotUsed(ctrl), 
                                        int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkLineWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->LinePicker->GetPath();
    if ( path == NULL )
      {
      this->State = vtkLineWidget::Outside;
      this->HighlightLine(0);
      return;
      }
    else
      {
      this->HighlightLine(1);
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::OnMiddleButtonUp (int vtkNotUsed(ctrl), 
                                      int vtkNotUsed(shift), 
                                      int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkLineWidget::Outside )
    {
    return;
    }

  this->State = vtkLineWidget::Start;
  this->HighlightLine(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::OnRightButtonDown (int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkLineWidget::Scaling;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->LinePicker->GetPath();
    if ( path == NULL )
      {
      this->State = vtkLineWidget::Outside;
      this->HighlightLine(0);
      return;
      }
    else
      {
      this->HighlightLine(1);
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::OnRightButtonUp (int vtkNotUsed(ctrl), 
                                     int vtkNotUsed(shift), 
                                     int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkLineWidget::Outside )
    {
    return;
    }

  this->State = vtkLineWidget::Start;
  this->HighlightLine(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkLineWidget::MovePoint1(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->LineSource->GetResolution();
  float *pt1 = this->LineSource->GetPoint1();

  float point1[3];
  point1[0] = pt1[0] + v[0];
  point1[1] = pt1[1] + v[1];
  point1[2] = pt1[2] + v[2];
  
  this->LineSource->SetPoint1(point1);
  this->LineSource->Update();

  this->PositionHandles();
}

void vtkLineWidget::MovePoint2(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->LineSource->GetResolution();
  float *pt2 = this->LineSource->GetPoint2();

  float point2[3];
  point2[0] = pt2[0] + v[0];
  point2[1] = pt2[1] + v[1];
  point2[2] = pt2[2] + v[2];
  
  this->LineSource->SetPoint2(point2);
  this->LineSource->Update();

  this->PositionHandles();
}

// Loop through all points and translate them
void vtkLineWidget::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->LineSource->GetResolution();
  float *pt1 = this->LineSource->GetPoint1();
  float *pt2 = this->LineSource->GetPoint2();

  float point1[3], point2[3];
  for (int i=0; i<3; i++)
    {
    point1[i] = pt1[i] + v[i];
    point2[i] = pt2[i] + v[i];
    }
  
  this->LineSource->SetPoint1(point1);
  this->LineSource->SetPoint2(point2);
  this->LineSource->Update();

  this->PositionHandles();
}

void vtkLineWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->LineSource->GetResolution();
  float *pt1 = this->LineSource->GetPoint1();
  float *pt2 = this->LineSource->GetPoint2();

  float center[3];
  center[0] = (pt1[0]+pt2[0]) / 2.0;
  center[1] = (pt1[1]+pt2[1]) / 2.0;
  center[2] = (pt1[2]+pt2[2]) / 2.0;

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
  
  // Move the end points
  float point1[3], point2[3];
  for (int i=0; i<3; i++)
    {
    point1[i] = sf * (pt1[i] - center[i]) + center[i];
    point2[i] = sf * (pt2[i] - center[i]) + center[i];
    }

  this->LineSource->SetPoint1(point1);
  this->LineSource->SetPoint2(point2);
  this->LineSource->Update();

  this->PositionHandles();
}

void vtkLineWidget::CreateDefaultProperties()
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
  
  if ( ! this->LineProperty )
    {
    this->LineProperty = vtkProperty::New();
    this->LineProperty->SetRepresentationToWireframe();
    this->LineProperty->SetAmbient(1.0);
    this->LineProperty->SetAmbientColor(1.0,1.0,1.0);
    this->LineProperty->SetLineWidth(2.0);
    }
  if ( ! this->SelectedLineProperty )
    {
    this->SelectedLineProperty = vtkProperty::New();
    this->SelectedLineProperty->SetRepresentationToWireframe();
    this->SelectedLineProperty->SetAmbient(1.0);
    this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
    this->SelectedLineProperty->SetLineWidth(2.0);
    }
}

void vtkLineWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  
  if ( this->AlignWithYAxis )
    {
    this->LineSource->SetPoint1(center[0],bounds[2],center[2]);
    this->LineSource->SetPoint2(center[0],bounds[3],center[2]);
    }
  else if ( this->AlignWithZAxis )
    {
    this->LineSource->SetPoint1(center[0],center[1],bounds[4]);
    this->LineSource->SetPoint2(center[0],center[1],bounds[5]);
    }
  else //default or x-aligned
    {
    this->LineSource->SetPoint1(bounds[0],center[1],center[2]);
    this->LineSource->SetPoint2(bounds[1],center[1],center[2]);
    }
  this->LineSource->Update();

  // Position the handles at the end of the lines
  this->PositionHandles();

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  for(i=0; i<2; i++)
    {
    this->HandleGeometry[i]->SetRadius(0.025*this->InitialLength);
    }
}

