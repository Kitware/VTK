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
#include "vtkCommand.h"

vtkStandardNewMacro(vtkEvent);


vtkEvent::vtkEvent()
{
  this->Modifier = vtkEvent::AnyModifier;
  this->KeyCode = 0;
  this->RepeatCount = 0;
  this->KeySym = 0;
  this->EventId = vtkCommand::NoEvent;
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
       strcmp(this->KeySym,e->KeySym) != 0 )
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkEvent::GetModifier(vtkRenderWindowInteractor* i)
{
  int modifier = 0;
  modifier |= (i->GetShiftKey()   ? vtkEvent::ShiftModifier   : 0);
  modifier |= (i->GetControlKey() ? vtkEvent::ControlModifier : 0);
  modifier |= (i->GetAltKey()     ? vtkEvent::AltModifier     : 0);

  return modifier;
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
