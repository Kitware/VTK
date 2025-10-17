// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkObject.h"

#include "vtkCommand.h"
#include "vtkDeprecation.h"
#include "vtkObjectFactory.h"
#include "vtkTimeStamp.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
// Initialize static member that controls warning display
static vtkTypeBool vtkObjectGlobalWarningDisplay = 1;

//------------------------------------------------------------------------------
// avoid dll boundary problems
#ifdef _WIN32
void* vtkObject::operator new(size_t nSize)
{
  void* p = malloc(nSize);
  return p;
}

//------------------------------------------------------------------------------
void vtkObject::operator delete(void* p)
{
  free(p);
}
#endif

//------------------------------------------------------------------------------
void vtkObject::SetGlobalWarningDisplay(vtkTypeBool val)
{
  vtkObjectGlobalWarningDisplay = val;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::GetGlobalWarningDisplay()
{
  return vtkObjectGlobalWarningDisplay;
}

//----------------------------------Command/Observer stuff-------------------
// The Command/Observer design pattern is used to invoke and dispatch events.
//
// The class vtkSubjectHelper keeps a std::map of observers:
//
//    vtkObserverKey{Priority, Tag} -> vtkObserver{Command, Event}.
//
// This implicitly sorts the observers by Priority and Tag, allowing O(log n)
// mutations. Uses std::map::emplace_hint for amortized O(1) insertion in the
// typical case that default priorities are used.
//
// Note that InvokeEvent may indirectly recursively call InvokeEvent or mutate
// the mapping. The `ListModified` mechanism supports this.
//
// For legacy compatibility reasons, the `Priority` additionally has a
// `Generation` value, which maintains the ordering behavior from before
// rewriting
// with std::map. If an observer would go on the end of the list and has equal
// priority to the observer currently on the end of the list, it is instead
// added in the second-to-last position. (i.e. the first observer to be added
// is "pinned" to the end of the list.) If an observer with lower priority is
// added, it is indeed added to the end.
//
// The `Generation` value achieves this in std::map by sorting first by
// priority, then by generation, then by tag. Whenever a new observer should
// truly be added to the end of the list (i.e. it has lower priority) then
// the `Generation` counter is incremented, and future observers may be
// placed after the prior "pinned" observer.
//

struct vtkPriority
{
  float Value;
  unsigned long Generation;
};

struct vtkObserverKey
{
  vtkPriority Priority;
  unsigned long Tag;
};

bool operator<(const vtkObserverKey& lhs, const vtkObserverKey& rhs)
{
  if (lhs.Priority.Value != rhs.Priority.Value)
  {
    return lhs.Priority.Value > rhs.Priority.Value; // Higher priorities go first.
  }

  if (lhs.Priority.Generation != rhs.Priority.Generation)
  {
    return lhs.Priority.Generation < rhs.Priority.Generation; // Later generations go last.
  }

  return lhs.Tag < rhs.Tag; // Otherwise go in tag order.
}

struct vtkObserver
{
  vtkObserver(vtkCommand* command, unsigned long event);

  // No-copy to ensure Command->Register and Command->UnRegister are called
  // appropriately.
  vtkObserver(const vtkObserver& other) = delete;
  vtkObserver& operator=(const vtkObserver& other) = delete;

  vtkObserver(vtkObserver&& other) noexcept;
  vtkObserver& operator=(vtkObserver&& other) noexcept;

  ~vtkObserver();

  vtkCommand* Command;
  unsigned long Event;
};

//------------------------------------------------------------------------------
// The vtkSubjectHelper keeps a list of observers and dispatches events to them.
// Currently vtkSubjectHelper is an internal class to vtkObject. However, due to
// requirements from the VTK widgets it may be necessary to break out the
// vtkSubjectHelper at some point (for reasons of event management, etc.)
//
class vtkSubjectHelper
{
public:
  vtkSubjectHelper()
    : Focus1(nullptr)
    , Focus2(nullptr)
    , NextTag(1)
    , Generation(1)
  {
  }

  ~vtkSubjectHelper();

  unsigned long AddObserver(unsigned long event, vtkCommand* cmd, float p);
  void RemoveObserver(unsigned long tag);
  void RemoveObservers(unsigned long event);
  void RemoveObservers(unsigned long event, vtkCommand* cmd);
  void RemoveAllObservers();
  vtkTypeBool InvokeEvent(unsigned long event, void* callData, vtkObject* self);
  vtkCommand* GetCommand(unsigned long tag);
  unsigned long GetTag(vtkCommand*);
  vtkTypeBool HasObserver(unsigned long event);
  vtkTypeBool HasObserver(unsigned long event, vtkCommand* cmd);

  void GrabFocus(vtkCommand* c1, vtkCommand* c2)
  {
    this->Focus1 = c1;
    this->Focus2 = c2;
  }

  void ReleaseFocus()
  {
    this->Focus1 = nullptr;
    this->Focus2 = nullptr;
  }

  void PrintSelf(ostream& os, vtkIndent indent);

  // `InvokeEvent` iterates over `Observers` and invokes callbacks that might
  // mutate `Observers` and invalidate iterators. To handle this, keep a flag
  // for each invocation of `InvokeEvent` to indicate that iterators are
  // invalidated.
  //
  // `ListModified` is a stack of flags with one entry for each level of
  // `InvokeEvent` recursion. Whenever `Observers` is modified, iterators are
  // invalidated, so all entries in `ListModified` are set.
  //
  // After each callback in `InvokeEvent`, check the top (current) flag. If not
  // set, iterators are still valid so use `++it`. If set, iterators are invalid
  // so find a new one by `std::map::upper_bound`.
  std::vector<bool> ListModified;

  // This is to support the GrabFocus() methods found in vtkInteractorObserver.
  // If one of these commands can handle an event, then ***only*** that command
  // may handle the event. (Except for passive commands, which are always
  // invoked first and ignore focus.)
  vtkCommand* Focus1;
  vtkCommand* Focus2;

protected:
  unsigned long NextTag;
  unsigned long Generation;
  std::map<vtkObserverKey, vtkObserver> Observers;

  // For fast RemoveObserver, GetCommand.
  std::unordered_map<unsigned long, vtkPriority> Priorities;
};

// ------------------------------------vtkObject----------------------

vtkObject* vtkObject::New()
{
  vtkObject* ret = new vtkObject;
  ret->InitializeObjectBase();
  return ret;
}

//------------------------------------------------------------------------------
// Create an object with Debug turned off and modified time initialized
// to zero.
vtkObject::vtkObject()
{
  this->Debug = false;
  this->SubjectHelper = nullptr;
  this->Modified(); // Ensures modified time > than any other time
  // initial reference count = 1 and reference counting on.
}

//------------------------------------------------------------------------------
vtkObject::~vtkObject()
{
  vtkDebugMacro(<< "Destructing!");

  // warn user if reference counting is on and the object is being referenced
  // by another object
  if (this->GetReferenceCount() > 0)
  {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count.");
  }
  delete this->SubjectHelper;
  this->SubjectHelper = nullptr;
}

//------------------------------------------------------------------------------
// Return the modification for this object.
vtkMTimeType vtkObject::GetMTime()
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
  if (this->SubjectHelper)
  {
    os << endl;
    this->SubjectHelper->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}

//------------------------------------------------------------------------------
// Turn debugging output on.
// The Modified() method is purposely not called since we do not want to affect
// the modification time when enabling debug output.
void vtkObject::DebugOn()
{
  this->Debug = true;
}

//------------------------------------------------------------------------------
// Turn debugging output off.
void vtkObject::DebugOff()
{
  this->Debug = false;
}

//------------------------------------------------------------------------------
// Get the value of the debug flag.
bool vtkObject::GetDebug()
{
  return this->Debug;
}

//------------------------------------------------------------------------------
// Set the value of the debug flag. A true value turns debugging on.
void vtkObject::SetDebug(bool debugFlag)
{
  this->Debug = debugFlag;
}

//------------------------------------------------------------------------------
// This method is called when vtkErrorMacro executes. It allows
// the debugger to break on error.
void vtkObject::BreakOnError() {}

//----------------------------------Command/Observer stuff-------------------
//

vtkObserver::vtkObserver(vtkCommand* command, unsigned long event)
  : Command(command)
  , Event(event)
{
  this->Command->Register(nullptr);
}

vtkObserver::vtkObserver(vtkObserver&& other) noexcept
  : Command(std::exchange(other.Command, nullptr))
  , Event(other.Event)
{
}

vtkObserver& vtkObserver::operator=(vtkObserver&& other) noexcept
{
  if (this == &other)
  {
    return *this;
  }
  if (this->Command != nullptr)
  {
    this->Command->UnRegister(nullptr);
  }
  this->Command = std::exchange(other.Command, nullptr);
  this->Event = other.Event;
  return *this;
}

vtkObserver::~vtkObserver()
{
  if (this->Command != nullptr)
  {
    this->Command->UnRegister(nullptr);
  }
}

//------------------------------------------------------------------------------
vtkSubjectHelper::~vtkSubjectHelper()
{
  this->Focus1 = nullptr;
  this->Focus2 = nullptr;
}

//------------------------------------------------------------------------------
unsigned long vtkSubjectHelper::AddObserver(unsigned long event, vtkCommand* cmd, float p)
{
  const unsigned long tag = this->NextTag++;

  // TODO: If there is no observer with this priority, add with priority {p, true}
  //       If there is, add one with priority {p, false}

  auto it = this->Observers.lower_bound(vtkObserverKey{ { p, this->Generation + 1 }, 0 });
  auto next_it = this->Observers.upper_bound(vtkObserverKey{ { p, this->Generation + 1 }, 0 });

  vtkPriority priority{};

  if (next_it == this->Observers.end())
  {
    this->Generation++;
    priority = { p, this->Generation + 1 };
  }
  else
  {
    priority = { p, this->Generation };
  }

  this->Observers.emplace_hint(it, vtkObserverKey{ priority, tag }, vtkObserver{ cmd, event });
  this->Priorities.emplace(tag, priority);
  this->ListModified.assign(this->ListModified.size(), true);
  return tag;
}

//------------------------------------------------------------------------------
void vtkSubjectHelper::RemoveObserver(unsigned long tag)
{
  const auto it_p = this->Priorities.find(tag);
  if (it_p == this->Priorities.end())
    return;

  const auto p = it_p->second;
  this->Priorities.erase(it_p);

  const auto it = this->Observers.find(vtkObserverKey{ p, tag });
  if (it == this->Observers.end())
    return;

  this->Observers.erase(it);

  if (!this->Observers.empty())
  {
    auto back = this->Observers.end();
    --back;
    if (back->first.Priority.Generation != this->Generation + 1)
    {
      vtkObserverKey key = back->first;
      key.Priority.Generation = this->Generation + 1;
      vtkObserver obs = std::move(back->second);
      this->Observers.erase(back);
      this->Observers.emplace_hint(this->Observers.end(), key, std::move(obs));
      this->Priorities.insert_or_assign(key.Tag, key.Priority);
    }
  }

  this->ListModified.assign(this->ListModified.size(), true);
}

//------------------------------------------------------------------------------
void vtkSubjectHelper::RemoveObservers(unsigned long event)
{
  bool modified = false;

  for (auto it = this->Observers.begin(); it != this->Observers.end();)
  {
    auto& [key, obs] = *it;
    if (obs.Event == event)
    {
      this->Priorities.erase(this->Priorities.find(key.Tag));
      it = this->Observers.erase(it);
      modified = true;
    }
    else
    {
      ++it;
    }
  }

  if (modified)
  {
    if (!this->Observers.empty())
    {
      auto back = this->Observers.end();
      --back;
      if (back->first.Priority.Generation != this->Generation + 1)
      {
        vtkObserverKey key = back->first;
        key.Priority.Generation = this->Generation + 1;
        vtkObserver obs = std::move(back->second);
        this->Observers.erase(back);
        this->Observers.emplace_hint(this->Observers.end(), key, std::move(obs));
        this->Priorities.insert_or_assign(key.Tag, key.Priority);
      }
    }

    this->ListModified.assign(this->ListModified.size(), true);
  }
}

//------------------------------------------------------------------------------
void vtkSubjectHelper::RemoveObservers(unsigned long event, vtkCommand* cmd)
{
  bool modified = false;

  for (auto it = this->Observers.begin(); it != this->Observers.end();)
  {
    auto& [key, obs] = *it;
    if (obs.Event == event && obs.Command == cmd)
    {
      this->Priorities.erase(this->Priorities.find(key.Tag));
      it = this->Observers.erase(it);
      modified = true;
    }
    else
    {
      ++it;
    }
  }

  if (modified)
  {
    if (!this->Observers.empty())
    {
      auto back = this->Observers.end();
      --back;
      if (back->first.Priority.Generation != this->Generation + 1)
      {
        vtkObserverKey key = back->first;
        key.Priority.Generation = this->Generation + 1;
        vtkObserver obs = std::move(back->second);
        this->Observers.erase(back);
        this->Observers.emplace_hint(this->Observers.end(), key, std::move(obs));
        this->Priorities.insert_or_assign(key.Tag, key.Priority);
      }
    }

    this->ListModified.assign(this->ListModified.size(), true);
  }
}

//------------------------------------------------------------------------------
void vtkSubjectHelper::RemoveAllObservers()
{
  if (this->Observers.empty())
  {
    return;
  }

  this->Priorities.clear();
  this->Observers.clear();
  this->ListModified.assign(this->ListModified.size(), true);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkSubjectHelper::HasObserver(unsigned long event)
{
  for (auto& [key, obs] : this->Observers)
  {
    if (obs.Event == event || obs.Event == vtkCommand::AnyEvent)
    {
      return 1;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkSubjectHelper::HasObserver(unsigned long event, vtkCommand* cmd)
{
  for (auto& [key, obs] : this->Observers)
  {
    if ((obs.Event == event || obs.Event == vtkCommand::AnyEvent) && obs.Command == cmd)
    {
      return 1;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkSubjectHelper::InvokeEvent(unsigned long event, void* callData, vtkObject* self)
{
  bool focusHandled = false;

  // When we invoke an event, the observer may add or remove observers.  To make
  // sure that the iteration over the observers goes smoothly, we capture any
  // change to the list with the ListModified ivar.  However, an observer may
  // also do something that causes another event to be invoked in this object.
  // That means that this method will be called recursively, which is why we use
  // a std::vector to store ListModified where the size of the vector is equal
  // to the depth of recursion. We always push upon entry to this method and
  // pop before returning.
  this->ListModified.push_back(false);

  // If an element with a tag greater than maxTag is found, that means it has
  // been added after InvokeEvent is called (as a side effect of calling an
  // element command. In that case, the element is discarded and not executed.
  const unsigned long maxTag = this->NextTag;

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
  for (auto it = this->Observers.begin(); it != this->Observers.end();)
  {
    auto& obs = it->second;
    auto key = it->first; // Make a copy because it may be invalidated

    if (obs.Command->GetPassiveObserver()                          //
      && (obs.Event == event || obs.Event == vtkCommand::AnyEvent) //
      && key.Tag < maxTag)
    {
      vtkCommand* cmd = obs.Command;
      cmd->Register(cmd);
      cmd->Execute(self, event, callData);
      cmd->UnRegister();
    }
    if (this->ListModified.back())
    {
      vtkGenericWarningMacro(
        << "Passive observer should not call AddObserver or RemoveObserver in callback.");
      it = this->Observers.upper_bound(key);
      this->ListModified.back() = false;
    }
    else
    {
      ++it;
    }
  }

  // 1. Focus loop
  //
  if (this->Focus1 || this->Focus2)
  {
    for (auto it = this->Observers.begin(); it != this->Observers.end();)
    {
      auto& obs = it->second;
      auto key = it->first; // Make a copy because it may be invalidated

      if (!obs.Command->GetPassiveObserver()                         //
        && (obs.Command == Focus1 || obs.Command == Focus2)          //
        && (obs.Event == event || obs.Event == vtkCommand::AnyEvent) //
        && key.Tag < maxTag)
      {
        focusHandled = true;
        vtkCommand* cmd = obs.Command;
        cmd->Register(cmd);
        cmd->SetAbortFlag(0);
        cmd->Execute(self, event, callData);
        // if the command set the abort flag, then stop firing events
        // and return
        if (cmd->GetAbortFlag())
        {
          cmd->UnRegister();
          this->ListModified.pop_back();
          return 1;
        }
        cmd->UnRegister();
      }
      if (this->ListModified.back())
      {
        it = this->Observers.upper_bound(key);
        this->ListModified.back() = false;
      }
      else
      {
        ++it;
      }
    }
  }

  // 2. Remainder loop
  //
  if (!focusHandled)
  {
    for (auto it = this->Observers.begin(); it != this->Observers.end();)
    {
      auto& obs = it->second;
      auto key = it->first; // Make a copy because it may be invalidated

      if (!obs.Command->GetPassiveObserver()                         //
        && (obs.Event == event || obs.Event == vtkCommand::AnyEvent) //
        && key.Tag < maxTag)
      {
        vtkCommand* cmd = obs.Command;
        cmd->Register(cmd);
        cmd->SetAbortFlag(0);
        cmd->Execute(self, event, callData);
        // if the command set the abort flag, then stop firing events
        // and return
        if (cmd->GetAbortFlag())
        {
          cmd->UnRegister();
          this->ListModified.pop_back();
          return 1;
        }
        cmd->UnRegister();
      }
      if (this->ListModified.back())
      {
        it = this->Observers.upper_bound(key);
        this->ListModified.back() = false;
      }
      else
      {
        ++it;
      }
    }
  }

  this->ListModified.pop_back();
  return 0;
}

//------------------------------------------------------------------------------
unsigned long vtkSubjectHelper::GetTag(vtkCommand* cmd)
{
  for (auto& [key, obs] : this->Observers)
  {
    if (obs.Command == cmd)
    {
      return key.Tag;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkCommand* vtkSubjectHelper::GetCommand(unsigned long tag)
{
  const auto it_p = this->Priorities.find(tag);
  if (it_p == this->Priorities.end())
  {
    return nullptr;
  }
  const auto p = it_p->second;

  const auto it = this->Observers.find(vtkObserverKey{ p, tag });
  if (it == this->Observers.end())
  {
    this->Priorities.erase(it_p);
    return nullptr;
  }
  return it->second.Command;
}

//------------------------------------------------------------------------------
void vtkSubjectHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Registered Observers:\n";
  vtkIndent outer = indent.GetNextIndent();
  vtkIndent inner = outer.GetNextIndent();

  if (this->Observers.empty())
  {
    os << outer << "(none)\n";
  }
  else
  {
    for (auto& [key, obs] : this->Observers)
    {
      os << outer << "vtkObserver (" << this << ")\n";
      os << inner << "Event: " << obs.Event << "\n";
      os << inner << "EventName: " << vtkCommand::GetStringFromEventId(obs.Event) << "\n";
      os << inner << "Command: " << obs.Command << "\n";
      os << inner << "Priority: " << key.Priority.Value << "\n";
      os << inner << "Generation: " << key.Priority.Generation << "\n";
      os << inner << "Tag: " << key.Tag << "\n";
    }
  }
}

//--------------------------------vtkObject observer-----------------------
unsigned long vtkObject::AddObserver(unsigned long event, vtkCommand* cmd, float p)
{
  // VTK_DEPRECATED_IN_9_5_0()
  // Remove this "#if/#endif" code block when removing 9.5.0 deprecations
#if VTK_DEPRECATION_LEVEL >= VTK_VERSION_CHECK(9, 4, 20241008)
  if (event == vtkCommand::WindowResizeEvent && this->IsA("vtkRenderWindowInteractor"))
  {
    vtkWarningMacro(
      "WindowResizeEvent will not be generated by vtkRenderWindowInteractor after VTK 9.6.\n"
      "Use ConfigureEvent instead, or observe WindowResizeEvent on the vtkRenderWindow.");
  }
#endif

  if (!this->SubjectHelper)
  {
    this->SubjectHelper = new vtkSubjectHelper;
  }
  return this->SubjectHelper->AddObserver(event, cmd, p);
}

//------------------------------------------------------------------------------
unsigned long vtkObject::AddObserver(const char* event, vtkCommand* cmd, float p)
{
  return this->AddObserver(vtkCommand::GetEventIdFromString(event), cmd, p);
}

//------------------------------------------------------------------------------
vtkCommand* vtkObject::GetCommand(unsigned long tag)
{
  if (this->SubjectHelper)
  {
    return this->SubjectHelper->GetCommand(tag);
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObserver(unsigned long tag)
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->RemoveObserver(tag);
  }
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObserver(vtkCommand* c)
{
  if (this->SubjectHelper)
  {
    unsigned long tag = this->SubjectHelper->GetTag(c);
    while (tag)
    {
      this->SubjectHelper->RemoveObserver(tag);
      tag = this->SubjectHelper->GetTag(c);
    }
  }
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObservers(unsigned long event)
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->RemoveObservers(event);
  }
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObservers(const char* event)
{
  this->RemoveObservers(vtkCommand::GetEventIdFromString(event));
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObservers(unsigned long event, vtkCommand* cmd)
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->RemoveObservers(event, cmd);
  }
}

//------------------------------------------------------------------------------
void vtkObject::RemoveObservers(const char* event, vtkCommand* cmd)
{
  this->RemoveObservers(vtkCommand::GetEventIdFromString(event), cmd);
}

//------------------------------------------------------------------------------
void vtkObject::RemoveAllObservers()
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->RemoveAllObservers();
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::InvokeEvent(unsigned long event, void* callData)
{
  if (this->SubjectHelper)
  {
    return this->SubjectHelper->InvokeEvent(event, callData, this);
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::InvokeEvent(const char* event, void* callData)
{
  return this->InvokeEvent(vtkCommand::GetEventIdFromString(event), callData);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::HasObserver(unsigned long event)
{
  if (this->SubjectHelper)
  {
    return this->SubjectHelper->HasObserver(event);
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::HasObserver(const char* event)
{
  return this->HasObserver(vtkCommand::GetEventIdFromString(event));
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::HasObserver(unsigned long event, vtkCommand* cmd)
{
  if (this->SubjectHelper)
  {
    return this->SubjectHelper->HasObserver(event, cmd);
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkObject::HasObserver(const char* event, vtkCommand* cmd)
{
  return this->HasObserver(vtkCommand::GetEventIdFromString(event), cmd);
}

//------------------------------------------------------------------------------
void vtkObject::InternalGrabFocus(vtkCommand* mouseEvents, vtkCommand* keypressEvents)
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->GrabFocus(mouseEvents, keypressEvents);
  }
}

//------------------------------------------------------------------------------
void vtkObject::InternalReleaseFocus()
{
  if (this->SubjectHelper)
  {
    this->SubjectHelper->ReleaseFocus();
  }
}

//------------------------------------------------------------------------------
void vtkObject::Modified()
{
  this->MTime.Modified();
  this->InvokeEvent(vtkCommand::ModifiedEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkObject::RegisterInternal(vtkObjectBase* o, vtkTypeBool check)
{
  // Print debugging messages.
  // TODO: This debug print is the only reason `RegisterInternal` is virtual.
  // Look into moving this into `vtkObjectBase` or to some other mechanism and
  // making the method non-virtual in the future.
  if (o)
  {
    vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o
                  << "), ReferenceCount = " << this->GetReferenceCount() + 1);
  }
  else
  {
    vtkDebugMacro(<< "Registered by nullptr, ReferenceCount = " << this->GetReferenceCount() + 1);
  }

  // Increment the reference count.
  this->Superclass::RegisterInternal(o, check);
}

//------------------------------------------------------------------------------
void vtkObject::UnRegisterInternal(vtkObjectBase* o, vtkTypeBool check)
{
  // Print debugging messages.
  // TODO: This debug print is the only reason `UnRegisterInternal` is virtual.
  // Look into moving this into `vtkObjectBase` or to some other mechanism and
  // making the method non-virtual in the future.
  if (o)
  {
    vtkDebugMacro(<< "UnRegistered by " << o->GetClassName() << " (" << o
                  << "), ReferenceCount = " << (this->GetReferenceCount() - 1));
  }
  else
  {
    vtkDebugMacro(<< "UnRegistered by nullptr, ReferenceCount = "
                  << (this->GetReferenceCount() - 1));
  }

  // Decrement the reference count.
  this->Superclass::UnRegisterInternal(o, check);
}

//------------------------------------------------------------------------------
void vtkObject::ObjectFinalize()
{
  // The object is about to be deleted. Invoke the delete event.
  this->InvokeEvent(vtkCommand::DeleteEvent, nullptr);

  // Clean out observers prior to entering destructor
  this->RemoveAllObservers();
}

//------------------------------------------------------------------------------
// Internal observer used by vtkObject::AddTemplatedObserver to add a
// vtkClassMemberCallbackBase instance as an observer to an event.
class vtkObjectCommandInternal : public vtkCommand
{
  vtkObject::vtkClassMemberCallbackBase* Callable;

public:
  static vtkObjectCommandInternal* New() { return new vtkObjectCommandInternal(); }

  vtkTypeMacro(vtkObjectCommandInternal, vtkCommand);

  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (this->Callable)
    {
      this->AbortFlagOff();
      if ((*this->Callable)(caller, eventId, callData))
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
  vtkObjectCommandInternal() { this->Callable = nullptr; }
  ~vtkObjectCommandInternal() override { delete this->Callable; }
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkObject::SetObjectName(const std::string& objectName)
{
  vtkDebugMacro(<< vtkObjectBase::GetObjectDescription() << "set object name to '" << objectName
                << "'");
  this->ObjectName = objectName;
}

//------------------------------------------------------------------------------
std::string vtkObject::GetObjectName() const
{
  return this->ObjectName;
}

//------------------------------------------------------------------------------
std::string vtkObject::GetObjectDescription() const
{
  std::stringstream s;
  s << this->Superclass::GetObjectDescription();
  if (!this->ObjectName.empty())
  {
    s << " '" << this->ObjectName << "'";
  }
  return s.str();
}

VTK_ABI_NAMESPACE_END
