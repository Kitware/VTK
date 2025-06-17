// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTextActorInterfaceInternal.h"

#include "Private/vtkFollowerTextPropertyAdaptor.h"
#include "vtkAxisActor.h"
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
  : FollowerAdaptor(
      std::make_unique<vtkFollowerTextPropertyAdaptor>(this->VectorFollower, this->RasterFollower))
{
  vtkNew<vtkPolyDataMapper> vectorTextMapper;
  vectorTextMapper->SetInputConnection(this->Vector->GetOutputPort());
  this->VectorFollower->SetMapper(vectorTextMapper);
  this->VectorFollower->SetEnableDistanceLOD(0);
  this->VectorFollower->AutoCenterOn();

  this->RasterFollower->SetProp3D(this->Raster3D);
  this->RasterFollower->SetEnableDistanceLOD(0);
  this->RasterFollower->AutoCenterOn();
}

//------------------------------------------------------------------------------
vtkTextActorInterfaceInternal::~vtkTextActorInterfaceInternal() = default;

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetInputText(const std::string& text)
{
  this->Vector->SetText(text.c_str());
  this->Raster3D->SetInput(text.c_str());
  this->Raster2D->SetInput(text.c_str());
}

//------------------------------------------------------------------------------
std::string vtkTextActorInterfaceInternal::GetInputText()
{
  return this->Vector->GetText();
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetCamera(vtkCamera* camera)
{
  this->VectorFollower->SetCamera(camera);
  this->RasterFollower->SetCamera(camera);
  this->Camera = camera;
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetAxis(vtkAxisActor* axis)
{
  this->VectorFollower->SetAxis(axis);
  this->RasterFollower->SetAxis(axis);
}

//------------------------------------------------------------------------------
vtkProp* vtkTextActorInterfaceInternal::GetActiveProp(bool overlay, bool vector)
{
  if (overlay)
  {
    return this->Raster2D;
  }
  else if (vector)
  {
    return this->VectorFollower;
  }
  else
  {
    return this->RasterFollower;
  }
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetTextProperty(
  vtkTextProperty* textProperty, vtkProperty* actorProperty)
{
  this->FollowerAdaptor->UpdateProperty(textProperty, actorProperty);
  this->Raster2D->SetTextProperty(textProperty);
  this->Raster3D->SetTextProperty(textProperty);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetActors(vtkPropCollection* collection)
{
  collection->AddItem(this->VectorFollower);
  collection->AddItem(this->RasterFollower);
  collection->AddItem(this->Raster3D);
  collection->AddItem(this->Raster2D);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::AdjustScale()
{
  double titleBounds[6];
  this->GetBounds(titleBounds);
  int titleActor3DBounds[4];
  this->Raster3D->GetBoundingBox(titleActor3DBounds);
  const double titleActor3DWidth =
    static_cast<double>(titleActor3DBounds[1] - titleActor3DBounds[0]);
  double scale = (titleBounds[1] - titleBounds[0]) / titleActor3DWidth;

  this->Raster3D->SetScale(scale);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScale(double scale)
{
  this->FollowerAdaptor->SetScale(scale);
  this->RasterFollower->SetScale(scale);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetBounds(double bounds[6])
{
  this->VectorFollower->GetMapper()->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::GetReferencePosition(double pos[3])
{
  this->VectorFollower->GetPosition(pos);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetPosition(double pos[3])
{
  this->VectorFollower->SetPosition(pos);
  this->RasterFollower->SetPosition(pos);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetDisplayPosition(double x, double y)
{
  this->Raster2D->SetPosition(x, y);
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
    this->Raster2D->SetOrientation(0.0);
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

  this->Raster2D->SetOrientation(orient);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScreenOffset(double offset)
{
  this->VectorFollower->SetScreenOffset(offset);
  this->RasterFollower->SetScreenOffset(offset);
}

//------------------------------------------------------------------------------
void vtkTextActorInterfaceInternal::SetScreenOffsetVector(double offset[2])
{
  this->VectorFollower->SetScreenOffsetVector(offset);
  this->RasterFollower->SetScreenOffsetVector(offset);
}

//------------------------------------------------------------------------------
vtkProp3DAxisFollower* vtkTextActorInterfaceInternal::GetFollower3D() const
{
  return this->RasterFollower;
}

//------------------------------------------------------------------------------
vtkAxisFollower* vtkTextActorInterfaceInternal::GetFollower() const
{
  return this->VectorFollower;
}

VTK_ABI_NAMESPACE_END
