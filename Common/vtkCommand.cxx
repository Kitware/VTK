/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCommand.h"
#include "vtkDebugLeaks.h"

// this list should only contain the initial, contiguous
// set of events and should not include UserEvent
static const char *vtkCommandEventStrings[] = {
  "NoEvent", 
  "AnyEvent",
  "DeleteEvent",
  "StartEvent",
  "EndEvent",
  "ProgressEvent",
  "PickEvent",
  "StartPickEvent",
  "EndPickEvent",
  "AbortCheckEvent",
  "ExitEvent", 
  "LeftButtonPressEvent",
  "LeftButtonReleaseEvent",
  "MiddleButtonPressEvent",
  "MiddleButtonReleaseEvent",
  "RightButtonPressEvent",
  "RightButtonReleaseEvent",
  "EnterEvent",
  "LeaveEvent",
  "KeyPressEvent",
  "KeyReleaseEvent",
  "CharEvent",
  "ExposeEvent",
  "ConfigureEvent",
  "TimerEvent",
  "MouseMoveEvent",
  "ResetCameraEvent",
  "ResetCameraClippingRangeEvent",
  "ModifiedEvent",
  "WindowLevelEvent",
  "StartWindowLevelEvent",
  "EndWindowLevelEvent",
  "ResetWindowLevelEvent",
  "NextDataEvent",
  "PushDataStartEvent",
  "SetOutputEvent",
  "EndOfDataEvent",
  "ErrorEvent",
  "WarningEvent",
  "StartInteractionEvent",
  "InteractionEvent",
  "EndInteractionEvent",
  "EnableEvent",
  "DisableEvent",
  "CreateTimerEvent",
  "DestroyTimerEvent",
  NULL
};

//----------------------------------------------------------------
vtkCommand::vtkCommand():AbortFlag(0)
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

const char *vtkCommand::GetStringFromEventId(unsigned long event)
{
  static unsigned long numevents = 0;
  
  // find length of table
  if (!numevents)
    {
    while (vtkCommandEventStrings[numevents] != NULL)
      {
      numevents++;
      }
    }

  if (event < numevents)
    {
    return vtkCommandEventStrings[event];
    }
  else if (event == vtkCommand::UserEvent)
    {
    return "UserEvent";
    }
  else
    {
    return "NoEvent";
    }
}
  
unsigned long vtkCommand::GetEventIdFromString(const char *event)
{  
  unsigned long i;

  for (i = 0; vtkCommandEventStrings[i] != NULL; i++)
    { 
    if (!strcmp(vtkCommandEventStrings[i],event))
      {
      return i;
      }
    }
  if (!strcmp("UserEvent",event))
    {
    return vtkCommand::UserEvent;
    }
  return vtkCommand::NoEvent;
}

  



