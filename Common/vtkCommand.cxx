/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommand.h"
#include "vtkDebugLeaks.h"

//----------------------------------------------------------------
vtkCommand::vtkCommand():AbortFlag(0),PassiveObserver(0)
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkCommand or subclass");
#endif
}

//----------------------------------------------------------------
void vtkCommand::UnRegister()
{
  int refcount = this->GetReferenceCount()-1;
  this->SetReferenceCount(refcount);
  if (refcount <= 0)
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkCommand or subclass");
#endif
    delete this;
    }
}

//----------------------------------------------------------------
const char *vtkCommand::GetStringFromEventId(unsigned long event)
{
  switch (event)
    {
#define _vtk_add_event(Enum)\
  case Enum: return #Enum;

  vtkAllEventsMacro()

#undef _vtk_add_event

  case UserEvent:
    return "UserEvent";

  case NoEvent:
    return "NoEvent";
    }

  // Unknown event. Original code was returning NoEvent, so I'll stick with
  // that.
  return "NoEvent";
}
  
//----------------------------------------------------------------
unsigned long vtkCommand::GetEventIdFromString(const char *event)
{  
  if (event)
    {
#define _vtk_add_event(Enum)\
    if (strcmp(event, #Enum) == 0) {return Enum;}
    vtkAllEventsMacro()
#undef _vtk_add_event

    if (strcmp("UserEvent",event) == 0)
      {
      return vtkCommand::UserEvent;
      }
    }

  return vtkCommand::NoEvent;
}

  



