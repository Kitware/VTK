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

#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkEventData.h"
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <map>

vtkStandardNewMacro(vtkOpenVRMenuWidget);

class vtkOpenVRMenuWidget::InternalElement
{
public:
  vtkCommand* Command;
  std::string Name;
  std::string Text;
  InternalElement() {}
};

//----------------------------------------------------------------------
vtkOpenVRMenuWidget::vtkOpenVRMenuWidget()
{
  // Set the initial state
  this->WidgetState = vtkOpenVRMenuWidget::Start;

  this->EventCommand = vtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(vtkOpenVRMenuWidget::EventCallback);

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select,
      this, vtkOpenVRMenuWidget::StartMenuAction);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkOpenVRMenuWidget::SelectMenuAction);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkOpenVRMenuWidget::MoveAction);
  }
}

//----------------------------------------------------------------------
vtkOpenVRMenuWidget::~vtkOpenVRMenuWidget()
{
  this->EventCommand->Delete();
}

void vtkOpenVRMenuWidget::PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd)
{
  vtkOpenVRMenuWidget::InternalElement* el = new vtkOpenVRMenuWidget::InternalElement();
  el->Text = text;
  el->Command = cmd;
  el->Name = name;
  this->Menus.push_front(el);

  static_cast<vtkOpenVRMenuRepresentation*>(this->WidgetRep)
    ->PushFrontMenuItem(name, text, this->EventCommand);

  this->Modified();
}

void vtkOpenVRMenuWidget::RenameMenuItem(const char* name, const char* text)
{
  for (auto itr : this->Menus)
  {
    if (itr->Name == name)
    {
      itr->Text = text;
    }
  }
  static_cast<vtkOpenVRMenuRepresentation*>(this->WidgetRep)->RenameMenuItem(name, text);
}

void vtkOpenVRMenuWidget::RemoveMenuItem(const char* name)
{
  for (auto itr = this->Menus.begin(); itr != this->Menus.end(); ++itr)
  {
    if ((*itr)->Name == name)
    {
      delete *itr;
      this->Menus.erase(itr);
      break;
    }
  }
  static_cast<vtkOpenVRMenuRepresentation*>(this->WidgetRep)->RemoveMenuItem(name);
}

void vtkOpenVRMenuWidget::RemoveAllMenuItems()
{
  while (this->Menus.size() > 0)
  {
    auto itr = this->Menus.begin();
    delete *itr;
    this->Menus.erase(itr);
  }
  static_cast<vtkOpenVRMenuRepresentation*>(this->WidgetRep)->RemoveAllMenuItems();
}

void vtkOpenVRMenuWidget::EventCallback(vtkObject*, unsigned long, void* clientdata, void* calldata)
{
  vtkOpenVRMenuWidget* self = static_cast<vtkOpenVRMenuWidget*>(clientdata);
  std::string name = static_cast<const char*>(calldata);

  for (auto& menu : self->Menus)
  {
    if (menu->Name == name)
    {
      menu->Command->Execute(
        self, vtkWidgetEvent::Select3D, static_cast<void*>(const_cast<char*>(menu->Name.c_str())));
    }
  }
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::ShowSubMenu(vtkOpenVRMenuWidget* w)
{
  w->SetInteractor(this->Interactor);
  w->Show(static_cast<vtkEventData*>(this->CallData));
}

void vtkOpenVRMenuWidget::Show(vtkEventData* ed)
{
  this->On();
  if (this->WidgetState == vtkOpenVRMenuWidget::Start)
  {
    if (!this->Parent)
    {
      this->GrabFocus(this->EventCallbackCommand);
    }
    this->CallData = ed;
    this->WidgetRep->StartComplexInteraction(this->Interactor, this, vtkWidgetEvent::Select, ed);

    this->WidgetState = vtkOpenVRMenuWidget::Active;
  }
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::StartMenuAction(vtkAbstractWidget* w)
{
  vtkOpenVRMenuWidget* self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  if (self->WidgetState == vtkOpenVRMenuWidget::Active)
  {
    if (!self->Parent)
    {
      self->ReleaseFocus();
    }

    self->Off();
    self->WidgetState = vtkOpenVRMenuWidget::Start;

    self->WidgetRep->EndComplexInteraction(
      self->Interactor, self, vtkWidgetEvent::Select, self->CallData);
  }
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::SelectMenuAction(vtkAbstractWidget* w)
{
  vtkOpenVRMenuWidget* self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenVRMenuWidget::Active)
  {
    return;
  }

  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->Off();
  self->WidgetState = vtkOpenVRMenuWidget::Start;

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
}

//-------------------------------------------------------------------------
void vtkOpenVRMenuWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkOpenVRMenuWidget* self = reinterpret_cast<vtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenVRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);
}

//----------------------------------------------------------------------
void vtkOpenVRMenuWidget::SetRepresentation(vtkOpenVRMenuRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
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
