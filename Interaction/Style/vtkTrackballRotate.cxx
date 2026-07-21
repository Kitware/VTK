// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTrackballRotate.h"

#include "vtkCamera.h"
#include "vtkInteractorStyleManipulator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <array>
#include <cstdlib>

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkTrackballRotate);

//-------------------------------------------------------------------------
vtkTrackballRotate::vtkTrackballRotate() = default;

//-------------------------------------------------------------------------
vtkTrackballRotate::~vtkTrackballRotate() = default;

//-------------------------------------------------------------------------
void vtkTrackballRotate::OnButtonDown(int, int, vtkRenderer* ren, vtkRenderWindowInteractor*)
{
  this->ComputeCenterOfRotationInDisplayCoordinates(ren);
}

//-------------------------------------------------------------------------
void vtkTrackballRotate::OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor*) {}

//-------------------------------------------------------------------------
void vtkTrackballRotate::OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (ren == nullptr)
  {
    return;
  }

  vtkNew<vtkTransform> transform;
  vtkCamera* camera = ren->GetActiveCamera();
  std::array<double, 3> temp;

  const double mouseMotionFactor = this->GetMouseMotionFactor();
  double scale = vtkMath::Norm(camera->GetPosition());
  if (scale <= 0.0)
  {
    scale = vtkMath::Norm(camera->GetFocalPoint());
    if (scale <= 0.0)
    {
      scale = 1.0;
    }
  }
  camera->GetFocalPoint(temp.data());
  std::transform(temp.begin(), temp.end(), temp.begin(), [scale](double v) { return v / scale; });
  camera->SetFocalPoint(temp.data());

  camera->GetPosition(temp.data());
  std::transform(temp.begin(), temp.end(), temp.begin(), [scale](double v) { return v / scale; });
  camera->SetPosition(temp.data());

  std::array<double, 3> center;
  this->GetCenterOfRotation(center.data());
  transform->Identity();
  std::transform(
    center.begin(), center.end(), center.begin(), [scale](double v) { return v / scale; });
  transform->Translate(center[0], center[1], center[2]);

  const int dx = rwi->GetLastEventPosition()[0] - x;
  const int dy = rwi->GetLastEventPosition()[1] - y;

  camera->OrthogonalizeViewUp();
  const int* size = ren->GetSize();

  if (this->KeyCode == 'x' || this->KeyCode == 'y' || this->KeyCode == 'z' ||
    this->KeyCode == 'X' || this->KeyCode == 'Y' || this->KeyCode == 'Z')
  {
    bool use_dx = std::abs(dx) > std::abs(dy);
    double delta = 360 * mouseMotionFactor * (use_dx ? dx * 1.0 / size[0] : dy * -1.0 / size[1]);
    double axis[3] = { 0, 0, 0 };
    switch (this->KeyCode)
    {
      case 'x':
      case 'X':
        axis[0] = 1.0;
        break;
      case 'y':
      case 'Y':
        axis[1] = 1.0;
        break;
      case 'z':
      case 'Z':
        axis[2] = 1.0;
        break;
      default:
        abort();
    }
    transform->RotateWXYZ(delta, axis[0], axis[1], axis[2]);
  }
  else
  {
    // azimuth
    std::array<double, 3> viewUp;
    camera->GetViewUp(viewUp.data());
    transform->RotateWXYZ(
      360.0 * dx / size[0] * mouseMotionFactor, viewUp[0], viewUp[1], viewUp[2]);

    std::array<double, 3> axis;
    // elevation
    vtkMath::Cross(camera->GetDirectionOfProjection(), viewUp, axis);
    transform->RotateWXYZ(-360.0 * dy / size[1] * mouseMotionFactor, axis.data());
  }

  // translate back
  transform->Translate(-center[0], -center[1], -center[2]);

  camera->ApplyTransform(transform);
  camera->OrthogonalizeViewUp();

  // For rescale back.
  camera->GetFocalPoint(temp.data());
  std::transform(temp.begin(), temp.end(), temp.begin(), [scale](double v) { return v * scale; });
  camera->SetFocalPoint(temp.data());

  camera->GetPosition(temp.data());
  std::transform(temp.begin(), temp.end(), temp.begin(), [scale](double v) { return v * scale; });
  camera->SetPosition(temp.data());

  if (auto* styleManipulator =
        vtkInteractorStyleManipulator::SafeDownCast(rwi->GetInteractorStyle()))
  {
    if (styleManipulator->GetAutoAdjustCameraClippingRange())
    {
      ren->ResetCameraClippingRange();
    }
  }
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkTrackballRotate::OnKeyUp(vtkRenderWindowInteractor* iren)
{
  if (iren->GetKeyCode() == this->KeyCode)
  {
    this->KeyCode = 0;
  }
}

//-------------------------------------------------------------------------
void vtkTrackballRotate::OnKeyDown(vtkRenderWindowInteractor* iren)
{
  if (this->KeyCode == 0)
  {
    this->KeyCode = iren->GetKeyCode();
  }
}

//-------------------------------------------------------------------------
void vtkTrackballRotate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KeyCode: " << this->KeyCode << endl;
}

VTK_ABI_NAMESPACE_END
