/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Object.cc
  Language:  C++
  Date:      30 Jun 1995
  Version:   1.19

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

#include "Object.hh"

// Description:
// Operator allows all subclasses of vtkObject to be printed via <<.
ostream& operator<<(ostream& os, vtkObject& o)
{
  o.Print(os);
  return os;
}

vtkObject::vtkObject()
{
  this->Debug = 0;
  this->Modified(); // Insures modified time > than any other time
}

vtkObject::~vtkObject() 
{
  vtkDebugMacro(<< "Destructing!");
}

// Description:
// Return the modification for this object.
unsigned long int vtkObject::GetMTime() 
{
  return this->MTime.GetMTime();
}

void vtkObject::Print(ostream& os)
{
  vtkIndent indent;

  this->PrintHeader(os,0); 
  this->PrintSelf(os, indent.GetNextIndent());
  this->PrintTrailer(os,0);
}

void vtkObject::PrintHeader(ostream& os, vtkIndent indent)
{
  os << indent << this->GetClassName() << " (" << this << ")\n";
}

// Description:
// Chaining method to print object instance variables as well as
// superclasses.
void vtkObject::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Debug: " << (this->Debug ? "On\n" : "Off\n");
  os << indent << "Modified Time: " << this->GetMTime() << "\n";
}

void vtkObject::PrintTrailer(ostream& os, vtkIndent indent)
{
  os << indent << "\n";
}

// Description:
// Turn debug printout on.
void vtkObject::DebugOn()
{
  this->Debug = 1;
}

// Description:
// Turn debug printout off.
void vtkObject::DebugOff()
{
  this->Debug = 0;
}

// Description:
// Get the value of the debug flag.
int vtkObject::GetDebug()
{
  return this->Debug;
}
