/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRefCount.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkRefCount.hh"

// Description:
// Construct with initial reference count = 1 and reference counting on.
vtkRefCount::vtkRefCount()
{
  this->RefCount = 1;
  this->ReferenceCounting = 1;
}

// Description:
// Overload vtkObject's Delete() method. For reference counted objects the
// Delete() method simply unregisters the use of the object. This may or
// may not result in the destruction of the object, depending upon whether 
// another object is referencing it.
void vtkRefCount::Delete()
{
  this->UnRegister((vtkObject *)NULL);
}

// Description:
// Destructor for reference counted objects. Reference counted objects should 
// almost always use the combination of new/Delete() to create and delete 
// objects. Automatic reference counted objects (i.e., creating them on the 
// stack) are not encouraged. However, if you desire to do this, you will 
// have to use the ReferenceCountingOff() method to avoid warning messages 
// when the objects are automatically deleted upon scope termination.
vtkRefCount::~vtkRefCount() 
{
  // warn user if reference counting is on and the object is being referenced
  // by another object
  if ( this->RefCount > 0 && this->ReferenceCounting )
    {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count");
    }
}

// Description:
// Increase the reference count (mark as used by another object).
void vtkRefCount::Register(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to Register an object which has reference counting turned off.");
    }
  this->RefCount++;
  vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o << ")");
}

// Description:
// Decrease the reference count (release by another object).
void vtkRefCount::UnRegister(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to UnRegister an object which has reference counting turned off.");
    }

  vtkDebugMacro(<< "UnRegistered by " <<o->GetClassName() << " (" << 0 << ")");

  if (--this->RefCount <= 0) delete this;
}

void vtkRefCount::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Reference Count: " << this->RefCount << "\n";
  os << indent << "Reference Counting: "<< (this->ReferenceCounting ? "On\n" : "Off\n");
}
