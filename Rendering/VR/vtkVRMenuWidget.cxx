/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRMenuWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVRMenuWidget.h"
#include "vtkVRMenuRepresentation.h"

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

vtkStandardNewMacro(vtkVRMenuWidget);

class vtkVRMenuWidget::InternalElement
{
public:
  vtkCommand* Command;
  std::string Name;
  std::string Text;
  InternalElement() = default;
};

//------------------------------------------------------------------------------
vtkVRMenuWidget::vtkVRMenuWidget()
{
  // Set the initial state
  this->WidgetState = vtkVRMenuWidget::Start;

  this->EventCommand = vtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(vtkVRMenuWidget::EventCallback);

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Menu3DEvent, ed, vtkWidgetEvent::Select, this, vtkVRMenuWidget::StartMenuAction);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkVRMenuWidget::SelectMenuAction);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkVRMenuWidget::MoveAction);
  }
}

//------------------------------------------------------------------------------
vtkVRMenuWidget::~vtkVRMenuWidget()
{
  this->EventCommand->Delete();
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd)
{
  vtkVRMenuWidget::InternalElement* el = new vtkVRMenuWidget::InternalElement();
  el->Text = text;
  el->Command = cmd;
  el->Name = name;
  this->Menus.push_front(el);

  static_cast<vtkVRMenuRepresentation*>(this->WidgetRep)
    ->PushFrontMenuItem(name, text, this->EventCommand);

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::RenameMenuItem(const char* name, const char* text)
{
  for (auto itr : this->Menus)
  {
    if (itr->Name == name)
    {
      itr->Text = text;
    }
  }
  static_cast<vtkVRMenuRepresentation*>(this->WidgetRep)->RenameMenuItem(name, text);
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::RemoveMenuItem(const char* name)
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
  static_cast<vtkVRMenuRepresentation*>(this->WidgetRep)->RemoveMenuItem(name);
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::RemoveAllMenuItems()
{
  while (!this->Menus.empty())
  {
    auto itr = this->Menus.begin();
    delete *itr;
    this->Menus.erase(itr);
  }
  static_cast<vtkVRMenuRepresentation*>(this->WidgetRep)->RemoveAllMenuItems();
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::EventCallback(vtkObject*, unsigned long, void* clientdata, void* calldata)
{
  vtkVRMenuWidget* self = static_cast<vtkVRMenuWidget*>(clientdata);
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
void vtkVRMenuWidget::ShowSubMenu(vtkVRMenuWidget* w)
{
  w->SetInteractor(this->Interactor);
  w->Show(static_cast<vtkEventData*>(this->CallData));
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::Show(vtkEventData* ed)
{
  this->On();
  if (this->WidgetState == vtkVRMenuWidget::Start)
  {
    if (!this->Parent)
    {
      this->GrabFocus(this->EventCallbackCommand);
    }
    this->CallData = ed;
    this->WidgetRep->StartComplexInteraction(this->Interactor, this, vtkWidgetEvent::Select, ed);

    this->WidgetState = vtkVRMenuWidget::Active;
  }
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::StartMenuAction(vtkAbstractWidget* w)
{
  vtkVRMenuWidget* self = reinterpret_cast<vtkVRMenuWidget*>(w);

  if (self->WidgetState == vtkVRMenuWidget::Active)
  {
    if (!self->Parent)
    {
      self->ReleaseFocus();
    }

    self->Off();
    self->WidgetState = vtkVRMenuWidget::Start;

    self->WidgetRep->EndComplexInteraction(
      self->Interactor, self, vtkWidgetEvent::Select, self->CallData);
  }
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::SelectMenuAction(vtkAbstractWidget* w)
{
  vtkVRMenuWidget* self = reinterpret_cast<vtkVRMenuWidget*>(w);

  if (self->WidgetState != vtkVRMenuWidget::Active)
  {
    return;
  }

  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->Off();
  self->WidgetState = vtkVRMenuWidget::Start;

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkVRMenuWidget* self = reinterpret_cast<vtkVRMenuWidget*>(w);

  if (self->WidgetState != vtkVRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::SetRepresentation(vtkVRMenuRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkVRMenuRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkVRMenuWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WidgetState: " << this->WidgetState << "\n";
}
