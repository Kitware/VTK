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

// This list should only contain the initial, contiguous
// set of events and should not include UserEvent
static const char *vtkCommandEventStrings[] = {
  "NoEvent", 
  "AnyEvent",
  "DeleteEvent",
  "StartEvent",
  "EndEvent",
  "RenderEvent",
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
  "MouseWheelForwardEvent",
  "MouseWheelBackwardEvent",
  "ActiveCameraEvent",
  "CreateCameraEvent",
  "ResetCameraEvent",
  "ResetCameraClippingRangeEvent",
  "ModifiedEvent",
  "WindowLevelEvent",
  "StartWindowLevelEvent",
  "EndWindowLevelEvent",
  "ResetWindowLevelEvent",
  "SetOutputEvent",
  "ErrorEvent",
  "WarningEvent",
  "StartInteractionEvent",
  "InteractionEvent",
  "EndInteractionEvent",
  "EnableEvent",
  "DisableEvent",
  "CreateTimerEvent",
  "DestroyTimerEvent",
  "PlacePointEvent",
  "PlaceWidgetEvent",
  "CursorChangedEvent",
  "ExecuteInformationEvent",
  "RenderWindowMessageEvent",
  "WrongTagEvent",
  "StartAnimationCueEvent",
  "AnimationCueTickEvent",
  "EndAnimationCueEvent",
  "VolumeMapperRenderEndEvent",
  "VolumeMapperRenderProgressEvent",
  "VolumeMapperRenderStartEvent",
  "VolumeMapperComputeGradientsEndEvent",
  "VolumeMapperComputeGradientsProgressEvent",
  "VolumeMapperComputeGradientsStartEvent",
  "WidgetModifiedEvent",
  "WidgetValueChangedEvent",
  "WidgetActivateEvent",
  "ConnectionCreatedEvent",
  "ConnectionClosedEvent",
  "DomainModifiedEvent",
  "PropertyModifiedEvent",
  "UpdateEvent",
  "RegisterEvent",
  "UnRegisterEvent",
  "UpdateInformationEvent",
  "AnnotationChangedEvent",
  "SelectionChangedEvent",
  "UpdatePropertyEvent",
  "ViewProgressEvent",
  "UpdateDataEvent",
  "CurrentChangedEvent",
  "ComputeVisiblePropBoundsEvent",
  "TDxMotionEvent", // 3D Connexion device event
  "TDxButtonPressEvent", // 3D Connexion device event
  "TDxButtonReleaseEvent", // 3D Connexion device event
  "HoverEvent",
  "LoadStateEvent",
  "SaveStateEvent",
  "StateChangedEvent",
  NULL
};

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
  
//----------------------------------------------------------------
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

  



