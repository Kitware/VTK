/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetCallbackMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkAbstractWidget.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include <map>

vtkStandardNewMacro(vtkWidgetCallbackMapper);


// Callbacks are stored as a pair of (Object,Method) in the map.
struct vtkCallbackPair
{
  vtkCallbackPair():Widget(0),Callback(0) {} //map requires empty constructor
  vtkCallbackPair(vtkAbstractWidget *w, vtkWidgetCallbackMapper::CallbackType f) :
    Widget(w),Callback(f) {}

  vtkAbstractWidget *Widget;
  vtkWidgetCallbackMapper::CallbackType Callback;
};


// The map tracks the correspondance between widget events and callbacks
class vtkCallbackMap : public std::map<unsigned long, vtkCallbackPair>
{
public:
  typedef vtkCallbackMap CallbackMapType;
  typedef std::map<unsigned long, vtkCallbackPair >::iterator CallbackMapIterator;
};    


//----------------------------------------------------------------------------
vtkWidgetCallbackMapper::vtkWidgetCallbackMapper()
{
  this->CallbackMap = new vtkCallbackMap;
  this->EventTranslator = NULL;
}

//----------------------------------------------------------------------------
vtkWidgetCallbackMapper::~vtkWidgetCallbackMapper()
{
  delete this->CallbackMap;
  if ( this->EventTranslator )
    {
    this->EventTranslator->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::SetEventTranslator(vtkWidgetEventTranslator *t)
{
  if ( this->EventTranslator != t )
    {
    if (this->EventTranslator)
      {
      this->EventTranslator->Delete();
      }
    this->EventTranslator = t;
    if (this->EventTranslator)
      {
      this->EventTranslator->Register(this);
      }
    
    this->Modified();
    }
}


//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::SetCallbackMethod(unsigned long VTKEvent, 
                                                 unsigned long widgetEvent, 
                                                 vtkAbstractWidget *w, 
                                                 CallbackType f)
{
  this->EventTranslator->SetTranslation(VTKEvent,widgetEvent);
  this->SetCallbackMethod(widgetEvent,w,f);
}


//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::SetCallbackMethod(unsigned long VTKEvent, 
                                                 int modifier, char keyCode, 
                                                 int repeatCount, const char* keySym,
                                                 unsigned long widgetEvent, 
                                                 vtkAbstractWidget *w, CallbackType f)
{
  this->EventTranslator->SetTranslation(VTKEvent,modifier,keyCode,repeatCount,keySym,
                                        widgetEvent);
  this->SetCallbackMethod(widgetEvent,w,f);
}


//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::SetCallbackMethod(unsigned long widgetEvent,
                                                 vtkAbstractWidget *w, CallbackType f)
{
  (*this->CallbackMap)[widgetEvent] = vtkCallbackPair(w,f);
}

//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::InvokeCallback(unsigned long widgetEvent)
{
  vtkCallbackMap::CallbackMapIterator iter = this->CallbackMap->find(widgetEvent);
  if ( iter != this->CallbackMap->end() )
    {
    vtkAbstractWidget *w = (*iter).second.Widget;
    CallbackType f = (*iter).second.Callback;
    (*f)(w);
    }
}

//----------------------------------------------------------------------------
void vtkWidgetCallbackMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Event Translator: ";
  if ( this->EventTranslator )
    {
    os << this->EventTranslator << "\n";
    }
  else
    {
    os << "(none)\n";
    }
}
