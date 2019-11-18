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

#ifdef VTK_DEBUG_LEAKS
static const char* leakname = "vtkCommand or subclass";
#endif

//----------------------------------------------------------------
vtkCommand::vtkCommand()
  : AbortFlag(0)
  , PassiveObserver(0)
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass(leakname);
#endif
}

//----------------------------------------------------------------
void vtkCommand::UnRegister()
{
  int refcount = this->GetReferenceCount() - 1;
  this->SetReferenceCount(refcount);
  if (refcount <= 0)
  {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass(leakname);
#endif
    delete this;
  }
}

//----------------------------------------------------------------
const char* vtkCommand::GetStringFromEventId(unsigned long event)
{
  switch (event)
  {
// clang-format off
#define _vtk_add_event(Enum)                                                                       \
  case Enum:                                                                                       \
    return #Enum;

  vtkAllEventsMacro()

#undef _vtk_add_event
      // clang-format on

      case UserEvent : return "UserEvent";

    case NoEvent:
      return "NoEvent";
  }

  // Unknown event. Original code was returning NoEvent, so I'll stick with
  // that.
  return "NoEvent";
}

//----------------------------------------------------------------
unsigned long vtkCommand::GetEventIdFromString(const char* event)
{
  if (event)
  {

// clang-format off
#define _vtk_add_event(Enum)                                                                       \
  if (strcmp(event, #Enum) == 0)                                                                   \
  {                                                                                                \
    return Enum;                                                                                   \
  }

    vtkAllEventsMacro()

#undef _vtk_add_event

    if (strcmp("UserEvent",event) == 0)
    {
      return vtkCommand::UserEvent;
    }
    // clang-format on
  }

  return vtkCommand::NoEvent;
}

bool vtkCommand::EventHasData(unsigned long event)
{
  switch (event)
  {
    case vtkCommand::Button3DEvent:
    case vtkCommand::Move3DEvent:
      return true;
    default:
      return false;
  }
}
