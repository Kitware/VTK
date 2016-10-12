/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplineWidget.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPickingManager.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSplineWidget);

vtkCxxSetObjectMacro(vtkSplineWidget, HandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, SelectedHandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, LineProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, SelectedLineProperty, vtkProperty);

vtkSplineWidget::vtkSplineWidget()
{
  this->State = vtkSplineWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkSplineWidget::ProcessEventsHandler);
  this->ProjectToPlane = 0;  //default off
  this->ProjectionNormal = 0;  //default YZ not used
  this->ProjectionPosition = 0.0;
  this->PlaneSource = NULL;
  this->Closed = 0;

  // Does this widget respond to interaction?
  this->ProcessEvents = 1;

  // Build the representation of the widget

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

  // Create the handles along a straight line within the bounds of a unit cube
  this->NumberOfHandles = 5;
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];
  int i;
  double u[3];
  double x0 = bounds[0];
  double x1 = bounds[1];
  double y0 = bounds[2];
  double y1 = bounds[3];
  double z0 = bounds[4];
  double z1 = bounds[5];
  double x;
  double y;
  double z;
  vtkPoints* points = vtkPoints::New(VTK_DOUBLE);
  points->SetNumberOfPoints(this->NumberOfHandles);

  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkPolyDataMapper* handleMapper = vtkPolyDataMapper::New();
    handleMapper->SetInputConnection(
      this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    u[0] = i/(this->NumberOfHandles - 1.0);
    x = (1.0 - u[0])*x0 + u[0]*x1;
    y = (1.0 - u[0])*y0 + u[0]*y1;
    z = (1.0 - u[0])*z0 + u[0]*z1;
    points->SetPoint(i, x, y, z);
    this->HandleGeometry[i]->SetCenter(x,y,z);
  }

  // vtkParametric spline acts as the interpolating engine
  this->ParametricSpline = vtkParametricSpline::New();
  this->ParametricSpline->Register(this);
  this->ParametricSpline->SetPoints(points);
  this->ParametricSpline->ParameterizeByLengthOff();
  points->Delete();
  this->ParametricSpline->Delete();

  // Define the points and line segments representing the spline
  this->Resolution = 499;

  this->ParametricFunctionSource = vtkParametricFunctionSource::New();
  this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
  this->ParametricFunctionSource->SetScalarModeToNone();
  this->ParametricFunctionSource->GenerateTextureCoordinatesOff();
  this->ParametricFunctionSource->SetUResolution( this->Resolution );
  this->ParametricFunctionSource->Update();

  vtkPolyDataMapper* lineMapper = vtkPolyDataMapper::New();
  lineMapper->SetInputConnection(
    this->ParametricFunctionSource->GetOutputPort());
  lineMapper->ImmediateModeRenderingOn();
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper( lineMapper );
  lineMapper->Delete();

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.0;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005);

  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = vtkCellPicker::New();
  this->LinePicker->SetTolerance(0.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->CurrentHandle = NULL;
  this->CurrentHandleIndex = -1;

  this->Transform = vtkTransform::New();

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->LineProperty = NULL;
  this->SelectedLineProperty = NULL;
  this->CreateDefaultProperties();
}

vtkSplineWidget::~vtkSplineWidget()
{
  if ( this->ParametricSpline )
  {
    this->ParametricSpline->UnRegister(this);
  }

  this->ParametricFunctionSource->Delete();

  this->LineActor->Delete();

  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete [] this->Handle;
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

  this->Transform->Delete();
}

void vtkSplineWidget::SetClosed(int closed)
{
  if ( this->Closed == closed )
  {
    return;
  }
  this->Closed = closed;
  this->ParametricSpline->SetClosed(this->Closed);

  this->BuildRepresentation();
}

void vtkSplineWidget::SetParametricSpline(vtkParametricSpline* spline)
{
  if ( this->ParametricSpline != spline )
  {
    // to avoid destructor recursion
    vtkParametricSpline *temp = this->ParametricSpline;
    this->ParametricSpline = spline;
    if (temp != NULL)
    {
      temp->UnRegister(this);
    }
    if (this->ParametricSpline != NULL)
    {
      this->ParametricSpline->Register(this);
      this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
    }
  }
}

void vtkSplineWidget::SetHandlePosition(int handle, double x,
                                        double y, double z)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkSplineWidget: handle index out of range.");
    return;
  }
  this->HandleGeometry[handle]->SetCenter(x,y,z);
  this->HandleGeometry[handle]->Update();
  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

void vtkSplineWidget::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle,xyz[0],xyz[1],xyz[2]);
}

void vtkSplineWidget::GetHandlePosition(int handle, double xyz[3])
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkSplineWidget: handle index out of range.");
    return;
  }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

double* vtkSplineWidget::GetHandlePosition(int handle)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkSplineWidget: handle index out of range.");
    return NULL;
  }

  return this->HandleGeometry[handle]->GetCenter();
}

void vtkSplineWidget::SetEnabled(int enabling)
{
  if ( !this->Interactor )
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

    if ( !this->CurrentRenderer )
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));
      if ( this->CurrentRenderer == NULL )
      {
        return;
      }
    }

    this->Enabled = 1;

    // Listen for the following events
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

    // Turn on the handles
    for ( int j = 0; j < this->NumberOfHandles; ++j )
    {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
    }
    this->BuildRepresentation();
    this->SizeHandles();

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
  }

  else //disabling----------------------------------------------------------
  {
    vtkDebugMacro(<<"Disabling line widget");

    if ( !this->Enabled ) //already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // Don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // Turn off the line
    this->CurrentRenderer->RemoveActor(this->LineActor);

    // Turn off the handles
    for ( int i = 0; i < this->NumberOfHandles; ++i )
    {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
    }

    this->CurrentHandle = NULL;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
  }

  this->Interactor->Render();
}

//----------------------------------------------------------------------
void vtkSplineWidget::RegisterPickers()
{
  this->Interactor->GetPickingManager()->AddPicker(this->HandlePicker, this);
  this->Interactor->GetPickingManager()->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------
void vtkSplineWidget::ProcessEventsHandler(vtkObject* vtkNotUsed(object),
                                  unsigned long event,
                                  void* clientdata,
                                  void* vtkNotUsed(calldata))
{
  vtkSplineWidget* self = reinterpret_cast<vtkSplineWidget *>( clientdata );

  // if ProcessEvents is Off, we ignore all interaction events.
  if (!self->GetProcessEvents())
  {
    return;
  }

  // Okay, let's do the right thing
  switch(event)
  {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
  }
}

void vtkSplineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ProcessEvents: "
    << (this->ProcessEvents? "On" : "Off") << "\n";

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
    os << indent << "Selected Handle Property: (none)\n";
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
  if ( this->ParametricSpline )
  {
    os << indent << "ParametricSpline: "
       << this->ParametricSpline << "\n";
  }
  else
  {
    os << indent << "ParametricSpline: (none)\n";
  }

  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Projection Normal: " << this->ProjectionNormal << "\n";
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";
  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Number Of Handles: " << this->NumberOfHandles << "\n";
  os << indent << "Closed: "
     << (this->Closed ? "On" : "Off") << "\n";
}

void vtkSplineWidget::ProjectPointsToPlane()
{
  if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE )
  {
    if ( this->PlaneSource != NULL )
    {
      this->ProjectPointsToObliquePlane();
    }
    else
    {
      vtkGenericWarningMacro(<<"Set the plane source for oblique projections...");
    }
  }
  else
  {
    this->ProjectPointsToOrthoPlane();
  }
}

void vtkSplineWidget::ProjectPointsToObliquePlane()
{
  double o[3];
  double u[3];
  double v[3];

  this->PlaneSource->GetPoint1(u);
  this->PlaneSource->GetPoint2(v);
  this->PlaneSource->GetOrigin(o);

  int i;
  for ( i = 0; i < 3; ++i )
  {
    u[i] = u[i] - o[i];
    v[i] = v[i] - o[i];
  }
  vtkMath::Normalize(u);
  vtkMath::Normalize(v);

  double o_dot_u = vtkMath::Dot(o,u);
  double o_dot_v = vtkMath::Dot(o,v);
  double fac1;
  double fac2;
  double ctr[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    fac1 = vtkMath::Dot(ctr,u) - o_dot_u;
    fac2 = vtkMath::Dot(ctr,v) - o_dot_v;
    ctr[0] = o[0] + fac1*u[0] + fac2*v[0];
    ctr[1] = o[1] + fac1*u[1] + fac2*v[1];
    ctr[2] = o[2] + fac1*u[2] + fac2*v[2];
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

void vtkSplineWidget::ProjectPointsToOrthoPlane()
{
  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

void vtkSplineWidget::BuildRepresentation()
{
  // Handles have changed position, re-compute the spline coeffs
  vtkPoints* points = this->ParametricSpline->GetPoints();
  if ( points->GetNumberOfPoints() != this->NumberOfHandles )
  {
    points->SetNumberOfPoints( this->NumberOfHandles );
  }

  double pt[3];
  int i;
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
  }
  this->ParametricSpline->Modified();
}

int vtkSplineWidget::HighlightHandle(vtkProp *prop)
{
  // First unhighlight anything picked
  if ( this->CurrentHandle )
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<vtkActor *>(prop);

  if ( this->CurrentHandle )
  {
    for ( int i = 0; i < this->NumberOfHandles; ++i ) // find handle
    {
      if ( this->CurrentHandle == this->Handle[i] )
      {
        this->ValidPick = 1;
        this->HandlePicker->GetPickPosition(this->LastPickPosition);
        this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
        return i;
      }
    }
  }
  return -1;
}

void vtkSplineWidget::HighlightLine(int highlight)
{
  if ( highlight )
  {
    this->ValidPick = 1;
    this->LinePicker->GetPickPosition(this->LastPickPosition);
    this->LineActor->SetProperty(this->SelectedLineProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->LineProperty);
  }
}

void vtkSplineWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) )
  {
    this->State = vtkSplineWidget::Outside;
    return;
  }

  this->State = vtkSplineWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if ( path != NULL )
  {
    this->CurrentHandleIndex = this->HighlightHandle(path->GetFirstNode()->GetViewProp());
  }
  else
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if ( path != NULL )
    {
      this->HighlightLine(1);
    }
    else
    {
      this->CurrentHandleIndex = this->HighlightHandle(NULL);
      this->State = vtkSplineWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnLeftButtonUp()
{
  if ( this->State == vtkSplineWidget::Outside ||
       this->State == vtkSplineWidget::Start )
  {
    return;
  }

  this->State = vtkSplineWidget::Start;
  this->HighlightHandle(NULL);
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) )
  {
    this->State = vtkSplineWidget::Outside;
    return;
  }

  if ( this->Interactor->GetControlKey() )
  {
    this->State = vtkSplineWidget::Spinning;
    this->CalculateCentroid();
  }
  else
  {
    this->State = vtkSplineWidget::Moving;
  }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if ( path == NULL )
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if ( path == NULL )
    {
      this->State = vtkSplineWidget::Outside;
      this->HighlightLine(0);
      return;
    }
    else
    {
      this->HighlightLine(1);
    }
  }
  else  //we picked a handle but lets make it look like the line is picked
  {
    this->HighlightLine(1);
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnMiddleButtonUp()
{
  if ( this->State == vtkSplineWidget::Outside ||
       this->State == vtkSplineWidget::Start )
  {
    return;
  }

  this->State = vtkSplineWidget::Start;
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) )
  {
    this->State = vtkSplineWidget::Outside;
    return;
  }

  if ( this->Interactor->GetShiftKey() )
  {
    this->State = vtkSplineWidget::Inserting;
  }
  else if ( this->Interactor->GetControlKey() )
  {
    this->State = vtkSplineWidget::Erasing;
  }
  else
  {
    this->State = vtkSplineWidget::Scaling;
  }

  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if ( path != NULL )
  {
    switch ( this->State )
    {
      // deny insertion over existing handles
      case vtkSplineWidget::Inserting:
        this->State = vtkSplineWidget::Outside;
        return;
      case vtkSplineWidget::Erasing:
        this->CurrentHandleIndex = \
        this->HighlightHandle(path->GetFirstNode()->GetViewProp());
        break;
      case vtkSplineWidget::Scaling:
        this->HighlightLine(1);
        break;
    }
  }
  else
  {
    // trying to erase handle but nothing picked
    if ( this->State == vtkSplineWidget::Erasing )
    {
      this->State = vtkSplineWidget::Outside;
      return;
    }
    // try to insert or scale so pick the line
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if ( path != NULL )
    {
      this->HighlightLine(1);
    }
    else
    {
      this->State = vtkSplineWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnRightButtonUp()
{
  if ( this->State == vtkSplineWidget::Outside ||
       this->State == vtkSplineWidget::Start )
  {
    return;
  }

  if ( this->State == vtkSplineWidget::Inserting )
  {
    this->InsertHandleOnLine(this->LastPickPosition);
  }
  else if ( this->State == vtkSplineWidget::Erasing )
  {
    int index = this->CurrentHandleIndex;
    this->CurrentHandleIndex = this->HighlightHandle(NULL);
    this->EraseHandle(index);
  }

  this->State = vtkSplineWidget::Start;
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkSplineWidget::Outside ||
       this->State == vtkSplineWidget::Start )
  {
    return;
  }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
  {
    return;
  }

  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1],
                              this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkSplineWidget::Moving )
  {
    // Okay to process
    if ( this->CurrentHandle )
    {
      this->MovePoint(prevPickPoint, pickPoint);
    }
    else // Must be moving the spline
    {
      this->Translate(prevPickPoint, pickPoint);
    }
  }
  else if ( this->State == vtkSplineWidget::Scaling )
  {
    this->Scale(prevPickPoint, pickPoint, X, Y);
  }
  else if ( this->State == vtkSplineWidget::Spinning )
  {
    camera->GetViewPlaneNormal(vpn);
    this->Spin(prevPickPoint, pickPoint, vpn);
  }

  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }

  this->BuildRepresentation();

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSplineWidget::MovePoint(double *p1, double *p2)
{
  if ( this->CurrentHandleIndex < 0 || this->CurrentHandleIndex >= this->NumberOfHandles )
  {
    vtkGenericWarningMacro(<<"Spline handle index out of range.");
    return;
  }
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *ctr = this->HandleGeometry[this->CurrentHandleIndex]->GetCenter();

  double newCtr[3];
  newCtr[0] = ctr[0] + v[0];
  newCtr[1] = ctr[1] + v[1];
  newCtr[2] = ctr[2] + v[2];

  this->HandleGeometry[this->CurrentHandleIndex]->SetCenter(newCtr);
  this->HandleGeometry[this->CurrentHandleIndex]->Update();
}

void vtkSplineWidget::Translate(double *p1, double *p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double newCtr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    double* ctr =  this->HandleGeometry[i]->GetCenter();
    for ( int j = 0; j < 3; ++j )
    {
      newCtr[j] = ctr[j] + v[j];
    }
     this->HandleGeometry[i]->SetCenter(newCtr);
     this->HandleGeometry[i]->Update();
  }
}

void vtkSplineWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double center[3] = {0.0,0.0,0.0};
  double avgdist = 0.0;
  double *prevctr = this->HandleGeometry[0]->GetCenter();
  double *ctr;

  center[0] += prevctr[0];
  center[1] += prevctr[1];
  center[2] += prevctr[2];

  int i;
  for ( i = 1; i < this->NumberOfHandles; ++i )
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    center[0] += ctr[0];
    center[1] += ctr[1];
    center[2] += ctr[2];
    avgdist += sqrt(vtkMath::Distance2BetweenPoints(ctr,prevctr));
    prevctr = ctr;
  }

  avgdist /= this->NumberOfHandles;

  center[0] /= this->NumberOfHandles;
  center[1] /= this->NumberOfHandles;
  center[2] /= this->NumberOfHandles;

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / avgdist;
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  // Move the handle points
  double newCtr[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    for ( int j = 0; j < 3; ++j )
    {
      newCtr[j] = sf * (ctr[j] - center[j]) + center[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

void vtkSplineWidget::Spin(double *p1, double *p2, double *vpn)
{
  // Mouse motion vector in world space
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Axis of rotation
  double axis[3] = {0.0,0.0,0.0};

  if ( this->ProjectToPlane )
  {
    if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE )
    {
      if (this->PlaneSource != NULL )
      {
        double* normal = this->PlaneSource->GetNormal();
        axis[0] = normal[0];
        axis[1] = normal[1];
        axis[2] = normal[2];
        vtkMath::Normalize(axis);
      }
      else
      {
        axis[ 0 ] = 1.0;
      }
    }
    else
    {
      axis[ this->ProjectionNormal ] = 1.0;
    }
  }
  else
  {
  // Create axis of rotation and angle of rotation
    vtkMath::Cross(vpn,v,axis);
    if ( vtkMath::Normalize(axis) == 0.0 )
    {
      return;
    }
  }

  // Radius vector (from mean center to cursor position)
  double rv[3] = {p2[0] - this->Centroid[0],
                  p2[1] - this->Centroid[1],
                  p2[2] - this->Centroid[2]};

  // Distance between center and cursor location
  double rs = vtkMath::Normalize(rv);

  // Spin direction
  double ax_cross_rv[3];
  vtkMath::Cross(axis,rv,ax_cross_rv);

  // Spin angle
  double theta = 360.0 * vtkMath::Dot(v,ax_cross_rv) / rs;

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(this->Centroid[0],this->Centroid[1],this->Centroid[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-this->Centroid[0],-this->Centroid[1],-this->Centroid[2]);

  // Set the handle points
  double newCtr[3];
  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Transform->TransformPoint(ctr,newCtr);
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

void vtkSplineWidget::CreateDefaultProperties()
{
  if ( !this->HandleProperty )
  {
    this->HandleProperty = vtkProperty::New();
    this->HandleProperty->SetColor(1,1,1);
  }
  if ( !this->SelectedHandleProperty )
  {
    this->SelectedHandleProperty = vtkProperty::New();
    this->SelectedHandleProperty->SetColor(1,0,0);
  }

  if ( !this->LineProperty )
  {
    this->LineProperty = vtkProperty::New();
    this->LineProperty->SetRepresentationToWireframe();
    this->LineProperty->SetAmbient(1.0);
    this->LineProperty->SetColor(1.0,1.0,0.0);
    this->LineProperty->SetLineWidth(2.0);
  }
  if ( !this->SelectedLineProperty )
  {
    this->SelectedLineProperty = vtkProperty::New();
    this->SelectedLineProperty->SetRepresentationToWireframe();
    this->SelectedLineProperty->SetAmbient(1.0);
    this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
    this->SelectedLineProperty->SetLineWidth(2.0);
  }
}

void vtkSplineWidget::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];
  this->AdjustBounds(bds, bounds, center);

  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }
  else  //place the center
  {
    // Create a default straight line within the data bounds
    double x0 = bounds[0];
    double x1 = bounds[1];
    double y0 = bounds[2];
    double y1 = bounds[3];
    double z0 = bounds[4];
    double z1 = bounds[5];
    double x;
    double y;
    double z;
    double u;
    for ( i = 0; i < this->NumberOfHandles; ++i )
    {
      u = i/(this->NumberOfHandles - 1.0);
      x = (1.0 - u)*x0 + u*x1;
      y = (1.0 - u)*y0 + u*y1;
      z = (1.0 - u)*z0 + u*z1;
      this->HandleGeometry[i]->SetCenter(x,y,z);
    }
  }

  for ( i = 0; i < 6; ++i )
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // Re-compute the spline coeffs
  this->BuildRepresentation();
  this->SizeHandles();
}

void vtkSplineWidget::SetProjectionPosition(double position)
{
  this->ProjectionPosition = position;
  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

void vtkSplineWidget::SetPlaneSource(vtkPlaneSource* plane)
{
  if (this->PlaneSource == plane)
  {
    return;
  }
  this->PlaneSource = plane;
}

void vtkSplineWidget::SetNumberOfHandles(int npts)
{
  if ( this->NumberOfHandles == npts )
  {
    return;
  }
  if (npts < 2)
  {
    vtkGenericWarningMacro(<<"vtkSplineWidget: minimum of 2 points required.");
    return;
  }

  double radius = this->HandleGeometry[0]->GetRadius();
  this->Initialize();

  this->NumberOfHandles = npts;

  // Create the handles
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];

  int i;
  double pt[3];
  double u[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkPolyDataMapper* handleMapper = vtkPolyDataMapper::New();
    handleMapper->SetInputConnection(
      this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    this->Handle[i]->SetProperty(this->HandleProperty);
    u[0] = i/(this->NumberOfHandles - 1.0);
    this->ParametricSpline->Evaluate(u, pt, NULL);
    this->HandleGeometry[i]->SetCenter(pt);
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
  }

  this->BuildRepresentation();

  if ( this->Interactor )
  {
    if ( !this->CurrentRenderer )
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));
    }
    if ( this->CurrentRenderer != NULL )
    {
      for ( i = 0; i < this->NumberOfHandles; ++i )
      {
        this->CurrentRenderer->AddViewProp(this->Handle[i]);
      }
      this->SizeHandles();
    }
      this->Interactor->Render();
  }
}

void vtkSplineWidget::Initialize(void)
{
  int i;
  if ( this->Interactor )
  {
    if ( !this->CurrentRenderer )
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));
    }
    if ( this->CurrentRenderer != NULL)
    {
      for ( i = 0; i < this->NumberOfHandles; ++i )
      {
        this->CurrentRenderer->RemoveViewProp(this->Handle[i]);
      }
    }
  }

  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandlePicker->DeletePickList(this->Handle[i]);
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }

  this->NumberOfHandles = 0;

  delete [] this->Handle;
  delete [] this->HandleGeometry;
}

void vtkSplineWidget::SetResolution(int resolution)
{
  if ( this->Resolution == resolution || resolution < (this->NumberOfHandles-1) )
  {
    return;
  }

  this->Resolution = resolution;
  this->ParametricFunctionSource->SetUResolution( this->Resolution );
  this->ParametricFunctionSource->Modified();
}

void vtkSplineWidget::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy( this->ParametricFunctionSource->GetOutput() );
}

void vtkSplineWidget::SizeHandles()
{
  double radius = this->vtk3DWidget::SizeHandles(1.0);
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->SetRadius(radius);
  }
}

double vtkSplineWidget::GetSummedLength()
{
  vtkPoints* points = this->ParametricFunctionSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if ( npts < 2 ) { return 0.0; }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts%2 == 0) ? npts-2 : npts-1;

  while ( i < imax )
  {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  if ( npts%2 == 0 )
  {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

void vtkSplineWidget::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
  }

  this->Centroid[0] /= this->NumberOfHandles;
  this->Centroid[1] /= this->NumberOfHandles;
  this->Centroid[2] /= this->NumberOfHandles;
}

void vtkSplineWidget::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2) { return; }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1){ return; }

  vtkIdType subid = this->LinePicker->GetSubId();

  vtkPoints* newpoints = vtkPoints::New(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles+1);

  int istart = vtkMath::Floor(subid*(this->NumberOfHandles + this->Closed - 1.0)/static_cast<double>(this->Resolution));
  int istop = istart + 1;
  int count = 0;
  int i;
  for ( i = 0; i <= istart; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  newpoints->SetPoint(count++,pos);

  for ( i = istop; i < this->NumberOfHandles; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

void vtkSplineWidget::EraseHandle(const int& index)
{
  if ( this->NumberOfHandles < 3 || index < 0 || index >= this->NumberOfHandles )
  {
    return;
  }

  vtkPoints* newpoints = vtkPoints::New(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles-1);
  int count = 0;
  for (int i = 0; i < this->NumberOfHandles; ++i )
  {
    if ( i != index )
    {
      newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
    }
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

void vtkSplineWidget::InitializeHandles(vtkPoints* points)
{
  if ( !points ){ return; }

  int npts = points->GetNumberOfPoints();
  if ( npts < 2 ){ return; }

  double p0[3];
  double p1[3];

  points->GetPoint(0,p0);
  points->GetPoint(npts-1,p1);

  if ( vtkMath::Distance2BetweenPoints(p0,p1) == 0.0 )
  {
    --npts;
    this->Closed = 1;
    this->ParametricSpline->ClosedOn();
  }

  this->SetNumberOfHandles(npts);
  int i;
  for ( i = 0; i < npts; ++i )
  {
    this->SetHandlePosition(i,points->GetPoint(i));
  }

  if ( this->Interactor && this->Enabled )
  {
    this->Interactor->Render();
  }
}

int vtkSplineWidget::IsClosed()
{
  if ( this->NumberOfHandles < 3 || !this->Closed ) { return 0; }

  vtkPolyData* lineData = this->ParametricFunctionSource->GetOutput();
  if ( !lineData || !(lineData->GetPoints()) )
  {
    vtkErrorMacro(<<"No line data to query geometric closure");
    return 0;
  }

  vtkPoints *points = lineData->GetPoints();
  int numPoints = points->GetNumberOfPoints();

  if ( numPoints < 3 )
  {
    return 0;
  }

  int numEntries = lineData->GetLines()->GetNumberOfConnectivityEntries();

  double p0[3];
  double p1[3];

  points->GetPoint( 0, p0 );
  points->GetPoint( numPoints - 1, p1 );
  int minusNth = ( p0[0] == p1[0] && p0[1] == p1[1] && p0[2] == p1[2] ) ? 1 : 0;
  int result;
  if ( minusNth ) //definitely closed
  {
    result = 1;
  }
  else       // not physically closed, check connectivity
  {
    result = ( ( numEntries - numPoints ) == 2 ) ? 1 : 0;
  }

  return result;
}
