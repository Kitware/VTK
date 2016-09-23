/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWidgetEvent.h"
#include "vtkObjectFactory.h"

// this list should only contain the initial, contiguous
// set of events and should not include UserEvent
static const char *vtkWidgetEventStrings[] = {
  "NoEvent",
  "Select",
  "EndSelect",
  "Delete",
  "Translate",
  "EndTranslate",
  "Scale",
  "EndScale",
  "Resize",
  "EndResize",
  "Rotate",
  "EndRotate",
  "Move",
  "SizeHandles",
  "AddPoint",
  "AddFinalPoint",
  "Completed",
  "TimedOut",
  "ModifyEvent",
  "Reset",
  NULL
};

vtkStandardNewMacro(vtkWidgetEvent);

//----------------------------------------------------------------------
const char *vtkWidgetEvent::GetStringFromEventId(unsigned long event)
{
  static unsigned long numevents = 0;

  // find length of table
  if (!numevents)
  {
    while (vtkWidgetEventStrings[numevents] != NULL)
    {
      numevents++;
    }
  }

  if (event < numevents)
  {
    return vtkWidgetEventStrings[event];
  }
  else
  {
    return "NoEvent";
  }
}

//----------------------------------------------------------------------
unsigned long vtkWidgetEvent::GetEventIdFromString(const char *event)
{
  unsigned long i;

  for (i = 0; vtkWidgetEventStrings[i] != NULL; i++)
  {
    if (!strcmp(vtkWidgetEventStrings[i],event))
    {
      return i;
    }
  }
  return vtkWidgetEvent::NoEvent;
}

//----------------------------------------------------------------------
void vtkWidgetEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
