/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <string.h>
#include <ctype.h>
#include "vtkCommand.h"

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
  "ConfigureEvent",
  "TimerEvent",
  "MouseMoveEvent",
  "ResetCameraEvent",
  "ResetCameraClippingRangeEvent",
  "ModifiedEvent",
  "WindowLevelEvent",
  NULL
};

//----------------------------------------------------------------
vtkCommand *vtkCommand::New()
{ 
  return new vtkCallbackCommand; 
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

//----------------------------------------------------------------
vtkCallbackCommand::vtkCallbackCommand() 
{ 
  this->ClientData = NULL;
  this->Callback = NULL; 
  this->ClientDataDeleteCallback = NULL;
}

vtkCallbackCommand::~vtkCallbackCommand() 
{ 
  if (this->ClientDataDeleteCallback)
    {
    this->ClientDataDeleteCallback(this->ClientData);
    }
}
  
void vtkCallbackCommand::Execute(vtkObject *caller, unsigned long event, 
				 void *callData)
{
  if (this->Callback)
    {
    this->Callback(caller, event, this->ClientData, callData);
    }
}
  
//----------------------------------------------------------------
vtkOldStyleCallbackCommand::vtkOldStyleCallbackCommand() 
{ 
  this->ClientData = NULL;
  this->Callback = NULL; 
  this->ClientDataDeleteCallback = NULL;
}
  
vtkOldStyleCallbackCommand::~vtkOldStyleCallbackCommand() 
{ 
  if (this->ClientDataDeleteCallback)
    {
    this->ClientDataDeleteCallback(this->ClientData);
    }
}
 
void vtkOldStyleCallbackCommand::Execute(vtkObject *,unsigned long, void *)
{
  if (this->Callback)
    {
    this->Callback(this->ClientData);
    }
}



