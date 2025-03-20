// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTextActorInterfacePrivate.h"

#include "vtkAxisFollower.h"
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
vtkTextActorInterfacePrivate::vtkTextActorInterfacePrivate()
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
vtkTextActorInterfacePrivate::vtkTextActorInterfacePrivate(
  vtkTextActorInterfacePrivate&&) = default;

//------------------------------------------------------------------------------
vtkTextActorInterfacePrivate::~vtkTextActorInterfacePrivate() = default;

//------------------------------------------------------------------------------
void vtkTextActorInterfacePrivate::SetInputText(const std::string& text)
{
  this->Vector->SetText(text.c_str());
  this->Actor3D->SetInput(text.c_str());
  this->Actor2D->SetInput(text.c_str());
}

//------------------------------------------------------------------------------
void vtkTextActorInterfacePrivate::SetCamera(vtkCamera* camera)
{
  this->Follower->SetCamera(camera);
  this->Follower3D->SetCamera(camera);
}

//------------------------------------------------------------------------------
vtkProp* vtkTextActorInterfacePrivate::GetActiveProp(bool overlay, bool vector)
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
void vtkTextActorInterfacePrivate::UpdateProperty(
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
void vtkTextActorInterfacePrivate::GetActors(vtkPropCollection* collection)
{
  collection->AddItem(this->Follower);
  collection->AddItem(this->Follower3D);
  collection->AddItem(this->Actor3D);
  collection->AddItem(this->Actor2D);
}

//------------------------------------------------------------------------------
void vtkTextActorHandlerPrivate::AdjustScale()
{
  double titleBounds[6];
  this->Follower->GetMapper()->GetBounds(titleBounds);
  int titleActor3DBounds[4];
  this->Actor3D->GetBoundingBox(titleActor3DBounds);
  const double titleActor3DWidth =
    static_cast<double>(titleActor3DBounds[1] - titleActor3DBounds[0]);
  double scale = (titleBounds[1] - titleBounds[0]) / titleActor3DWidth;

  this->Actor3D->SetScale(scale);
}

//------------------------------------------------------------------------------
void vtkTextActorHandlerPrivate::SetScale(double s)
{
  this->Follower->SetScale(s);
  this->Follower3D->SetScale(s);
}

VTK_ABI_NAMESPACE_END
