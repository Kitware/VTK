/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObject.h"
#include "vtkDebugLeaks.h"
#include "vtkCommand.h"

// Initialize static member that controls warning display
static int vtkObjectGlobalWarningDisplay = 1;


// avoid dll boundary problems
#ifdef _WIN32
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

//----------------------------------Command/Observer stuff-------------------
//
class vtkObserver
{
 public:
  vtkObserver():Command(0),Event(0),Tag(0),Next(0),Priority(0.0) {}
  ~vtkObserver();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkCommand *Command;
  unsigned long Event;
  unsigned long Tag;
  vtkObserver *Next;
  float Priority;
};

void vtkObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkObserver (" << this << ")\n";
  indent = indent.GetNextIndent();
  os << indent << "Event: " << this->Event << "\n";
  os << indent << "EventName: " << vtkCommand::GetStringFromEventId(this->Event) << "\n";
  os << indent << "Command: " << this->Command << "\n";
  os << indent << "Priority: " << this->Priority << "\n";
  os << indent << "Tag: " << this->Tag << "\n";
}

class vtkSubjectHelper
{
public:
  vtkSubjectHelper():Start(0),Count(1) {}
  ~vtkSubjectHelper();
  
  unsigned long AddObserver(unsigned long event, vtkCommand *cmd, float p);
  void RemoveObserver(unsigned long tag);
  void RemoveObservers(unsigned long event);
  void InvokeEvent(unsigned long event, void *callData, vtkObject *self);
  vtkCommand *GetCommand(unsigned long tag);
  unsigned long GetTag(vtkCommand*);
  int HasObserver(unsigned long event);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkObserver *Start;
  unsigned long Count;
};

// ------------------------------------vtkObject----------------------
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
  if ( this->SubjectHelper )
    {
    this->SubjectHelper->PrintSelf(os,indent);
    }
  else
    {
    os << indent << "Registered Events: (none)\n";
    }
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

void vtkObject::CollectRevisions(ostream& os)
{
  os << "vtkObject 1.75\n";
}

//----------------------------------Command/Observer stuff-------------------
//

vtkObserver::~vtkObserver()
{
  this->Command->UnRegister();
}

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
}


unsigned long vtkSubjectHelper::
AddObserver(unsigned long event, vtkCommand *cmd, float p)
{
  vtkObserver *elem;

  // initialize the new observer element
  elem = new vtkObserver;
  elem->Priority = p;
  elem->Next = NULL;
  elem->Event = event;
  elem->Command = cmd;
  cmd->Register();
  elem->Tag = this->Count;
  this->Count++;

  // now insert into the list
  // if no other elements in the list then this is Start
  if (!this->Start)
    {
    this->Start = elem;
    }
  else
    {
    // insert high priority first
    vtkObserver* prev = 0;
    vtkObserver* pos = this->Start;
    while(pos->Priority >= elem->Priority && pos->Next)
      {
      prev = pos;
      pos = pos->Next;
      }
    // pos is Start and elem should not be start
    if(pos->Priority > elem->Priority)
      {
      pos->Next = elem;
      }
    else
      {
      if(prev)
        {
        prev->Next = elem;
        }
      elem->Next = pos;
      // check to see if the new element is the start
      if(pos == this->Start)
        {
        this->Start = elem;
        }
      }
    }
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

void vtkSubjectHelper::RemoveObservers(unsigned long event)
{
  vtkObserver *elem;
  vtkObserver *prev;
  vtkObserver *next;
  
  elem = this->Start;
  prev = NULL;
  while (elem)
    {
    if (elem->Event == event)
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
      int abort = 0;
      elem->Command->SetAbortFlagPointer(&abort);
      elem->Command->Execute(self,event,callData);
      // if the command set the abort flag, then stop firing events
      // and return
      if(abort)
        {
        return;
        }
      }
    elem = next;
    }  
}

unsigned long vtkSubjectHelper::GetTag(vtkCommand* cmd)
{
  vtkObserver *elem = this->Start;
  while (elem)
    {
    if (elem->Command == cmd)
      {
      return elem->Tag;
      }
    elem = elem->Next;
    }  
  return 0;
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

void vtkSubjectHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Registered Observers:\n";
  indent = indent.GetNextIndent();
  vtkObserver *elem = this->Start;
  if ( !elem )
    {
    os << indent << "(none)\n";
    return;
    }
  
  for ( ; elem; elem=elem->Next )
    {
    elem->PrintSelf(os, indent);
    }  
}

//--------------------------------vtkObject observer-----------------------
unsigned long vtkObject::AddObserver(unsigned long event, vtkCommand *cmd, float p)
{
  if (!this->SubjectHelper)
    {
    this->SubjectHelper = new vtkSubjectHelper;
    }
  return this->SubjectHelper->AddObserver(event,cmd, p);
}

unsigned long vtkObject::AddObserver(const char *event,vtkCommand *cmd, float p)
{
  return this->AddObserver(vtkCommand::GetEventIdFromString(event), cmd, p);
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

void vtkObject::RemoveObserver(vtkCommand* c)
{
  if (this->SubjectHelper)
    {
    unsigned long tag = this->SubjectHelper->GetTag(c);
    while(tag)
      {
      this->SubjectHelper->RemoveObserver(tag);
      tag = this->SubjectHelper->GetTag(c);
      }
    }
}

void vtkObject::RemoveObservers(unsigned long event)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->RemoveObservers(event);
    }
}

void vtkObject::RemoveObservers(const char *event)
{
  this->RemoveObservers(vtkCommand::GetEventIdFromString(event));
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

void vtkObject::Modified()
{
  this->MTime.Modified();
  this->InvokeEvent(vtkCommand::ModifiedEvent,NULL);
}

