/*=========================================================================

  Program:   Visualization Library
  Module:    Object.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

#include "Object.hh"

// Description:
// Operator allows all subclasses of vlObject to be printed via <<.
ostream& operator<<(ostream& os, vlObject& o)
{
  o.Print(os);
  return os;
}

vlObject::vlObject()
{
  this->Debug = 0;
  this->Modified(); // Insures modified time > than any other time
}

vlObject::~vlObject() 
{
  vlDebugMacro(<< "Destructing!");
}

// Description:
// Return the modification for this object.
unsigned long int vlObject::GetMTime() 
{
  return this->MTime.GetMTime();
}

void vlObject::Print(ostream& os)
{
  vlIndent indent;

  this->PrintHeader(os,0); 
  this->PrintSelf(os, indent.GetNextIndent());
  this->PrintTrailer(os,0);
}

void vlObject::PrintHeader(ostream& os, vlIndent indent)
{
  os << indent << this->GetClassName() << " (" << this << ")\n";
}

// Description:
// Chaining method to print object instance variables as well as
// superclasses.
void vlObject::PrintSelf(ostream& os, vlIndent indent)
{
  os << indent << "Debug: " << (this->Debug ? "On\n" : "Off\n");
  os << indent << "Modified Time: " << this->GetMTime() << "\n";
}

void vlObject::PrintTrailer(ostream& os, vlIndent indent)
{
  os << indent << "\n";
}

// Description:
// Turn debug printout on.
void vlObject::DebugOn()
{
  this->Debug = 1;
}

// Description:
// Turn debug printout off.
void vlObject::DebugOff()
{
  this->Debug = 0;
}

// Description:
// Get the value of the debug flag.
int vlObject::GetDebug()
{
  return this->Debug;
}
