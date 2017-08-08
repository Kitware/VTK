/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRMenuWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenVRMenuWidget.h"
#include "vtkOpenVRMenuRepresentation.h"

#include "vtkEventData.h"
#include "vtkNew.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkInteractorStyle3D.h"
#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"

#include <map>

class vtkPropMap : public std::map<vtkProp*, vtkStdString> {};
typedef std::map<vtkProp*, vtkStdString>::iterator vtkPropMapIterator;

vtkStandardNewMacro(vtkOpenVRMenuWidget);

//----------------------------------------------------------------------
vtkOpenVRMenuWidget::vtkOpenVRMenuWidget()
{
  // Set the initial state
  this->WidgetState = vtkOpenVRMenuWidget::Start;

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent,
      ed.Get(), vtkWidgetEvent::Select,
      this, vtkOpenVRMenuWidget::StartMenuAction);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent,
      ed.Get(), vtkWidgetEvent::Select3D,
      this, vtkOpenVRMenuWidget::SelectMenuAction);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent,
      ed.Get(), vtkWidgetEvent::Move3D,
      this, vtkOpenVRMenuWidget::MoveAction);
  }

  this->PropMap = new vtkPropMap;
}

//----------------------------------------------------------------------
vtkOpenVRMenuWidget::~vtkOpenVRMenuWidget()
{
  this->PropMap->clear();
  delete this->PropMap;
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::StartMenuAction(vtkAbstractWidget *w)
{
  vtkOpenVRMenuWidget *self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  // basically toggle the display of the menu
  if (self->WidgetState == vtkOpenVRMenuWidget::Start)
  {
    if ( ! self->Parent )
    {
      self->GrabFocus(self->EventCallbackCommand);
    }
    self->WidgetRep->StartComplexInteraction(
      self->Interactor, self, vtkWidgetEvent::Select, self->CallData);

    self->WidgetState = vtkOpenVRMenuWidget::Active;
  }
  else
  {
    if ( ! self->Parent )
    {
      self->ReleaseFocus();
    }
    self->WidgetRep->EndComplexInteraction(
      self->Interactor, self, vtkWidgetEvent::Select, self->CallData);

    self->WidgetState = vtkOpenVRMenuWidget::Start;
  }
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::SelectMenuAction(vtkAbstractWidget *w)
{
  vtkOpenVRMenuWidget *self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenVRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
  if ( ! self->Parent )
  {
    self->ReleaseFocus();
  }
  self->WidgetState = vtkOpenVRMenuWidget::Start;
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkOpenVRMenuWidget *self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenVRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);
}

//----------------------------------------------------------------------
void vtkOpenVRMenuWidget::
SetRepresentation(vtkOpenVRMenuRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(
    reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
void vtkOpenVRMenuWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkOpenVRMenuRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkOpenVRMenuWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
