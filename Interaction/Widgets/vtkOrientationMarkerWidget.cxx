/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientationMarkerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrientationMarkerWidget.h"

#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(vtkOrientationMarkerWidget);

vtkCxxSetObjectMacro(vtkOrientationMarkerWidget, OrientationMarker, vtkProp);

class vtkOrientationMarkerWidgetObserver : public vtkCommand
{
public:
  static vtkOrientationMarkerWidgetObserver *New()
    {return new vtkOrientationMarkerWidgetObserver;};

  vtkOrientationMarkerWidgetObserver()
    {
    this->OrientationMarkerWidget = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event, void *calldata)
    {
      if (this->OrientationMarkerWidget)
        {
        this->OrientationMarkerWidget->ExecuteCameraUpdateEvent(wdg, event, calldata);
        }
    }

  vtkOrientationMarkerWidget *OrientationMarkerWidget;
};

//-------------------------------------------------------------------------
vtkOrientationMarkerWidget::vtkOrientationMarkerWidget()
{
  this->StartEventObserverId = 0;
  this->EventCallbackCommand->SetCallback( vtkOrientationMarkerWidget::ProcessEvents );

  this->Observer = vtkOrientationMarkerWidgetObserver::New();
  this->Observer->OrientationMarkerWidget = this;

  this->Tolerance = 7;
  this->Moving = 0;

  this->Renderer = vtkRenderer::New();
  this->Renderer->SetViewport( 0.0, 0.0, 0.2, 0.2 );
  this->Renderer->SetLayer( 1 );
  this->Renderer->InteractiveOff();

  this->Priority = 0.55;
  this->OrientationMarker = NULL;
  this->State = vtkOrientationMarkerWidget::Outside;
  this->Interactive = 1;

  this->Outline = vtkPolyData::New();
  this->Outline->Allocate();
  vtkPoints *points = vtkPoints::New();
  vtkIdType ptIds[5];
  ptIds[4] = ptIds[0] = points->InsertNextPoint( 1, 1, 0 );
  ptIds[1] = points->InsertNextPoint( 2, 1, 0 );
  ptIds[2] = points->InsertNextPoint( 2, 2, 0 );
  ptIds[3] = points->InsertNextPoint( 1, 2, 0 );

  this->Outline->SetPoints( points );
  this->Outline->InsertNextCell( VTK_POLY_LINE, 5, ptIds );

  vtkCoordinate *tcoord = vtkCoordinate::New();
  tcoord->SetCoordinateSystemToDisplay();

  vtkPolyDataMapper2D *mapper = vtkPolyDataMapper2D::New();
  mapper->SetInputData(this->Outline);
  mapper->SetTransformCoordinate( tcoord );

  this->OutlineActor = vtkActor2D::New();
  this->OutlineActor->SetMapper( mapper );
  this->OutlineActor->SetPosition( 0, 0 );
  this->OutlineActor->SetPosition2( 1, 1 );

  points->Delete();
  mapper->Delete();
  tcoord->Delete();
}

//-------------------------------------------------------------------------
vtkOrientationMarkerWidget::~vtkOrientationMarkerWidget()
{
  this->Observer->Delete();
  this->Renderer->Delete();
  this->SetOrientationMarker( NULL );
  this->OutlineActor->Delete();
  this->Outline->Delete();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
    {
    vtkErrorMacro("The interactor must be set prior to enabling/disabling widget");
    }

  if (enabling)
    {
    if (this->Enabled)
      {
      return;
      }

    if (!this->OrientationMarker)
      {
      vtkErrorMacro("An orientation marker must be set prior to enabling/disabling widget");
      return;
      }

    if (!this->CurrentRenderer)
      {
      this->SetCurrentRenderer( this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));

      if (this->CurrentRenderer == NULL)
        {
        return;
        }
      }

    this->Enabled = 1;

    vtkRenderWindow* renwin = this->CurrentRenderer->GetRenderWindow();
    renwin->AddRenderer( this->Renderer );
    if (renwin->GetNumberOfLayers() < 2)
      {
      renwin->SetNumberOfLayers( 2 );
      }

    this->CurrentRenderer->AddViewProp( this->OutlineActor );
    this->OutlineActor->VisibilityOff();
    this->Renderer->AddViewProp( this->OrientationMarker );
    this->OrientationMarker->VisibilityOn();

    if (this->Interactive)
      {
      vtkRenderWindowInteractor *i = this->Interactor;
      if ( this->EventCallbackCommand )
        {
        i->AddObserver( vtkCommand::MouseMoveEvent,
          this->EventCallbackCommand, this->Priority );
        i->AddObserver( vtkCommand::LeftButtonPressEvent,
          this->EventCallbackCommand, this->Priority );
        i->AddObserver( vtkCommand::LeftButtonReleaseEvent,
          this->EventCallbackCommand, this->Priority );
        }
      }

    vtkCamera* pcam = this->CurrentRenderer->GetActiveCamera();
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    if (pcam && cam)
      {
      cam->SetParallelProjection( pcam->GetParallelProjection() );
      }

    // We need to copy the camera before the compositing observer is called.
    // Compositing temporarily changes the camera to display an image.
    this->StartEventObserverId = this->CurrentRenderer->AddObserver(
      vtkCommand::StartEvent, this->Observer, 1 );
    this->InvokeEvent( vtkCommand::EnableEvent, NULL );
    }
  else
    {
    if (!this->Enabled)
      {
      return;
      }

    this->Enabled = 0;
    this->Interactor->RemoveObserver( this->EventCallbackCommand );

    this->OrientationMarker->VisibilityOff();
    this->Renderer->RemoveViewProp( this->OrientationMarker );
    this->OutlineActor->VisibilityOff();
    this->CurrentRenderer->RemoveViewProp( this->OutlineActor );

    // if the render window is still around, remove our renderer from it
    if (this->CurrentRenderer->GetRenderWindow())
      {
      this->CurrentRenderer->GetRenderWindow()->
        RemoveRenderer( this->Renderer );
      }
    if ( this->StartEventObserverId != 0 )
      {
      this->CurrentRenderer->RemoveObserver( this->StartEventObserverId );
      }

    this->InvokeEvent( vtkCommand::DisableEvent, NULL );
    this->SetCurrentRenderer( NULL );
    }
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ExecuteCameraUpdateEvent(vtkObject *vtkNotUsed(o),
                                   unsigned long vtkNotUsed(event),
                                   void *vtkNotUsed(calldata))
{
  if (!this->CurrentRenderer)
    {
    return;
    }

  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  double pos[3], fp[3], viewup[3];
  cam->GetPosition( pos );
  cam->GetFocalPoint( fp );
  cam->GetViewUp( viewup );

  cam = this->Renderer->GetActiveCamera();
  cam->SetPosition( pos );
  cam->SetFocalPoint( fp );
  cam->SetViewUp( viewup );
  this->Renderer->ResetCamera();

  this->UpdateOutline();
}

//-------------------------------------------------------------------------
int vtkOrientationMarkerWidget::ComputeStateBasedOnPosition(int X, int Y,
                                                    int *pos1, int *pos2)
{
  if ( X < (pos1[0]-this->Tolerance) || (pos2[0]+this->Tolerance) < X ||
       Y < (pos1[1]-this->Tolerance) || (pos2[1]+this->Tolerance) < Y )
    {
    return vtkOrientationMarkerWidget::Outside;
    }

  // if we are not outside and the left mouse button wasn't clicked,
  // then we are inside, otherwise we are moving

  int result = this->Moving ? vtkOrientationMarkerWidget::Translating :
      vtkOrientationMarkerWidget::Inside;

  int e1 = 0;
  int e2 = 0;
  int e3 = 0;
  int e4 = 0;
  if (X - pos1[0] < this->Tolerance)
    {
    e1 = 1;
    }
  if (pos2[0] - X < this->Tolerance)
    {
    e3 = 1;
    }
  if (Y - pos1[1] < this->Tolerance)
    {
    e2 = 1;
    }
  if (pos2[1] - Y < this->Tolerance)
    {
    e4 = 1;
    }

  // are we on a corner or an edge?
  if (e1)
    {
    if (e2)
      {
      result = vtkOrientationMarkerWidget::AdjustingP1; // lower left
      }
    if (e4)
      {
      result = vtkOrientationMarkerWidget::AdjustingP4; // upper left
      }
    }
  if (e3)
    {
    if (e2)
      {
      result = vtkOrientationMarkerWidget::AdjustingP2; // lower right
      }
    if (e4)
      {
      result = vtkOrientationMarkerWidget::AdjustingP3;  // upper right
      }
    }

  return result;
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetCursor(int state)
{
  switch (state)
    {
    case vtkOrientationMarkerWidget::AdjustingP1:
      this->RequestCursorShape( VTK_CURSOR_SIZESW );
      break;
    case vtkOrientationMarkerWidget::AdjustingP3:
      this->RequestCursorShape( VTK_CURSOR_SIZENE );
      break;
    case vtkOrientationMarkerWidget::AdjustingP2:
      this->RequestCursorShape( VTK_CURSOR_SIZESE );
      break;
    case vtkOrientationMarkerWidget::AdjustingP4:
      this->RequestCursorShape( VTK_CURSOR_SIZENW );
      break;
    case vtkOrientationMarkerWidget::Translating:
      this->RequestCursorShape( VTK_CURSOR_SIZEALL );
      break;
    case vtkOrientationMarkerWidget::Inside:
      this->RequestCursorShape( VTK_CURSOR_SIZEALL );
      break;
    case vtkOrientationMarkerWidget::Outside:
      this->RequestCursorShape( VTK_CURSOR_DEFAULT );
      break;
    }
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                    unsigned long event,
                                    void *clientdata,
                                    void* vtkNotUsed(calldata))
{
  vtkOrientationMarkerWidget *self =
    reinterpret_cast<vtkOrientationMarkerWidget*>( clientdata );

  if (!self->GetInteractive())
    {
    return;
    }

  switch (event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    }
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::OnLeftButtonDown()
{
  // We're only here if we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  double vp[4];
  this->Renderer->GetViewport( vp );

  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  int pos1[2] = { static_cast<int>( vp[0] ), static_cast<int>( vp[1] ) };
  int pos2[2] = { static_cast<int>( vp[2] ), static_cast<int>( vp[3] ) };

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  // flag that we are attempting to adjust or move the outline
  this->Moving = 1;
  this->State = this->ComputeStateBasedOnPosition( X, Y, pos1, pos2 );
  this->SetCursor( this->State );

  if (this->State == vtkOrientationMarkerWidget::Outside)
    {
    this->Moving = 0;
    return;
    }

  this->EventCallbackCommand->SetAbortFlag( 1 );
  this->StartInteraction();
  this->InvokeEvent( vtkCommand::StartInteractionEvent, NULL );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::OnLeftButtonUp()
{
  if (this->State == vtkOrientationMarkerWidget::Outside)
    {
    return;
    }

  // finalize any corner adjustments
  this->SquareRenderer();
  this->UpdateOutline();

  // stop adjusting
  this->State = vtkOrientationMarkerWidget::Outside;
  this->Moving = 0;

  this->RequestCursorShape( VTK_CURSOR_DEFAULT );
  this->EndInteraction();
  this->InvokeEvent( vtkCommand::EndInteractionEvent, NULL );
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SquareRenderer()
{
  int *size = this->Renderer->GetSize();
  if (size[0] == 0 || size[1] == 0)
    {
    return;
    }

  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  // get the minimum viewport edge size
  //
  double dx = vp[2] - vp[0];
  double dy = vp[3] - vp[1];

  if (dx != dy)
    {
    double delta = dx < dy ? dx : dy;

    switch (this->State)
      {
      case vtkOrientationMarkerWidget::AdjustingP1:
        vp[2] = vp[0] + delta;
        vp[3] = vp[1] + delta;
        break;
      case vtkOrientationMarkerWidget::AdjustingP2:
        vp[0] = vp[2] - delta;
        vp[3] = vp[1] + delta;
        break;
      case vtkOrientationMarkerWidget::AdjustingP3:
        vp[0] = vp[2] - delta;
        vp[1] = vp[3] - delta;
        break;
      case vtkOrientationMarkerWidget::AdjustingP4:
        vp[2] = vp[0] + delta;
        vp[1] = vp[3] - delta;
        break;
      case vtkOrientationMarkerWidget::Translating:
        delta = (dx + dy)*0.5;
        vp[0] = ((vp[0]+vp[2])-delta)*0.5;
        vp[1] = ((vp[1]+vp[3])-delta)*0.5;
        vp[2] = vp[0]+delta;
        vp[3] = vp[1]+delta;
        break;
      }
    this->Renderer->DisplayToNormalizedDisplay( vp[0], vp[1] );
    this->Renderer->DisplayToNormalizedDisplay( vp[2], vp[3] );
    this->Renderer->SetViewport( vp );
    }
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::UpdateOutline()
{
  double vp[4];
  this->Renderer->GetViewport( vp );

  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  vtkPoints *points = this->Outline->GetPoints();

  points->SetPoint( 0, vp[0]+1, vp[1]+1, 0 );
  points->SetPoint( 1, vp[2]-1, vp[1]+1, 0 );
  points->SetPoint( 2, vp[2]-1, vp[3]-1, 0 );
  points->SetPoint( 3, vp[0]+1, vp[3]-1, 0 );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetInteractive(int interact)
{
  if (this->Interactor && this->Enabled)
    {
    if (this->Interactive == interact)
      {
      return;
      }
    if (interact)
      {
      vtkRenderWindowInteractor *i = this->Interactor;
      if ( this->EventCallbackCommand )
        {
        i->AddObserver( vtkCommand::MouseMoveEvent,
          this->EventCallbackCommand, this->Priority );
        i->AddObserver( vtkCommand::LeftButtonPressEvent,
          this->EventCallbackCommand, this->Priority );
        i->AddObserver( vtkCommand::LeftButtonReleaseEvent,
          this->EventCallbackCommand, this->Priority );
        }
      }
    else
      {
      this->Interactor->RemoveObserver( this->EventCallbackCommand );
      }
    this->Interactive = interact;
    this->Interactor->Render();
    }
  else
    {
    vtkGenericWarningMacro("Set interactor and Enabled before changing \
      interaction.");
    }
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  double vp[4];
  this->Renderer->GetViewport( vp );

  // compute display bounds of the widget to see if we are inside or outside
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  int pos1[2] = { static_cast<int>( vp[0] ), static_cast<int>( vp[1] ) };
  int pos2[2] = { static_cast<int>( vp[2] ), static_cast<int>( vp[3] ) };

  int state = this->ComputeStateBasedOnPosition( X, Y, pos1, pos2);
  this->State = this->Moving ? this->State : state;
  this->SetCursor( this->State );
  this->OutlineActor->SetVisibility( this->State );

  if (this->State == vtkOrientationMarkerWidget::Outside || !this->Moving)
    {
    this->Interactor->Render();
    return;
    }

  // based on the state set when the left mouse button is clicked,
  // adjust the renderer's viewport
  switch (this->State)
    {
    case vtkOrientationMarkerWidget::AdjustingP1:
      this->ResizeBottomLeft( X, Y );
      break;
    case vtkOrientationMarkerWidget::AdjustingP2:
      this->ResizeBottomRight( X, Y );
      break;
    case vtkOrientationMarkerWidget::AdjustingP3:
      this->ResizeTopRight( X, Y );
      break;
    case vtkOrientationMarkerWidget::AdjustingP4:
      this->ResizeTopLeft( X, Y );
      break;
    case vtkOrientationMarkerWidget::Translating:
      this->MoveWidget( X, Y );
      break;
    }

  this->UpdateOutline();
  this->EventCallbackCommand->SetAbortFlag( 1 );
  this->InvokeEvent( vtkCommand::InteractionEvent, NULL );
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::MoveWidget(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  int *size = this->CurrentRenderer->GetSize();

  double vp[4];
  this->Renderer->GetViewport( vp );
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2] + dx, vp[3] + dy };

  if (newPos[0] < 0.0)
    {
    newPos[0] = 0.0;
    newPos[2] = vp[2] - vp[0];
    this->StartPosition[0] = static_cast<int>( 0.5*newPos[2] );
    }
  if (newPos[1] < 0.0)
    {
    newPos[1] = 0.0;
    newPos[3] = vp[3] - vp[1];
    this->StartPosition[1] = static_cast<int>( 0.5*newPos[3] );
    }
  if (newPos[2] >= size[0])
    {
    newPos[2] = size[0];
    newPos[0] = newPos[2] - (vp[2] - vp[0]);
    this->StartPosition[0] = static_cast<int>( (newPos[0] + \
                             0.5*(vp[2] - vp[0])) );
    }
  if (newPos[3] >= size[1])
    {
    newPos[3] = size[1];
    newPos[1] = newPos[3] - (vp[3] - vp[1]);
    this->StartPosition[1] = static_cast<int>( (newPos[1] + \
                             0.5*(vp[3] - vp[1])) );
    }

  this->Renderer->DisplayToNormalizedDisplay( newPos[0], newPos[1] );
  this->Renderer->DisplayToNormalizedDisplay( newPos[2], newPos[3] );

  this->Renderer->SetViewport( newPos );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeTopLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy))/2;

  if (dx <= 0 && dy >= 0) // make bigger
    {
    dx = -delta;
    dy = delta;
    }
  else if (dx >= 0 && dy <= 0) // make smaller
    {
    dx = delta;
    dy = -delta;
    }
  else
    {
    return;
    }

  int *size = this->CurrentRenderer->GetSize();

  double vp[4];
  this->Renderer->GetViewport( vp );
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  double newPos[4] = { vp[0] + dx, vp[1], vp[2], vp[3] + dy };

  if (newPos[0] < 0.0)
    {
    newPos[0] = 0.0;
    }
  if (newPos[0] > newPos[2] - this->Tolerance)  // keep from making it too small
    {
    newPos[0] = newPos[2] - this->Tolerance;
    }
  if (newPos[3] > size[1])
    {
    newPos[3] = size[1];
    }
  if (newPos[3] < newPos[1] + this->Tolerance)
    {
    newPos[3] = newPos[1] + this->Tolerance;
    }

  this->StartPosition[0] = static_cast<int>( newPos[0] );
  this->StartPosition[1] = static_cast<int>( newPos[3] );

  this->Renderer->DisplayToNormalizedDisplay( newPos[0], newPos[1] );
  this->Renderer->DisplayToNormalizedDisplay( newPos[2], newPos[3] );

  this->Renderer->SetViewport( newPos );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeTopRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy))/2;

  if (dx >= 0 && dy >= 0) // make bigger
    {
    dx = delta;
    dy = delta;
    }
  else if (dx <= 0 && dy <= 0) // make smaller
    {
    dx = -delta;
    dy = -delta;
    }
  else
    {
    return;
    }

  int *size = this->CurrentRenderer->GetSize();

  double vp[4];
  this->Renderer->GetViewport( vp );
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  double newPos[4] = { vp[0], vp[1], vp[2] + dx, vp[3] + dy };

  if (newPos[2] > size[0])
    {
    newPos[2] = size[0];
    }
  if (newPos[2] < newPos[0] + this->Tolerance)  // keep from making it too small
    {
    newPos[2] = newPos[0] + this->Tolerance;
    }
  if (newPos[3] > size[1])
    {
    newPos[3] = size[1];
    }
  if (newPos[3] < newPos[1] + this->Tolerance)
    {
    newPos[3] = newPos[1] + this->Tolerance;
    }

  this->StartPosition[0] = static_cast<int>( newPos[2] );
  this->StartPosition[1] = static_cast<int>( newPos[3] );

  this->Renderer->DisplayToNormalizedDisplay( newPos[0], newPos[1] );
  this->Renderer->DisplayToNormalizedDisplay( newPos[2], newPos[3] );

  this->Renderer->SetViewport( newPos );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeBottomRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy))/2;

  if (dx >= 0 && dy <= 0) // make bigger
    {
    dx = delta;
    dy = -delta;
    }
  else if (dx <= 0 && dy >= 0) // make smaller
    {
    dx = -delta;
    dy = delta;
    }
  else
    {
    return;
    }

  int *size = this->CurrentRenderer->GetSize();

  double vp[4];
  this->Renderer->GetViewport( vp );
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  double newPos[4] = { vp[0], vp[1] + dy, vp[2] + dx, vp[3] };

  if (newPos[2] > size[0])
    {
    newPos[2] = size[0];
    }
  if (newPos[2] < newPos[0] + this->Tolerance)  // keep from making it too small
    {
    newPos[2] = newPos[0] + this->Tolerance;
    }
  if (newPos[1] < 0.0)
    {
    newPos[1] = 0.0;
    }
  if (newPos[1] > newPos[3] - this->Tolerance)
    {
    newPos[1] = newPos[3] - this->Tolerance;
    }

  this->StartPosition[0] = static_cast<int>( newPos[2] );
  this->StartPosition[1] = static_cast<int>( newPos[1] );

  this->Renderer->DisplayToNormalizedDisplay( newPos[0], newPos[1] );
  this->Renderer->DisplayToNormalizedDisplay( newPos[2], newPos[3] );

  this->Renderer->SetViewport( newPos );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeBottomLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy))/2;

  if (dx <= 0 && dy <= 0) // make bigger
    {
    dx = -delta;
    dy = -delta;
    }
  else if (dx >= 0 && dy >= 0) // make smaller
    {
    dx = delta;
    dy = delta;
    }
  else
    {
    return;
    }

  double vp[4];
  this->Renderer->GetViewport( vp );
  this->Renderer->NormalizedDisplayToDisplay( vp[0], vp[1] );
  this->Renderer->NormalizedDisplayToDisplay( vp[2], vp[3] );

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2], vp[3] };

  if (newPos[0] < 0.0)
    {
    newPos[0] = 0.0;
    }
  if (newPos[0] > newPos[2] - this->Tolerance)  // keep from making it too small
    {
    newPos[0] = newPos[2] - this->Tolerance;
    }
  if (newPos[1] < 0.0)
    {
    newPos[1] = 0.0;
    }
  if (newPos[1] > newPos[3] - this->Tolerance)
    {
    newPos[1] = newPos[3] - this->Tolerance;
    }

  this->StartPosition[0] = static_cast<int>( newPos[0] );
  this->StartPosition[1] = static_cast<int>( newPos[1] );

  this->Renderer->DisplayToNormalizedDisplay( newPos[0], newPos[1] );
  this->Renderer->DisplayToNormalizedDisplay( newPos[2], newPos[3] );

  this->Renderer->SetViewport( newPos );
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetOutlineColor(double r, double g, double b)
{
  this->OutlineActor->GetProperty()->SetColor( r, g, b );
  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

//-------------------------------------------------------------------------
double* vtkOrientationMarkerWidget::GetOutlineColor()
{
  return this->OutlineActor->GetProperty()->GetColor();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetViewport(double minX, double minY,
                                  double maxX, double maxY)
{
  this->Renderer->SetViewport( minX, minY, maxX, maxY );
}

//-------------------------------------------------------------------------
double* vtkOrientationMarkerWidget::GetViewport()
{
  return this->Renderer->GetViewport();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OrientationMarker: " << this->OrientationMarker << endl;
  os << indent << "Interactive: " << this->Interactive << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
}
