/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebInteractionEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebInteractionEvent.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkWebInteractionEvent);
//----------------------------------------------------------------------------
vtkWebInteractionEvent::vtkWebInteractionEvent() :
  Buttons(0),
  Modifiers(0),
  KeyCode(0),
  X(0.0),
  Y(0.0),
  Scroll(0.0),
  RepeatCount(0)
{
}

//----------------------------------------------------------------------------
vtkWebInteractionEvent::~vtkWebInteractionEvent()
{
}

//----------------------------------------------------------------------------
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
