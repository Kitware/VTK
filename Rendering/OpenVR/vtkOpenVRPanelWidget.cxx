/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPanelWidget.h"
#include "vtkOpenVRPanelRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"

#include <map>

class vtkPropMap : public std::map<vtkProp*, vtkStdString> {};
typedef std::map<vtkProp*, vtkStdString>::iterator vtkPropMapIterator;

vtkStandardNewMacro(vtkOpenVRPanelWidget);

//----------------------------------------------------------------------
vtkOpenVRPanelWidget::vtkOpenVRPanelWidget()
{
  vtkNew<vtkEventDataMove3D> edR;
  edR->SetDevice(vtkEventDataDevice::RightController);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent,
    edR.Get(), vtkWidgetEvent::Move3D,
    this, vtkOpenVRPanelWidget::Update);

  vtkNew<vtkEventDataMove3D> edL;
  edL->SetDevice(vtkEventDataDevice::LeftController);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent,
    edL.Get(), vtkWidgetEvent::Move3D,
    this, vtkOpenVRPanelWidget::Update);

  this->PropMap = new vtkPropMap;

  for (int i = 0; i < vtkEventDataNumberOfDevices; i++)
  {
    this->HoveringDevice[i] = 0;
  }
}

//----------------------------------------------------------------------
vtkOpenVRPanelWidget::~vtkOpenVRPanelWidget()
{
  this->PropMap->clear();
  delete this->PropMap;
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::Update(vtkAbstractWidget *w)
{
  vtkOpenVRPanelWidget *self = reinterpret_cast<vtkOpenVRPanelWidget*>(w);
  vtkEventData *edata = static_cast<vtkEventData *>(self->CallData);
  vtkEventDataDevice3D *ed = edata->GetAsEventDataDevice3D();

  if (!ed)
  {
    return;
  }

  if (!self->GetInteractor() ||
    !self->GetInteractor()->GetInteractorStyle())
  {
    return;
  }

  vtkOpenVRInteractorStyle *is = reinterpret_cast<vtkOpenVRInteractorStyle*>
    (self->GetInteractor()->GetInteractorStyle());
  if (!is)
  {
    return;
  }

  double e[3]; //dummy event
  e[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);
  e[2] = static_cast<double>(self->Interactor->GetEventPosition()[2]);

  int dev = static_cast<int>(ed->GetDevice());

  //Compute interaction state
  if (is->GetInteractionPicker()->GetPath())
  {
    vtkProp* prop =
      is->GetInteractionPicker()->GetPath()->GetFirstNode()->GetViewProp();

    vtkPropMapIterator iter =
      self->PropMap->find(prop);

    if (iter != self->PropMap->end())
    {
      //Current device is hovering
      self->HoveringDevice[dev] = 1;

      //Set representation text and hovered prop
      reinterpret_cast<vtkOpenVRPanelRepresentation*>(self->WidgetRep)->
        SetText(&(*iter).second);
      reinterpret_cast<vtkOpenVRPanelRepresentation*>(self->WidgetRep)->
        SetHoveredProp((*iter).first);
    }
    else
    {
      //Current device is not hovering
      self->HoveringDevice[dev] = 0;
    }
  }
  else
  {
    //Current device is not hovering
    self->HoveringDevice[dev] = 0;
  }


  int hovering = 0;
  //Do we have a device hovering a prop?
  for (int i = 0; i < vtkEventDataNumberOfDevices; i++)
  {
    hovering += self->HoveringDevice[i];

    // End interaction if a device is active
    if (is->GetInteractionState(static_cast<vtkEventDataDevice>(i)))
    {
      self->WidgetRep->EndWidgetInteraction(e);
      return;
    }
  }
  //Start interaction if a prop is hovered by any device
  if (hovering > 0)
  {
    self->WidgetRep->StartWidgetInteraction(e);
  }
  else
  {
    self->WidgetRep->EndWidgetInteraction(e);
  }
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::
SetRepresentation(vtkOpenVRPanelRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(
    reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkOpenVRPanelRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::AddTooltip(vtkProp *prop, vtkStdString* str)
{
  assert(prop);
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter == this->PropMap->end())
  {
    (*this->PropMap)[prop] = *str;
    this->Modified();
  }
  else
  {
    (*iter).second = *str;
  }
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::AddTooltip(vtkProp *prop, const char* str)
{
  vtkStdString s;
  if (str)
  {
    s = vtkStdString(str);
  }
  this->AddTooltip(prop, &s);
}
