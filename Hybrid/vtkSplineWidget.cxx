/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineWidget.cxx
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
#include "vtkSplineWidget.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCardinalSpline.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSpline.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkSplineWidget, "1.16");
vtkStandardNewMacro(vtkSplineWidget);

vtkCxxSetObjectMacro(vtkSplineWidget, HandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, SelectedHandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, LineProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkSplineWidget, SelectedLineProperty, vtkProperty);

vtkSplineWidget::vtkSplineWidget()
{
  this->State = vtkSplineWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkSplineWidget::ProcessEvents);
  this->ProjectToPlane = 0;  //default off
  this->ProjectionNormal = 0;  //default YZ not used
  this->ProjectionPosition = 0.0;
  this->PlaneSource = NULL;
  this->SplinePositions = NULL;
  this->Closed = 0;
  this->Offset = 0.0;

  // Build the representation of the widget

  this->XSpline = this->CreateDefaultSpline();
  this->XSpline->Register(this);
  this->XSpline->Delete();
  this->YSpline = this->CreateDefaultSpline();
  this->YSpline->Register(this);
  this->YSpline->Delete();
  this->ZSpline = this->CreateDefaultSpline();
  this->ZSpline->Register(this);
  this->ZSpline->Delete();

  this->XSpline->ClosedOff();
  this->YSpline->ClosedOff();
  this->ZSpline->ClosedOff();

  // Default bounds to get started
  float bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Create the handles along a straight line within the data bounds
  this->NumberOfHandles = 5;
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleMapper   = new vtkPolyDataMapper* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];
  int i;
  float position;
  float x0 = bounds[0];
  float x1 = bounds[1];
  float y0 = bounds[2];
  float y1 = bounds[3];
  float z0 = bounds[4];
  float z1 = bounds[5];
  float x;
  float y;
  float z;
  for (i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    position = i / (this->NumberOfHandles - 1.0);
    x = (1.0-position)*x0 + position*x1;
    y = (1.0-position)*y0 + position*y1;
    z = (1.0-position)*z0 + position*z1;
    this->XSpline->AddPoint(i, x);
    this->YSpline->AddPoint(i, y);
    this->ZSpline->AddPoint(i, z);
    this->HandleGeometry[i]->SetCenter(x,y,z);
    }

  this->XSpline->Compute();
  this->YSpline->Compute();
  this->ZSpline->Compute();

  // Define the points and line segments representing the spline
  this->Resolution = 499;
  this->NumberOfSplinePoints = this->Resolution + 1;
  this->SplinePositions = new float [this->NumberOfSplinePoints];

  vtkPoints* points = vtkPoints::New();
  points->Allocate(this->NumberOfSplinePoints);

  // Interpolate x, y and z by using the three spline filters and
  // create new points
  float factor = (this->NumberOfHandles + this->Offset - 1.0)/
                 (this->NumberOfSplinePoints - 1.0);
  for (i=0; i<this->NumberOfSplinePoints; i++)
    {
    position = i * factor;
    this->SplinePositions[i] = position;
    points->InsertPoint(i, XSpline->Evaluate(position),
                           YSpline->Evaluate(position),
                           ZSpline->Evaluate(position));
    }

  // Create the polyline representation of the spline
  //
  vtkCellArray* lines = vtkCellArray::New();
  lines->Allocate(lines->EstimateSize(this->Resolution,2));
  lines->InsertNextCell(this->NumberOfSplinePoints);

  for (i=0 ; i<this->NumberOfSplinePoints; i++)
    {
    lines->InsertCellPoint(i);
    }

  this->LineData = vtkPolyData::New();
  this->LineData->SetPoints(points);
  points->Delete();
  this->LineData->SetLines(lines);
  lines->Delete();

  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput( this->LineData ) ;
  this->LineMapper->ImmediateModeRenderingOn();
  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper( this->LineMapper);

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.0;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005);
  for (i=0; i<this->NumberOfHandles; i++)
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
  delete [] this->SplinePositions;

  if ( this->XSpline)
    {
    this->XSpline->UnRegister(this);
    }
  if ( this->YSpline)
    {
    this->YSpline->UnRegister(this);
    }
  if ( this->ZSpline)
    {
    this->ZSpline->UnRegister(this);
    }

  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineData->Delete();

  for (int i=0; i<this->NumberOfHandles; i++)
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

  this->Transform->Delete();
}

void vtkSplineWidget::SetClosed(int closed)
{
  if (this->Closed == closed)
    {
    return;
    }
  this->Closed = closed;
  this->XSpline->SetClosed(this->Closed);
  this->YSpline->SetClosed(this->Closed);
  this->ZSpline->SetClosed(this->Closed);

  this->Offset  = (this->Closed)? 1.0 : 0.0;

  float factor = (this->NumberOfHandles + this->Offset - 1.0)/
                 (this->NumberOfSplinePoints - 1.0);
  for (int i=0; i<this->NumberOfSplinePoints; i++)
    {
    this->SplinePositions[i] = i * factor;
    }

  this->BuildRepresentation();
}

// Creates an instance of a vtkCardinalSpline by default
vtkSpline* vtkSplineWidget::CreateDefaultSpline()
{
  return vtkCardinalSpline::New();
}

void vtkSplineWidget::SetXSpline(vtkSpline* spline)
{
  if (this->XSpline != spline)
    {
    // to avoid destructor recursion
    vtkSpline *temp = this->XSpline;
    this->XSpline = spline;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->XSpline != NULL)
      {
      this->XSpline->Register(this);
      }
    }
}

void vtkSplineWidget::SetYSpline(vtkSpline* spline)
{
  if (this->YSpline != spline)
    {
    // to avoid destructor recursion
    vtkSpline *temp = this->YSpline;
    this->YSpline = spline;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->YSpline != NULL)
      {
      this->YSpline->Register(this);
      }
    }
}

void vtkSplineWidget::SetZSpline(vtkSpline* spline)
{
  if (this->XSpline != spline)
    {
    // to avoid destructor recursion
    vtkSpline *temp = this->ZSpline;
    this->ZSpline = spline;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->ZSpline != NULL)
      {
      this->ZSpline->Register(this);
      }
    }
}

void vtkSplineWidget::SetHandlePosition(int handle, float x, float y, float z)
{
  if(handle < 0 || handle >= this->NumberOfHandles)
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

void vtkSplineWidget::SetHandlePosition(int handle, float xyz[3])
{
  this->SetHandlePosition(handle,xyz[0],xyz[1],xyz[2]);
}

void vtkSplineWidget::GetHandlePosition(int handle, float xyz[3])
{
  if(handle < 0 || handle >= this->NumberOfHandles)
    {
    vtkErrorMacro(<<"vtkSplineWidget: handle index out of range.");
    return;
    }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

float* vtkSplineWidget::GetHandlePosition(int handle)
{
  if(handle < 0 || handle >= this->NumberOfHandles)
    {
    vtkErrorMacro(<<"vtkSplineWidget: handle index out of range.");
    return NULL;
    }

  return this->HandleGeometry[handle]->GetCenter();
}

void vtkSplineWidget::SetEnabled(int enabling)
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

    if ( ! this->CurrentRenderer )
      {
      this->CurrentRenderer = this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]);
      if (this->CurrentRenderer == NULL)
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
    for (int j=0; j<this->NumberOfHandles; j++)
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

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }

    this->Enabled = 0;

    // Don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // Turn off the line
    this->CurrentRenderer->RemoveActor(this->LineActor);

    // Turn off the handles
    for (int i=0; i<this->NumberOfHandles; i++)
      {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
      }

    this->CurrentHandle = NULL;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->CurrentRenderer = NULL;
    }

  this->Interactor->Render();
}

void vtkSplineWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                  unsigned long event,
                                  void* clientdata,
                                  void* vtkNotUsed(calldata))
{
  vtkSplineWidget* self = reinterpret_cast<vtkSplineWidget *>( clientdata );

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
  if ( this->XSpline )
    {
    os << indent << "XSpline: "
       << this->XSpline << "\n";
    }
  else
    {
    os << indent << "XSpline: (none)\n";
    }
  if ( this->YSpline )
    {
    os << indent << "YSpline: "
       << this->YSpline << "\n";
    }
  else
    {
    os << indent << "YSpline: (none)\n";
    }
  if ( this->ZSpline )
    {
    os << indent << "ZSpline: "
       << this->ZSpline << "\n";
    }
  else
    {
    os << indent << "ZSpline: (none)\n";
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
  if (this->ProjectionNormal == VTK_PROJECTION_OBLIQUE)
    {
    if(this->PlaneSource != NULL)
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
  float o[3];
  float u[3];
  float v[3];

  this->PlaneSource->GetPoint1(u);
  this->PlaneSource->GetPoint2(v);
  this->PlaneSource->GetOrigin(o);

  int i;
  for(i=0;i<3;i++)
    {
    u[i]=u[i]-o[i];
    v[i]=v[i]-o[i];
    }
  vtkMath::Normalize(u);
  vtkMath::Normalize(v);

  float o_dot_u = vtkMath::Dot(o,u);
  float o_dot_v = vtkMath::Dot(o,v);
  float fac1;
  float fac2;
  float ctr[3];
  for (i=0; i<this->NumberOfHandles; i++)
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
  float ctr[3];
  for (int i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[  this->ProjectionNormal ] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
    }
}

void vtkSplineWidget::BuildRepresentation()
{
  // Handles have changed position, re-compute the spline coeffs
  this->XSpline->RemoveAllPoints();
  this->YSpline->RemoveAllPoints();
  this->ZSpline->RemoveAllPoints();

  float ctr[3];
  int i;
  for (i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->XSpline->AddPoint(i,ctr[0]);
    this->YSpline->AddPoint(i,ctr[1]);
    this->ZSpline->AddPoint(i,ctr[2]);
    }

  this->XSpline->Compute();
  this->YSpline->Compute();
  this->ZSpline->Compute();

  vtkPoints* points = this->LineData->GetPoints();
  float position;
  for (i = 0; i<this->NumberOfSplinePoints; i++)
    {
    position = this->SplinePositions[i];
    points->SetPoint(i,this->XSpline->Evaluate(position),
                       this->YSpline->Evaluate(position),
                       this->ZSpline->Evaluate(position));
    }
}

int vtkSplineWidget::HighlightHandle(vtkProp *prop)
{
  // First unhighlight anything picked
  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->HandleProperty);
    }

  this->CurrentHandle = (vtkActor *)prop;

  if ( this->CurrentHandle )
    {
    for (int i=0; i<this->NumberOfHandles; i++) // find handle
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
  vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
  if ( ren != this->CurrentRenderer )
    {
    this->State = vtkSplineWidget::Outside;
    return;
    }

  this->State = vtkSplineWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
  vtkAssemblyPath *path;
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->CurrentHandleIndex = this->HighlightHandle(path->GetFirstNode()->GetProp());
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
  vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
  if ( ren != this->CurrentRenderer )
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
  vtkAssemblyPath *path;
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->LinePicker->GetPath();
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
  vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
  if ( ren != this->CurrentRenderer )
    {
    this->State = vtkSplineWidget::Outside;
    return;
    }

  this->State = vtkSplineWidget::Scaling;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->LinePicker->GetPath();
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

void vtkSplineWidget::OnRightButtonUp()
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

  vtkRenderer *renderer = this->Interactor->FindPokedRenderer(X,Y);
  vtkCamera *camera = renderer->GetActiveCamera();
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

  float *ctr = this->HandleGeometry[ this->CurrentHandleIndex ]->GetCenter();

  float newCtr[3];
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

  float newCtr[3];
  for (int i = 0; i< this->NumberOfHandles; i++)
    {
    float* ctr =  this->HandleGeometry[i]->GetCenter();
    for (int j=0; j<3; j++)
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

  float center[3] = {0.0,0.0,0.0};
  float avgdist = 0.0;
  float *prevctr = this->HandleGeometry[0]->GetCenter();
  float *ctr;

  center[0] += prevctr[0];
  center[1] += prevctr[1];
  center[2] += prevctr[2];

  int i;
  for (i = 1; i<this->NumberOfHandles; i++)
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
  float sf = vtkMath::Norm(v) / avgdist;
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }

  // Move the handle points
  float newCtr[3];
  for (i = 0; i< this->NumberOfHandles; i++)
    {
    ctr = this->HandleGeometry[i]->GetCenter();
    for (int j=0; j<3; j++)
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
    if(this->ProjectionNormal == VTK_PROJECTION_OBLIQUE && this->PlaneSource != NULL)
      {
      float* normal = this->PlaneSource->GetNormal();
      axis[0] = normal[0];
      axis[1] = normal[1];
      axis[2] = normal[2];
      vtkMath::Normalize(axis);
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
  float rs = vtkMath::Normalize(rv);

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
  float newCtr[3];
  float ctr[3];
  for (int i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Transform->TransformPoint(ctr,newCtr);
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
    }
}

void vtkSplineWidget::CreateDefaultProperties()
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
    this->LineProperty->SetColor(1.0,1.0,0.0);
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

void vtkSplineWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];
  this->AdjustBounds(bds, bounds, center);

  if ( this->ProjectToPlane )
    {
    this->ProjectPointsToPlane();
    }
  else  //place the center
    {
    // Create a default straight line within the data bounds
    float x0 = bounds[0];
    float x1 = bounds[1];
    float y0 = bounds[2];
    float y1 = bounds[3];
    float z0 = bounds[4];
    float z1 = bounds[5];
    float x;
    float y;
    float z;
    float position;
    for (i=0; i<this->NumberOfHandles; i++)
      {
      position = i / (this->NumberOfHandles - 1.0);
      x = (1.0-position)*x0 + position*x1;
      y = (1.0-position)*y0 + position*y1;
      z = (1.0-position)*z0 + position*z1;
      this->HandleGeometry[i]->SetCenter(x,y,z);
      }
    }

  for (i=0; i<6; i++)
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

void vtkSplineWidget::SetProjectionPosition(float position)
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
  if (this->NumberOfHandles == npts)
    {
    return;
    }
  if (npts < 2)
    {
    vtkGenericWarningMacro(<<"vtkSplineWidget: minimum of 2 points required.");
    return;
    }
      
  float radius = this->HandleGeometry[0]->GetRadius();
  float factor = (this->NumberOfHandles - 1.0)/(npts - 1.0);
  this->Initialize();

  this->NumberOfHandles = npts;

  // Create the handles
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleMapper   = new vtkPolyDataMapper* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];

  float x,y,z;
  int i;
  for (i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    this->Handle[i]->SetProperty(this->HandleProperty);
    x = XSpline->Evaluate(i * factor);
    y = YSpline->Evaluate(i * factor);
    z = ZSpline->Evaluate(i * factor);
    this->HandleGeometry[i]->SetCenter(x,y,z);
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
    }

  factor = (this->NumberOfHandles + this->Offset - 1.0)/
           (this->NumberOfSplinePoints - 1.0);

  for (i=0; i<this->NumberOfSplinePoints; i++)
    {
    this->SplinePositions[i] = i * factor;
    }

  this->BuildRepresentation();

  if ( this->Interactor )
    {
    this->CurrentRenderer = this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],
    this->Interactor->GetLastEventPosition()[1]);
    if (this->CurrentRenderer != NULL)
      {
      for (i=0; i<this->NumberOfHandles; i++)
        {
        this->CurrentRenderer->AddProp(this->Handle[i]);
        }
      }
      this->Interactor->Render();
    }
}

void vtkSplineWidget::Initialize(void)
{
  int i;
  if ( this->Interactor )
    {
    this->CurrentRenderer = this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],
    this->Interactor->GetLastEventPosition()[1]);
    if ( this->CurrentRenderer != NULL)
      {
      for (i=0; i<this->NumberOfHandles; i++)
        {
        this->CurrentRenderer->RemoveProp(this->Handle[i]);
        }
      }
    }

  for (i=0; i<this->NumberOfHandles; i++)
    {
    this->HandlePicker->DeletePickList(this->Handle[i]);
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }

  this->NumberOfHandles = 0;

  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;
}

void vtkSplineWidget::SetResolution(int resolution)
{
  if (this->Resolution == resolution || resolution < (this->NumberOfHandles-1))
    {
    return;
    }

  this->NumberOfSplinePoints = resolution + 1;

  if(resolution > this->Resolution)  //only delete when necessary
    {
    delete [] this->SplinePositions;
    if ( (this->SplinePositions = new float[this->NumberOfSplinePoints]) == NULL )
      {
      vtkErrorMacro(<<"vtkSplineWidget: failed to reallocate SplinePositions.");
      return;
      }
    }

  this->Resolution = resolution;

  vtkPoints* newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfSplinePoints);
  vtkCellArray *newLines  = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(this->Resolution,2));

  float factor = (this->NumberOfHandles + this->Offset - 1.0)/
                 (this->NumberOfSplinePoints - 1.0);
  float position;
  int i;
  for (i=0; i<this->NumberOfSplinePoints; i++)
    {
    position = i * factor;
    this->SplinePositions[i] = position;
    newPoints->InsertPoint(i, XSpline->Evaluate(position),
                           YSpline->Evaluate(position),
                           ZSpline->Evaluate(position));
    }

  newLines->InsertNextCell(this->NumberOfSplinePoints);
  for (i=0; i < this->NumberOfSplinePoints; i++)
    {
    newLines->InsertCellPoint(i);
    }

  this->LineData->SetPoints(newPoints);
  newPoints->Delete();
  this->LineData->SetLines(newLines);
  newLines->Delete();
}

void vtkSplineWidget::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->LineData);
}

void vtkSplineWidget::SizeHandles()
{
  float radius = this->vtk3DWidget::SizeHandles(1.0);
  for (int i=0; i<this->NumberOfHandles; i++)
    {
    this->HandleGeometry[i]->SetRadius(radius);
    }
}

float vtkSplineWidget::GetSummedLength()
{
  vtkPoints* points = this->LineData->GetPoints();
  int npts = points->GetNumberOfPoints();

  if (npts < 2) { return 0.0f; }

  float a[3];
  float b[3];
  float sum = 0.0f;
  int i = 0;
  points->GetPoint(i,a);
  int imax = (npts%2 == 0)?npts-2:npts-1;

  while(i<imax)
    {
    points->GetPoint(i+1,b);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a,b));
    i = i + 2;
    points->GetPoint(i,a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a,b));
    }

  if(npts%2 == 0)
    {
    points->GetPoint(i+1,b);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a,b));
    }

  return sum;
}

void vtkSplineWidget::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  float ctr[3];
  for (int i = 0; i<this->NumberOfHandles; i++)
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

