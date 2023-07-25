// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOrientationMarkerWidget.h"

#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProp.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrientationMarkerWidget);

vtkCxxSetObjectMacro(vtkOrientationMarkerWidget, OrientationMarker, vtkProp);

class vtkOrientationMarkerWidgetObserver : public vtkCommand
{
public:
  static vtkOrientationMarkerWidgetObserver* New()
  {
    return new vtkOrientationMarkerWidgetObserver;
  }

  vtkOrientationMarkerWidgetObserver() { this->OrientationMarkerWidget = nullptr; }

  void Execute(vtkObject* wdg, unsigned long event, void* calldata) override
  {
    if (this->OrientationMarkerWidget)
    {
      this->OrientationMarkerWidget->ExecuteCameraUpdateEvent(wdg, event, calldata);
    }
  }

  vtkOrientationMarkerWidget* OrientationMarkerWidget;
};

//------------------------------------------------------------------------------
vtkOrientationMarkerWidget::vtkOrientationMarkerWidget()
{
  this->StartEventObserverId = 0;
  this->EventCallbackCommand->SetCallback(vtkOrientationMarkerWidget::ProcessEvents);

  this->Observer = vtkOrientationMarkerWidgetObserver::New();
  this->Observer->OrientationMarkerWidget = this;

  this->Tolerance = 7;
  this->Moving = 0;

  this->Viewport[0] = 0.0;
  this->Viewport[1] = 0.0;
  this->Viewport[2] = 0.2;
  this->Viewport[3] = 0.2;

  this->Renderer = vtkRenderer::New();
  this->Renderer->SetLayer(1);
  this->Renderer->InteractiveOff();

  this->Priority = 0.55;
  this->OrientationMarker = nullptr;
  this->State = vtkOrientationMarkerWidget::Outside;
  this->Interactive = 1;

  this->Outline = vtkPolyData::New();
  this->Outline->AllocateExact(128, 128);
  vtkPoints* points = vtkPoints::New();
  vtkIdType ptIds[5];
  ptIds[4] = ptIds[0] = points->InsertNextPoint(1, 1, 0);
  ptIds[1] = points->InsertNextPoint(2, 1, 0);
  ptIds[2] = points->InsertNextPoint(2, 2, 0);
  ptIds[3] = points->InsertNextPoint(1, 2, 0);

  this->Outline->SetPoints(points);
  this->Outline->InsertNextCell(VTK_POLY_LINE, 5, ptIds);

  vtkCoordinate* tcoord = vtkCoordinate::New();
  tcoord->SetCoordinateSystemToDisplay();

  vtkPolyDataMapper2D* mapper = vtkPolyDataMapper2D::New();
  mapper->SetInputData(this->Outline);
  mapper->SetTransformCoordinate(tcoord);

  this->OutlineActor = vtkActor2D::New();
  this->OutlineActor->SetMapper(mapper);
  this->OutlineActor->SetPosition(0, 0);
  this->OutlineActor->SetPosition2(1, 1);
  this->OutlineActor->VisibilityOff();

  points->Delete();
  mapper->Delete();
  tcoord->Delete();
}

//------------------------------------------------------------------------------
vtkOrientationMarkerWidget::~vtkOrientationMarkerWidget()
{
  if (this->Enabled)
  {
    this->TearDownWindowInteraction();
  }

  this->Observer->Delete();
  this->Observer = nullptr;
  this->Renderer->Delete();
  this->Renderer = nullptr;
  this->SetOrientationMarker(nullptr);
  this->OutlineActor->Delete();
  this->Outline->Delete();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetEnabled(int value)
{
  if (!this->Interactor)
  {
    vtkErrorMacro("The interactor must be set prior to enabling/disabling widget");
  }

  if (value != this->Enabled)
  {
    if (value)
    {
      if (!this->OrientationMarker)
      {
        vtkErrorMacro("An orientation marker must be set prior to enabling/disabling widget");
        return;
      }

      if (!this->CurrentRenderer)
      {
        int* pos = this->Interactor->GetLastEventPosition();
        this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(pos[0], pos[1]));

        if (this->CurrentRenderer == nullptr)
        {
          return;
        }
      }

      this->UpdateInternalViewport();

      this->SetupWindowInteraction();
      this->Enabled = 1;
      this->InvokeEvent(vtkCommand::EnableEvent, nullptr);
    }
    else
    {
      this->InvokeEvent(vtkCommand::DisableEvent, nullptr);
      this->Enabled = 0;
      this->TearDownWindowInteraction();
      this->SetCurrentRenderer(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetupWindowInteraction()
{
  vtkRenderWindow* renwin = this->CurrentRenderer->GetRenderWindow();
  renwin->AddRenderer(this->Renderer);
  if (renwin->GetNumberOfLayers() < 2)
  {
    renwin->SetNumberOfLayers(2);
  }

  this->CurrentRenderer->AddViewProp(this->OutlineActor);

  this->Renderer->AddViewProp(this->OrientationMarker);
  this->OrientationMarker->VisibilityOn();

  if (this->Interactive)
  {
    vtkRenderWindowInteractor* interactor = this->Interactor;
    if (this->EventCallbackCommand)
    {
      interactor->AddObserver(
        vtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
      interactor->AddObserver(
        vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
      interactor->AddObserver(
        vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    }
  }

  vtkCamera* pcam = this->CurrentRenderer->GetActiveCamera();
  vtkCamera* cam = this->Renderer->GetActiveCamera();
  if (pcam && cam)
  {
    cam->SetParallelProjection(pcam->GetParallelProjection());
  }

  // We need to copy the camera before the compositing observer is called.
  // Compositing temporarily changes the camera to display an image.
  this->StartEventObserverId =
    this->CurrentRenderer->AddObserver(vtkCommand::StartEvent, this->Observer, 1);
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::TearDownWindowInteraction()
{
  if (this->StartEventObserverId != 0)
  {
    this->CurrentRenderer->RemoveObserver(this->StartEventObserverId);
  }

  this->Interactor->RemoveObserver(this->EventCallbackCommand);

  this->OrientationMarker->VisibilityOff();
  this->Renderer->RemoveViewProp(this->OrientationMarker);

  this->CurrentRenderer->RemoveViewProp(this->OutlineActor);

  // if the render window is still around, remove our renderer from it
  vtkRenderWindow* renwin = this->CurrentRenderer->GetRenderWindow();
  if (renwin)
  {
    renwin->RemoveRenderer(this->Renderer);
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ExecuteCameraUpdateEvent(
  vtkObject* vtkNotUsed(o), unsigned long vtkNotUsed(event), void* vtkNotUsed(calldata))
{
  if (!this->CurrentRenderer)
  {
    return;
  }

  vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  double pos[3], fp[3], viewup[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  cam->GetViewUp(viewup);

  cam = this->Renderer->GetActiveCamera();
  cam->SetPosition(pos);
  cam->SetFocalPoint(fp);
  cam->SetViewUp(viewup);
  this->Renderer->ResetCamera();
  cam->Zoom(this->Zoom);

  this->UpdateOutline();
}

//------------------------------------------------------------------------------
int vtkOrientationMarkerWidget::ComputeStateBasedOnPosition(int X, int Y, int* pos1, int* pos2)
{
  if (X < (pos1[0] - this->Tolerance) || (pos2[0] + this->Tolerance) < X ||
    Y < (pos1[1] - this->Tolerance) || (pos2[1] + this->Tolerance) < Y)
  {
    return vtkOrientationMarkerWidget::Outside;
  }

  // if we are not outside and the left mouse button wasn't clicked,
  // then we are inside, otherwise we are moving

  int result =
    this->Moving ? vtkOrientationMarkerWidget::Translating : vtkOrientationMarkerWidget::Inside;

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
      result = vtkOrientationMarkerWidget::AdjustingP3; // upper right
    }
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetCursor(int state)
{
  switch (state)
  {
    case vtkOrientationMarkerWidget::AdjustingP1:
      this->RequestCursorShape(VTK_CURSOR_SIZESW);
      break;
    case vtkOrientationMarkerWidget::AdjustingP3:
      this->RequestCursorShape(VTK_CURSOR_SIZENE);
      break;
    case vtkOrientationMarkerWidget::AdjustingP2:
      this->RequestCursorShape(VTK_CURSOR_SIZESE);
      break;
    case vtkOrientationMarkerWidget::AdjustingP4:
      this->RequestCursorShape(VTK_CURSOR_SIZENW);
      break;
    case vtkOrientationMarkerWidget::Translating:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    case vtkOrientationMarkerWidget::Inside:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    case vtkOrientationMarkerWidget::Outside:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ProcessEvents(
  vtkObject* vtkNotUsed(object), unsigned long event, void* clientdata, void* vtkNotUsed(calldata))
{
  vtkOrientationMarkerWidget* self = reinterpret_cast<vtkOrientationMarkerWidget*>(clientdata);

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

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::EndInteraction()
{
  this->OnLeftButtonUp();

  // Send a position large enough to always be offscreen to signal an end to the interaction
  this->Interactor->SetEventPosition(VTK_INT_MAX, VTK_INT_MAX);
  this->OnMouseMove();
}

//-------------------------------------------------------------------------
void vtkOrientationMarkerWidget::OnLeftButtonDown()
{
  // We're only here if we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  int pos1[2] = { static_cast<int>(vp[0]), static_cast<int>(vp[1]) };
  int pos2[2] = { static_cast<int>(vp[2]), static_cast<int>(vp[3]) };

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  // flag that we are attempting to adjust or move the outline
  this->Moving = 1;
  this->State = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->SetCursor(this->State);

  if (this->State == vtkOrientationMarkerWidget::Outside)
  {
    this->Moving = 0;
    return;
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
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

  this->RequestCursorShape(VTK_CURSOR_DEFAULT);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SquareRenderer()
{
  int* size = this->Renderer->GetSize();
  if (size[0] == 0 || size[1] == 0)
  {
    return;
  }

  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

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
        delta = (dx + dy) * 0.5;
        vp[0] = ((vp[0] + vp[2]) - delta) * 0.5;
        vp[1] = ((vp[1] + vp[3]) - delta) * 0.5;
        vp[2] = vp[0] + delta;
        vp[3] = vp[1] + delta;
        break;
    }
    this->Renderer->DisplayToNormalizedDisplay(vp[0], vp[1]);
    this->Renderer->DisplayToNormalizedDisplay(vp[2], vp[3]);
    this->Renderer->SetViewport(vp);
    this->UpdateViewport();
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::UpdateOutline()
{
  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  vtkPoints* points = this->Outline->GetPoints();

  points->SetPoint(0, vp[0] + 1, vp[1] + 1, 0);
  points->SetPoint(1, vp[2] - 1, vp[1] + 1, 0);
  points->SetPoint(2, vp[2] - 1, vp[3] - 1, 0);
  points->SetPoint(3, vp[0] + 1, vp[3] - 1, 0);
  this->Outline->Modified();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetInteractive(vtkTypeBool interact)
{
  if (this->Interactor && this->Enabled)
  {
    if (this->Interactive == interact)
    {
      return;
    }
    if (interact)
    {
      vtkRenderWindowInteractor* i = this->Interactor;
      if (this->EventCallbackCommand)
      {
        i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
        i->AddObserver(
          vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
        i->AddObserver(
          vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
      }
    }
    else
    {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
    this->Interactive = interact;
    this->Modified();
  }
  else
  {
    vtkGenericWarningMacro("Set interactor and Enabled before changing \
      interaction.");
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  double vp[4];
  this->Renderer->GetViewport(vp);

  // compute display bounds of the widget to see if we are inside or outside
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  int pos1[2] = { static_cast<int>(vp[0]), static_cast<int>(vp[1]) };
  int pos2[2] = { static_cast<int>(vp[2]), static_cast<int>(vp[3]) };

  int state = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->State = this->Moving ? this->State : state;
  this->SetCursor(this->State);
  this->OutlineActor->SetVisibility(this->State);

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
      this->ResizeBottomLeft(X, Y);
      break;
    case vtkOrientationMarkerWidget::AdjustingP2:
      this->ResizeBottomRight(X, Y);
      break;
    case vtkOrientationMarkerWidget::AdjustingP3:
      this->ResizeTopRight(X, Y);
      break;
    case vtkOrientationMarkerWidget::AdjustingP4:
      this->ResizeTopLeft(X, Y);
      break;
    case vtkOrientationMarkerWidget::Translating:
      this->MoveWidget(X, Y);
      break;
  }

  this->UpdateOutline();
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::MoveWidget(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2] + dx, vp[3] + dy };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
    newPos[2] = currentViewport[0] + (vp[2] - vp[0]);
    this->StartPosition[0] = static_cast<int>((newPos[2] - 0.5 * (vp[2] - vp[0])));
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
    newPos[3] = currentViewport[1] + (vp[3] - vp[1]);
    this->StartPosition[1] = static_cast<int>((newPos[3] - 0.5 * (vp[3] - vp[1])));
  }
  if (newPos[2] >= currentViewport[2])
  {
    newPos[2] = currentViewport[2];
    newPos[0] = currentViewport[2] - (vp[2] - vp[0]);
    this->StartPosition[0] = static_cast<int>((newPos[0] + 0.5 * (vp[2] - vp[0])));
  }
  if (newPos[3] >= currentViewport[3])
  {
    newPos[3] = currentViewport[3];
    newPos[1] = currentViewport[3] - (vp[3] - vp[1]);
    this->StartPosition[1] = static_cast<int>((newPos[1] + 0.5 * (vp[3] - vp[1])));
  }

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeTopLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  // If the size of this widget is constrained, then the minimum dimension size
  // should be used instead of the default Tolerance.
  int actualMinDimensionSize = this->ShouldConstrainSize ? this->MinDimensionSize : this->Tolerance;

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

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1], vp[2], vp[3] + dy };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
  }
  // Constrain the widget width to the minimum size.
  if (newPos[0] > newPos[2] - actualMinDimensionSize)
  {
    newPos[0] = newPos[2] - actualMinDimensionSize;
  }
  // Constrain the widget width to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[0] < newPos[2] - this->MaxDimensionSize)
  {
    newPos[0] = newPos[2] - this->MaxDimensionSize;
  }
  if (newPos[3] > currentViewport[3])
  {
    newPos[3] = currentViewport[3];
  }
  // Constrain the widget height to the minimum size.
  if (newPos[3] < newPos[1] + actualMinDimensionSize)
  {
    newPos[3] = newPos[1] + actualMinDimensionSize;
  }
  // Constrain the widget height to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[3] > newPos[1] + this->MaxDimensionSize)
  {
    newPos[3] = newPos[1] + this->MaxDimensionSize;
  }

  this->StartPosition[0] = static_cast<int>(newPos[0]);
  this->StartPosition[1] = static_cast<int>(newPos[3]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeTopRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  // If the size of this widget is constrained, then the minimum dimension size
  // should be used instead of the default Tolerance.
  int actualMinDimensionSize = this->ShouldConstrainSize ? this->MinDimensionSize : this->Tolerance;

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

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0], vp[1], vp[2] + dx, vp[3] + dy };

  if (newPos[2] > currentViewport[2])
  {
    newPos[2] = currentViewport[2];
  }
  // Constrain the widget width to the minimum size.
  if (newPos[2] < newPos[0] + actualMinDimensionSize)
  {
    newPos[2] = newPos[0] + actualMinDimensionSize;
  }
  // Constrain the widget width to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[2] > newPos[0] + this->MaxDimensionSize)
  {
    newPos[2] = newPos[0] + this->MaxDimensionSize;
  }
  if (newPos[3] > currentViewport[3])
  {
    newPos[3] = currentViewport[3];
  }
  // Constrain the widget height to the minimum size.
  if (newPos[3] < newPos[1] + actualMinDimensionSize)
  {
    newPos[3] = newPos[1] + actualMinDimensionSize;
  }
  // Constrain the widget height to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[3] > newPos[1] + this->MaxDimensionSize)
  {
    newPos[3] = newPos[1] + this->MaxDimensionSize;
  }

  this->StartPosition[0] = static_cast<int>(newPos[2]);
  this->StartPosition[1] = static_cast<int>(newPos[3]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeBottomRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  // If the size of this widget is constrained, then the minimum dimension size
  // should be used instead of the default Tolerance.
  int actualMinDimensionSize = this->ShouldConstrainSize ? this->MinDimensionSize : this->Tolerance;

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

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0], vp[1] + dy, vp[2] + dx, vp[3] };

  if (newPos[2] > currentViewport[2])
  {
    newPos[2] = currentViewport[2];
  }
  // Constrain the widget width to the minimum size.
  if (newPos[2] < newPos[0] + actualMinDimensionSize)
  {
    newPos[2] = newPos[0] + actualMinDimensionSize;
  }
  // Constrain the widget width to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[2] > newPos[0] + this->MaxDimensionSize)
  {
    newPos[2] = newPos[0] + this->MaxDimensionSize;
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
  }
  // Constrain the widget height to the minimum size.
  if (newPos[1] > newPos[3] - actualMinDimensionSize)
  {
    newPos[1] = newPos[3] - actualMinDimensionSize;
  }
  // Constrain the widget height to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[1] < newPos[3] - this->MaxDimensionSize)
  {
    newPos[1] = newPos[3] - this->MaxDimensionSize;
  }

  this->StartPosition[0] = static_cast<int>(newPos[2]);
  this->StartPosition[1] = static_cast<int>(newPos[1]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeBottomLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  // If the size of this widget is constrained, then the minimum dimension size
  // should be used instead of the default Tolerance.
  int actualMinDimensionSize = this->ShouldConstrainSize ? this->MinDimensionSize : this->Tolerance;

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

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2], vp[3] };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
  }
  // Constrain the widget width to the minimum size.
  if (newPos[0] > newPos[2] - actualMinDimensionSize)
  {
    newPos[0] = newPos[2] - actualMinDimensionSize;
  }
  // Constrain the widget width to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[0] < newPos[2] - this->MaxDimensionSize)
  {
    newPos[0] = newPos[2] - this->MaxDimensionSize;
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
  }
  // Constrain the widget height to the minimum size.
  if (newPos[1] > newPos[3] - actualMinDimensionSize)
  {
    newPos[1] = newPos[3] - actualMinDimensionSize;
  }
  // Constrain the widget height to the maximum size if required.
  else if (this->ShouldConstrainSize && newPos[1] < newPos[3] - this->MaxDimensionSize)
  {
    newPos[1] = newPos[3] - this->MaxDimensionSize;
  }

  this->StartPosition[0] = static_cast<int>(newPos[0]);
  this->StartPosition[1] = static_cast<int>(newPos[1]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetOutlineColor(double r, double g, double b)
{
  this->OutlineActor->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkOrientationMarkerWidget::GetOutlineColor()
{
  return this->OutlineActor->GetProperty()->GetColor();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::UpdateViewport()
{
  if (!this->CurrentRenderer)
  {
    return;
  }
  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);

  double vp[4];
  this->Renderer->GetViewport(vp);

  double cvpRange[2];
  for (int i = 0; i < 2; ++i)
  {
    cvpRange[i] = currentViewport[i + 2] - currentViewport[i];
    this->Viewport[i] = (vp[i] - currentViewport[i]) / cvpRange[i];
    this->Viewport[i + 2] = (vp[i + 2] - currentViewport[i]) / cvpRange[i];
  }
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::UpdateInternalViewport()
{
  if (!this->Renderer || !this->GetCurrentRenderer())
  {
    return;
  }

  // Compute the viewport for the widget w.r.t. to the current renderer
  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  double vp[4], currentViewportRange[2];
  for (int i = 0; i < 2; ++i)
  {
    currentViewportRange[i] = currentViewport[i + 2] - currentViewport[i];
    vp[i] = this->Viewport[i] * currentViewportRange[i] + currentViewport[i];
    vp[i + 2] = this->Viewport[i + 2] * currentViewportRange[i] + currentViewport[i];
  }
  this->Renderer->SetViewport(vp);
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::Modified()
{
  this->UpdateInternalViewport();
  this->vtkInteractorObserver::Modified();
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OrientationMarker: " << this->OrientationMarker << endl;
  os << indent << "Interactive: " << this->Interactive << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "Zoom: " << this->Zoom << endl;
  os << indent << "Viewport: (" << this->Viewport[0] << ", " << this->Viewport[1] << ", "
     << this->Viewport[2] << ", " << this->Viewport[3] << ")\n";
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::SetShouldConstrainSize(const vtkTypeBool shouldConstrainSize)
{
  // noop if the value doesn't change
  if (this->ShouldConstrainSize == shouldConstrainSize)
  {
    return;
  }

  // Set value
  this->Modified();
  this->ShouldConstrainSize = shouldConstrainSize;

  // Resize to fit constraints if required
  if (this->ShouldConstrainSize)
  {
    this->ResizeToFitSizeConstraints();
  }
}

//------------------------------------------------------------------------------
bool vtkOrientationMarkerWidget::SetSizeConstraintDimensionSizes(
  int minDimensionSize, int maxDimensionSize)
{
  // noop if the value doesn't change
  if (this->MinDimensionSize == minDimensionSize && this->MaxDimensionSize == maxDimensionSize)
  {
    return true;
  }

  // Enforce valid ranges and tolerances
  if (minDimensionSize < this->Tolerance || maxDimensionSize < this->Tolerance ||
    minDimensionSize > maxDimensionSize)
  {
    return false;
  }

  // Set values
  this->Modified();
  this->MinDimensionSize = minDimensionSize;
  this->MaxDimensionSize = maxDimensionSize;

  // Resize to fit constraints if required
  if (this->ShouldConstrainSize)
  {
    this->ResizeToFitSizeConstraints();
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOrientationMarkerWidget::ResizeToFitSizeConstraints()
{
  if (!this->ShouldConstrainSize)
  {
    return;
  }

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double dx = vp[2] - vp[0];
  double dy = vp[3] - vp[1];
  double delta = 0.0;

  // Check if widget is smaller than min size constraint.
  if (dx < this->MinDimensionSize || dy < this->MinDimensionSize)
  {
    delta = this->MinDimensionSize;
  }
  // Check if widget is larger than max size constraint.
  else if (dx > this->MaxDimensionSize || dy > this->MaxDimensionSize)
  {
    delta = this->MaxDimensionSize;
  }
  // Check if widget is not square.
  else if (dx != dy)
  {
    delta = dx < dy ? dx : dy;
  }

  // If widget size is outside of current size constraints or is not square,
  // then resize the widget.
  if (delta > 0.0)
  {
    // NOTE: As the user is not triggering this resize by dragging a corner
    //       of the widget, there is no information on which corners should
    //       remain unchanged and which should be modified.  Therefore this
    //       resize of the widget is based on the Translating state code in
    //       SquareRenderer, which changes all 4 corner coordinates. Modify
    //       this functionality if we want to add the ability to specify a
    //       corner that should remain unchanged.
    vp[0] = ((vp[0] + vp[2]) - delta) * 0.5;
    vp[1] = ((vp[1] + vp[3]) - delta) * 0.5;
    vp[2] = vp[0] + delta;
    vp[3] = vp[1] + delta;
    this->Renderer->DisplayToNormalizedDisplay(vp[0], vp[1]);
    this->Renderer->DisplayToNormalizedDisplay(vp[2], vp[3]);
    this->Renderer->SetViewport(vp);
    this->UpdateViewport();
    this->UpdateOutline();
  }
}
VTK_ABI_NAMESPACE_END
