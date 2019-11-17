/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelRepresentation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPanelRepresentation.h"

#include "vtkCamera.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkQuaternion.h"
#include "vtkRenderer.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkVectorOperators.h"

#include "vtkOpenVRRenderWindowInteractor.h"

// todo
// - use text bounds for position/scale (done)
// - fix bearing to be correct (done)
// - make sure adjustment of the panel works for controllers (done)
// - add option to remove crop planes

vtkStandardNewMacro(vtkOpenVRPanelRepresentation);

//----------------------------------------------------------------------------
vtkOpenVRPanelRepresentation::vtkOpenVRPanelRepresentation()
{
  this->TextActor = vtkTextActor3D::New();
  this->TextActor->GetTextProperty()->SetFontSize(17);
  this->Text = "This is a Panel Widget";
  this->TextActor->SetInput(this->Text.c_str());

  vtkTextProperty* prop = this->TextActor->GetTextProperty();
  this->TextActor->ForceOpaqueOn();
  this->TextActor->SetUserMatrix(vtkMatrix4x4::New());
  this->TextActor->GetUserMatrix()->Delete();
  prop->SetFontFamilyToTimes();
  prop->SetFrame(1);
  prop->SetFrameWidth(12);
  prop->SetFrameColor(0.0, 0.0, 0.0);
  prop->SetBackgroundOpacity(0.5);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(25);

  this->InteractionState = vtkOpenVRPanelRepresentation::Outside;
  this->CoordinateSystem = World;
  this->AllowAdjustment = true;
}

//----------------------------------------------------------------------------
vtkOpenVRPanelRepresentation::~vtkOpenVRPanelRepresentation()
{
  this->TextActor->Delete();
}

void vtkOpenVRPanelRepresentation::SetCoordinateSystemToWorld()
{
  if (this->CoordinateSystem == World)
  {
    return;
  }
  this->TextActor->GetUserMatrix()->Identity();
  this->CoordinateSystem = World;
  this->Modified();
}

void vtkOpenVRPanelRepresentation::SetCoordinateSystemToHMD()
{
  if (this->CoordinateSystem == HMD)
  {
    return;
  }
  this->CoordinateSystem = HMD;
  this->Modified();
}

void vtkOpenVRPanelRepresentation::SetCoordinateSystemToLeftController()
{
  if (this->CoordinateSystem == LeftController)
  {
    return;
  }
  this->CoordinateSystem = LeftController;
  this->Modified();
}

void vtkOpenVRPanelRepresentation::SetCoordinateSystemToRightController()
{
  if (this->CoordinateSystem == RightController)
  {
    return;
  }
  this->CoordinateSystem = RightController;
  this->Modified();
}

int vtkOpenVRPanelRepresentation::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata, int)
{
  if (!this->AllowAdjustment || this->InteractionState == vtkOpenVRPanelRepresentation::Moving)
  {
    return this->InteractionState;
  }

  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[4];
    edd->GetWorldPosition(pos);
    pos[3] = 1.0;

    const double* bds = this->TextActor->GetBounds();
    double length = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));
    double tolerance = length * 0.05;
    if (pos[0] > bds[0] - tolerance && pos[0] < bds[1] + tolerance && pos[1] > bds[2] - tolerance &&
      pos[1] < bds[3] + tolerance && pos[2] > bds[4] - tolerance && pos[2] < bds[5] + tolerance)
    {
      this->InteractionState = vtkOpenVRPanelRepresentation::Moving;
    }
    else
    {
      this->InteractionState = vtkOpenVRPanelRepresentation::Outside;
    }
  }

  return this->InteractionState;
}

void vtkOpenVRPanelRepresentation::StartComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    edd->GetWorldPosition(this->StartEventPosition);
    this->LastEventPosition[0] = this->StartEventPosition[0];
    this->LastEventPosition[1] = this->StartEventPosition[1];
    this->LastEventPosition[2] = this->StartEventPosition[2];
    edd->GetWorldOrientation(this->StartEventOrientation);
    std::copy(
      this->StartEventOrientation, this->StartEventOrientation + 4, this->LastEventOrientation);
  }
}

void vtkOpenVRPanelRepresentation::ComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double eventPos[3];
    edd->GetWorldPosition(eventPos);
    double eventDir[4];
    edd->GetWorldOrientation(eventDir);

    // Process the motion
    if (this->InteractionState == vtkOpenVRPanelRepresentation::Moving)
    {
      // this->TranslateOutline(this->LastEventPosition, eventPos);
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }

    // Book keeping
    this->LastEventPosition[0] = eventPos[0];
    this->LastEventPosition[1] = eventPos[1];
    this->LastEventPosition[2] = eventPos[2];
    std::copy(eventDir, eventDir + 4, this->LastEventOrientation);
    this->Modified();
  }
}

void vtkOpenVRPanelRepresentation::EndComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*)
{
  this->InteractionState = vtkOpenVRPanelRepresentation::Outside;
}

//----------------------------------------------------------------------------
// Loop through all points and translate and rotate them
void vtkOpenVRPanelRepresentation::UpdatePose(
  double* p1, double* orient1, double* p2, double* orient2)
{
  if (this->CoordinateSystem == World)
  {
    this->UpdatePropPose(this->TextActor, p1, orient1, p2, orient2);
    return;
  }

  if (this->CoordinateSystem == HMD)
  {
    vtkMatrix4x4* mat = this->TextActor->GetUserMatrix();
    mat->Register(this);
    this->TextActor->SetUserMatrix(nullptr);

    this->TempMatrix->DeepCopy(mat);
    this->TempMatrix->Invert();
    double p14[4];
    std::copy(p1, p1 + 3, p14);
    p14[3] = 1.0;
    double p24[4];
    std::copy(p2, p2 + 3, p24);
    p24[3] = 1.0;
    this->TempMatrix->MultiplyPoint(p14, p14);
    this->TempMatrix->MultiplyPoint(p24, p24);

    double trans[3];
    for (int i = 0; i < 3; i++)
    {
      trans[i] = p24[i] - p14[i];
    }

    vtkTransform* newTransform = this->TempTransform;

    // changes in Z adjust the scale
    double ratio = (0.5 + trans[2] / this->LastScale) / 0.5;
    double* scale = this->TextActor->GetScale();
    this->TextActor->SetScale(scale[0] * ratio, scale[1] * ratio, scale[2] * ratio);
    this->TextActor->AddPosition(trans[0], trans[1], 0.0);

    // compute the net rotation
    vtkQuaternion<double> q1;
    q1.SetRotationAngleAndAxis(
      vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
    vtkQuaternion<double> q2;
    q2.SetRotationAngleAndAxis(
      vtkMath::RadiansFromDegrees(orient2[0]), orient2[1], orient2[2], orient2[3]);
    q1.Conjugate();
    q2 = q2 * q1;
    double axis[4];
    axis[0] = vtkMath::DegreesFromRadians(q2.GetRotationAngleAndAxis(axis + 1));

    newTransform->Identity();
    newTransform->PostMultiply();
    newTransform->Concatenate(this->TempMatrix);
    newTransform->TransformNormal(axis + 1, axis + 1);

    vtkMatrix4x4* oldMatrix = this->TempMatrix;
    this->TextActor->GetMatrix(oldMatrix);

    newTransform->Identity();
    newTransform->PostMultiply();
    newTransform->Concatenate(oldMatrix);
    newTransform->Translate(-(p14[0]), -(p14[1]), -(p14[2]));
    newTransform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);
    newTransform->Translate(p14[0], p14[1], p14[2]);

    this->TextActor->SetPosition(newTransform->GetPosition());
    this->TextActor->SetOrientation(newTransform->GetOrientation());
    this->TextActor->SetUserMatrix(mat);
    mat->UnRegister(this);
  }

  if (this->CoordinateSystem == LeftController || this->CoordinateSystem == RightController)
  {
    vtkMatrix4x4* mat = this->TextActor->GetUserMatrix();
    mat->Register(this);
    this->TextActor->SetUserMatrix(nullptr);

    this->TempMatrix->DeepCopy(mat);
    this->TempMatrix->Invert();
    double p14[4];
    std::copy(p1, p1 + 3, p14);
    p14[3] = 1.0;
    double p24[4];
    std::copy(p2, p2 + 3, p24);
    p24[3] = 1.0;
    this->TempMatrix->MultiplyPoint(p14, p14);
    this->TempMatrix->MultiplyPoint(p24, p24);

    double trans[3];
    for (int i = 0; i < 3; i++)
    {
      trans[i] = p24[i] - p14[i];
    }
    this->TextActor->AddPosition(trans[0], trans[1], trans[2]);

    vtkTransform* newTransform = this->TempTransform;

    // compute the net rotation
    vtkQuaternion<double> q1;
    q1.SetRotationAngleAndAxis(
      vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
    vtkQuaternion<double> q2;
    q2.SetRotationAngleAndAxis(
      vtkMath::RadiansFromDegrees(orient2[0]), orient2[1], orient2[2], orient2[3]);
    q1.Conjugate();
    q2 = q2 * q1;
    double axis[4];
    axis[0] = vtkMath::DegreesFromRadians(q2.GetRotationAngleAndAxis(axis + 1));

    newTransform->Identity();
    newTransform->PostMultiply();
    newTransform->Concatenate(this->TempMatrix);
    newTransform->TransformNormal(axis + 1, axis + 1);

    vtkMatrix4x4* oldMatrix = this->TempMatrix;
    this->TextActor->GetMatrix(oldMatrix);

    newTransform->Identity();
    newTransform->PostMultiply();
    newTransform->Concatenate(oldMatrix);
    newTransform->Translate(-(p14[0]), -(p14[1]), -(p14[2]));
    newTransform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);
    newTransform->Translate(p14[0], p14[1], p14[2]);

    this->TextActor->SetPosition(newTransform->GetPosition());
    this->TextActor->SetOrientation(newTransform->GetOrientation());
    this->TextActor->SetUserMatrix(mat);
    mat->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->TextActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::ComputeMatrix(vtkRenderer* ren)
{
  // check whether or not need to rebuild the matrix
  // only rebuild on left eye otherwise we get two different
  // poses for two eyes
  vtkCamera* cam = ren->GetActiveCamera();
  if (this->CoordinateSystem != World && cam->GetLeftEye())
  {
    vtkOpenVRRenderWindow* rw = static_cast<vtkOpenVRRenderWindow*>(ren->GetVTKWindow());

    if (this->CoordinateSystem == HMD)
    {
      vtkTransform* vt = cam->GetViewTransformObject();
      vt->GetInverse(this->TextActor->GetUserMatrix());

      if (rw->GetPhysicalScale() != this->LastScale)
      {
        double ratio = rw->GetPhysicalScale() / this->LastScale;
        double* scale = this->TextActor->GetScale();
        this->TextActor->SetScale(scale[0] * ratio, scale[1] * ratio, scale[2] * ratio);
        double* pos = this->TextActor->GetPosition();
        this->TextActor->SetPosition(pos[0] * ratio, pos[1] * ratio, -0.5 * rw->GetPhysicalScale());
        this->LastScale = rw->GetPhysicalScale();
      }
      else
      {
        double* pos = this->TextActor->GetPosition();
        this->TextActor->SetPosition(pos[0], pos[1], -0.5 * rw->GetPhysicalScale());
      }
    }

    if (this->CoordinateSystem == LeftController)
    {
      vr::TrackedDevicePose_t* tdPose;
      rw->GetTrackedDevicePose(vtkEventDataDevice::LeftController, &tdPose);
      if (tdPose && tdPose->bPoseIsValid)
      {
        vtkNew<vtkMatrix4x4> poseMatrixWorld;
        static_cast<vtkOpenVRRenderWindowInteractor*>(rw->GetInteractor())
          ->ConvertOpenVRPoseToMatrices(*tdPose, poseMatrixWorld);
        this->TextActor->GetUserMatrix()->DeepCopy(poseMatrixWorld);
      }
    }

    if (this->CoordinateSystem == RightController)
    {
      vr::TrackedDevicePose_t* tdPose;
      rw->GetTrackedDevicePose(vtkEventDataDevice::RightController, &tdPose);
      if (tdPose && tdPose->bPoseIsValid)
      {
        vtkNew<vtkMatrix4x4> poseMatrixWorld;
        static_cast<vtkOpenVRRenderWindowInteractor*>(rw->GetInteractor())
          ->ConvertOpenVRPoseToMatrices(*tdPose, poseMatrixWorld);
        this->TextActor->GetUserMatrix()->DeepCopy(poseMatrixWorld);
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkOpenVRPanelRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  // make sure the device has the same matrix
  if (true /* HMD coords */)
  {
    vtkRenderer* ren = static_cast<vtkRenderer*>(v);
    this->ComputeMatrix(ren);
  }

  int count = this->TextActor->RenderOpaqueGeometry(v);
  return count;
}

//-----------------------------------------------------------------------------
int vtkOpenVRPanelRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int count = this->TextActor->RenderTranslucentPolygonalGeometry(v);

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkOpenVRPanelRepresentation::HasTranslucentPolygonalGeometry()
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
void vtkOpenVRPanelRepresentation::PlaceWidget(double bds[6])
{
  this->TextActor->GetUserMatrix()->Identity();
  if (this->CoordinateSystem == World)
  {
    // center the planel
    this->TextActor->SetPosition(
      0.5 * (bds[0] + bds[1]), 0.5 * (bds[2] + bds[3]), 0.5 * (bds[4] + bds[5]));

    double length = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));
    this->TextActor->SetScale(length / 700.0, length / 700.0, length / 700.0);
    this->LastScale = length;
  }

  if (this->CoordinateSystem != World)
  {
    double scale = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));
    this->TextActor->SetScale(scale / 700.0, scale / 700.0, scale / 700.0);
    this->LastScale = scale;
    this->TextActor->SetPosition(0.0, 0.0, -0.5 * scale);
  }
}

void vtkOpenVRPanelRepresentation::PlaceWidgetExtended(
  const double* bds, const double* normal, const double* upvec, double scale)
{
  this->TextActor->GetUserMatrix()->Identity();
  this->TextActor->SetOrientation(0, 0, 0);
  this->LastScale = scale;

  // grab the bounding box of the text
  // so we can position and scale to that
  int tbounds[4];
  this->TextActor->GetBoundingBox(tbounds);
  int maxdim = tbounds[1] - tbounds[0];
  if (tbounds[3] - tbounds[2] > maxdim)
  {
    maxdim = tbounds[3] - tbounds[2];
  }
  // should always be at least 50 pixels
  // for any reasonable string.
  maxdim = maxdim > 50 ? maxdim : 50.0;

  // make the normal ortho to upvec
  vtkVector3d nup(upvec);
  nup.Normalize();
  vtkVector3d nvpn(normal);
  nvpn.Normalize();
  vtkVector3d nvright = nup.Cross(nvpn);
  nvright.Normalize();
  nvpn = nvright.Cross(nup);

  double basis[16];
  basis[0] = nvright[0];
  basis[4] = nvright[1];
  basis[8] = nvright[2];
  basis[12] = 0;
  basis[1] = nup[0];
  basis[5] = nup[1];
  basis[9] = nup[2];
  basis[13] = 0;
  basis[2] = nvpn[0];
  basis[6] = nvpn[1];
  basis[10] = nvpn[2];
  basis[14] = 0;
  basis[3] = 0;
  basis[7] = 0;
  basis[11] = 0;
  basis[15] = 1;

  vtkNew<vtkTransform> basisT;
  basisT->SetMatrix(basis);
  this->TextActor->SetOrientation(basisT->GetOrientation());

  if (this->CoordinateSystem == World)
  {
    // center the planel and default size to 400cm
    this->TextActor->SetScale(0.4 * scale / maxdim, 0.4 * scale / maxdim, 0.4 * scale / maxdim);
    this->TextActor->SetPosition(
      0.5 * (bds[0] + bds[1]), 0.5 * (bds[2] + bds[3]), 0.5 * (bds[4] + bds[5]));
  }

  if (this->CoordinateSystem == LeftController || this->CoordinateSystem == RightController)
  {
    // position the planel and default size to 400cm
    this->TextActor->SetScale(0.4 / maxdim, 0.4 / maxdim, 0.4 / maxdim);
    this->TextActor->SetPosition(0.5 * (bds[0] + bds[1]) - 0.2 * (tbounds[1] - tbounds[0]) / maxdim,
      0.5 * (bds[2] + bds[3]), 0.5 * (bds[4] + bds[5]));
  }

  if (this->CoordinateSystem == HMD)
  {
    // center the planel and default size to 400cm
    this->TextActor->SetScale(0.4 * scale / maxdim, 0.4 * scale / maxdim, 0.4 * scale / maxdim);
    this->TextActor->SetPosition(-0.2 * (tbounds[1] - tbounds[0]) * scale / maxdim,
      -0.2 * (tbounds[3] - tbounds[2]) * scale / maxdim, -0.5 * scale);
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::BuildRepresentation()
{
  // if (this->GetMTime() > this->BuildTime)
  // {
  //   this->TextActor3D->SetPosition(tpos);

  //   // scale should cover 10% of FOV
  //   double fov = ren->GetActiveCamera()->GetViewAngle();
  //   double tsize = 0.1*2.0*atan(fov*0.5); // 10% of fov
  //   tsize /= 200.0;  // about 200 pixel texture map
  //   scale *= tsize;
  //   this->TextActor3D->SetScale(scale, scale, scale);
  //   this->TextActor3D->SetInput(text.c_str());

  //   this->BuildTime.Modified();
  // }
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::SetText(const char* text)
{
  if (this->Text == text)
  {
    return;
  }
  this->Text = text;

  this->TextActor->SetInput(this->Text.c_str());
  this->Modified();
}
