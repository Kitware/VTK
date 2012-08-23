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
#include "vtkPickingManager.h"
#include "vtkCommand.h"
#include "vtkImageData.h"

#include <map>

vtkStandardNewMacro(vtkBalloonWidget);

//-- Define the PIMPLd array of vtkProp and vtkString --
struct vtkBalloon
{
  vtkStdString Text;
  vtkImageData *Image;

  vtkBalloon() : Text(), Image(0) {}
  vtkBalloon(vtkStdString *str, vtkImageData *img)
    {
      this->Text = *str;
      this->Image = img;
      if ( this->Image )
        {
        this->Image->Register(NULL);
        }
    }
  vtkBalloon(const char *str, vtkImageData *img)
    {
      this->Text = vtkStdString(str);
      this->Image = img;
      if ( this->Image )
        {
        this->Image->Register(NULL);
        }
    }
  ~vtkBalloon()
    {
      if (this->Image)
        {
        this->Image->UnRegister(NULL);
        }
    }
  void operator=(const vtkBalloon &balloon)
    {
      this->Text = balloon.Text;

      // Don't leak if we already have an image.
      if( this->Image )
        {
        this->Image->UnRegister(NULL);
        this->Image = NULL;
        }

      this->Image = balloon.Image;
      if ( this->Image )
        {
        this->Image->Register(NULL);
        }
    }
  int operator==(const vtkBalloon &balloon) const
    {
      if ( this->Image == balloon.Image )
        {
        if ( this->Text == balloon.Text )
          {
          return 1;
          }
        }
      return 0;
    }
  int operator!=(const vtkBalloon &balloon) const
    {
      return !(*this == balloon);
    }
};


class vtkPropMap : public std::map<vtkProp*,vtkBalloon> {};
typedef std::map<vtkProp*,vtkBalloon>::iterator vtkPropMapIterator;


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

  this->PropMap->clear();
  delete this->PropMap;
}

//----------------------------------------------------------------------
void vtkBalloonWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  if ( this->Interactor &&
       this->Interactor->GetRenderWindow())
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

  this->PickersModified();
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBalloonWidget::RegisterPickers()
{
  this->Interactor->GetPickingManager()->AddPicker(this->Picker, this);
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
void vtkBalloonWidget::AddBalloon(vtkProp *prop, vtkStdString *str,
                                  vtkImageData *img)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter == this->PropMap->end() || (*this->PropMap)[prop] != vtkBalloon(str,img) )
    {
    (*this->PropMap)[prop] = vtkBalloon(str,img);
    if ( prop != NULL )
      {
      this->Picker->DeletePickList(prop); //ensure only entered once
      this->Picker->AddPickList(prop);
      }
    this->Modified();
    }
}

//-------------------------------------------------------------------------
void vtkBalloonWidget::AddBalloon(vtkProp *prop, const char *str,
                                  vtkImageData *img)
{
  vtkStdString s;
  if ( str )
    {
    s = vtkStdString(str);
    }
  this->AddBalloon(prop,&s,img);
}


//-------------------------------------------------------------------------
void vtkBalloonWidget::RemoveBalloon(vtkProp *prop)
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
const char *vtkBalloonWidget::GetBalloonString(vtkProp *prop)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter != this->PropMap->end() )
    {
    return (*iter).second.Text.c_str();
    }
  return NULL;
}

//-------------------------------------------------------------------------
vtkImageData *vtkBalloonWidget::GetBalloonImage(vtkProp *prop)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter != this->PropMap->end() )
    {
    return (*iter).second.Image;
    }
  return NULL;
}

//-------------------------------------------------------------------------
void vtkBalloonWidget::
UpdateBalloonString(vtkProp *prop, const char *str)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter != this->PropMap->end() )
    {
    (*iter).second.Text = str;
    this->WidgetRep->Modified();
    }
}


//-------------------------------------------------------------------------
void vtkBalloonWidget::
UpdateBalloonImage(vtkProp *prop, vtkImageData *image)
{
  vtkPropMapIterator iter = this->PropMap->find(prop);
  if ( iter != this->PropMap->end() )
    {
    (*iter).second.Image = image;
    this->WidgetRep->Modified();
    }
}


//-------------------------------------------------------------------------
int vtkBalloonWidget::SubclassHoverAction()
{
  double e[2];
  e[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  if ( this->CurrentProp )
    {
    this->CurrentProp->UnRegister(this);
    this->CurrentProp = NULL;
    }

  vtkAssemblyPath* path = this->GetAssemblyPath(e[0], e[1], 0., this->Picker);

  if ( path != NULL )
    {
    vtkPropMapIterator iter =
      this->PropMap->find(path->GetFirstNode()->GetViewProp());
    if ( iter != this->PropMap->end() )
      {
      this->CurrentProp = (*iter).first;
      this->CurrentProp->Register(this);
      reinterpret_cast<vtkBalloonRepresentation*>(this->WidgetRep)->
        SetBalloonText((*iter).second.Text);
      reinterpret_cast<vtkBalloonRepresentation*>(this->WidgetRep)->
        SetBalloonImage((*iter).second.Image);
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
