/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBalloonWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkStdString.h"
#include "vtkProp.h"
#include "vtkPropPicker.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkAssemblyPath.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include <vtkstd/map>

vtkCxxRevisionMacro(vtkBalloonWidget, "1.2");
vtkStandardNewMacro(vtkBalloonWidget);

//-- Define the PIMPLd array of vtkProp and vtkString --
class vtkPropMap : public vtkstd::map<vtkProp*,vtkStdString> {};
typedef vtkstd::map<vtkProp*,vtkStdString>::iterator vtkPropMapIterator;


//-------------------------------------------------------------------------
vtkBalloonWidget::vtkBalloonWidget()
{
  this->Picker = vtkPropPicker::New();
  this->Picker->PickFromListOn();
  
  this->CurrentProp = NULL;
  this->PropMap = new vtkPropMap;
}

//-------------------------------------------------------------------------
vtkBalloonWidget::~vtkBalloonWidget()
{
  this->Picker->Delete();

  if ( this->CurrentProp )
    {
    this->CurrentProp->Delete();
    this->CurrentProp = NULL;
    }
  
  delete this->PropMap;
}

//----------------------------------------------------------------------
void vtkBalloonWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  if ( this->Interactor)
    {
    this->SetCurrentRenderer(this->Interactor->GetRenderWindow()->
                             GetRenderers()->GetFirstRenderer());
    }
  if ( !this->CurrentRenderer)
    {
    return;
    }

  if ( enabling )
    {
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);
    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);
    }
  else
    {
    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);
    this->SetCurrentRenderer(NULL);
    }
}

//----------------------------------------------------------------------
void vtkBalloonWidget::SetPicker(vtkAbstractPropPicker *picker)
{
  if ( picker == NULL || picker == this->Picker )
    {
    return;
    }

  // Configure picker appropriately
  picker->PickFromListOn();

  this->Picker->Delete();
  this->Picker = picker;
  this->Picker->Register(this);
  this->Modified();
}


//----------------------------------------------------------------------
void vtkBalloonWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkBalloonRepresentation::New();
    }
}

//-------------------------------------------------------------------------
void vtkBalloonWidget::AddBalloonText(vtkProp *prop, vtkStdString *str)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter == this->PropMap->end() || (*this->PropMap)[prop] != *str )
    {
    (*this->PropMap)[prop] = *str;
    if ( prop != NULL )
      {
      this->Picker->AddPickList(prop);
      }
    this->Modified();
    }
}

//-------------------------------------------------------------------------
void vtkBalloonWidget::AddBalloonText(vtkProp *prop, const char *str)
{
  vtkStdString s = vtkStdString(str);
  this->AddBalloonText(prop,&s);
}


//-------------------------------------------------------------------------
void vtkBalloonWidget::RemoveBalloonText(vtkProp *prop)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter != this->PropMap->end() )
    {
    this->PropMap->erase(iter);
    if ( prop != NULL )
      {
      this->Picker->DeletePickList(prop);
      }
    this->Modified();
    }
}


//-------------------------------------------------------------------------
int vtkBalloonWidget::SubclassHoverAction()
{
  double e[2];
  e[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  vtkRenderer *ren = 
    this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
  if ( this->CurrentProp )
    {
    this->CurrentProp->UnRegister(this);
    this->CurrentProp = NULL;
    }
  this->Picker->Pick(e[0],e[1],0.0,ren);
  vtkAssemblyPath *path = this->Picker->GetPath();
  if ( path != NULL )
    {
    vtkPropMapIterator iter = 
      this->PropMap->find(path->GetFirstNode()->GetViewProp());
    if ( iter != this->PropMap->end() )
      {
      this->CurrentProp = (*iter).first;
      this->CurrentProp->Register(this);
      reinterpret_cast<vtkBalloonRepresentation*>(this->WidgetRep)->
        SetBalloonText((*iter).second);
      this->WidgetRep->StartWidgetInteraction(e);
      this->Render();
      }
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkBalloonWidget::SubclassEndHoverAction()
{
  double e[2];
  e[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  this->WidgetRep->EndWidgetInteraction(e);
  this->Render();

  return 1;
}


//-------------------------------------------------------------------------
void vtkBalloonWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Current Prop: ";
  if ( this->CurrentProp )
    {
    os << this->CurrentProp << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Picker: " << this->Picker << "\n";
}
