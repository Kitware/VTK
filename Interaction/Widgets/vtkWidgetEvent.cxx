// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWidgetEvent.h"
#include "vtkObjectFactory.h"

// this list should only contain the initial, contiguous
// set of events and should not include UserEvent
VTK_ABI_NAMESPACE_BEGIN
static const char* vtkWidgetEventStrings[] = {
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
  "HoverLeave",
  nullptr,
};

vtkStandardNewMacro(vtkWidgetEvent);

//------------------------------------------------------------------------------
const char* vtkWidgetEvent::GetStringFromEventId(unsigned long event)
{
  static unsigned long numevents = 0;

  // find length of table
  if (!numevents)
  {
    while (vtkWidgetEventStrings[numevents] != nullptr)
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

//------------------------------------------------------------------------------
unsigned long vtkWidgetEvent::GetEventIdFromString(const char* event)
{
  unsigned long i;

  for (i = 0; vtkWidgetEventStrings[i] != nullptr; i++)
  {
    if (!strcmp(vtkWidgetEventStrings[i], event))
    {
      return i;
    }
  }
  return vtkWidgetEvent::NoEvent;
}

//------------------------------------------------------------------------------
void vtkWidgetEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
