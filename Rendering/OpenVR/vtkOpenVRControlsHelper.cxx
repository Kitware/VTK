/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRControlsHelper.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRControlsHelper.h"

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkLineSource.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkOpenVRControlsHelper);

//----------------------------------------------------------------------------
vtkOpenVRControlsHelper::vtkOpenVRControlsHelper()
{
  // The text
  this->Text = vtkStdString("");
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
  this->ComponentName = vtkStdString("trigger");
  this->DrawSide = vtkOpenVRControlsHelper::Right;
  this->ButtonSide = vtkOpenVRControlsHelper::Back;

  this->EnabledOff();
  this->VisibilityOff();

  this->ControlPositionLC[0] = 0.;
  this->ControlPositionLC[1] = 0.;
  this->ControlPositionLC[2] = 0.;

  this->MoveCallbackCommand = vtkCallbackCommand::New();
  this->MoveCallbackCommand->SetClientData(this);
  this->MoveCallbackCommand->SetCallback(vtkOpenVRControlsHelper::MoveEvent);
  this->MoveCallbackCommand->SetPassiveObserver(1);

  this->Device = vtkEventDataDevice::Unknown;
  this->Renderer = nullptr;

  this->NeedUpdate = false;
  this->LabelVisible = false;
}

//----------------------------------------------------------------------------
vtkOpenVRControlsHelper::~vtkOpenVRControlsHelper()
{
  this->SetRenderer(nullptr);

  this->TextActor->Delete();

  this->LineMapper->Delete();
  this->LineActor->Delete();
  this->LineSource->Delete();

  this->MoveCallbackCommand->Delete();
}

void vtkOpenVRControlsHelper::SetDevice(vtkEventDataDevice val)
{
  if (this->Device == val)
  {
    return;
  }

  this->Device = val;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::InitControlPosition()
{
  vtkOpenVRRenderWindowInteractor* iren = static_cast<vtkOpenVRRenderWindowInteractor*>(
    this->Renderer->GetRenderWindow()->GetInteractor());
  if (!iren)
  {
    return;
  }

  vtkOpenVRRenderWindow* renWin = static_cast<vtkOpenVRRenderWindow*>(iren->GetRenderWindow());
  if (!renWin)
  {
    return;
  }

  // Get the active controller device
  vtkEventDataDevice controller = this->Device;

  // Get the active controller model
  vtkOpenVRModel* mod = renWin->GetTrackedDeviceModel(controller);

  // Hide controls tooltips if the controller is off
  if (!mod)
  {
    this->LabelVisible = false;
    return;
  }

  // Compute the component position offset. It corresponds to the vector from the
  // controller origin to the button origin, expressed in local coordinates.
  uint32_t nbOfComponents =
    renWin->GetOpenVRRenderModels()->GetComponentCount(mod->GetName().c_str());
  // for all existing components
  for (uint32_t i = 0; i < nbOfComponents; i++)
  {
    char componentName[100];
    // get the component name
    renWin->GetOpenVRRenderModels()->GetComponentName(
      mod->GetName().c_str(), i, componentName, 100);
    vtkStdString strComponentName = vtkStdString(componentName);

    if (strComponentName == this->ComponentName)
    {
      vr::RenderModel_ControllerMode_State_t pState;
      vr::RenderModel_ComponentState_t pComponentState;
      vr::VRControllerState_t cstate;

      // Get the controller state
      renWin->GetHMD()->GetControllerState(
        renWin->GetTrackedDeviceIndexForDevice(controller), &cstate, sizeof(cstate));

      // Get the component state
      renWin->GetOpenVRRenderModels()->GetComponentState(
        mod->GetName().c_str(), this->ComponentName.c_str(), &cstate, &pState, &pComponentState);

      vr::HmdMatrix34_t mTrackingToLocal = pComponentState.mTrackingToComponentLocal;

      // Save position offset
      this->ControlPositionLC[0] = mTrackingToLocal.m[0][3];
      this->ControlPositionLC[1] = mTrackingToLocal.m[1][3];
      this->ControlPositionLC[2] = mTrackingToLocal.m[2][3];

      break; // Don't need to check other components. break.
    }
  }
}

void vtkOpenVRControlsHelper::MoveEvent(vtkObject*, unsigned long, void* clientdata, void* calldata)
{
  vtkOpenVRControlsHelper* self = static_cast<vtkOpenVRControlsHelper*>(clientdata);

  vtkOpenVRRenderWindow* renWin =
    static_cast<vtkOpenVRRenderWindow*>(self->Renderer->GetRenderWindow());

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

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::UpdateRepresentation()
{
  this->NeedUpdate = false;
  if (!this->Enabled)
  {
    return;
  }

  if (this->Renderer && this->Renderer->GetRenderWindow() &&
    this->Renderer->GetRenderWindow()->GetInteractor())
  {
    vtkOpenVRRenderWindow* renWin =
      static_cast<vtkOpenVRRenderWindow*>(this->Renderer->GetRenderWindow());
    if (!renWin)
    {
      return;
    }

    vtkOpenVRRenderWindowInteractor* iren =
      static_cast<vtkOpenVRRenderWindowInteractor*>(renWin->GetInteractor());
    if (!iren)
    {
      return;
    }
    // Update physical scale
    double physicalScale = iren->GetPhysicalScale();

    // Get the active controller device
    vtkEventDataDevice controller = this->Device;

    // Hide controls tooltips if the controller is off
    vtkOpenVRModel* mod = renWin->GetTrackedDeviceModel(controller);
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

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::ReleaseGraphicsResources(vtkWindow* w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkOpenVRControlsHelper::RenderOpaqueGeometry(vtkViewport* v)
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

//-----------------------------------------------------------------------------
int vtkOpenVRControlsHelper::RenderTranslucentPolygonalGeometry(vtkViewport* v)
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

//-----------------------------------------------------------------------------
vtkTypeBool vtkOpenVRControlsHelper::HasTranslucentPolygonalGeometry()
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int result = 0;

  result |= this->TextActor->HasTranslucentPolygonalGeometry();

  return result;
}

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::BuildRepresentation()
{
  ////Compute text size in world coordinates
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

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenVRControlsHelper::SetText(vtkStdString _text)
{
  if (this->Text == _text)
  {
    return;
  }
  this->Text = _text;

  this->TextActor->SetInput(this->Text);
  this->Modified();
}

void vtkOpenVRControlsHelper::SetEnabled(bool val)
{
  if (val == this->Enabled)
  {
    return;
  }

  this->Enabled = val;
  this->SetVisibility(this->Enabled);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkOpenVRControlsHelper::SetRenderer(vtkRenderer* ren)
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

//----------------------------------------------------------------------------
vtkRenderer* vtkOpenVRControlsHelper::GetRenderer()
{
  return this->Renderer;
}
