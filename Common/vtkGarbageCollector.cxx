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

#include <vtkstd/set>
#include <vtkstd/queue>

vtkCxxRevisionMacro(vtkGarbageCollector, "1.2");

//----------------------------------------------------------------------------
class vtkGarbageCollectorQueue: public vtkstd::queue<vtkObjectBase*> {};
class vtkGarbageCollectorQueued: public vtkstd::set<vtkObjectBase*> {};

//----------------------------------------------------------------------------
vtkGarbageCollector::vtkGarbageCollector(vtkGarbageCollectorQueue* q,
                                         vtkGarbageCollectorQueued* qd)
{
  this->Queue = q;
  this->Queued = qd;
  this->NetCount = 0;
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
  // Allocate as much as possible on the stack.  This code runs every
  // time UnRegister is called on an object supporting garbage
  // collection.  It may be worth modifying the queue and set to use a
  // local array for storage until the size grows beyond some
  // constant.
  vtkGarbageCollectorQueue objectQueue;
  vtkGarbageCollectorQueued objectQueued;
  vtkGarbageCollector collector(&objectQueue, &objectQueued);
  collector.CheckReferenceLoops(root);
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::CheckReferenceLoops(vtkObjectBase* root)
{
  // This performs a BFS traversal of the reference graph associated
  // with the given root object.  If the total reference count of the
  // connected component is 0 when not counting internal references,
  // the entire component is deleted.

  // Fake an internal reference to the root to initialize the queue.
  this->Queued->clear();
  this->NetCount = 1;
  this->ReportReference(root);

  // Loop until the queue is empty.
  while(!this->Queue->empty())
    {
    // Get the next object from the queue.
    vtkObjectBase* obj = this->Queue->front();
    this->Queue->pop();

    // Tell the object to report its references to us.
    obj->ReportReferences(this);
    }

  // If the net reference count is 0, delete the entire connected
  // component of the reference graph.
  if(this->NetCount == 0)
    {
    // Report the connected component to be deleted.
    vtkstd::set<vtkObjectBase*>::iterator obj;
#ifndef VTK_LEAN_AND_MEAN
    if(this->Debug && vtkObject::GetGlobalWarningDisplay())
      {
      ostrstream msg;
      msg << "Deleting connected component of reference graph:\n";
      for(obj = this->Queued->begin(); obj != this->Queued->end(); ++obj)
        {
        msg << "  " << (*obj)->GetClassName() << "(" << *obj << ")\n";
        }
      msg << ends;
      vtkDebugMacro(<< msg.str());
      msg.rdbuf()->freeze(0);
      }
#endif

    // Make sure this garbage collector is not deleted.
    this->Register(this);

    // Notify all objects they are about to be garbage collected.
    // They will disable reference loop checking.
    for(obj = this->Queued->begin(); obj != this->Queued->end(); ++obj)
      {
      (*obj)->GarbageCollectionStarting();
      }

    // Disconnect the reference graph.
    for(obj = this->Queued->begin(); obj != this->Queued->end(); ++obj)
      {
      (*obj)->RemoveReferences();
      }

    // Notify all objects they have been garbage collected.  They will
    // delete themselves.
    for(obj = this->Queued->begin(); obj != this->Queued->end(); ++obj)
      {
      (*obj)->GarbageCollectionFinishing();
      }

    // It is now safe to delete this garbage collector.
    this->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkGarbageCollector::ReportReference(vtkObjectBase* obj)
{
  if(obj)
    {
    // Add the object to the queue if it has not already been added.
    if(this->Queued->find(obj) == this->Queued->end())
      {
      // Include the references to the object.
      this->NetCount += obj->GetReferenceCount();

      // Queue the object.
      this->Queued->insert(obj);
      this->Queue->push(obj);
      }

    // This is an internal reference.
    --this->NetCount;
    }
}
