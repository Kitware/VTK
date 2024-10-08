// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEvent.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEvent);

vtkEvent::vtkEvent()
{
  this->Modifier = vtkEvent::AnyModifier;
  this->KeyCode = 0;
  this->RepeatCount = 0;
  this->KeySym = nullptr;
  this->EventId = vtkCommand::NoEvent;
}

vtkEvent::~vtkEvent()
{
  delete[] this->KeySym;
}

// Comparison against event with no modifiers
bool vtkEvent::operator==(unsigned long VTKEvent) const
{
  return this->EventId == VTKEvent;
}

// Comparison against event with modifiers
bool vtkEvent::operator==(vtkEvent* e) const
{
  if (this->EventId != e->EventId)
  {
    return false;
  }
  if (this->Modifier != vtkEvent::AnyModifier && e->Modifier != vtkEvent::AnyModifier &&
    this->Modifier != e->Modifier)
  {
    return false;
  }
  if (this->KeyCode != '\0' && e->KeyCode != '\0' && this->KeyCode != e->KeyCode)
  {
    return false;
  }
  if (this->RepeatCount != 0 && e->RepeatCount != 0 && this->RepeatCount != e->RepeatCount)
  {
    return false;
  }
  if (this->KeySym != nullptr && e->KeySym != nullptr && strcmp(this->KeySym, e->KeySym) != 0)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkEvent::GetModifier(vtkRenderWindowInteractor* i)
{
  int modifier = 0;
  modifier |= (i->GetShiftKey() ? vtkEvent::ShiftModifier : 0);
  modifier |= (i->GetControlKey() ? vtkEvent::ControlModifier : 0);
  modifier |= (i->GetAltKey() ? vtkEvent::AltModifier : 0);

  return modifier;
}

//------------------------------------------------------------------------------
void vtkEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  // List all the events and their translations
  os << indent << "Event Id: " << this->EventId << "\n";

  os << indent << "Modifier: ";
  if (this->Modifier == -1)
  {
    os << "Any\n";
  }
  else if (this->Modifier == 0)
  {
    os << "None\n";
  }
  else
  {
    os << this->Modifier << "\n";
  }

  os << indent << "Key Code: ";
  if (this->KeyCode == 0)
  {
    os << "Any\n";
  }
  else
  {
    os << this->KeyCode << "\n";
  }

  os << indent << "Repeat Count: ";
  if (this->RepeatCount == 0)
  {
    os << "Any\n";
  }
  else
  {
    os << this->RepeatCount << "\n";
  }

  os << indent << "Key Sym: ";
  if (this->KeySym == nullptr)
  {
    os << "Any\n";
  }
  else
  {
    os << this->KeySym << "\n";
  }
}
VTK_ABI_NAMESPACE_END
