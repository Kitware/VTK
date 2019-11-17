/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLAvatar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLAvatar.h"

#include "vtkAvatarHead.h"          // geometry for head
#include "vtkAvatarLeftForeArm.h"   // geometry for arm
#include "vtkAvatarLeftHand.h"      // geometry for hand
#include "vtkAvatarLeftUpperArm.h"  // geometry for arm
#include "vtkAvatarRightForeArm.h"  // geometry for arm
#include "vtkAvatarRightHand.h"     // geometry for hand
#include "vtkAvatarRightUpperArm.h" // geometry for arm
#include "vtkAvatarTorso.h"         // geometry for torso
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkFlagpoleLabel.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRay.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkVectorOperators.h"
#include "vtkXMLPolyDataReader.h"

#include <cmath>

namespace
{
void setOrientation(vtkTransform* trans, const double* orientation)
{
  trans->Identity();
  trans->RotateZ(orientation[2]);
  trans->RotateX(orientation[0]);
  trans->RotateY(orientation[1]);
}

void MultiplyComponents(double a[3], double scale[3])
{
  a[0] *= scale[0];
  a[1] *= scale[1];
  a[2] *= scale[2];
}

// calculate a rotation purely around Vup, using an approximate Vr (right)
// that isn't orthogonal.
// Reverse Vr, front if torso isn't facing the same way as the head.
void getTorsoTransform(
  vtkTransform* trans, double Vup[3], double inVr[3], double HeadOrientation[3])
{
  double Vr[3] = { inVr[0], inVr[1], inVr[2] };
  // temporarily use trans for head orientation
  setOrientation(trans, HeadOrientation);
  if (Vr[0] == 0 && Vr[1] == 0 && Vr[2] == 0)
  {
    // no information from hands, use head orientation and Vup.
    Vr[2] = 1;
    trans->TransformPoint(Vr, Vr);
  }

  // make Vr orthogonal to Vup
  double Vtemp[3] = { Vup[0], Vup[1], Vup[2] };
  vtkMath::MultiplyScalar(Vtemp, vtkMath::Dot(Vup, Vr));
  vtkMath::Subtract(Vr, Vtemp, Vr);
  vtkMath::Normalize(Vr);
  // get third basis vector
  double Vfr[3];
  vtkMath::Cross(Vup, Vr, Vfr);
  // temporarily use trans to test Vfr versus head orientation
  double Vhead[3] = { 1, 0, 0 };
  trans->TransformPoint(Vhead, Vhead);
  if (vtkMath::Dot(Vfr, Vhead) < 0)
  {
    // torso is facing behind the head. Swap.
    Vr[0] = -Vr[0];
    Vr[1] = -Vr[1];
    Vr[2] = -Vr[2];
    Vfr[0] = -Vfr[0];
    Vfr[1] = -Vfr[1];
    Vfr[2] = -Vfr[2];
  }
  // make new rotation matrix. Basis vectors form the rotation piece.
  trans->Identity();
  vtkNew<vtkMatrix4x4> mat;
  trans->GetMatrix(mat);
  for (int i = 0; i < 3; ++i)
  {
    mat->SetElement(i, 0, Vfr[i]);
    mat->SetElement(i, 1, Vup[i]);
    mat->SetElement(i, 2, Vr[i]);
  }
  trans->SetMatrix(mat);
}

// create a triangle between the shoulder and hand, in the plane
// parallel to the up vector, so elbow is always "down".
void getElbowPosition(
  double* outElbow, double Vup[3], double inShoulder[3], double inHand[3], double scale)
{
  const double forearmLength = 0.87 * scale;
  const double upperLength = 0.97 * scale;
  vtkVector3d shVec(inShoulder);
  vtkVector3d shoulderHand = vtkVector3d(inHand) - shVec;
  double shLength = shoulderHand.Normalize();
  vtkVector3d out;
  if (shLength >= forearmLength + upperLength)
  {
    out = shVec + shoulderHand * (shLength - forearmLength);
  }
  else
  {
    vtkVector3d planeNorm = shoulderHand.Cross(vtkVector3d(Vup));
    vtkVector3d toElbow = shoulderHand.Cross(planeNorm).Normalized();
    // heron's formula to find area, using 1/2 perimeter
    double p = 0.5 * (forearmLength + upperLength + shLength);
    double area = sqrt(p * (p - forearmLength) * (p - upperLength) * (p - shLength));
    double height = 2 * area / shLength;
    // distance along base to the right triangle
    double upperBase = sqrt(upperLength * upperLength - height * height);
    out = shVec + (upperBase * shoulderHand) + (height * toElbow);
  }
  outElbow[0] = out[0];
  outElbow[1] = out[1];
  outElbow[2] = out[2];
  return;
}

void rotateToPoint(double* outOrient, vtkTransform* trans, double target[3], double start[3])
{
  double leftUpperDir[3], cross[3], startDir[3] = { 1, 0, 0 };
  vtkMath::Subtract(target, start, leftUpperDir);
  vtkMath::Cross(startDir, leftUpperDir, cross);
  vtkMath::Normalize(cross);
  double angle = vtkMath::AngleBetweenVectors(startDir, leftUpperDir) * 180 / vtkMath::Pi();
  trans->Identity();
  trans->RotateWXYZ(angle, cross);
  trans->GetOrientation(outOrient);
}
} // namespace

vtkStandardNewMacro(vtkOpenGLAvatar);

vtkOpenGLAvatar::vtkOpenGLAvatar()
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetReadFromInputString(true);
  reader->SetInputString(std::string(vtkAvatarHead, vtkAvatarHead + sizeof vtkAvatarHead));
  reader->Update();

  this->HeadMapper->SetInputData(reader->GetOutput());
  this->HeadActor->SetMapper(this->HeadMapper);

  vtkNew<vtkXMLPolyDataReader> reader2;
  reader2->SetReadFromInputString(true);
  reader2->SetInputString(
    std::string(vtkAvatarLeftHand, vtkAvatarLeftHand + sizeof vtkAvatarLeftHand));
  reader2->Update();
  this->LeftHandMapper->SetInputData(reader2->GetOutput());
  this->LeftHandActor->SetMapper(this->LeftHandMapper);

  vtkNew<vtkXMLPolyDataReader> reader3;
  reader3->SetReadFromInputString(true);
  reader3->SetInputString(
    std::string(vtkAvatarRightHand, vtkAvatarRightHand + sizeof vtkAvatarRightHand));
  reader3->Update();
  this->RightHandMapper->SetInputData(reader3->GetOutput());
  this->RightHandActor->SetMapper(this->RightHandMapper);

  const unsigned char* models[NUM_BODY] = {
    vtkAvatarTorso,
    vtkAvatarLeftForeArm,
    vtkAvatarRightForeArm,
    vtkAvatarLeftUpperArm,
    vtkAvatarRightUpperArm,
  };
  size_t modelSize[NUM_BODY] = {
    sizeof vtkAvatarTorso,
    sizeof vtkAvatarLeftForeArm,
    sizeof vtkAvatarRightForeArm,
    sizeof vtkAvatarLeftUpperArm,
    sizeof vtkAvatarRightUpperArm,
  };

  this->GetProperty()->SetDiffuse(0.7);
  this->GetProperty()->SetAmbient(0.3);
  this->GetProperty()->SetSpecular(0.0);
  // link properties, share color.
  this->HeadActor->SetProperty(this->GetProperty());
  this->LeftHandActor->SetProperty(this->GetProperty());
  this->RightHandActor->SetProperty(this->GetProperty());

  for (int i = 0; i < NUM_BODY; ++i)
  {
    if (!models[i])
      continue;
    vtkNew<vtkXMLPolyDataReader> reader4;
    reader4->SetReadFromInputString(true);
    reader4->SetInputString(std::string(models[i], models[i] + modelSize[i]));
    reader4->Update();
    this->BodyMapper[i]->SetInputData(reader4->GetOutput());
    this->BodyActor[i]->SetMapper(this->BodyMapper[i]);

    this->BodyActor[i]->SetProperty(this->GetProperty());
  }

  // text box doesn't render unless set:
  this->LabelActor->SetForceOpaque(true);
  this->LabelActor->GetTextProperty()->SetFontSize(12);
  this->LabelActor->GetTextProperty()->SetColor(1.0, 1.0, .4);
  this->LabelActor->GetTextProperty()->SetJustificationToCentered();
  this->LabelActor->GetTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
  this->LabelActor->GetTextProperty()->SetBackgroundOpacity(1.0);
}

vtkOpenGLAvatar::~vtkOpenGLAvatar() = default;

// Actual Avatar render method.
int vtkOpenGLAvatar::RenderOpaqueGeometry(vtkViewport* vp)
{
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);
  if (this->GetIsOpaque())
  {
    vtkOpenGLClearErrorMacro();

    this->CalcBody();

    this->HeadActor->SetScale(this->GetScale());
    this->HeadActor->SetPosition(this->HeadPosition);
    this->HeadActor->SetOrientation(this->HeadOrientation);
    this->LeftHandActor->SetScale(this->GetScale());
    this->LeftHandActor->SetPosition(this->LeftHandPosition);
    this->LeftHandActor->SetOrientation(this->LeftHandOrientation);
    this->RightHandActor->SetScale(this->GetScale());
    this->RightHandActor->SetPosition(this->RightHandPosition);
    this->RightHandActor->SetOrientation(this->RightHandOrientation);

    // send a render; update pipeline
    if (this->HeadActor->GetVisibility())
    {
      this->HeadActor->RenderOpaqueGeometry(ren);
    }
    if (this->LeftHandActor->GetVisibility())
    {
      this->LeftHandActor->RenderOpaqueGeometry(ren);
    }
    if (this->RightHandActor->GetVisibility())
    {
      this->RightHandActor->RenderOpaqueGeometry(ren);
    }
    for (int i = 0; i < NUM_BODY; ++i)
    {
      this->BodyActor[i]->SetScale(this->GetScale());
      this->BodyActor[i]->SetPosition(this->BodyPosition[i]);
      this->BodyActor[i]->SetOrientation(this->BodyOrientation[i]);
      if (this->BodyActor[i]->GetVisibility())
      {
        this->BodyActor[i]->RenderOpaqueGeometry(ren);
      }
    }
    if (this->LeftRay->GetShow() || this->RightRay->GetShow())
    {
      auto renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
      vtkOpenVRCamera* cam = static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera());
      if (renWin && cam)
      {

        vtkNew<vtkTransform> trans;
        vtkNew<vtkMatrix4x4> mat;
        vtkMatrix4x4* wcdc; // we only need this one.
        vtkMatrix4x4* wcvc;
        vtkMatrix3x3* norms;
        vtkMatrix4x4* vcdc;
        // VRRay needs the complete model -> device (screen) transform.
        // Get world -> device from the camera.
        cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
        vtkNew<vtkMatrix4x4> controller2device;

        if (this->LeftRay->GetShow())
        {
          trans->Translate(this->LeftHandPosition);
          // RotateZ, RotateX, and finally RotateY
          trans->RotateZ(this->LeftHandOrientation[2]);
          trans->RotateX(this->LeftHandOrientation[0]);
          trans->RotateY(this->LeftHandOrientation[1]);
          // VRRay and avatar are off by 90.
          trans->RotateY(-90);
          trans->GetMatrix(mat);
          // OpenGL expects transpose of VTK transforms.
          mat->Transpose();
          trans->SetMatrix(mat);
          vtkMatrix4x4::Multiply4x4(trans->GetMatrix(), wcdc, controller2device);
          this->LeftRay->Render(renWin, controller2device);
        }
        if (this->RightRay->GetShow())
        {
          trans->Identity();
          trans->Translate(this->RightHandPosition);
          trans->RotateZ(this->RightHandOrientation[2]);
          trans->RotateX(this->RightHandOrientation[0]);
          trans->RotateY(this->RightHandOrientation[1]);
          trans->RotateY(-90);
          trans->GetMatrix(mat);
          mat->Transpose();
          trans->SetMatrix(mat);
          vtkMatrix4x4::Multiply4x4(trans->GetMatrix(), wcdc, controller2device);
          this->RightRay->Render(renWin, controller2device);
        }
      }
    }
    if (this->LabelActor->GetInput())
    {
      vtkVector3d basePos =
        vtkVector3d(this->HeadPosition) + 0.5 * this->GetScale()[0] * vtkVector3d(this->UpVector);
      vtkVector3d labelPos =
        vtkVector3d(this->HeadPosition) + 0.7 * this->GetScale()[0] * vtkVector3d(this->UpVector);
      this->LabelActor->SetBasePosition(basePos[0], basePos[1], basePos[2]);
      this->LabelActor->SetTopPosition(labelPos[0], labelPos[1], labelPos[2]);
      this->LabelActor->RenderOpaqueGeometry(ren);
    }

    vtkOpenGLCheckErrorMacro("failed after Render");
    return 1;
  }
  return 0;
}

int vtkOpenGLAvatar::RenderTranslucentPolygonalGeometry(vtkViewport* vp)
{
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);
  if (this->LabelActor->GetInput())
  {
    this->LabelActor->RenderTranslucentPolygonalGeometry(ren);
    return 1;
  }
  return 0;
}

void vtkOpenGLAvatar::CalcBody()
{
  this->BodyPosition[TORSO][0] = this->HeadPosition[0];
  this->BodyPosition[TORSO][1] = this->HeadPosition[1];
  this->BodyPosition[TORSO][2] = this->HeadPosition[2];

  vtkNew<vtkTransform> trans;
  double scale[3];
  this->GetScale(scale);
  // get an approximate elbow position, rigidly attaching forearm to hand,
  // to use for torso orientation.
  double leftElbowPos[3] = { -0.85, 0.02, 0 };
  setOrientation(trans, this->LeftHandOrientation);
  MultiplyComponents(leftElbowPos, scale);
  trans->TransformPoint(leftElbowPos, leftElbowPos);
  leftElbowPos[0] += this->LeftHandPosition[0];
  leftElbowPos[1] += this->LeftHandPosition[1];
  leftElbowPos[2] += this->LeftHandPosition[2];

  double rightElbowPos[3] = { -0.85, 0.02, 0 };
  setOrientation(trans, this->RightHandOrientation);
  MultiplyComponents(rightElbowPos, scale);
  trans->TransformPoint(rightElbowPos, rightElbowPos);
  rightElbowPos[0] += this->RightHandPosition[0];
  rightElbowPos[1] += this->RightHandPosition[1];
  rightElbowPos[2] += this->RightHandPosition[2];

  // keep the head orientation in the direction of the up vector.
  // use the vector between the hands as a guide for torso's rotation (Vright).
  double torsoRight[3] = { 0, 0, 0 };
  if (this->UseLeftHand && this->UseRightHand)
  {
    vtkMath::Subtract(rightElbowPos, leftElbowPos, torsoRight);
  }
  else if (this->UseLeftHand)
  {
    vtkMath::Subtract(this->HeadPosition, leftElbowPos, torsoRight);
  }
  else if (this->UseRightHand)
  {
    vtkMath::Subtract(rightElbowPos, this->HeadPosition, torsoRight);
  }
  // else no hands, and torsoRight remains zero.

  getTorsoTransform(trans, this->UpVector, torsoRight, this->HeadOrientation);

  trans->GetOrientation(this->BodyOrientation[TORSO]);

  this->BodyPosition[LEFT_FORE][0] = this->LeftHandPosition[0];
  this->BodyPosition[LEFT_FORE][1] = this->LeftHandPosition[1];
  this->BodyPosition[LEFT_FORE][2] = this->LeftHandPosition[2];

  // forearm extends along -x, so use a reversed target to get the correct
  // rotation to the elbow.
  getElbowPosition(
    leftElbowPos, this->UpVector, this->BodyPosition[LEFT_UPPER], this->LeftHandPosition, scale[0]);
  rotateToPoint(
    this->BodyOrientation[LEFT_FORE], trans, this->BodyPosition[LEFT_FORE], leftElbowPos);

  this->BodyPosition[RIGHT_FORE][0] = this->RightHandPosition[0];
  this->BodyPosition[RIGHT_FORE][1] = this->RightHandPosition[1];
  this->BodyPosition[RIGHT_FORE][2] = this->RightHandPosition[2];

  getElbowPosition(rightElbowPos, this->UpVector, this->BodyPosition[RIGHT_UPPER],
    this->RightHandPosition, scale[0]);
  rotateToPoint(
    this->BodyOrientation[RIGHT_FORE], trans, this->BodyPosition[RIGHT_FORE], rightElbowPos);

  // Attach upper arm at shoulder, and rotate to hit the end of the forearm.
  // end of forearm, relative to the hand at 0, is elbow pos.
  double shoulderPos[3] = { -0.138, -0.53, -0.60 };
  setOrientation(trans, this->BodyOrientation[TORSO]);
  // calculate relative left shoulder position (to torso)
  MultiplyComponents(shoulderPos, scale);
  trans->TransformPoint(shoulderPos, this->BodyPosition[LEFT_UPPER]);

  // move with torso
  this->BodyPosition[LEFT_UPPER][0] += this->BodyPosition[TORSO][0];
  this->BodyPosition[LEFT_UPPER][1] += this->BodyPosition[TORSO][1];
  this->BodyPosition[LEFT_UPPER][2] += this->BodyPosition[TORSO][2];

  shoulderPos[2] = +0.60 * scale[2];
  // calculate relative right shoulder position (to torso)
  trans->TransformPoint(shoulderPos, this->BodyPosition[RIGHT_UPPER]);

  // move with torso
  this->BodyPosition[RIGHT_UPPER][0] += this->BodyPosition[TORSO][0];
  this->BodyPosition[RIGHT_UPPER][1] += this->BodyPosition[TORSO][1];
  this->BodyPosition[RIGHT_UPPER][2] += this->BodyPosition[TORSO][2];

  // orient the upper arms to aim at the elbow.
  // upper-arm extends along +x at zero rotation. rotate (1,0,0) to
  // vector between shoulder and elbow.
  rotateToPoint(
    this->BodyOrientation[LEFT_UPPER], trans, leftElbowPos, this->BodyPosition[LEFT_UPPER]);

  rotateToPoint(
    this->BodyOrientation[RIGHT_UPPER], trans, rightElbowPos, this->BodyPosition[RIGHT_UPPER]);
}

// Multiple sub-actors require a custom bounding box calc.
double* vtkOpenGLAvatar::GetBounds()
{
  vtkDebugMacro(<< "Getting Bounds");
  vtkBoundingBox bbox;

  bbox.AddBounds(this->HeadActor->GetBounds());
  bbox.AddBounds(this->RightHandActor->GetBounds());
  bbox.AddBounds(this->LeftHandActor->GetBounds());
  for (int i = 0; i < NUM_BODY; ++i)
  {
    bbox.AddBounds(this->BodyActor[i]->GetBounds());
  }

  bbox.GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkOpenGLAvatar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLAvatar::SetUseLeftHand(bool val)
{
  this->Superclass::SetUseLeftHand(val);
  this->LeftHandActor->SetVisibility(val);
  this->BodyActor[LEFT_FORE]->SetVisibility(val);
  bool upperViz = val && !this->ShowHandsOnly;
  this->BodyActor[LEFT_UPPER]->SetVisibility(upperViz);
}
void vtkOpenGLAvatar::SetUseRightHand(bool val)
{
  this->Superclass::SetUseRightHand(val);
  this->RightHandActor->SetVisibility(val);
  this->BodyActor[RIGHT_FORE]->SetVisibility(val);
  bool upperViz = val && !this->ShowHandsOnly;
  this->BodyActor[RIGHT_UPPER]->SetVisibility(upperViz);
}

void vtkOpenGLAvatar::SetShowHandsOnly(bool val)
{
  this->Superclass::SetShowHandsOnly(val);
  this->HeadActor->SetVisibility(!val);
  this->BodyActor[TORSO]->SetVisibility(!val);
  bool upperViz = !val && this->BodyActor[LEFT_UPPER]->GetVisibility();
  this->BodyActor[LEFT_UPPER]->SetVisibility(upperViz);
  upperViz = !val && this->BodyActor[RIGHT_UPPER]->GetVisibility();
  this->BodyActor[RIGHT_UPPER]->SetVisibility(upperViz);
}

void vtkOpenGLAvatar::SetLeftShowRay(bool val)
{
  this->LeftRay->SetShow(val);
}

void vtkOpenGLAvatar::SetRightShowRay(bool val)
{
  this->RightRay->SetShow(val);
}

void vtkOpenGLAvatar::SetRayLength(double length)
{
  this->LeftRay->SetLength(length);
  this->RightRay->SetLength(length);
}

void vtkOpenGLAvatar::SetLabel(const char* label)
{
  this->LabelActor->SetInput(label);
}

vtkTextProperty* vtkOpenGLAvatar::GetLabelTextProperty()
{
  return this->LabelActor->GetTextProperty();
}
