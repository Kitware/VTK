// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFollowerTextPropertyAdaptor.h"

#include "vtkAxisActor.h"
#include "vtkAxisFollower.h"
#include "vtkObjectFactory.h"
#include "vtkProp3DAxisFollower.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"

namespace utils
{
// We use 12 as default size, as in vtkTextProperty
constexpr int DefaultFontSize = 12;
};

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkFollowerTextPropertyAdaptor::vtkFollowerTextPropertyAdaptor(
  vtkAxisFollower* follower, vtkProp3DAxisFollower* propFollower)
  : MapperFollower(follower)
  , PropFollower(propFollower)
{
  this->ModifiedCallback->SetCallback(vtkFollowerTextPropertyAdaptor::OnModified);
  this->ModifiedCallback->SetClientData(this);
}

//------------------------------------------------------------------------------
vtkFollowerTextPropertyAdaptor::~vtkFollowerTextPropertyAdaptor() = default;

//------------------------------------------------------------------------------
void vtkFollowerTextPropertyAdaptor::OnModified(vtkObject*, unsigned long, void* clientData, void*)
{
  auto adaptor = reinterpret_cast<vtkFollowerTextPropertyAdaptor*>(clientData);
  adaptor->MapperFollower->GetAxis()->Modified();
  adaptor->PropFollower->Modified();
}

//------------------------------------------------------------------------------
void vtkFollowerTextPropertyAdaptor::UpdateProperty(
  vtkTextProperty* textProperty, vtkProperty* actorProperty)
{
  // no text property here. Use standard prop, and override color/opacity
  this->MapperFollower->GetProperty()->DeepCopy(actorProperty);
  this->MapperFollower->GetProperty()->SetColor(textProperty->GetColor());
  this->MapperFollower->GetProperty()->SetOpacity(textProperty->GetOpacity());
  this->MapperFollower->SetOrientation(0, 0, textProperty->GetOrientation());

  // mimics font size
  double prevScale[3];
  this->MapperFollower->GetScale(prevScale);
  double scale = prevScale[0] / this->FontScale;
  // Use font size change factor to rescale.
  double size = textProperty->GetFontSize();
  this->FontScale = size / utils::DefaultFontSize;
  this->MapperFollower->SetScale(scale * this->FontScale);

  textProperty->RemoveObserver(this->TextPropObserverId);
  this->TextPropObserverId =
    textProperty->AddObserver(vtkCommand::ModifiedEvent, this->ModifiedCallback);
}

//------------------------------------------------------------------------------
void vtkFollowerTextPropertyAdaptor::SetScale(double scale)
{
  this->MapperFollower->SetScale(this->FontScale * scale);
}

//------------------------------------------------------------------------------
double vtkFollowerTextPropertyAdaptor::GetFontScale()
{
  return this->FontScale;
}

VTK_ABI_NAMESPACE_END
