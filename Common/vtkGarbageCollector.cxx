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

#include <vtkstd/map>
#include <vtkstd/queue>
#include <vtkstd/stack>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkGarbageCollector, "1.8");

//----------------------------------------------------------------------------
class vtkGarbageCollectorInternals
{
public:
  struct Entry;

  // Map from each object to its garbage collection entry.
  typedef vtkstd::map<vtkObjectBase*, Entry> EntriesType;
  struct Entry
  {
    Entry(): Root(), InComponent(0), VisitOrder(0), Queued(0), References() {}

    // The candidate root for the component containing this object.
    EntriesType::iterator Root;

    // Mark whether the object has been assigned to a component.
    int InComponent;

    // Mark the order in which object's are visited by Tarjan's algorithm.
    int VisitOrder;

    // Whether this entry has been queued while computing net reference count.
    int Queued;

    // The list of references reported by this entry's object.
    typedef vtkstd::vector<EntriesType::iterator> ReferencesType;
    ReferencesType References;
  };

  // The main garbage collector instance.
  vtkGarbageCollector* External;

  // The set of objects that have been visited during the DFS.
  EntriesType Entries;

  // The stack of objects forming the connected components.
  typedef vtkstd::stack<EntriesType::iterator> StackType;
  StackType Stack;

  // The object currently being explored.
  EntriesType::iterator Current;

  // Count for visit order.
  int Count;

  // The objects in the root's connected component.
  typedef vtkstd::vector<vtkObjectBase*> ComponentType;
  ComponentType Component;

  // Find the strongly connected components reachable from the given root.
  EntriesType::iterator FindStrongComponents(vtkObjectBase*);

  // Callback from objects to report references.
  void ReportReference(vtkObjectBase*);

  // Node visitor for Tarjan's algorithm.
  EntriesType::iterator VisitTarjan(vtkObjectBase*);

  // Find/Print/Delete the set of objects in the root's strongly
  // connected component.
  int FindComponent(EntriesType::iterator);
  void PrintComponent(ostream& os);
  void DeleteComponent();
};

//----------------------------------------------------------------------------
vtkGarbageCollector::vtkGarbageCollector(vtkGarbageCollectorInternals* internal)
{
  this->Internal = internal;
  this->Internal->External = this;
  this->Internal->Current = this->Internal->Entries.end();
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
void vtkGarbageCollector::Register(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::UnRegister(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::Check(vtkObjectBase* root)
{
  // Allocate as much as possible on the runtime stack.  This code
  // runs every time UnRegister is called on an object supporting
  // garbage collection.  It may be worth modifying the stack and map
  // to use a local array for storage until the size grows beyond some
  // constant.
  vtkGarbageCollectorInternals internal;
  vtkGarbageCollector collector(&internal);

  // Uncomment these lines to get reference loop debugging for objects
  // with Debug flag set:
  //if(vtkObject* obj = vtkObject::SafeDownCast(root))
  //  {
  //  collector.SetDebug(obj->GetDebug());
  //  }

  collector.SetDebug(1);

  // Do collection if necessary.
  collector.CheckReferenceLoops(root);

  // Avoid destruction message.
  collector.SetDebug(0);
}

//----------------------------------------------------------------------------
#ifdef VTK_LEAN_AND_MEAN
void vtkGarbageCollector::ReportReference(vtkObjectBase* obj, const char*)
{
  // Forward call to the internal implementation.
  if(obj)
    {
    this->Internal->ReportReference(obj);
    }
}
#else
void vtkGarbageCollector::ReportReference(vtkObjectBase* obj, const char* desc)
{
  if(obj)
    {
    // Report debugging information if requested.
    if(this->Debug && vtkObject::GetGlobalWarningDisplay())
      {
      vtkObjectBase* current = this->Internal->Current->first;
      ostrstream msg;
      msg << "ReportReference: "
          << current->GetClassName() << "(" << current << ") "
          << (desc?desc:"")
          << " -> " << obj->GetClassName() << "(" << obj << ")";
      msg << ends;
      vtkDebugMacro(<< msg.str());
      msg.rdbuf()->freeze(0);
      }

    // Forward call to the internal implementation.
    this->Internal->ReportReference(obj);
    }
}
#endif

//----------------------------------------------------------------------------
void vtkGarbageCollector::ReportReferences(vtkObjectBase* obj)
{
#ifndef VTK_LEAN_AND_MEAN
  // Report debugging information if requested.
  if(this->Debug && vtkObject::GetGlobalWarningDisplay())
    {
    vtkObjectBase* current = this->Internal->Current->first;
    vtkDebugMacro("Requesting references from "
                  << current->GetClassName() << "("
                  << current << ") with reference count "
                  << current->GetReferenceCount());
    }
#endif
  obj->ReportReferences(this);
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::RemoveReferences(vtkObjectBase* obj)
{
  obj->RemoveReferences();
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::GarbageCollectionStarting(vtkObjectBase* obj)
{
  obj->GarbageCollectionStarting();
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::GarbageCollectionFinishing(vtkObjectBase* obj)
{
  obj->GarbageCollectionFinishing();
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::CheckReferenceLoops(vtkObjectBase* root)
{
  // This traverses the reference graph associated with the given root
  // object.  If the total reference count of the strongly connected
  // component is 0 when not counting internal references, the entire
  // component is deleted.

  vtkDebugMacro("Starting reference graph walk with root "
                << root->GetClassName() << "(" << root << ")");

  // Find the strongly connected components reachable from this root.
  vtkGarbageCollectorInternals::EntriesType::iterator
    rootEntry = this->Internal->FindStrongComponents(root);

  vtkDebugMacro("Finished reference graph walk with root "
                << root->GetClassName() << "(" << root << ")");

  // Find the net reference count of the component containing the root.
  int netCount = this->Internal->FindComponent(rootEntry);

#ifndef VTK_LEAN_AND_MEAN
  if(this->Debug && vtkObject::GetGlobalWarningDisplay())
    {
    ostrstream msg;
    msg << "Identified strongly connected component with net reference count "
        << netCount << ":";
    this->Internal->PrintComponent(msg);
    msg << ends;
    vtkDebugMacro(<< msg.str());
    msg.rdbuf()->freeze(0);
    }
#endif

  // If the net reference count is zero, delete the component.
  if(netCount == 0)
    {
    vtkDebugMacro("Deleting strongly connected component of reference graph.");
    this->Internal->DeleteComponent();
    }
}

//----------------------------------------------------------------------------
int vtkGarbageCollectorInternals::FindComponent(EntriesType::iterator root)
{
  // The queue of objects while checking the net reference count.
  typedef vtkstd::queue<EntriesType::iterator> QueueType;
  QueueType entryQueue;

  // Initialize the queue with the root object.
  int netCount = root->first->GetReferenceCount();
  this->Component.push_back(root->first);
  root->second.Queued = 1;
  entryQueue.push(root);

  // Loop until the queue is empty.
  while(!entryQueue.empty())
    {
    // Get the next object from the queue.
    EntriesType::iterator v = entryQueue.front();
    entryQueue.pop();

    // Process the references to objects in the component.
    for(Entry::ReferencesType::iterator r = v->second.References.begin();
        r != v->second.References.end(); ++r)
      {
      EntriesType::iterator w = *r;
      if(w->second.Root == root)
        {
        if(!w->second.Queued)
          {
          // Include the references to this object.
          netCount += w->first->GetReferenceCount();

          // Add the object to the list of objects in the component.
          this->Component.push_back(w->first);

          // Queue the object.
          w->second.Queued = 1;
          entryQueue.push(w);
          }

        // This is an internal reference, so decrement the net count.
        --netCount;
        }
      }
    }

  return netCount;
}

//----------------------------------------------------------------------------
vtkGarbageCollectorInternals::EntriesType::iterator
vtkGarbageCollectorInternals::FindStrongComponents(vtkObjectBase* root)
{
  // Use Tarjan's algorithm to visit the reference graph and mark
  // strongly connected components.
  this->Count = 0;
  return this->VisitTarjan(root);
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorInternals::ReportReference(vtkObjectBase* obj)
{
  // Get the source and destination of this reference.
  EntriesType::iterator v = this->Current;
  EntriesType::iterator w = this->Entries.find(obj);

  // Visit the destination of this reference if it has not been visited.
  if(w == this->Entries.end())
    {
    w = this->VisitTarjan(obj);
    }

  // If the destination has not yet been assigned to a component,
  // check if it is a better potential root for the current object.
  if(!w->second.InComponent)
    {
    if(w->second.Root->second.VisitOrder < v->second.Root->second.VisitOrder)
      {
      v->second.Root = w->second.Root;
      }
    }

  // Save this reference.
  v->second.References.push_back(w);
}

//----------------------------------------------------------------------------
vtkGarbageCollectorInternals::EntriesType::iterator
vtkGarbageCollectorInternals::VisitTarjan(vtkObjectBase* obj)
{
  // Create an entry for the object.
  EntriesType::value_type entry(obj, Entry());
  EntriesType::iterator v = this->Entries.insert(entry).first;

  // Initialize the entry and push it onto the stack of graph nodes.
  v->second.Root = v;
  v->second.InComponent = 0;
  v->second.VisitOrder = ++this->Count;
  this->Stack.push(v);

  // Process the references from this node.
  EntriesType::iterator saveCurrent = this->Current;
  this->Current = v;
  this->External->ReportReferences(v->first);
  this->Current = saveCurrent;

  // If we have found a component mark its members.
  if(v->second.Root == v)
    {
    EntriesType::iterator w;
    do
      {
      w = this->Stack.top();
      this->Stack.pop();
      w->second.InComponent = 1;
      w->second.Root = v;
      } while(w != v);
    }

  return v;
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorInternals::PrintComponent(ostream& os)
{
  for(ComponentType::iterator i = this->Component.begin();
      i != this->Component.end(); ++i)
    {
    os << "\n  " << (*i)->GetClassName() << "(" << *i << ")";
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollectorInternals::DeleteComponent()
{
  ComponentType::iterator obj;

  // Notify all objects they are about to be garbage collected.
  // They will disable reference loop checking.
  for(obj = this->Component.begin(); obj != this->Component.end(); ++obj)
    {
    vtkGarbageCollector::GarbageCollectionStarting(*obj);
    }

  // Disconnect the reference graph.
  for(obj = this->Component.begin(); obj != this->Component.end(); ++obj)
    {
    vtkGarbageCollector::RemoveReferences(*obj);
    }

  // Notify all objects they have been garbage collected.  They will
  // delete themselves.
  for(obj = this->Component.begin(); obj != this->Component.end(); ++obj)
    {
    vtkGarbageCollector::GarbageCollectionFinishing(*obj);
    }
}
