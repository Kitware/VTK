// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraOrientationWidget.h"
#include "vtkAbstractWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCameraInterpolator.h"
#include "vtkCameraOrientationRepresentation.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraOrientationWidget);

//----------------------------------------------------------------------------
vtkCameraOrientationWidget::vtkCameraOrientationWidget()
{
  // Define widget events.
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, 0,
    0, nullptr, vtkWidgetEvent::Select, this, vtkCameraOrientationWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::NoModifier,
    0, 0, nullptr, vtkWidgetEvent::EndSelect, this, vtkCameraOrientationWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent, vtkEvent::NoModifier, 0, 0,
    nullptr, vtkWidgetEvent::Rotate, this, vtkCameraOrientationWidget::MoveAction);

  this->CameraInterpolator->SetInterpolationTypeToSpline();

  vtkNew<vtkRenderer> renderer;
  this->SetDefaultRenderer(renderer);
  renderer->SetViewport(0.8, 0.8, 1.0, 1.0);
  renderer->GetActiveCamera()->ParallelProjectionOff();
  renderer->GetActiveCamera()->Dolly(0.25);
  renderer->InteractiveOff();
  renderer->SetLayer(1);
  renderer->AddObserver(
    vtkCommand::StartEvent, this, &vtkCameraOrientationWidget::OrientWidgetRepresentation);
}

//------------------------------------------------------------------------------
vtkRenderer* vtkCameraOrientationWidget::GetParentRenderer()
{
  return this->ParentRenderer;
}

//------------------------------------------------------------------------------
void vtkCameraOrientationWidget::SetParentRenderer(vtkRenderer* parentRen)
{
  if (this->ParentRenderer == parentRen)
  {
    return;
  }

  // detach from previous parent renderer
  if (this->ParentRenderer != nullptr)
  {
    auto renWin = this->ParentRenderer->GetRenderWindow();
    if (renWin != nullptr)
    {
      if (renWin->HasRenderer(this->DefaultRenderer))
      {
        renWin->RemoveRenderer(this->DefaultRenderer);
      }
      const int& numLayers = renWin->GetNumberOfLayers();
      renWin->SetNumberOfLayers(numLayers - 1);
      renWin->RemoveObserver(this->ResizeObserverTag);
    }
  }

  // attach to given parent.
  if (parentRen != nullptr)
  {
    auto renWin = parentRen->GetRenderWindow();
    if (renWin != nullptr)
    {
      if (!renWin->HasRenderer(this->DefaultRenderer))
      {
        renWin->AddRenderer(this->DefaultRenderer);
      }
      this->SetInteractor(renWin->GetInteractor());
      const int& numLayers = renWin->GetNumberOfLayers();
      renWin->SetNumberOfLayers(numLayers + 1);
      this->ResizeObserverTag = renWin->AddObserver(
        vtkCommand::WindowResizeEvent, this, &vtkCameraOrientationWidget::SquareResize);
    }
  }

  // assign
  this->ParentRenderer = parentRen;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCameraOrientationRepresentation::New();
  }
}

void vtkCameraOrientationWidget::ComputeWidgetState(int X, int Y, int modify /* =0*/)
{
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(this->WidgetRep);
  if (rep == nullptr)
  {
    return;
  }

  // Compute and get representation's interaction state.
  this->WidgetRep->ComputeInteractionState(X, Y, modify);
  const auto& interactionState = rep->GetInteractionStateAsEnum();

  // Synchronize widget state with representation.
  if (interactionState == vtkCameraOrientationRepresentation::InteractionStateType::Outside)
  {
    this->WidgetState = WidgetStateType::Inactive;
  }
  else if (interactionState == vtkCameraOrientationRepresentation::InteractionStateType::Hovering)
  {
    this->WidgetState = WidgetStateType::Hot;
  }

  // Refresh representation to match interaction state.
  rep->ApplyInteractionState(interactionState);
  this->Render();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::SelectAction(vtkAbstractWidget* w)
{
  // cast to ourself
  vtkCameraOrientationWidget* const self = vtkCameraOrientationWidget::SafeDownCast(w);
  if (self == nullptr)
  {
    return;
  }

  // can only be selected if already hot.
  if (self->WidgetState != WidgetStateType::Hot)
  {
    return;
  }

  // Get event position.
  const int& X = self->Interactor->GetEventPosition()[0];
  const int& Y = self->Interactor->GetEventPosition()[1];

  // Begin widget interaction.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  // we're now activated
  self->WidgetState = WidgetStateType::Active;

  // this captures the event position.
  self->WidgetRep->StartWidgetInteraction(e);
  self->GrabFocus(self->EventCallbackCommand);

  self->EventCallbackCommand->AbortFlagOn();
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent);
  self->Render();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::EndSelectAction(vtkAbstractWidget* w)
{
  // cast to ourself
  vtkCameraOrientationWidget* const self = vtkCameraOrientationWidget::SafeDownCast(w);
  if (self == nullptr)
  {
    return;
  }
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(self->WidgetRep);
  if (rep == nullptr)
  {
    return;
  }

  // can only be deselected if already selected
  if (self->WidgetState != WidgetStateType::Active)
  {
    return;
  }

  if (self->ParentRenderer == nullptr)
  {
    return;
  }

  // get event position.
  const int& X = self->Interactor->GetEventPosition()[0];
  const int& Y = self->Interactor->GetEventPosition()[1];

  // end widget interaction.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->EndWidgetInteraction(e);

  // deactivate widget.
  self->WidgetState = WidgetStateType::Inactive;

  // synchronize orientations
  if (rep->IsAnyHandleSelected() &&
    (rep->GetInteractionStateAsEnum() ==
      vtkCameraOrientationRepresentation::InteractionStateType::Hovering))
  {
    double back[3], up[3];
    rep->GetBack(back);
    rep->GetUp(up);

    self->OrientParentCamera(back, up);
    // this fires off animation if needed
    if (self->Animate)
    {
      // update gizmo and camera to new orientation step by step.
      for (int i = 0; i < self->AnimatorTotalFrames; ++i)
      {
        self->InterpolateCamera(i);
        self->ParentRenderer->ResetCamera();
        self->Render();
      }
    }
    else
    {
      self->ParentRenderer->ResetCamera();
      self->Render();
    }
  }

  // one might move the mouse out of the widget's interactive area during animation
  // need to compute state.
  self->ComputeWidgetState(X, Y, 1);

  self->ReleaseFocus();
  self->EventCallbackCommand->AbortFlagOn();
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent);
  self->Render();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::MoveAction(vtkAbstractWidget* w)
{
  // cast to ourself
  vtkCameraOrientationWidget* const self = vtkCameraOrientationWidget::SafeDownCast(w);
  if (self == nullptr)
  {
    return;
  }
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(self->WidgetRep);
  if (rep == nullptr)
  {
    return;
  }

  // Get event position.
  const int& X = self->Interactor->GetEventPosition()[0];
  const int& Y = self->Interactor->GetEventPosition()[1];

  // can only rotate if previously selected, else simply compute widget state.
  if (self->WidgetState != WidgetStateType::Active)
  {
    self->ComputeWidgetState(X, Y, 1);
    return;
  }
  else // pick handle.
  {
    rep->ComputeInteractionState(X, Y, 0);
  }

  if (self->ParentRenderer == nullptr)
  {
    return;
  }
  auto cam = self->ParentRenderer->GetActiveCamera();
  if (cam == nullptr)
  {
    return;
  }

  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  // compute representation's azimuth, elevation
  self->WidgetRep->WidgetInteraction(e);

  // copy widget's az, elev to parent cam.
  cam->Azimuth(rep->GetAzimuth());
  cam->Elevation(rep->GetElevation());
  cam->OrthogonalizeViewUp();
  self->ParentRenderer->ResetCameraClippingRange();
  if (self->Interactor->GetLightFollowCamera())
  {
    self->ParentRenderer->UpdateLightsGeometryToFollowCamera();
  }

  self->EventCallbackCommand->AbortFlagOn();
  self->InvokeEvent(vtkCommand::InteractionEvent);
  self->Render();
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationWidget::OrientParentCamera(double back[3], double up[3])
{
  if (this->ParentRenderer == nullptr)
  {
    return;
  }

  vtkCamera* cam = this->ParentRenderer->GetActiveCamera();

  this->CameraInterpolator->Initialize();

  // get old camera vars
  double dstPos[3] = {}, srcPos[3] = {}, srcUp[3] = {}, focalP[3] = {}, distV[3] = {};
  cam->GetFocalPoint(focalP);
  cam->GetPosition(srcPos);
  cam->GetViewUp(srcUp);
  this->CameraInterpolator->AddCamera(0, cam);

  // move camera to look down 'back'
  vtkMath::Subtract(srcPos, focalP, distV);
  double dist = vtkMath::Norm(distV);
  for (int i = 0; i < 3; ++i)
  {
    dstPos[i] = focalP[i] - back[i] * dist;
  }

  // set new camera vars
  cam->SetFocalPoint(focalP);
  cam->SetPosition(dstPos);
  cam->SetViewUp(up);
  cam->ComputeViewPlaneNormal();
  this->CameraInterpolator->AddCamera(this->AnimatorTotalFrames - 1, cam);
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationWidget::OrientWidgetRepresentation()
{
  if (this->ParentRenderer == nullptr)
  {
    return;
  }
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(this->WidgetRep);
  if (rep == nullptr)
  {
    return;
  }
  vtkCamera* cam = this->ParentRenderer->GetActiveCamera();
  if (cam != nullptr)
  {
    const double* orient = cam->GetOrientationWXYZ();
    const double& angle = orient[0];
    const double* axis = orient + 1;

    rep->GetTransform()->Identity();
    rep->GetTransform()->RotateWXYZ(angle, axis);
  }
}

void vtkCameraOrientationWidget::InterpolateCamera(int t)
{
  if (this->ParentRenderer == nullptr)
  {
    return;
  }
  vtkCamera* cam = this->ParentRenderer->GetActiveCamera();
  if (cam == nullptr)
  {
    return;
  }
  this->CameraInterpolator->InterpolateCamera(t, cam);
  cam->OrthogonalizeViewUp(); // the interpolation results in invalid view up, sometimes ..
  cam->ComputeViewPlaneNormal();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::SquareResize()
{
  if (this->DefaultRenderer == nullptr)
  {
    return;
  }

  auto renWin = this->DefaultRenderer->GetRenderWindow();
  if (renWin == nullptr)
  {
    return;
  }
  if (renWin->GetNeverRendered())
  {
    return;
  }

  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(this->WidgetRep);
  if (rep == nullptr)
  {
    return;
  }

  const int* const size = rep->GetSize();
  const int maxSz = (size[0] > size[1]) ? size[0] : size[1];
  const int* const padding = rep->GetPadding();
  const auto& anchoredTo = rep->GetAnchorPosition();
  double xmin = 0., xmax = 0., ymin = 0., ymax = 0.;
  // vp: ViewPort | pad: Padding | w: width | h: height
  const double vpw = static_cast<double>(maxSz) / renWin->GetActualSize()[0];
  const double vph = static_cast<double>(maxSz) / renWin->GetActualSize()[1];
  const double vppadw = static_cast<double>(padding[0]) / renWin->GetActualSize()[0];
  const double vppadh = static_cast<double>(padding[1]) / renWin->GetActualSize()[1];

  switch (anchoredTo)
  {
    case vtkCameraOrientationRepresentation::AnchorType::LowerLeft:
      xmin = 0. + vppadw;
      xmax = vpw + vppadw;
      ymin = 0. + vppadw;
      ymax = vph + vppadw;
      break;
    case vtkCameraOrientationRepresentation::AnchorType::LowerRight:
      xmin = 1. - vpw - vppadw;
      xmax = 1. - vppadw;
      ymin = 0. + vppadw;
      ymax = vph + vppadw;
      break;
    case vtkCameraOrientationRepresentation::AnchorType::UpperLeft:
      xmin = 0.0 + vppadw;
      xmax = vpw + vppadw;
      ymin = 1. - vph - vppadh;
      ymax = 1. - vppadh;
      break;
    case vtkCameraOrientationRepresentation::AnchorType::UpperRight:
      xmin = 1. - vpw - vppadw;
      xmax = 1. - vppadw;
      ymin = 1. - vph - vppadh;
      ymax = 1. - vppadh;
      break;
    default:
      break;
  }
  this->DefaultRenderer->SetViewport(xmin, ymin, xmax, ymax);
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  switch (this->WidgetState)
  {
    case WidgetStateType::Inactive:
      os << indent << "Inactive" << endl;
      break;
    case WidgetStateType::Hot:
      os << indent << "Hot" << endl;
      break;
    case WidgetStateType::Active:
      os << indent << "Active" << endl;
      break;
    default:
      break;
  }
  if (this->ParentRenderer != nullptr)
  {
    os << indent << "ParentRenderer:" << endl;
    this->ParentRenderer->PrintSelf(os, indent);
  }
  os << indent << "CameraInterpolator:" << endl;
  this->CameraInterpolator->PrintSelf(os, indent);
  os << indent << "Animate: " << (this->Animate ? "True" : "False");
  os << indent << "AnimatorTotalFrames: " << this->AnimatorTotalFrames;
}
VTK_ABI_NAMESPACE_END
