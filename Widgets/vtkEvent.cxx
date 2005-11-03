/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEvent.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkEvent, "1.2");
vtkStandardNewMacro(vtkEvent);


vtkEvent::vtkEvent()
{
  this->Modifier = vtkEvent::AnyModifier;
  this->KeyCode = 0;
  this->RepeatCount = 0;
  this->KeySym = 0;
}

vtkEvent::~vtkEvent()
{
  if ( this->KeySym )
    {
    delete [] this->KeySym;
    }
}

// Comparison against event with no modifiers
int vtkEvent::operator==(unsigned long VTKEvent)
{
  if ( this->EventId == VTKEvent ) 
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

// Comparison against event with modifiers
int vtkEvent::operator==(vtkEvent *e)
{
  if ( this->EventId != e->EventId ) 
    {
    return 0;
    }
  if ( this->Modifier != vtkEvent::AnyModifier &&
       e->Modifier != vtkEvent::AnyModifier &&
       this->Modifier != e->Modifier )
    {
    return 0;
    }
  if ( this->KeyCode != '\0' && e->KeyCode != '\0' && 
       this->KeyCode != e->KeyCode )
    {
    return 0;
    }
  if ( this->RepeatCount != 0 && e->RepeatCount != 0 && 
       this->RepeatCount != e->RepeatCount )
    {
    return 0;
    }
  if ( this->KeySym != NULL && e->KeySym != NULL && 
       !strcmp(this->KeySym,e->KeySym) )
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkEvent::GetModifier(vtkRenderWindowInteractor* i)
{
  int ctrlKey = i->GetControlKey();
  int shiftKey = i->GetShiftKey();
  
  if ( ctrlKey == 0 && shiftKey == 0 )
    {
    return vtkEvent::AnyModifier;
    }
  if ( ctrlKey == 0 && shiftKey == 1 )
    {
    return vtkEvent::ShiftModifier;
    }
  if ( ctrlKey == 1 && shiftKey == 0 )
    {
    return vtkEvent::ControlModifier;
    }
  if ( ctrlKey == 1 && shiftKey == 1 )
    {
    return (vtkEvent::ControlModifier | vtkEvent::ShiftModifier);
    }
  return vtkEvent::AnyModifier;
}


//----------------------------------------------------------------------------
void vtkEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  //List all the events and their translations
  os << indent << "Event Id: " << this->EventId << "\n";

  os << indent << "Modifier: ";
  if ( this->Modifier == -1 )
    {
    os << "Any\n";
    }
  else if ( this->Modifier == 0 )
    {
    os << "None\n";
    }
  else
    {
    os << this->Modifier << "\n";
    }
  
  os << indent << "Key Code: ";
  if ( this->KeyCode == 0 )
    {
    os << "Any\n";
    }
  else
    {
    os << this->KeyCode << "\n";
    }
  
  os << indent << "Repeat Count: ";
  if ( this->RepeatCount == 0 )
    {
    os << "Any\n";
    }
  else
    {
    os << this->RepeatCount << "\n";
    }
  
  os << indent << "Key Sym: ";
  if ( this->KeySym == 0 )
    {
    os << "Any\n";
    }
  else
    {
    os << this->KeySym << "\n";
    }
  
}
