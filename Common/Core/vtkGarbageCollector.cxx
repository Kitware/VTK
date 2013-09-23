/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGarbageCollector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGarbageCollector.h"

#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointerBase.h"

#include <vtksys/ios/sstream>
#include <queue>
#include <stack>
#include <vector>

// Leave the hashing version off for now.
#define VTK_GARBAGE_COLLECTOR_HASH 0

#if VTK_GARBAGE_COLLECTOR_HASH
# include <vtksys/hash_set.hxx>
# include <vtksys/hash_map.hxx>
#else
# include <map>
# include <set>
#endif

#include <cassert>

vtkStandardNewMacro(vtkGarbageCollector);

#if VTK_GARBAGE_COLLECTOR_HASH
struct vtkGarbageCollectorHash
{
  size_t operator()(void* p) const { return reinterpret_cast<size_t>(p); }
};
#endif

class vtkGarbageCollectorSingleton;

//----------------------------------------------------------------------------
// The garbage collector singleton.  In order to support delayed
// collection vtkObjectBase::UnRegister passes references to the
// singleton instead of decrementing the reference count.  At some
// point collection occurs and accounts for these references.  This
// MUST be default initialized to zero by the compiler and is
// therefore not initialized here.  The ClassInitialize and
// ClassFinalize methods handle this instance.
static vtkGarbageCollectorSingleton* vtkGarbageCollectorSingletonInstance;

//----------------------------------------------------------------------------
// Global debug setting.  This flag specifies whether a collector
// should print debugging output.  This must be default initialized to
// zero by the compiler and is therefore not initialized here.  The
// ClassInitialize and ClassFinalize methods handle it.
static int vtkGarbageCollectorGlobalDebugFlag;

//----------------------------------------------------------------------------
// The thread identifier of the main thread.  Delayed garbage
// collection is supported only for objects in the main thread.  This
// is initialized when the program loads.  All garbage collection
// calls test whether they are called from this thread.  If not, no
// references are accepted by the singleton.  This must be default
// initialized to zero by the compiler and is therefore not
// initialized here.  The ClassInitialize and ClassFinalize methods
// handle it.
static vtkMultiThreaderIDType vtkGarbageCollectorMainThread;

//----------------------------------------------------------------------------
vtkGarbageCollector::vtkGarbageCollector()
{
}

//----------------------------------------------------------------------------
vtkGarbageCollector::~vtkGarbageCollector()
{
  this->SetReferenceCount(0);
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::SetGlobalDebugFlag(int flag)
{
  vtkGarbageCollectorGlobalDebugFlag = flag;
}

//----------------------------------------------------------------------------
int vtkGarbageCollector::GetGlobalDebugFlag()
{
  return vtkGarbageCollectorGlobalDebugFlag;
}

//----------------------------------------------------------------------------
// Friendship interface listing non-public methods the garbage
// collector can call on vtkObjectBase.
class vtkGarbageCollectorToObjectBaseFriendship
{
public:
  static void ReportReferences(vtkGarbageCollector* self, vtkObjectBase* obj)
    {
    obj->ReportReferences(self);
    }
  static void RegisterBase(vtkObjectBase* obj)
    {
    // Call vtkObjectBase::RegisterInternal directly to make sure the
    // object does not try to report the call back to the garbage
    // collector and no debugging output is shown.
    obj->vtkObjectBase::RegisterInternal(0, 0);
    }
  static void UnRegisterBase(vtkObjectBase* obj)
    {
    // Call vtkObjectBase::UnRegisterInternal directly to make sure
    // the object does not try to report the call back to the garbage
    // collector and no debugging output is shown.
    obj->vtkObjectBase::UnRegisterInternal(0, 0);
    }
  static void Register(vtkObjectBase* obj, vtkObjectBase* from)
    {
    // Call RegisterInternal directly to make sure the object does not
    // try to report the call back to the garbage collector.
    obj->RegisterInternal(from, 0);
    }
  static void UnRegister(vtkObjectBase* obj, vtkObjectBase* from)
    {
    // Call UnRegisterInternal directly to make sure the object does
    // not try to report the call back to the garbage collector.
    obj->UnRegisterInternal(from, 0);
    }
};

//----------------------------------------------------------------------------
// Function to test whether caller is the main thread.
static int vtkGarbageCollectorIsMainThread()
{
  return
    vtkMultiThreader::ThreadsEqual(vtkGarbageCollectorMainThread,
                                   vtkMultiThreader::GetCurrentThreadID());
}

//----------------------------------------------------------------------------
// Singleton to hold discarded references.
class vtkGarbageCollectorSingleton
{
public:
  vtkGarbageCollectorSingleton();
  ~vtkGarbageCollectorSingleton();

  // Internal implementation of vtkGarbageCollector::GiveReference.
  int GiveReference(vtkObjectBase* obj);

  // Internal implementation of vtkGarbageCollector::TakeReference.
  int TakeReference(vtkObjectBase* obj);

  // Called by GiveReference to decide whether to accept a reference.
  int CheckAccept();

  // Push/Pop deferred collection.
  void DeferredCollectionPush();
  void DeferredCollectionPop();

  // Map from object to number of stored references.
#if VTK_GARBAGE_COLLECTOR_HASH
  typedef vtksys::hash_map<vtkObjectBase*, int, vtkGarbageCollectorHash>
    ReferencesType;
#else
  typedef std::map<vtkObjectBase*, int> ReferencesType;
#endif
  ReferencesType References;

  // The number of references stored in the map.
  int TotalNumberOfReferences;

  // The number of times DeferredCollectionPush has been called not
  // matched by a DeferredCollectionPop.
  int DeferredCollectionCount;
};

//----------------------------------------------------------------------------
// Internal implementation subclass.
class vtkGarbageCollectorImpl: public vtkGarbageCollector
{
public:
  vtkTypeMacro(vtkGarbageCollectorImpl, vtkGarbageCollector);

  vtkGarbageCollectorImpl();
  ~vtkGarbageCollectorImpl();

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void Register(vtkObjectBase*);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void UnRegister(vtkObjectBase*);

  // Perform a collection check.
  void CollectInternal(vtkObjectBase* root);


// Sun's compiler is broken and does not allow access to protected members from
// nested class
// protected:
  //--------------------------------------------------------------------------
  // Internal data structure types.

#if VTK_GARBAGE_COLLECTOR_HASH
  typedef vtksys::hash_map<vtkObjectBase*, int, vtkGarbageCollectorHash>
    ReferencesType;
#else
  typedef std::map<vtkObjectBase*, int> ReferencesType;
#endif
  struct ComponentType;

  struct Entry;
  struct EntryEdge
  {
    Entry* Reference;
    void* Pointer;
    EntryEdge(Entry* r, void* p): Reference(r), Pointer(p) {}
  };

  // Store garbage collection entries keyed by object.
  struct Entry
  {
    Entry(vtkObjectBase* obj): Object(obj), Root(0), Component(0),
                               VisitOrder(0), Count(0), GarbageCount(0),
                               References() {}
    ~Entry() { assert(this->GarbageCount == 0); }

    // The object corresponding to this entry.
    vtkObjectBase* Object;

    // The candidate root for the component containing this object.
    Entry* Root;

    // The component to which the object is assigned, if any.
    ComponentType* Component;

    // Mark the order in which object's are visited by Tarjan's algorithm.
    int VisitOrder;

    // The number of references from outside the component not
    // counting the garbage collector references.
    int Count;

    // The number of references held by the garbage collector.
    int GarbageCount;

    // The list of references reported by this entry's object.
    typedef std::vector<EntryEdge> ReferencesType;
    ReferencesType References;
  };

  // Compare entries by object pointer for quick lookup.
#if VTK_GARBAGE_COLLECTOR_HASH
  struct EntryCompare
  {
    bool operator()(Entry* l, Entry* r) const
      { return l->Object == r->Object; }
  };
  struct EntryHash
  {
    size_t operator()(Entry* e) const
      { return e?reinterpret_cast<size_t>(e->Object):0; }
  };
#else
  struct EntryCompare
  {
    std::less<vtkObjectBase*> Compare;
    bool operator()(Entry* l, Entry* r) const
      { return Compare(l->Object, r->Object); }
  };
#endif

  // Represent a strongly connected component of the reference graph.
  typedef std::vector<Entry*> ComponentBase;
  struct ComponentType: public ComponentBase
  {
    typedef ComponentBase::iterator iterator;
    ComponentType(): NetCount(0), Identifier(0) {}
    ~ComponentType()
      { for(iterator i = begin(), iend = end(); i != iend; ++i) { (*i)->Component = 0; } }

    // The net reference count of the component.
    int NetCount;

    // The component identifier.
    int Identifier;
  };

  //--------------------------------------------------------------------------
  // Internal data objects.

  // The set of objects that have been visited.
#if VTK_GARBAGE_COLLECTOR_HASH
  typedef vtksys::hash_set<Entry*, EntryHash, EntryCompare> VisitedType;
#else
  typedef std::set<Entry*, EntryCompare> VisitedType;
#endif
  VisitedType Visited;

  // Count the number of components found to give each an identifier
  // for use in debugging messages.
  int NumberOfComponents;

  // The set of components found that have not yet leaked.
#if VTK_GARBAGE_COLLECTOR_HASH
  typedef vtksys::hash_set<ComponentType*, vtkGarbageCollectorHash>
    ComponentsType;
#else
  typedef std::set<ComponentType*> ComponentsType;
#endif
  ComponentsType ReferencedComponents;

  // Queue leaked components for deletion.
  std::queue<ComponentType*> LeakedComponents;

  // The stack of objects forming the connected components.  This is
  // used in the implementation of Tarjan's algorithm.
  std::stack<Entry*> Stack;

  // The object whose references are currently being traced by
  // Tarjan's algorithm.  Used during the ReportReferences callback.
  Entry* Current;

  // Count for visit order of Tarjan's algorithm.
  int VisitCount;

  // The singleton instance from which to take references when passing
  // references to the entries.
  vtkGarbageCollectorSingleton* Singleton;

  //--------------------------------------------------------------------------
  // Internal implementation methods.

  // Walk the reference graph using Tarjan's algorithm to identify
  // strongly connected components.
  void FindComponents(vtkObjectBase* root);

  // Get the entry for the given object.  This may visit the object.
  Entry* MaybeVisit(vtkObjectBase*);

  // Node visitor for Tarjan's algorithm.
  Entry* VisitTarjan(vtkObjectBase*);

  // Callback from objects to report references.
  void Report(vtkObjectBase* obj, void* ptr);
  virtual void Report(vtkObjectBase* obj, void* ptr, const char* desc);

  // Collect the objects of the given leaked component.
  void CollectComponent(ComponentType* c);

  // Print the given component as a debugging message.
  void PrintComponent(ComponentType* c);

  // Subtract references the component holds to itself.
  void SubtractInternalReferences(ComponentType* c);

  // Subtract references the component holds to other components.
  void SubtractExternalReferences(ComponentType* c);

  // Subtract one reference from the given entry.  If the entry's
  // component is left with no references, it is queued as a leaked
  // component.
  void SubtractReference(Entry* e);

  // Transfer references from the garbage collector to the entry for
  // its object.
  void PassReferencesToEntry(Entry* e);

  // Flush all collector references to the object in an entry.
  void FlushEntryReferences(Entry* e);

private:
  vtkGarbageCollectorImpl(const vtkGarbageCollectorImpl&);  // Not implemented.
  void operator=(const vtkGarbageCollectorImpl&);  // Not implemented.
};

//----------------------------------------------------------------------------
vtkGarbageCollectorImpl::vtkGarbageCollectorImpl()
{
  // Set debugging state.
  this->SetDebug(vtkGarbageCollectorGlobalDebugFlag);

  // Take references from the singleton only in the main thread.
  if(vtkGarbageCollectorIsMainThread())
    {
    this->Singleton = vtkGarbageCollectorSingletonInstance;
    }
  else
    {
    this->Singleton = 0;
    }

  // Initialize reference graph walk implementation.
  this->VisitCount = 0;
  this->Current = 0;
  this->NumberOfComponents = 0;
}

//----------------------------------------------------------------------------
vtkGarbageCollectorImpl::~vtkGarbageCollectorImpl()
{
  // The collector implementation should have left these empty.
  assert(this->Current == 0);
  assert(this->Stack.empty());
  assert(this->LeakedComponents.empty());

  // Clear component list.
  for(ComponentsType::iterator c = this->ReferencedComponents.begin(), cend = this->ReferencedComponents.end();
      c != cend; ++c)
    {
    delete *c;
    }
  this->ReferencedComponents.clear();

  // Clear visited list.
  for(VisitedType::iterator v = this->Visited.begin(), vend = this->Visited.end();
      v != vend;)
    {
    // Increment the iterator before deleting because the hash table
    // compare function dereferences the pointer.
    delete *v++;
    }
  this->Visited.clear();

  // Disable debugging to avoid destruction message.
  this->SetDebug(0);
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::Register(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::UnRegister(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::CollectInternal(vtkObjectBase* root)
{
  // Identify strong components.
  this->FindComponents(root);

  // Delete all the leaked components.
  while(!this->LeakedComponents.empty())
    {
    // Get the next leaked component.
    ComponentType* c = this->LeakedComponents.front();
    this->LeakedComponents.pop();

    // Subtract this component's references to other components.  This
    // may cause others to be queued.
    this->SubtractExternalReferences(c);

    // Collect the members of this component.
    this->CollectComponent(c);

    // We are done with this component.
    delete c;
    }

#ifndef NDEBUG
  // Print remaining referenced components for debugging.
  for(ComponentsType::iterator i = this->ReferencedComponents.begin(), iend = this->ReferencedComponents.end();
      i != iend; ++i)
    {
    this->PrintComponent(*i);
    }
#endif

  // Flush remaining references owned by entries in referenced
  // components.
  for(ComponentsType::iterator c = this->ReferencedComponents.begin(), cend = this->ReferencedComponents.end();
      c != cend; ++c)
    {
    for(ComponentType::iterator j = (*c)->begin(), jend = (*c)->end();
        j != jend; ++j)
      {
      this->FlushEntryReferences(*j);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::FindComponents(vtkObjectBase* root)
{
  // Walk the references from the given object, if any.
  if(root)
    {
    this->MaybeVisit(root);
    }
}

//----------------------------------------------------------------------------
vtkGarbageCollectorImpl::Entry*
vtkGarbageCollectorImpl::MaybeVisit(vtkObjectBase* obj)
{
  // Check for an existing entry.
  assert(obj != 0);
  Entry e(obj);
  VisitedType::iterator i = this->Visited.find(&e);
  if(i == this->Visited.end())
    {
    // Visit the object to create the entry.
    return this->VisitTarjan(obj);
    }
  // else Return the existing entry.
  return *i;
}

//----------------------------------------------------------------------------
vtkGarbageCollectorImpl::Entry*
vtkGarbageCollectorImpl::VisitTarjan(vtkObjectBase* obj)
{
  // Create an entry for the object.
  Entry* v = new Entry(obj);
  this->Visited.insert(v);

  // Initialize the entry and push it onto the stack of graph nodes.
  v->Root = v;
  v->Component = 0;
  v->VisitOrder = ++this->VisitCount;
  this->PassReferencesToEntry(v);
  this->Stack.push(v);

  vtkDebugMacro("Requesting references from "
                << v->Object->GetClassName() << "("
                << v->Object << ") with reference count "
                << (v->Object->GetReferenceCount()-v->GarbageCount));

  // Process the references from this node.
  Entry* saveCurrent = this->Current;
  this->Current = v;
  vtkGarbageCollectorToObjectBaseFriendship::ReportReferences(this, v->Object);
  this->Current = saveCurrent;

  // Check if we have found a component.
  if(v->Root == v)
    {
    // Found a new component.
    ComponentType* c = new ComponentType;
    c->Identifier = ++this->NumberOfComponents;
    Entry* w;
    do
      {
      // Get the next member of the component.
      w = this->Stack.top();
      this->Stack.pop();

      // Assign the member to the component.
      w->Component = c;
      w->Root = v;
      c->push_back(w);

      // Include this member's reference count in the component total.
      c->NetCount += w->Count;
      } while(w != v);

    // Save the component.
    this->ReferencedComponents.insert(c);

    // Print the component for debugging.
    this->PrintComponent(c);

    // Remove internal references from the component.
    this->SubtractInternalReferences(c);
    }

  return v;
}

//----------------------------------------------------------------------------
#ifdef NDEBUG
void vtkGarbageCollectorImpl::Report(vtkObjectBase* obj, void* ptr,
                                     const char*)
{
  // All calls should be given the pointer.
  assert(ptr != 0);

  // Forward call to the internal implementation.
  if(obj)
    {
    this->Report(obj, ptr);
    }
}
#else
void vtkGarbageCollectorImpl::Report(vtkObjectBase* obj, void* ptr,
                                     const char* desc)
{
  // All calls should be given the pointer.
  assert(ptr != 0);

  if(obj)
    {
    // Report debugging information if requested.
    if(this->Debug && vtkObject::GetGlobalWarningDisplay())
      {
      vtkObjectBase* current = this->Current->Object;
      vtksys_ios::ostringstream msg;
      msg << "Report: "
          << current->GetClassName() << "(" << current << ") "
          << (desc?desc:"")
          << " -> " << obj->GetClassName() << "(" << obj << ")";
      vtkDebugMacro(<< msg.str().c_str());
      }

    // Forward call to the internal implementation.
    this->Report(obj, ptr);
    }
}
#endif

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::Report(vtkObjectBase* obj, void* ptr)
{
  // Get the source and destination of this reference.
  Entry* v = this->Current;
  Entry* w = this->MaybeVisit(obj);

  // If the destination has not yet been assigned to a component,
  // check if it is a better potential root for the current object.
  if(!w->Component)
    {
    if(w->Root->VisitOrder < v->Root->VisitOrder)
      {
      v->Root = w->Root;
      }
    }

  // Save this reference.
  v->References.push_back(EntryEdge(w, ptr));
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::CollectComponent(ComponentType* c)
{
  ComponentType::iterator e, eend;

  // Print out the component for debugging.
  this->PrintComponent(c);

  // Get an extra reference to all objects in the component so that
  // they are not deleted until all references are removed.
  for(e = c->begin(), eend = c->end(); e != eend; ++e)
    {
    vtkGarbageCollectorToObjectBaseFriendship::Register((*e)->Object, this);
    }

  // Disconnect the reference graph.
  for(e = c->begin(), eend = c->end(); e != eend; ++e)
    {
    // Loop over all references made by this entry's object.
    Entry* entry = *e;
    for(unsigned int i = 0; i < entry->References.size(); ++i)
      {
      // Get a pointer to the object referenced.
      vtkObjectBase* obj = entry->References[i].Reference->Object;

      // Get a pointer to the pointer holding the reference.
      void** ptr = static_cast<void**>(entry->References[i].Pointer);

      // Set the pointer holding the reference to NULL.  The
      // destructor of the object that reported this reference must
      // deal with this.
      *ptr = 0;

      // Remove the reference to the object referenced without
      // recursively collecting.  We already know about the object.
      vtkGarbageCollectorToObjectBaseFriendship::UnRegister(obj,
                                                            entry->Object);
      }
    }

  // Remove the Entries' references to objects.
  for(e = c->begin(), eend = c->end(); e != eend; ++e)
    {
    this->FlushEntryReferences(*e);
    }

  // Only our extra reference to each object remains.  Delete the
  // objects.
  for(e = c->begin(), eend = c->end(); e != eend; ++e)
    {
    assert((*e)->Object->GetReferenceCount() == 1);
    vtkGarbageCollectorToObjectBaseFriendship::UnRegister((*e)->Object, this);
    }
}

//----------------------------------------------------------------------------
#ifndef NDEBUG
void vtkGarbageCollectorImpl::PrintComponent(ComponentType* c)
{
  if(this->Debug && vtkObject::GetGlobalWarningDisplay())
    {
    vtksys_ios::ostringstream msg;
    msg << "Identified strongly connected component "
        << c->Identifier << " with net reference count "
        << c->NetCount << ":";
    for(ComponentType::iterator i = c->begin(), iend = c->end(); i != iend; ++i)
      {
      vtkObjectBase* obj = (*i)->Object;
      int count = (*i)->Count;
      msg << "\n  " << obj->GetClassName() << "(" << obj << ")"
          << " with " << count << " external "
          << ((count == 1)? "reference" : "references");
      }
    vtkDebugMacro(<< msg.str().c_str());
    }
}
#else
void vtkGarbageCollectorImpl::PrintComponent(ComponentType*)
{
}
#endif

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::SubtractInternalReferences(ComponentType* c)
{
  // Loop over all members of the component.
  for(ComponentType::iterator i = c->begin(), iend = c->end(); i != iend; ++i)
    {
    Entry* v = *i;

    // Loop over all references from this member.
    for(Entry::ReferencesType::iterator r = v->References.begin(), rend = v->References.end();
        r != rend; ++r)
      {
      Entry* w = r->Reference;

      // If this reference points inside the component, subtract it.
      if(v->Component == w->Component)
        {
        this->SubtractReference(w);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::SubtractExternalReferences(ComponentType* c)
{
  // Loop over all members of the component.
  for(ComponentType::iterator i = c->begin(), iend = c->end(); i != iend; ++i)
    {
    Entry* v = *i;

    // Loop over all references from this member.
    for(Entry::ReferencesType::iterator r = v->References.begin(), rend = v->References.end();
        r != rend; ++r)
      {
      Entry* w = r->Reference;

      // If this reference points outside the component, subtract it.
      if(v->Component != w->Component)
        {
        this->SubtractReference(w);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::SubtractReference(Entry* e)
{
  // The component should not be leaked before we get here.
  assert(e->Component != 0);
  assert(e->Component->NetCount > 0);

  vtkDebugMacro("Subtracting reference to object "
                << e->Object->GetClassName() << "(" << e->Object << ")"
                << " in component " << e->Component->Identifier << ".");

  // Decrement the entry's reference count.
  --e->Count;

  // If the component's net count is now zero, move it to the queue of
  // leaked component.
  if(--e->Component->NetCount == 0)
    {
    this->ReferencedComponents.erase(e->Component);
    this->LeakedComponents.push(e->Component);
    vtkDebugMacro("Component " << e->Component->Identifier << " is leaked.");
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::PassReferencesToEntry(Entry* e)
{
  // Get the number of references the collector holds.
  e->GarbageCount = 0;
  if(this->Singleton)
    {
    ReferencesType::iterator i = this->Singleton->References.find(e->Object);
    if(i != this->Singleton->References.end())
      {
      // Pass these references from the singleton to the entry.
      e->GarbageCount = i->second;
      this->Singleton->References.erase(i);
      this->Singleton->TotalNumberOfReferences -= e->GarbageCount;
      }
    }

  // Make sure the entry has at least one reference to the object.
  // This ensures the object in components of size 1 is not deleted
  // until we delete the component.
  if(e->GarbageCount == 0)
    {
    vtkGarbageCollectorToObjectBaseFriendship::RegisterBase(e->Object);
    ++e->GarbageCount;
    }

  // Subtract the garbage count from the object's reference count.
  e->Count = e->Object->GetReferenceCount() - e->GarbageCount;
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorImpl::FlushEntryReferences(Entry* e)
{
  while(e->GarbageCount > 0)
    {
    vtkGarbageCollectorToObjectBaseFriendship::UnRegisterBase(e->Object);
    --e->GarbageCount;
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::ClassInitialize()
{
  // Set default debugging state.
  vtkGarbageCollectorGlobalDebugFlag = 0;

  // Record the id of the main thread.
  vtkGarbageCollectorMainThread = vtkMultiThreader::GetCurrentThreadID();

  // Allocate the singleton used for delayed collection in the main
  // thread.
  vtkGarbageCollectorSingletonInstance = new vtkGarbageCollectorSingleton;
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::ClassFinalize()
{
  // We are done with the singleton.  Delete it and reset the pointer.
  // Other singletons may still cause garbage collection of VTK
  // objects, they just will not have the option of deferred
  // collection.  In order to get it they need only to include
  // vtkGarbageCollectorManager.h so that this singleton stays around
  // longer.
  delete vtkGarbageCollectorSingletonInstance;
  vtkGarbageCollectorSingletonInstance = 0;
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::Report(vtkObjectBase*, void*, const char*)
{
  vtkErrorMacro("vtkGarbageCollector::Report should be overridden.");
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::Collect()
{
  // This must be called only from the main thread.
  assert(vtkGarbageCollectorIsMainThread());

  // Keep collecting until no deferred checks exist.
  while(vtkGarbageCollectorSingletonInstance &&
        vtkGarbageCollectorSingletonInstance->TotalNumberOfReferences > 0)
    {
    // Collect starting from one deferred object at a time.  Each
    // check will remove at least the starting object and possibly
    // other objects from the singleton's references.
    vtkObjectBase* root =
      vtkGarbageCollectorSingletonInstance->References.begin()->first;
    vtkGarbageCollector::Collect(root);
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::Collect(vtkObjectBase* root)
{
  // Create a collector instance.
  vtkGarbageCollectorImpl collector;

  vtkDebugWithObjectMacro((&collector), "Starting collection check.");

  // Collect leaked objects.
  collector.CollectInternal(root);

  vtkDebugWithObjectMacro((&collector), "Finished collection check.");
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::DeferredCollectionPush()
{
  // This must be called only from the main thread.
  assert(vtkGarbageCollectorIsMainThread());

  // Forward the call to the singleton.
  if(vtkGarbageCollectorSingletonInstance)
    {
    vtkGarbageCollectorSingletonInstance->DeferredCollectionPush();
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::DeferredCollectionPop()
{
  // This must be called only from the main thread.
  assert(vtkGarbageCollectorIsMainThread());

  // Forward the call to the singleton.
  if(vtkGarbageCollectorSingletonInstance)
    {
    vtkGarbageCollectorSingletonInstance->DeferredCollectionPop();
    }
}

//----------------------------------------------------------------------------
int vtkGarbageCollector::GiveReference(vtkObjectBase* obj)
{
  // We must have an object.
  assert(obj != 0);

  // See if the singleton will accept a reference.
  if(vtkGarbageCollectorIsMainThread() &&
     vtkGarbageCollectorSingletonInstance)
    {
    return vtkGarbageCollectorSingletonInstance->GiveReference(obj);
    }

  // Could not accept the reference.
  return 0;
}

//----------------------------------------------------------------------------
int vtkGarbageCollector::TakeReference(vtkObjectBase* obj)
{
  // We must have an object.
  assert(obj != 0);

  // See if the singleton has a reference.
  if(vtkGarbageCollectorIsMainThread() &&
     vtkGarbageCollectorSingletonInstance)
    {
    return vtkGarbageCollectorSingletonInstance->TakeReference(obj);
    }

  // No reference is available.
  return 0;
}

//----------------------------------------------------------------------------
vtkGarbageCollectorSingleton::vtkGarbageCollectorSingleton()
{
  this->TotalNumberOfReferences = 0;
  this->DeferredCollectionCount = 0;
}

//----------------------------------------------------------------------------
vtkGarbageCollectorSingleton::~vtkGarbageCollectorSingleton()
{
  // There should be no deferred collections left.
  assert(this->TotalNumberOfReferences == 0);
}

//----------------------------------------------------------------------------
int vtkGarbageCollectorSingleton::GiveReference(vtkObjectBase* obj)
{
  // Check if we can store a reference to the object in the map.
  if(this->CheckAccept())
    {
    // Create a reference to the object.
    ReferencesType::iterator i = this->References.find(obj);
    if(i == this->References.end())
      {
      // This is a new object.  Create a map entry for it.
      this->References.insert(ReferencesType::value_type(obj, 1));
      }
    else
      {
      ++i->second;
      }
    ++this->TotalNumberOfReferences;
    return 1;
    }

  // We did not accept the reference.
  return 0;
}

//----------------------------------------------------------------------------
int vtkGarbageCollectorSingleton::TakeReference(vtkObjectBase* obj)
{
  // If we have a reference to the object hand it back to the caller.
  ReferencesType::iterator i = this->References.find(obj);
  if(i != this->References.end())
    {
    // Remove our reference to the object.
    --this->TotalNumberOfReferences;
    if(--i->second == 0)
      {
      // If we have no more references to the object, remove its map
      // entry.
      this->References.erase(i);
      }
    return 1;
    }

  // We do not have a reference to the object.
  return 0;
}

//----------------------------------------------------------------------------
int vtkGarbageCollectorSingleton::CheckAccept()
{
  // Accept the reference only if deferred collection is enabled.  It
  // is tempting to put a check against TotalNumberOfReferences here
  // to collect every so many deferred calls, but this will NOT work.
  // Some objects call UnRegister on other objects during
  // construction.  We do not want to perform deferred collection
  // while an object is under construction because the reference walk
  // might call ReportReferences on a partially constructed object!
  return this->DeferredCollectionCount > 0;
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorSingleton::DeferredCollectionPush()
{
  if(++this->DeferredCollectionCount <= 0)
    {
    // Deferred collection is disabled.  Collect immediately.
    vtkGarbageCollector::Collect();
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorSingleton::DeferredCollectionPop()
{
  if(--this->DeferredCollectionCount <= 0)
    {
    // Deferred collection is disabled.  Collect immediately.
    vtkGarbageCollector::Collect();
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorReportInternal(vtkGarbageCollector* collector,
                                       vtkObjectBase* obj, void* ptr,
                                       const char* desc)
{
  collector->Report(obj, ptr, desc);
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorReport(vtkGarbageCollector* collector,
                               vtkSmartPointerBase& ptr,
                               const char* desc)
{
  ptr.Report(collector, desc);
}
