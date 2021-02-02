/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRMenuWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenXRMenuWidget.h"
#include "vtkOpenXRMenuRepresentation.h"

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

vtkStandardNewMacro(vtkOpenXRMenuWidget);

class vtkOpenXRMenuWidget::InternalElement
{
public:
  vtkCommand* Command;
  std::string Name;
  std::string Text;
  InternalElement() {}
};

//------------------------------------------------------------------------------
vtkOpenXRMenuWidget::vtkOpenXRMenuWidget()
{
  // Set the initial state
  this->WidgetState = vtkOpenXRMenuWidget::Start;

  this->EventCommand = vtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(vtkOpenXRMenuWidget::EventCallback);

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Menu3DEvent, ed, vtkWidgetEvent::Select,
      this, vtkOpenXRMenuWidget::StartMenuAction);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkOpenXRMenuWidget::SelectMenuAction);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkOpenXRMenuWidget::MoveAction);
  }
}

//------------------------------------------------------------------------------
vtkOpenXRMenuWidget::~vtkOpenXRMenuWidget()
{
  this->EventCommand->Delete();
}

void vtkOpenXRMenuWidget::PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd)
{
  vtkOpenXRMenuWidget::InternalElement* el = new vtkOpenXRMenuWidget::InternalElement();
  el->Text = text;
  el->Command = cmd;
  el->Name = name;
  this->Menus.push_front(el);

  static_cast<vtkOpenXRMenuRepresentation*>(this->WidgetRep)
    ->PushFrontMenuItem(name, text, this->EventCommand);

  this->Modified();
}

void vtkOpenXRMenuWidget::RenameMenuItem(const char* name, const char* text)
{
  for (auto itr : this->Menus)
  {
    if (itr->Name == name)
    {
      itr->Text = text;
    }
  }
  static_cast<vtkOpenXRMenuRepresentation*>(this->WidgetRep)->RenameMenuItem(name, text);
}

void vtkOpenXRMenuWidget::RemoveMenuItem(const char* name)
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
  static_cast<vtkOpenXRMenuRepresentation*>(this->WidgetRep)->RemoveMenuItem(name);
}

void vtkOpenXRMenuWidget::RemoveAllMenuItems()
{
  while (this->Menus.size() > 0)
  {
    auto itr = this->Menus.begin();
    delete *itr;
    this->Menus.erase(itr);
  }
  static_cast<vtkOpenXRMenuRepresentation*>(this->WidgetRep)->RemoveAllMenuItems();
}

void vtkOpenXRMenuWidget::EventCallback(vtkObject*, unsigned long, void* clientdata, void* calldata)
{
  vtkOpenXRMenuWidget* self = static_cast<vtkOpenXRMenuWidget*>(clientdata);
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

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::ShowSubMenu(vtkOpenXRMenuWidget* w)
{
  w->SetInteractor(this->Interactor);
  w->Show(static_cast<vtkEventData*>(this->CallData));
}

void vtkOpenXRMenuWidget::Show(vtkEventData* ed)
{
  this->On();
  if (this->WidgetState == vtkOpenXRMenuWidget::Start)
  {
    if (!this->Parent)
    {
      this->GrabFocus(this->EventCallbackCommand);
    }
    this->CallData = ed;
    this->WidgetRep->StartComplexInteraction(this->Interactor, this, vtkWidgetEvent::Select, ed);

    this->WidgetState = vtkOpenXRMenuWidget::Active;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::StartMenuAction(vtkAbstractWidget* w)
{
  vtkOpenXRMenuWidget* self = reinterpret_cast<vtkOpenXRMenuWidget*>(w);

  if (self->WidgetState == vtkOpenXRMenuWidget::Active)
  {
    if (!self->Parent)
    {
      self->ReleaseFocus();
    }

    self->Off();
    self->WidgetState = vtkOpenXRMenuWidget::Start;

    self->WidgetRep->EndComplexInteraction(
      self->Interactor, self, vtkWidgetEvent::Select, self->CallData);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::SelectMenuAction(vtkAbstractWidget* w)
{
  vtkOpenXRMenuWidget* self = reinterpret_cast<vtkOpenXRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenXRMenuWidget::Active)
  {
    return;
  }

  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->Off();
  self->WidgetState = vtkOpenXRMenuWidget::Start;

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkOpenXRMenuWidget* self = reinterpret_cast<vtkOpenXRMenuWidget*>(w);

  if (self->WidgetState != vtkOpenXRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::SetRepresentation(vtkOpenXRMenuRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkOpenXRMenuRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRMenuWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
