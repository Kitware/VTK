/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkObject.h"

// Initialize static member that controls warning display
static int vtkObjectGlobalWarningDisplay = 1;


// avoid dll boundary problems
#ifdef _WIN32
void* vtkObject::operator new(size_t nSize, const char *, int)
{
void* p=malloc(nSize);
return p;
}

void* vtkObject::operator new(size_t nSize)
{
void* p=malloc(nSize);
return p;
}

void vtkObject::operator delete( void *p )
{
free(p);
}
#endif 

void vtkObject::SetGlobalWarningDisplay(int val)
{
  vtkObjectGlobalWarningDisplay = val;
}

int vtkObject::GetGlobalWarningDisplay()
{
  return vtkObjectGlobalWarningDisplay;
}

// This operator allows all subclasses of vtkObject to be printed via <<.
// It in turn invokes the Print method, which in turn will invoke the
// PrintSelf method that all objects should define, if they have anything
// interesting to print out.
ostream& operator<<(ostream& os, vtkObject& o)
{
  o.Print(os);
  return os;
}

// Create an object with Debug turned off and modified time initialized 
// to zero.
vtkObject::vtkObject()
{
  this->Debug = 0;
  this->Modified(); // Insures modified time > than any other time
  // initial reference count = 1 and reference counting on.
  this->ReferenceCount = 1;
  this->ReferenceCounting = 1;
}

// Delete a vtk object. This method should always be used to delete an object 
// when the new operator was used to create it. Using the C++ delete method
// will not work with reference counting.
void vtkObject::Delete() 
{
  this->UnRegister((vtkObject *)NULL);
}

vtkObject::~vtkObject() 
{
  vtkDebugMacro(<< "Destructing!");

  // warn user if reference counting is on and the object is being referenced
  // by another object
  if ( this->ReferenceCount > 0 && this->ReferenceCounting )
    {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count.");
    }
}

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

// Chaining method to print an object's instance variables, as well as
// its superclasses.
void vtkObject::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Debug: " << (this->Debug ? "On\n" : "Off\n");
  os << indent << "Modified Time: " << this->GetMTime() << "\n";
  os << indent << "Reference Count: " << this->ReferenceCount << "\n";
  os << indent << "Reference Counting: "<< (this->ReferenceCounting ? "On\n" : "Off\n");

}

void vtkObject::PrintTrailer(ostream& os, vtkIndent indent)
{
  os << indent << "\n";
}

// Turn debugging output on.
void vtkObject::DebugOn()
{
  this->Debug = 1;
}

// Turn debugging output off.
void vtkObject::DebugOff()
{
  this->Debug = 0;
}

// Get the value of the debug flag.
unsigned char vtkObject::GetDebug()
{
  return this->Debug;
}

// Set the value of the debug flag. A non-zero value turns debugging on.
void vtkObject::SetDebug(unsigned char debugFlag)
{
  this->Debug = debugFlag;
}


// This method is called when vtkErrorMacro executes. It allows 
// the debugger to break on error.
void vtkObject::BreakOnError()
{
}



// Description:
// Sets the reference count (use with care)
void vtkObject::SetReferenceCount(int ref)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to Register an object which has reference counting turned off.");
    }
  this->ReferenceCount = ref;
  vtkDebugMacro(<< "Reference Count set to " << this->ReferenceCount);
}

// Description:
// Increase the reference count (mark as used by another object).
void vtkObject::Register(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to Register an object which has reference counting turned off.");
    }
  this->ReferenceCount++;
  vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o 
    << "), ReferenceCount = " << this->ReferenceCount);

  if (this->ReferenceCount <= 0)
    {
    delete this;
    }
}

// Description:
// Decrease the reference count (release by another object).
void vtkObject::UnRegister(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to UnRegister an object which has reference counting turned off.");
    }

  if (o)
    {
    vtkDebugMacro(<< "UnRegistered by " 
      << o->GetClassName() << " (" << o << "), ReferenceCount = "
      << (this->ReferenceCount-1));
    }
  else
    {
    vtkDebugMacro(<< "UnRegistered by NULL, ReferenceCount = "
     << (this->ReferenceCount-1));
    }

  if (--this->ReferenceCount <= 0)
    {
    delete this;
    }
}

