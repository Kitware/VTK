// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommand.h"
#include "vtkDebug.h"
#include "vtkDebugLeaks.h"

//----------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkCommand::vtkCommand()
  : AbortFlag(0)
  , PassiveObserver(0)
{
  // This is "too early" to call this because `this->GetClassName()` is not set
  // up for the subclasses yet. Instead, because `vtkCommand` has a public
  // constructor, `GetDebugClassName` is implemented to handle this usage
  // pattern.
  this->InitializeObjectBase();
}

//----------------------------------------------------------------
void vtkCommand::UnRegister()
{
  this->UnRegister(nullptr);
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
    case vtkCommand::ViewerMovement3DEvent:
    case vtkCommand::Menu3DEvent:
    case vtkCommand::NextPose3DEvent:
    case vtkCommand::Clip3DEvent:
    case vtkCommand::PositionProp3DEvent:
    case vtkCommand::Pick3DEvent:
    case vtkCommand::Select3DEvent:
    case vtkCommand::Elevation3DEvent:
      return true;
    default:
      return false;
  }
}

const char* vtkCommand::GetDebugClassName() const
{
  return "vtkCommand or subclass";
}
VTK_ABI_NAMESPACE_END
