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
#include "vtkInteractorStyle.h"
#include "vtkMath.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <array>

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

namespace
{
double GetCameraScale(vtkCamera* cam)
{
  double scale = vtkMath::Norm(cam->GetPosition());
  if (scale <= 0.0)
  {
    scale = vtkMath::Norm(cam->GetFocalPoint());
    if (scale <= 0.0)
    {
      scale = 1.0;
    }
  }
  return scale;
}
}

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

  // Initialize a default renderer.
  vtkNew<vtkRenderer> renderer;
  renderer->SetViewport(0.8, 0.8, 1.0, 1.0);
  renderer->GetActiveCamera()->ParallelProjectionOff();
  renderer->GetActiveCamera()->Dolly(0.25);
  renderer->InteractiveOff();
  renderer->SetLayer(1);
  this->SetDefaultRenderer(renderer);
}

//----------------------------------------------------------------------------
vtkCameraOrientationWidget::~vtkCameraOrientationWidget() = default;

//------------------------------------------------------------------------------
vtkRenderer* vtkCameraOrientationWidget::GetParentRenderer()
{
  return this->ParentRenderer;
}

//------------------------------------------------------------------------------
void vtkCameraOrientationWidget::SetDefaultRenderer(vtkRenderer* renderer)
{
  if (renderer == this->DefaultRenderer)
  {
    return;
  }
  // remove reorientation observer
  if (this->DefaultRenderer != nullptr)
  {
    this->DefaultRenderer->RemoveObserver(this->ReorientObserverTag);
  }
  const bool wasEnabled = this->Enabled;
  if (this->Enabled)
  {
    // remove previous default renderer from render window.
    if (this->Interactor)
    {
      this->Interactor->GetRenderWindow()->RemoveRenderer(this->DefaultRenderer);
    }
    this->SetEnabled(false);
  }

  // install observer to sync camera widget orientation with that of parent renderer's camera
  this->ReorientObserverTag = renderer->AddObserver(
    vtkCommand::StartEvent, this, &vtkCameraOrientationWidget::OrientWidgetRepresentation);
  this->Superclass::SetDefaultRenderer(renderer);

  if (wasEnabled)
  {
    this->SetEnabled(true);
    if (this->Interactor)
    {
      this->Interactor->GetRenderWindow()->AddRenderer(this->DefaultRenderer);
    }
  }
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
      // In order to occupy sufficient space as per the padding and size of the representation,
      // the widget always invokes the SquareResize callback at the beginning of every frame.
      // We do it like that because the viewport (xmin,xmax, ymin, ymax) of the DefaultRenderer
      // may be different than the previously computed values. Otherwise, in a
      // serialization/deserialization setup, the viewport values could revert back since a resize
      // event is never triggered upon deserialization. This approach is acceptable since the
      // SquareResize method is quite efficient.
      this->ResizeObserverTag = renWin->AddObserver(
        vtkCommand::StartEvent, this, &vtkCameraOrientationWidget::SquareResize);
    }
  }

  // assign
  this->ParentRenderer = parentRen;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::SetRepresentation(vtkCameraOrientationRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
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
    if (self->Animate && self->AnimationTimerObserverTag == -1)
    {
      // update gizmo and camera to new orientation step by step.
      self->StartAnimation();
      return;
    }
    else
    {
      if (auto* style = vtkInteractorStyle::SafeDownCast(self->Interactor->GetInteractorStyle()))
      {
        if (style->GetAutoAdjustCameraClippingRange())
        {
          self->ParentRenderer->ResetCameraClippingRange();
        }
      }
      if (self->ShouldResetCamera)
      {
        self->ParentRenderer->ResetCamera();
      }
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
void vtkCameraOrientationWidget::StartAnimation()
{
  this->AnimatorCurrentFrame = 1;
  this->AnimationTimerId = this->Interactor->CreateRepeatingTimer(1);
  this->AnimationTimerObserverTag = this->Interactor->AddObserver(
    vtkCommand::TimerEvent, this, &vtkCameraOrientationWidget::PlayAnimationSingleFrame);
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::PlayAnimationSingleFrame(
  vtkObject*, unsigned long event, void* callData)
{
  if (event == vtkCommand::TimerEvent &&
    (*reinterpret_cast<int*>(callData)) == this->AnimationTimerId)
  {
    if (this->AnimatorCurrentFrame < this->AnimatorTotalFrames)
    {
      this->InterpolateCamera(this->AnimatorCurrentFrame);
      if (auto* style = vtkInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle()))
      {
        if (style->GetAutoAdjustCameraClippingRange())
        {
          this->ParentRenderer->ResetCameraClippingRange();
        }
      }
      if (this->ShouldResetCamera)
      {
        this->ParentRenderer->ResetCamera();
      }
      this->Render();
      this->AnimatorCurrentFrame++;
    }
    else
    {
      this->StopAnimation();
    }
  }
}

//----------------------------------------------------------------------------
void vtkCameraOrientationWidget::StopAnimation()
{
  if (this->Interactor->DestroyTimer(this->AnimationTimerId))
  {
    this->Interactor->RemoveObserver(this->AnimationTimerObserverTag);
    this->AnimationTimerObserverTag = -1;
    // get event position.
    const int& X = this->Interactor->GetEventPosition()[0];
    const int& Y = this->Interactor->GetEventPosition()[1];
    // one might have moved the mouse out of the widget's interactive area during animation
    // need to compute state.
    this->ComputeWidgetState(X, Y, 1);

    this->ReleaseFocus();
    this->EventCallbackCommand->AbortFlagOn();
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Render();
  }
  else
  {
    vtkErrorMacro(<< "Failed to stop animation timer " << this->AnimationTimerId);
  }
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
  }
  else // pick handle.
  {
    rep->ComputeInteractionState(X, Y, 0);
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

    // Pivot about the interactor style's center of rotation when enabled,
    // otherwise about the focal point (legacy behavior). Captured in world
    // coordinates before the camera is normalized by the scale below.
    std::array<double, 3> center;
    cam->GetFocalPoint(center.data());
    bool autoAdjustClipRange = false;
    if (auto* style = vtkInteractorStyle::SafeDownCast(self->Interactor->GetInteractorStyle()))
    {
      if (self->UseCenterOfRotation)
      {
        style->GetCenterOfRotation(center.data());
      }
      autoAdjustClipRange = style->GetAutoAdjustCameraClippingRange();
    }

    // copy widget's az, elev to parent cam.
    vtkNew<vtkTransform> transform;
    const double scale = ::GetCameraScale(cam);
    std::array<double, 3> temp;
    cam->GetFocalPoint(temp.data());
    vtkMath::MultiplyScalar(temp.data(), 1.0 / scale);
    cam->SetFocalPoint(temp.data());

    cam->GetPosition(temp.data());
    vtkMath::MultiplyScalar(temp.data(), 1.0 / scale);
    cam->SetPosition(temp.data());
    cam->OrthogonalizeViewUp();

    transform->Identity();
    vtkMath::MultiplyScalar(center.data(), 1.0 / scale);
    transform->Translate(center[0], center[1], center[2]);
    transform->RotateWXYZ(rep->GetAzimuth(), cam->GetViewUp());
    std::array<double, 3> elevAxis;
    vtkMath::Cross(cam->GetViewUp(), cam->GetDirectionOfProjection(), elevAxis);
    transform->RotateWXYZ(rep->GetElevation(), elevAxis.data());
    transform->Translate(-center[0], -center[1], -center[2]);
    cam->ApplyTransform(transform);
    cam->OrthogonalizeViewUp();

    // For rescale back.
    cam->GetFocalPoint(temp.data());
    vtkMath::MultiplyScalar(temp.data(), scale);
    cam->SetFocalPoint(temp.data());

    cam->GetPosition(temp.data());
    vtkMath::MultiplyScalar(temp.data(), scale);
    cam->SetPosition(temp.data());

    if (autoAdjustClipRange)
    {
      self->ParentRenderer->ResetCameraClippingRange();
    }
    if (self->Interactor->GetLightFollowCamera())
    {
      self->ParentRenderer->UpdateLightsGeometryToFollowCamera();
    }

    self->EventCallbackCommand->AbortFlagOn();
    self->InvokeEvent(vtkCommand::InteractionEvent);
  }
  if (self->WidgetState != WidgetStateType::Inactive)
  {
    self->Render();
  }
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
  std::array<double, 3> srcPos, srcFp, srcUp;
  cam->GetFocalPoint(srcFp.data());
  cam->GetPosition(srcPos.data());
  cam->GetViewUp(srcUp.data());
  this->CameraInterpolator->AddCamera(0, cam);

  // Pivot about the interactor style's center of rotation when enabled,
  // otherwise about the focal point (legacy behavior).
  std::array<double, 3> center = srcFp;
  if (this->UseCenterOfRotation && this->Interactor != nullptr)
  {
    if (auto* style = vtkInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle()))
    {
      style->GetCenterOfRotation(center.data());
    }
  }

  // source frame: view direction, orthogonalized view up, right
  std::array<double, 3> srcDir, srcOrthoUp, srcRight;
  vtkMath::Subtract(srcFp.data(), srcPos.data(), srcDir.data());
  vtkMath::Normalize(srcDir.data());
  const double srcUpDotDir = vtkMath::Dot(srcUp.data(), srcDir.data());
  // compute a view up vector that is perpendicular to the view direction
  // with Gram-Schmidt orthogonalization.
  for (int i = 0; i < 3; ++i)
  {
    srcOrthoUp[i] = srcUp[i] - srcUpDotDir * srcDir[i];
  }
  vtkMath::Normalize(srcOrthoUp.data());
  vtkMath::Cross(srcDir.data(), srcOrthoUp.data(), srcRight.data());

  // target frame from 'back' and 'up'
  std::array<double, 3> dstDir = { back[0], back[1], back[2] };
  vtkMath::Normalize(dstDir.data());
  std::array<double, 3> dstUp = { up[0], up[1], up[2] };
  // Gram-Schmidt orthogonalization to ensure dstUp is perpendicular to dstDir
  const double dstUpDotDir = vtkMath::Dot(dstUp.data(), dstDir.data());
  for (int i = 0; i < 3; ++i)
  {
    dstUp[i] -= dstUpDotDir * dstDir[i];
  }
  vtkMath::Normalize(dstUp.data());
  std::array<double, 3> dstRight;
  vtkMath::Cross(dstDir.data(), dstUp.data(), dstRight.data());

  // rotation = dstBasis * srcBasis^T maps the source frame onto the target
  // frame.
  double srcBasis[3][3], srcBasisT[3][3], dstBasis[3][3], rotation[3][3];
  for (int i = 0; i < 3; ++i)
  {
    srcBasis[i][0] = srcRight[i];
    srcBasis[i][1] = srcOrthoUp[i];
    srcBasis[i][2] = srcDir[i];
    dstBasis[i][0] = dstRight[i];
    dstBasis[i][1] = dstUp[i];
    dstBasis[i][2] = dstDir[i];
  }
  vtkMath::Transpose3x3(srcBasis, srcBasisT);
  vtkMath::Multiply3x3(dstBasis, srcBasisT, rotation);

  // rotate the camera rig rigidly about the center
  std::array<double, 3> dstPos, dstFp;
  vtkMath::Subtract(srcPos.data(), center.data(), dstPos.data());
  vtkMath::Subtract(srcFp.data(), center.data(), dstFp.data());
  vtkMath::Multiply3x3(rotation, dstPos.data(), dstPos.data());
  vtkMath::Multiply3x3(rotation, dstFp.data(), dstFp.data());
  vtkMath::Add(dstPos.data(), center.data(), dstPos.data());
  vtkMath::Add(dstFp.data(), center.data(), dstFp.data());

  // set new camera vars
  cam->SetFocalPoint(dstFp.data());
  cam->SetPosition(dstPos.data());
  cam->SetViewUp(dstUp.data());
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

//-----------------------------------------------------------------------------
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
      ymin = 0. + vppadh;
      ymax = vph + vppadh;
      break;
    case vtkCameraOrientationRepresentation::AnchorType::LowerRight:
      xmin = 1. - vpw - vppadw;
      xmax = 1. - vppadw;
      ymin = 0. + vppadh;
      ymax = vph + vppadh;
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
      os << indent << "Inactive" << '\n';
      break;
    case WidgetStateType::Hot:
      os << indent << "Hot" << '\n';
      break;
    case WidgetStateType::Active:
      os << indent << "Active" << '\n';
      break;
    default:
      break;
  }
  os << indent << "ParentRenderer: ";
  if (this->ParentRenderer != nullptr)
  {
    os << this->ParentRenderer->GetObjectDescription() << '\n';
    this->ParentRenderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "CameraInterpolator:" << this->CameraInterpolator->GetObjectDescription() << '\n';
  this->CameraInterpolator->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Animate: " << (this->Animate ? "True" : "False") << '\n';
  os << indent << "AnimatorTotalFrames: " << this->AnimatorTotalFrames << '\n';
}
VTK_ABI_NAMESPACE_END
