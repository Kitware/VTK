/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkObject.h"
#include "vtkDebugLeaks.h"
#include "vtkCommand.h"

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
  this->ReferenceCount = 1;
  this->SubjectHelper = NULL;
  this->Modified(); // Insures modified time > than any other time
  // initial reference count = 1 and reference counting on.
}

// Delete a vtk object. This method should always be used to delete an object 
// when the new operator was used to create it. Using the C++ delete method
// will not work with reference counting.
void vtkObject::Delete() 
{
  this->UnRegister((vtkObject *)NULL);
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
  this->ReferenceCount = ref;
  vtkDebugMacro(<< "Reference Count set to " << this->ReferenceCount);
}

// Description:
// Increase the reference count (mark as used by another object).
void vtkObject::Register(vtkObject* o)
{
  this->ReferenceCount++;
  if ( o )
    {
    vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o 
                  << "), ReferenceCount = " << this->ReferenceCount);
    }
  else
    {
    vtkDebugMacro(<< "Registered by NULL, ReferenceCount = " 
                  << this->ReferenceCount);
    }               
  if (this->ReferenceCount <= 0)
    {
    delete this;
    }
}

// Description:
// Decrease the reference count (release by another object).
void vtkObject::UnRegister(vtkObject* o)
{
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
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass(this->GetClassName());
#endif
    // invoke the delete method
    this->InvokeEvent(vtkCommand::DeleteEvent,NULL);
    delete this;
    }
}

int vtkObject::IsTypeOf(const char *name) 
{
  if ( !strcmp("vtkObject",name) )
    {
    return 1;
    }
  return 0;
}

int vtkObject::IsA(const char *type)
{
  return this->vtkObject::IsTypeOf(type);
}

vtkObject *vtkObject::SafeDownCast(vtkObject *o)
{
  return (vtkObject *)o;
}


//
// define the SubjectHelper class and implement the Add/Remove methods
//
class vtkObserver
{
 public:
  vtkObserver()
    {this->Event = 0; this->Next = NULL; this->Command = NULL; this->Tag = 0;};
  ~vtkObserver();

  vtkCommand *Command;
  unsigned long Event;
  unsigned long Tag;
  vtkObserver *Next;
};

vtkObserver::~vtkObserver()
{
  delete this->Command;
}

class vtkSubjectHelper
{
public:
  vtkSubjectHelper() {this->Start = NULL; this->End = NULL; this->Count = 1;};
  ~vtkSubjectHelper();
  
  unsigned long AddObserver(unsigned long event, vtkCommand *cmd);
  void RemoveObserver(unsigned long tag);
  void InvokeEvent(unsigned long event, void *callData, vtkObject *self);
  vtkCommand *GetCommand(unsigned long tag);
  int HasObserver(unsigned long event);
  
protected:
  vtkObserver *Start;
  vtkObserver *End;
  unsigned long Count;
};

vtkSubjectHelper::~vtkSubjectHelper()
{
  vtkObserver *elem = this->Start;
  vtkObserver *next;
  while (elem)
    {
    next = elem->Next;
    delete elem;
    elem = next;
    }
  this->Start = NULL;
  this->End = NULL;
}


unsigned long vtkSubjectHelper::
AddObserver(unsigned long event, vtkCommand *cmd)
{
  vtkObserver *elem;

  elem = new vtkObserver;
  
  if (!this->Start)
    {
    this->Start = elem;
    }
  else
    {
    this->End->Next = elem;
    }
  this->End = elem;

  elem->Event = event;
  elem->Command = cmd;
  elem->Next = NULL;
  elem->Tag = this->Count;
  this->Count++;
  return elem->Tag;
}

void vtkSubjectHelper::RemoveObserver(unsigned long tag)
{
  vtkObserver *elem;
  vtkObserver *prev;
  vtkObserver *next;
  
  elem = this->Start;
  prev = NULL;
  while (elem)
    {
    if (elem->Tag == tag)
      {
      if (prev)
        {
        prev->Next = elem->Next;
        next = prev->Next;
        }
      else
        {
        this->Start = elem->Next;
        next = this->Start;
        }
      if (elem == this->End)
        {
        this->End = prev;
        }
      delete elem;
      elem = next;
      }
    else
      {
      prev = elem;
      elem = elem->Next;
      }
    }
}

int vtkSubjectHelper::HasObserver(unsigned long event)
{
  vtkObserver *elem = this->Start;
  while (elem)
    {
    if (elem->Event == event || elem->Event == vtkCommand::AnyEvent)
      {
      return 1;
      }
    elem = elem->Next;
    }  
  return 0;
}

void vtkSubjectHelper::InvokeEvent(unsigned long event, void *callData,
                                   vtkObject *self)
{
  vtkObserver *elem = this->Start;
  vtkObserver *next;
  while (elem)
    {
    // store the next pointer because elem could disappear due to Command
    next = elem->Next;
    if (elem->Event == event || elem->Event == vtkCommand::AnyEvent)
      {
      elem->Command->Execute(self,event,callData);
      }
    elem = next;
    }  
}

vtkCommand *vtkSubjectHelper::GetCommand(unsigned long tag)
{
  vtkObserver *elem = this->Start;
  while (elem)
    {
    if (elem->Tag == tag)
      {
      return elem->Command;
      }
    elem = elem->Next;
    }  
  return NULL;
}

unsigned long vtkObject::AddObserver(unsigned long event, vtkCommand *cmd)
{
  if (!this->SubjectHelper)
    {
    this->SubjectHelper = new vtkSubjectHelper;
    }
  return this->SubjectHelper->AddObserver(event,cmd);
}

unsigned long vtkObject::AddObserver(const char *event,vtkCommand *cmd)
{
  return this->AddObserver(vtkCommand::GetEventIdFromString(event), cmd);
}

vtkCommand *vtkObject::GetCommand(unsigned long tag)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->GetCommand(tag);
    }
  return NULL;
}

void vtkObject::RemoveObserver(unsigned long tag)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->RemoveObserver(tag);
    }
}

void vtkObject::InvokeEvent(unsigned long event, void *callData)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->InvokeEvent(event,callData, this);
    }
}

void vtkObject::InvokeEvent(const char *event, void *callData)
{
  this->InvokeEvent(vtkCommand::GetEventIdFromString(event), callData);
}

int vtkObject::HasObserver(unsigned long event)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->HasObserver(event);
    }
  return 0;
}

int vtkObject::HasObserver(const char *event)
{
  return this->HasObserver(vtkCommand::GetEventIdFromString(event));
}

vtkObject::~vtkObject() 
{
  vtkDebugMacro(<< "Destructing!");

  // warn user if reference counting is on and the object is being referenced
  // by another object
  if ( this->ReferenceCount > 0)
    {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count.");
    }
  delete this->SubjectHelper;
  this->SubjectHelper = NULL;
}
