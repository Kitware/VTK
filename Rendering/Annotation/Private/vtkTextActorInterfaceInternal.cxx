// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTextActorInterfaceInternal.h"

#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DAxisFollower.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkVectorText.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkTextActorInterfaceInternal::vtkTextActorInterfaceInternal()
{
  vtkNew<vtkPolyDataMapper> vectorTextMapper;
  vectorTextMapper->SetInputConnection(this->Vector->GetOutputPort());
  this->Follower->SetMapper(vectorTextMapper);
  this->Follower->SetEnableDistanceLOD(0);
  this->Follower->AutoCenterOn();

  this->Follower3D->SetProp3D(this->Actor3D);
  this->Follower3D->SetEnableDistanceLOD(0);
  this->Follower3D->AutoCenterOn();
}

//------------------------------------------------------------------------------
vtkTextActorInterfaceInternal::~vtkTextActorInterfaceInternal() = default;

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetInputText(const std::string& text)
{
  this->Vector->SetText(text.c_str());
  this->Actor3D->SetInput(text.c_str());
  this->Actor2D->SetInput(text.c_str());
}

//------------------------------------------------------------------------------
std::string vtkTextActorInterfaceInternal::GetInputText()
{
  return this->Vector->GetText();
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetCamera(vtkCamera* camera)
{
  this->Follower->SetCamera(camera);
  this->Follower3D->SetCamera(camera);
  this->Camera = camera;
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetAxis(vtkAxisActor* axis)
{
  this->Follower->SetAxis(axis);
  this->Follower3D->SetAxis(axis);
}

//------------------------------------------------------------------------------
vtkProp* vtkTextActorInterfaceInternal::GetActiveProp(bool overlay, bool vector)
{
  if (overlay)
  {
    return this->Actor2D;
  }
  else if (vector)
  {
    return this->Follower;
  }
  else
  {
    return this->Follower3D;
  }
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::UpdateProperty(
  vtkTextProperty* textProperty, vtkProperty* actorProperty)
{
  // no text property here. Use standard prop, and override color/opacity
  this->Follower->GetProperty()->DeepCopy(actorProperty);
  this->Follower->GetProperty()->SetColor(textProperty->GetColor());
  this->Follower->GetProperty()->SetOpacity(textProperty->GetOpacity());
  this->Follower->SetOrientation(0, 0, textProperty->GetOrientation());

  this->Actor2D->SetTextProperty(textProperty);
  this->Actor3D->SetTextProperty(textProperty);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetAmbient(double amb)
{
  this->Follower->GetProperty()->SetAmbient(amb);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetDiffuse(double diffuse)
{
  this->Follower->GetProperty()->SetDiffuse(diffuse);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetActors(vtkPropCollection* collection)
{
  collection->AddItem(this->Follower);
  collection->AddItem(this->Follower3D);
  collection->AddItem(this->Actor3D);
  collection->AddItem(this->Actor2D);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::AdjustScale()
{
  double titleBounds[6];
  this->GetBounds(titleBounds);
  int titleActor3DBounds[4];
  this->Actor3D->GetBoundingBox(titleActor3DBounds);
  const double titleActor3DWidth =
    static_cast<double>(titleActor3DBounds[1] - titleActor3DBounds[0]);
  double scale = (titleBounds[1] - titleBounds[0]) / titleActor3DWidth;

  this->Actor3D->SetScale(scale);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScale(double s)
{
  this->Follower->SetScale(s);
  this->Follower3D->SetScale(s);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetBounds(double bounds[6])
{
  this->Follower->GetMapper()->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetReferencePosition(double pos[3])
{
  this->Follower->GetPosition(pos);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetPosition(double pos[3])
{
  this->Follower->SetPosition(pos);
  this->Follower3D->SetPosition(pos);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetDisplayPosition(double x, double y)
{
  this->Actor2D->SetPosition(x, y);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::RotateActor2DFromAxisProjection(double p1[3], double p2[3])
{
  vtkMatrix4x4* matModelView = this->Camera->GetModelViewTransformMatrix();
  double nearPlane = this->Camera->GetClippingRange()[0];

  // Need view coordinate points.
  double viewPt1[4] = { p1[0], p1[1], p1[2], 1.0 };
  double viewPt2[4] = { p2[0], p2[1], p2[2], 1.0 };

  matModelView->MultiplyPoint(viewPt1, viewPt1);
  matModelView->MultiplyPoint(viewPt2, viewPt2);

  if (viewPt1[2] == 0.0 || viewPt2[2] == 0.0)
  {
    return;
  }

  double p1Pjt[3] = { -nearPlane * viewPt1[0] / viewPt1[2], -nearPlane * viewPt1[1] / viewPt1[2],
    -nearPlane };
  double p2Pjt[3] = { -nearPlane * viewPt2[0] / viewPt2[2], -nearPlane * viewPt2[1] / viewPt2[2],
    -nearPlane };

  double axisOnScreen[2] = { p2Pjt[0] - p1Pjt[0], p2Pjt[1] - p1Pjt[1] };
  double x[2] = { 1.0, 0.0 }, y[2] = { 0.0, 1.0 };

  double dotProd = vtkMath::Dot2D(x, axisOnScreen);

  double orient = 0.0;
  if (vtkMath::Norm2D(axisOnScreen) == 0.0)
  {
    this->Actor2D->SetOrientation(0.0);
    return;
  }
  else
  {
    orient = acos(dotProd / vtkMath::Norm2D(axisOnScreen));
    orient = vtkMath::DegreesFromRadians(orient);
  }

  // adjust angle
  if (vtkMath::Dot2D(y, axisOnScreen) < 0.0)
  {
    orient *= -1.0;
  }

  if (vtkMath::Dot2D(x, axisOnScreen) < 0.0)
  {
    orient += 180.0;
  }

  this->Actor2D->SetOrientation(orient);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScreenOffset(double offset)
{
  this->Follower->SetScreenOffset(offset);
  this->Follower3D->SetScreenOffset(offset);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScreenOffsetVector(double offset[2])
{
  this->Follower->SetScreenOffsetVector(offset);
  this->Follower3D->SetScreenOffsetVector(offset);
}

//------------------------------------------------------------------------------
vtkProp3DAxisFollower* vtkTextActorInterfaceInternal::GetFollower3D() const
{
  return this->Follower3D;
}

//------------------------------------------------------------------------------
vtkAxisFollower* vtkTextActorInterfaceInternal::GetFollower() const
{
  return this->Follower;
}

VTK_ABI_NAMESPACE_END
