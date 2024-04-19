// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebInteractionEvent.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebInteractionEvent);
//------------------------------------------------------------------------------
vtkWebInteractionEvent::vtkWebInteractionEvent()
  : Buttons(0)
  , Modifiers(0)
  , KeyCode(0)
  , X(0.0)
  , Y(0.0)
  , Scroll(0.0)
  , RepeatCount(0)
{
}

//------------------------------------------------------------------------------
vtkWebInteractionEvent::~vtkWebInteractionEvent() = default;

//------------------------------------------------------------------------------
void vtkWebInteractionEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Buttons: " << this->Buttons << endl;
  os << indent << "Modifiers: " << this->Modifiers << endl;
  os << indent << "KeyCode: " << static_cast<int>(this->KeyCode) << endl;
  os << indent << "X: " << this->X << endl;
  os << indent << "Y: " << this->Y << endl;
  os << indent << "RepeatCount: " << this->RepeatCount << endl;
  os << indent << "Scroll: " << this->Scroll << endl;
}
VTK_ABI_NAMESPACE_END
