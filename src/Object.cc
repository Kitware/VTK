/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Object.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "Object.hh"

// Description:
// Operator allows all subclasses of vtkObject to be printed via <<.
ostream& operator<<(ostream& os, vtkObject& o)
{
  o.Print(os);
  return os;
}

// Description:
// Create an object with Debug turned off and modified time initialized 
// to zero.
vtkObject::vtkObject()
{
  this->Debug = 0;
  this->Modified(); // Insures modified time > than any other time
}

// Description:
// Delete a vtk object. Delete() should always be used to delete an object 
// when the new operator is used.
void vtkObject::Delete() 
{
  delete this;
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
