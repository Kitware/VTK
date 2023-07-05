// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRControlsHelper.h"

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkVRRenderWindow.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkVRControlsHelper::vtkVRControlsHelper()
{
  // The text
  this->Text = {};
  this->TextActor = vtkTextActor3D::New();
  this->TextActor->GetTextProperty()->SetFontSize(30);
  this->TextActor->SetInput(this->Text.c_str());

  vtkTextProperty* prop = this->TextActor->GetTextProperty();
  this->TextActor->ForceOpaqueOn();

  prop->SetFontFamilyToTimes();
  prop->SetFrame(1);
  prop->SetFrameWidth(12);
  prop->SetFrameColor(0.0, 0.0, 0.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(20);

  // The line
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetPoint1(0, 0, 0);
  this->LineSource->SetPoint2(0, 0, -1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineActor = vtkActor::New();
  this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->LineActor->SetMapper(this->LineMapper);

  // Tooltip default option
  this->ComponentName = "trigger";
  this->DrawSide = vtkVRControlsHelper::Right;
  this->ButtonSide = vtkVRControlsHelper::Back;

  this->EnabledOff();
  this->VisibilityOff();

  this->ControlPositionLC[0] = 0.;
  this->ControlPositionLC[1] = 0.;
  this->ControlPositionLC[2] = 0.;

  this->MoveCallbackCommand = vtkCallbackCommand::New();
  this->MoveCallbackCommand->SetClientData(this);
  this->MoveCallbackCommand->SetCallback(vtkVRControlsHelper::MoveEvent);
  this->MoveCallbackCommand->SetPassiveObserver(1);

  this->Device = vtkEventDataDevice::Unknown;
  this->Renderer = nullptr;

  this->NeedUpdate = false;
  this->LabelVisible = false;
}

//------------------------------------------------------------------------------
vtkVRControlsHelper::~vtkVRControlsHelper()
{
  this->SetRenderer(nullptr);

  this->TextActor->Delete();

  this->LineMapper->Delete();
  this->LineActor->Delete();
  this->LineSource->Delete();

  this->MoveCallbackCommand->Delete();
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::SetDevice(vtkEventDataDevice val)
{
  if (this->Device == val)
  {
    return;
  }

  this->Device = val;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::MoveEvent(vtkObject*, unsigned long, void* clientdata, void* calldata)
{
  vtkVRControlsHelper* self = static_cast<vtkVRControlsHelper*>(clientdata);

  vtkVRRenderWindow* renWin = static_cast<vtkVRRenderWindow*>(self->Renderer->GetRenderWindow());

  vtkEventData* ed = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* ed3 = ed->GetAsEventDataDevice3D();
  if (ed3 && self->Enabled && ed3->GetType() == vtkCommand::Move3DEvent &&
    ed3->GetDevice() == self->Device)
  {
    const double* controllerPositionWC = ed3->GetWorldPosition();
    std::copy(controllerPositionWC, controllerPositionWC + 3, self->LastEventPosition);

    const double* wxyz = ed3->GetWorldOrientation();
    std::copy(wxyz, wxyz + 4, self->LastEventOrientation);

    std::copy(renWin->GetPhysicalTranslation(), renWin->GetPhysicalTranslation() + 3,
      self->LastPhysicalTranslation);

    self->NeedUpdate = true;
  }
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::UpdateRepresentation()
{
  this->NeedUpdate = false;
  if (!this->Enabled)
  {
    return;
  }

  if (this->Renderer && this->Renderer->GetRenderWindow() &&
    this->Renderer->GetRenderWindow()->GetInteractor())
  {
    vtkVRRenderWindow* renWin = static_cast<vtkVRRenderWindow*>(this->Renderer->GetRenderWindow());
    if (!renWin)
    {
      return;
    }

    // Update physical scale
    double physicalScale = renWin->GetPhysicalScale();

    // Get the active controller device
    vtkEventDataDevice controller = this->Device;

    // Hide controls tooltips if the controller is off
    vtkVRModel* mod = renWin->GetModelForDevice(controller);
    if (!mod)
    {
      this->LabelVisible = false;
      return;
    }

    // Get the controls offset position in the controller local coordinate system
    if (this->ControlPositionLC[0] == 0. && this->ControlPositionLC[1] == 0. &&
      this->ControlPositionLC[2] == 0.)
    {
      this->InitControlPosition();
    }

    // Controller position and world orientation
    double* ptrans = renWin->GetPhysicalTranslation();
    this->LastEventPosition[0] += this->LastPhysicalTranslation[0] - ptrans[0];
    this->LastEventPosition[1] += this->LastPhysicalTranslation[1] - ptrans[1];
    this->LastEventPosition[2] += this->LastPhysicalTranslation[2] - ptrans[2];
    const double* controllerPositionWC = this->LastEventPosition;
    const double* wxyz = this->LastEventOrientation;

    // todo use ivar here
    this->TempTransform->Identity();
    this->TempTransform->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);

    double* frameForward = this->Renderer->GetActiveCamera()->GetDirectionOfProjection();
    // Controller up direction in WC
    double* controllerUpWC = this->TempTransform->TransformDoubleVector(0.0, 1.0, 0.0);

    // Compute scale factor. It reaches its max value when the control button
    //  faces the camera. This results in tooltips popping from the controller.
    double dotFactor = -vtkMath::Dot(controllerUpWC, frameForward);

    // minimize scaling of the TextActor. ~MultiSampling
    double reductionFactor = 0.5;

    // Make the dot product always positive on the button side
    dotFactor *= this->ButtonSide * reductionFactor * physicalScale;

    if (dotFactor > 0.0)
    {
      // We are looking on the right side, show tooltip
      this->LabelVisible = true;

      double PPI = 450;                  // Screen resolution in pixels per inch
      double FontSizeFactor = 1.0 / PPI; // Map font size to world coordinates
      this->TextActor->SetScale(FontSizeFactor * dotFactor, FontSizeFactor * dotFactor, 1.0);
    }
    else
    {
      // We are looking on the wrong side, hide tooltip
      this->LabelVisible = false;
      return;
    }

    // Control origin in world coordinates.
    // It corresponds to the vector from the controller position to the
    // position of the button in world coordinates.
    double* controlOriginWC = this->TempTransform->TransformDoublePoint(this->ControlPositionLC);

    // Control position
    double controlPositionWC[3] = { controllerPositionWC[0] + controlOriginWC[0] * physicalScale,
      controllerPositionWC[1] + controlOriginWC[1] * physicalScale,
      controllerPositionWC[2] + controlOriginWC[2] * physicalScale };

    // Frame main directions in WC
    double* frameUp = this->Renderer->GetActiveCamera()->GetViewUp();
    double frameRight[3];
    vtkMath::Cross(frameForward, frameUp, frameRight);
    vtkMath::Normalize(frameRight);

    // Define an offset along the frame right direction and the controller
    // up direction.
    double tooltipOffset = 0.15;

    // Set the origin to the bottom-left or bottom-right corner depending on
    // the button draw side.
    double frameOrigin[3] = { (1 - this->DrawSide) / 2. * this->FrameSize[0] * frameRight[0] *
        dotFactor * this->DrawSide,
      (1 - this->DrawSide) / 2. * this->FrameSize[0] * frameRight[1] * dotFactor * this->DrawSide,
      (1 - this->DrawSide) / 2. * this->FrameSize[0] * frameRight[2] * dotFactor * this->DrawSide };
    // Position of the frame
    double framePosition[3] = { controlPositionWC[0] + frameOrigin[0],
      controlPositionWC[1] + frameOrigin[1], controlPositionWC[2] + frameOrigin[2] };

    controllerUpWC = this->TempTransform->TransformDoubleVector(0.0, 1.0, 0.0);

    // Apply offset along the frame right axis
    framePosition[0] += tooltipOffset * frameRight[0] * dotFactor * this->DrawSide;
    framePosition[1] += tooltipOffset * frameRight[1] * dotFactor * this->DrawSide;
    framePosition[2] += tooltipOffset * frameRight[2] * dotFactor * this->DrawSide;

    // Apply offset along the controller up axis
    framePosition[0] += tooltipOffset * controllerUpWC[0] * dotFactor * this->ButtonSide;
    framePosition[1] += tooltipOffset * controllerUpWC[1] * dotFactor * this->ButtonSide;
    framePosition[2] += tooltipOffset * controllerUpWC[2] * dotFactor * this->ButtonSide;

    double* ori = this->Renderer->GetActiveCamera()->GetOrientationWXYZ();
    this->TempTransform->Identity();
    this->TempTransform->RotateWXYZ(-ori[0], ori[1], ori[2], ori[3]);

    // Update Text Actor
    this->TextActor->SetPosition(framePosition);
    this->TextActor->SetOrientation(this->TempTransform->GetOrientation());

    // Update Line Actor
    // WARNING: Transforming the Actor is cheaper than setting the geometry
    double lineAnchor[3] = { framePosition[0] - frameOrigin[0], framePosition[1] - frameOrigin[1],
      framePosition[2] - frameOrigin[2] };

    double lineDirection[3] = { controlPositionWC[0] - lineAnchor[0],
      controlPositionWC[1] - lineAnchor[1], controlPositionWC[2] - lineAnchor[2] };

    this->LineActor->SetPosition(controlPositionWC);
    this->LineActor->SetScale(vtkMath::Norm(lineDirection));

    double z[3] = { 0, 0, 1 };
    double w = vtkMath::AngleBetweenVectors(lineDirection, z);
    double xyz[3];
    vtkMath::Cross(lineDirection, z, xyz);
    this->TempTransform->Identity();
    this->TempTransform->RotateWXYZ(vtkMath::DegreesFromRadians(-w), xyz[0], xyz[1], xyz[2]);
    this->LineActor->SetOrientation(this->TempTransform->GetOrientation());
  }
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::ReleaseGraphicsResources(vtkWindow* w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkVRControlsHelper::RenderOpaqueGeometry(vtkViewport* v)
{
  if (this->NeedUpdate)
  {
    this->UpdateRepresentation();
  }

  if (!this->LabelVisible)
  {
    return 0;
  }

  int count = 0;

  count += this->TextActor->RenderOpaqueGeometry(v);
  count += this->LineActor->RenderOpaqueGeometry(v);

  return count;
}

//------------------------------------------------------------------------------
int vtkVRControlsHelper::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  if (this->NeedUpdate)
  {
    this->UpdateRepresentation();
  }

  if (!this->LabelVisible)
  {
    return 0;
  }

  int count = 0;

  count += this->TextActor->RenderTranslucentPolygonalGeometry(v);

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkVRControlsHelper::HasTranslucentPolygonalGeometry()
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int result = 0;

  result |= this->TextActor->HasTranslucentPolygonalGeometry();

  return result;
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::BuildRepresentation()
{
  // Compute text size in world coordinates
  int bbox[4] = { 0, 0, 0, 0 };
  this->TextActor->GetBoundingBox(bbox);

  double PPI = 450;                  // Screen resolution in pixels per inch
  double FontSizeFactor = 1.0 / PPI; // Map font size to world coordinates

  double textSize[2] = { static_cast<double>(bbox[1] - bbox[0]),
    static_cast<double>(bbox[3] - bbox[2]) };

  double textFrameWidth = this->TextActor->GetTextProperty()->GetFrameWidth();

  this->FrameSize[0] = (textSize[0] - 2.0 * textFrameWidth) * FontSizeFactor;
  this->FrameSize[1] = (textSize[1] - 2.0 * textFrameWidth) * FontSizeFactor;
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FrameSize: (" << this->FrameSize[0] << ", " << this->FrameSize[1] << ")\n";
  this->TextActor->PrintSelf(os, indent);
  os << indent << "Text: " << this->Text << "\n";
  this->LineSource->PrintSelf(os, indent);
  this->LineMapper->PrintSelf(os, indent);
  this->LineActor->PrintSelf(os, indent);
  os << indent << "ComponentName: " << this->ComponentName << "\n";
  os << indent << "DrawSide: " << this->DrawSide << "\n";
  os << indent << "ButtonSide: " << this->ButtonSide << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "ControlPositionLC: (" << this->ControlPositionLC[0] << ", "
     << this->ControlPositionLC[1] << "," << this->ControlPositionLC[2] << ")\n";
  os << indent << "LastPhysicalTranslation: (" << this->LastPhysicalTranslation[0] << ", "
     << this->LastPhysicalTranslation[1] << ")\n";
  os << indent << "LastEventPosition: (" << this->LastEventPosition[0] << ", "
     << this->LastEventPosition[1] << "," << this->LastEventPosition[2] << ")\n";
  os << indent << "LastEventOrientation: (" << this->LastEventOrientation[0] << ", "
     << this->LastEventOrientation[1] << ", " << this->LastEventOrientation[2] << ","
     << this->LastEventOrientation[3] << ")\n";
  os << indent << "NeedUpdate: " << this->NeedUpdate << "\n";
  os << indent << "LabelVisible: " << this->LabelVisible << "\n";
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::SetText(const std::string& _text)
{
  if (this->Text == _text)
  {
    return;
  }
  this->Text = _text;

  this->TextActor->SetInput(this->Text.c_str());
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::SetEnabled(bool val)
{
  if (val == this->Enabled)
  {
    return;
  }

  this->Enabled = val;
  this->SetVisibility(this->Enabled);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRControlsHelper::SetRenderer(vtkRenderer* ren)
{
  if (ren == this->Renderer)
  {
    return;
  }

  if (this->Renderer)
  {
    vtkRenderWindowInteractor* i =
      static_cast<vtkRenderWindow*>(this->Renderer->GetVTKWindow())->GetInteractor();
    i->RemoveObserver(this->ObserverTag);
  }

  this->Renderer = ren;
  if (this->Renderer)
  {
    vtkRenderWindowInteractor* i =
      static_cast<vtkRenderWindow*>(this->Renderer->GetVTKWindow())->GetInteractor();
    this->ObserverTag = i->AddObserver(vtkCommand::Move3DEvent, this->MoveCallbackCommand, 10.0);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
vtkRenderer* vtkVRControlsHelper::GetRenderer()
{
  return this->Renderer;
}
VTK_ABI_NAMESPACE_END
