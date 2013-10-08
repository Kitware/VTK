/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObject.h"

#include "vtkCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkTimeStamp.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <vector>

// Initialize static member that controls warning display
static int vtkObjectGlobalWarningDisplay = 1;


//----------------------------------------------------------------------------
// avoid dll boundary problems
#ifdef _WIN32
void* vtkObject::operator new(size_t nSize)
{
  void* p=malloc(nSize);
  return p;
}

//----------------------------------------------------------------------------
void vtkObject::operator delete( void *p )
{
  free(p);
}
#endif

//----------------------------------------------------------------------------
void vtkObject::SetGlobalWarningDisplay(int val)
{
  vtkObjectGlobalWarningDisplay = val;
}

//----------------------------------------------------------------------------
int vtkObject::GetGlobalWarningDisplay()
{
  return vtkObjectGlobalWarningDisplay;
}

//----------------------------------Command/Observer stuff-------------------
// The Command/Observer design pattern is used to invoke and dispatch events.
// The class vtkSubjectHelper keeps a list of observers (which in turn keep
// an instance of vtkCommand) which respond to registered events.
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

//----------------------------------------------------------------------------
// The vtkSubjectHelper keeps a list of observers and dispatches events to them.
// It also invokes the vtkCommands associated with the observers. Currently
// vtkSubjectHelper is an internal class to vtkObject. However, due to requirements
// from the VTK widgets it may be necessary to break out the vtkSubjectHelper at
// some point (for reasons of event management, etc.)
class vtkSubjectHelper
{
public:
  vtkSubjectHelper():ListModified(0),Focus1(0),Focus2(0),Start(0),Count(1) {}
  ~vtkSubjectHelper();

  unsigned long AddObserver(unsigned long event, vtkCommand *cmd, float p);
  void RemoveObserver(unsigned long tag);
  void RemoveObservers(unsigned long event);
  void RemoveObservers(unsigned long event, vtkCommand *cmd);
  void RemoveAllObservers();
  int InvokeEvent(unsigned long event, void *callData, vtkObject *self);
  vtkCommand *GetCommand(unsigned long tag);
  unsigned long GetTag(vtkCommand*);
  int HasObserver(unsigned long event);
  int HasObserver(unsigned long event, vtkCommand *cmd);
  void GrabFocus(vtkCommand *c1, vtkCommand *c2) {this->Focus1 = c1; this->Focus2 = c2;}
  void ReleaseFocus() {this->Focus1 = NULL; this->Focus2 = NULL;}
  void PrintSelf(ostream& os, vtkIndent indent);

  int         ListModified;

  // This is to support the GrabFocus() methods found in vtkInteractorObserver.
  vtkCommand *Focus1;
  vtkCommand *Focus2;

protected:
  vtkObserver  *Start;
  unsigned long Count;
};

// ------------------------------------vtkObject----------------------

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkObject);

vtkObject *vtkObject::New()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkObject");
#endif
  return new vtkObject;
}


//----------------------------------------------------------------------------
// Create an object with Debug turned off and modified time initialized
// to zero.
vtkObject::vtkObject()
{
  this->Debug = 0;
  this->SubjectHelper = NULL;
  this->Modified(); // Insures modified time > than any other time
  // initial reference count = 1 and reference counting on.
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Return the modification for this object.
unsigned long int vtkObject::GetMTime()
{
  return this->MTime.GetMTime();
}

// Chaining method to print an object's instance variables, as well as
// its superclasses.
void vtkObject::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Debug: " << (this->Debug ? "On\n" : "Off\n");
  os << indent << "Modified Time: " << this->GetMTime() << "\n";
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Registered Events: ";
  if ( this->SubjectHelper )
    {
    os << endl;
    this->SubjectHelper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}

//----------------------------------------------------------------------------
// Turn debugging output on.
// The Modified() method is purposely not called since we do not want to affect
// the modification time when enabling debug output.
void vtkObject::DebugOn()
{
  this->Debug = 1;
}

//----------------------------------------------------------------------------
// Turn debugging output off.
void vtkObject::DebugOff()
{
  this->Debug = 0;
}

//----------------------------------------------------------------------------
// Get the value of the debug flag.
unsigned char vtkObject::GetDebug()
{
  return this->Debug;
}

//----------------------------------------------------------------------------
// Set the value of the debug flag. A non-zero value turns debugging on.
void vtkObject::SetDebug(unsigned char debugFlag)
{
  this->Debug = debugFlag;
}


//----------------------------------------------------------------------------
// This method is called when vtkErrorMacro executes. It allows
// the debugger to break on error.
void vtkObject::BreakOnError()
{
}

//----------------------------------Command/Observer stuff-------------------
//

//----------------------------------------------------------------------------
vtkObserver::~vtkObserver()
{
  this->Command->UnRegister(0);
}

//----------------------------------------------------------------------------
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
  this->Focus1 = NULL;
  this->Focus2 = NULL;
}


//----------------------------------------------------------------------------
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
  cmd->Register(0);
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

//----------------------------------------------------------------------------
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

  this->ListModified = 1;
}

//----------------------------------------------------------------------------
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

  this->ListModified = 1;
}

//----------------------------------------------------------------------------
void vtkSubjectHelper::RemoveObservers(unsigned long event, vtkCommand *cmd)
{
  vtkObserver *elem;
  vtkObserver *prev;
  vtkObserver *next;

  elem = this->Start;
  prev = NULL;
  while (elem)
    {
    if (elem->Event == event && elem->Command == cmd)
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

  this->ListModified = 1;
}

//----------------------------------------------------------------------------
void vtkSubjectHelper::RemoveAllObservers()
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkSubjectHelper::HasObserver(unsigned long event, vtkCommand *cmd)
{
  vtkObserver *elem = this->Start;
  while (elem)
    {
    if ((elem->Event == event || elem->Event == vtkCommand::AnyEvent) &&
        elem->Command == cmd)
      {
      return 1;
      }
    elem = elem->Next;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkSubjectHelper::InvokeEvent(unsigned long event, void *callData,
                                   vtkObject *self)
{
  int focusHandled = 0;

  // When we invoke an event, the observer may add or remove observers.  To make
  // sure that the iteration over the observers goes smoothly, we capture any
  // change to the list with the ListModified ivar.  However, an observer may
  // also do something that causes another event to be invoked in this object.
  // That means that this method will be called recursively, which means that we
  // will obliterate the ListModified flag that the first call is relying on.
  // To get around this, save the previous ListModified value on the stack and
  // then restore it before leaving.
  int saveListModified = this->ListModified;
  this->ListModified = 0;

  // We also need to save what observers we have called on the stack (lest it
  // get overridden in the event invocation).  Also make sure that we do not
  // invoke any new observers that were added during another observer's
  // invocation.
  typedef std::vector<unsigned long> VisitedListType;
  VisitedListType visited;
  vtkObserver *elem = this->Start;
  // If an element with a tag greater than maxTag is found, that means it has
  // been added after InvokeEvent is called (as a side effect of calling an
  // element command. In that case, the element is discarded and not executed.
  const unsigned long maxTag = this->Count;

  // Loop two or three times, giving preference to passive observers
  // and focus holders, if any.
  //
  // 0. Passive observer loop
  //   Loop over all observers and execute those that are passive observers.
  //   These observers should not affect the state of the system in any way,
  //   and should not be allowed to abort the event.
  //
  // 1. Focus loop
  //   If there is a focus holder, loop over all observers and execute
  //   those associated with either focus holder. Set focusHandled to
  //   indicate that a focus holder handled the event.
  //
  // 2. Remainder loop
  //   If no focus holder handled the event already, loop over the
  //   remaining observers. This loop will always get executed when there
  //   is no focus holder.

  // 0. Passive observer loop
  //
  vtkObserver *next;
  while (elem)
    {
    // store the next pointer because elem could disappear due to Command
    next = elem->Next;
    if (elem->Command->GetPassiveObserver() &&
        (elem->Event == event || elem->Event == vtkCommand::AnyEvent) &&
        elem->Tag < maxTag)
      {
      VisitedListType::iterator vIter =
        std::lower_bound(visited.begin(), visited.end(), elem->Tag);
      if (vIter == visited.end() || *vIter != elem->Tag)
        {
        // Sorted insertion by tag to speed-up future searches at limited
        // insertion cost because it reuses the search iterator already at the
        // correct location
        visited.insert(vIter, elem->Tag);
        vtkCommand* command = elem->Command;
        command->Register(command);
        elem->Command->Execute(self,event,callData);
        command->UnRegister();
        }
      }
    if (this->ListModified)
      {
      vtkGenericWarningMacro(<<"Passive observer should not call AddObserver or RemoveObserver in callback.");
      elem = this->Start;
      this->ListModified = 0;
      }
    else
      {
      elem = next;
      }
    }

  // 1. Focus loop
  //
  if (this->Focus1 || this->Focus2)
    {
    elem = this->Start;
    while (elem)
      {
      // store the next pointer because elem could disappear due to Command
      next = elem->Next;
      if (((this->Focus1 == elem->Command) || (this->Focus2 == elem->Command)) &&
          (elem->Event == event || elem->Event == vtkCommand::AnyEvent) &&
          elem->Tag < maxTag)
        {
        VisitedListType::iterator vIter =
          std::lower_bound(visited.begin(), visited.end(), elem->Tag);
        if (vIter == visited.end() || *vIter != elem->Tag)
          {
          // Don't execute the remainder loop
          focusHandled = 1;
          // Sorted insertion by tag to speed-up future searches at limited
          // insertion cost because it reuses the search iterator already at the
          // correct location
          visited.insert(vIter, elem->Tag);
          vtkCommand* command = elem->Command;
          command->Register(command);
          command->SetAbortFlag(0);
          elem->Command->Execute(self,event,callData);
          // if the command set the abort flag, then stop firing events
          // and return
          if(command->GetAbortFlag())
            {
            command->UnRegister();
            this->ListModified = saveListModified;
            return 1;
            }
          command->UnRegister();
          }
        }
      if (this->ListModified)
        {
        elem = this->Start;
        this->ListModified = 0;
        }
      else
        {
        elem = next;
        }
      }
    }

  // 2. Remainder loop
  //
  if (!focusHandled)
    {
    elem = this->Start;
    while (elem)
      {
      // store the next pointer because elem could disappear due to Command
      next = elem->Next;
      if ((elem->Event == event || elem->Event == vtkCommand::AnyEvent) &&
          elem->Tag < maxTag)
        {
        VisitedListType::iterator vIter =
          std::lower_bound(visited.begin(), visited.end(), elem->Tag);
        if (vIter == visited.end() || *vIter != elem->Tag)
          {
          // Sorted insertion by tag to speed-up future searches at limited
          // insertion cost because it reuses the search iterator already at the
          // correct location
          visited.insert(vIter, elem->Tag);
          vtkCommand* command = elem->Command;
          command->Register(command);
          command->SetAbortFlag(0);
          elem->Command->Execute(self,event,callData);
          // if the command set the abort flag, then stop firing events
          // and return
          if(command->GetAbortFlag())
            {
            command->UnRegister();
            this->ListModified = saveListModified;
            return 1;
            }
          command->UnRegister();
          }
        }
      if (this->ListModified)
        {
        elem = this->Start;
        this->ListModified = 0;
        }
      else
        {
        elem = next;
        }
      }
    }

  this->ListModified = saveListModified;
  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
unsigned long vtkObject::AddObserver(const char *event,vtkCommand *cmd, float p)
{
  return this->AddObserver(vtkCommand::GetEventIdFromString(event), cmd, p);
}

//----------------------------------------------------------------------------
vtkCommand *vtkObject::GetCommand(unsigned long tag)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->GetCommand(tag);
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkObject::RemoveObserver(unsigned long tag)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->RemoveObserver(tag);
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkObject::RemoveObservers(unsigned long event)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->RemoveObservers(event);
    }
}

//----------------------------------------------------------------------------
void vtkObject::RemoveObservers(const char *event)
{
  this->RemoveObservers(vtkCommand::GetEventIdFromString(event));
}

//----------------------------------------------------------------------------
void vtkObject::RemoveObservers(unsigned long event, vtkCommand *cmd)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->RemoveObservers(event, cmd);
    }
}

//----------------------------------------------------------------------------
void vtkObject::RemoveObservers(const char *event, vtkCommand *cmd)
{
  this->RemoveObservers(vtkCommand::GetEventIdFromString(event), cmd);
}

//----------------------------------------------------------------------------
void vtkObject::RemoveAllObservers()
{
  if ( this->SubjectHelper )
    {
    this->SubjectHelper->RemoveAllObservers();
    }
}

//----------------------------------------------------------------------------
int vtkObject::InvokeEvent(unsigned long event, void *callData)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->InvokeEvent(event,callData, this);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkObject::InvokeEvent(const char *event, void *callData)
{
  return this->InvokeEvent(vtkCommand::GetEventIdFromString(event), callData);
}

//----------------------------------------------------------------------------
int vtkObject::HasObserver(unsigned long event)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->HasObserver(event);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkObject::HasObserver(const char *event)
{
  return this->HasObserver(vtkCommand::GetEventIdFromString(event));
}

//----------------------------------------------------------------------------
int vtkObject::HasObserver(unsigned long event, vtkCommand *cmd)
{
  if (this->SubjectHelper)
    {
    return this->SubjectHelper->HasObserver(event, cmd);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkObject::HasObserver(const char *event, vtkCommand *cmd)
{
  return this->HasObserver(vtkCommand::GetEventIdFromString(event), cmd);
}

//----------------------------------------------------------------------------
void vtkObject::InternalGrabFocus(vtkCommand *mouseEvents, vtkCommand *keypressEvents)
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->GrabFocus(mouseEvents,keypressEvents);
    }
}

//----------------------------------------------------------------------------
void vtkObject::InternalReleaseFocus()
{
  if (this->SubjectHelper)
    {
    this->SubjectHelper->ReleaseFocus();
    }
}


//----------------------------------------------------------------------------
void vtkObject::Modified()
{
  this->MTime.Modified();
  this->InvokeEvent(vtkCommand::ModifiedEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkObject::RegisterInternal(vtkObjectBase* o, int check)
{
  // Print debugging messages.
  if(o)
    {
    vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o
                  << "), ReferenceCount = " << this->ReferenceCount+1);
    }
  else
    {
    vtkDebugMacro(<< "Registered by NULL, ReferenceCount = "
                  << this->ReferenceCount+1);
    }

  // Increment the reference count.
  this->Superclass::RegisterInternal(o, check);
}

//----------------------------------------------------------------------------
void vtkObject::UnRegisterInternal(vtkObjectBase* o, int check)
{
  // Print debugging messages.
  if(o)
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

  if(this->ReferenceCount == 1)
    {
    // The reference count is 1, so the object is about to be deleted.
    // Invoke the delete event.
    this->InvokeEvent(vtkCommand::DeleteEvent, 0);

    // Clean out observers prior to entering destructor
    this->RemoveAllObservers();
    }

  // Decrement the reference count.
  this->Superclass::UnRegisterInternal(o, check);
}

//----------------------------------------------------------------------------
// Internal observer used by vtkObject::AddTemplatedObserver to add a
// vtkClassMemberCallbackBase instance as an observer to an event.
class vtkObjectCommandInternal : public vtkCommand
{
  vtkObject::vtkClassMemberCallbackBase* Callable;
public:
  static vtkObjectCommandInternal* New()
    { return new vtkObjectCommandInternal(); }

  vtkTypeMacro(vtkObjectCommandInternal, vtkCommand);
  virtual void Execute(
    vtkObject *caller, unsigned long eventId, void *callData)
    {
    if (this->Callable)
      {
      this->AbortFlagOff();
      if((*this->Callable)(caller, eventId, callData))
        {
        this->AbortFlagOn();
        }
      }
    }

  // Takes over the ownership of \c callable.
  void SetCallable(vtkObject::vtkClassMemberCallbackBase* callable)
    {
    delete this->Callable;
    this->Callable = callable;
    }

protected:
  vtkObjectCommandInternal()
    {
    this->Callable = NULL;
    }
  ~vtkObjectCommandInternal()
    {
    delete this->Callable;
    }
};

//----------------------------------------------------------------------------
unsigned long vtkObject::AddTemplatedObserver(
  unsigned long event, vtkObject::vtkClassMemberCallbackBase* callable, float priority)
{
  vtkObjectCommandInternal* command = vtkObjectCommandInternal::New();
  // Takes over the ownership of \c callable.
  command->SetCallable(callable);
  unsigned long id = this->AddObserver(event, command, priority);
  command->Delete();
  return id;
}
