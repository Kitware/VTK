/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGarbageCollector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGarbageCollector - Detect and break reference loops
// .SECTION Description
// vtkGarbageCollector is used by VTK classes that may be involved in
// reference counting loops (such as Source <-> Output).  It detects
// connected components of the reference graph that have been
// disconnected from the main graph and deletes them.  Objects that
// use it call CheckReferenceLoops from their UnRegister method and
// pass themselves as the root for a search.  The garbage collector
// then uses the ReportReferences method to search the reference graph
// and construct a net reference count for the object's connected
// component.  If the net reference count is zero, RemoveReferences is
// called on all objects to break references and the entire set of
// objects is then deleted.
//
// To enable garbage collection for a class, add these members:
//
//  public:
//   virtual void Register(vtkObjectBase* o)
//     {
//     this->RegisterInternal(o, this->GarbageCollectionCheck);
//     }
//   virtual void UnRegister(vtkObjectBase* o)
//     {
//     this->UnRegisterInternal(o, this->GarbageCollectionCheck);
//     }
//
//  protected:
//   // Initialize to 1 in the constructor.
//   int GarbageCollectionCheck;
//
//   virtual void ReportReferences(vtkGarbageCollector* collector)
//     {
//     // Report references held by this object that may be in a loop.
//     this->Superclass::ReportReferences(collector);
//     collector->ReportReference(this->OtherObject);
//     }
//
//   virtual void RemoveReferences()
//     {
//     // Remove references to objects reported in ReportReferences.
//     if(this->OtherObject)
//       {
//       this->OtherObject->UnRegister(this);
//       this->OtherObject = 0;
//       }
//     this->Superclass::RemoveReferences();
//     }
//
//   virtual void GarbageCollectionStarting()
//     {
//     this->GarbageCollectionCheck = 0;
//     this->Superclass::GarbageCollectionStarting();
//     }
//
// The implementations should be in the .cxx file in practice.
//
// If subclassing from a class that already supports garbage
// collection, one need only provide the ReportReferences and
// RemoveReferences methods.

#ifndef __vtkGarbageCollector_h
#define __vtkGarbageCollector_h

#include "vtkObject.h"
#include "vtkGarbageCollectorManager.h" // Needed for singleton initialization.

class vtkGarbageCollectorInternals;

class VTK_COMMON_EXPORT vtkGarbageCollector : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkGarbageCollector,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Check for a strongly connected component in the reference graph
  // starting at the given object.  If the net reference count of the
  // component is zero the component is deleted.
  static void Check(vtkObjectBase* root);

  // Description:
  // Called by UnRegister method of an object that supports garbage
  // collection.  The UnRegister may not actually decrement the
  // reference count, but instead hands the reference to the garbage
  // collector.  If a reference can be given, this method accepts it
  // from the caller by returning 1.  If the reference cannot be
  // accepted then it returns 0.  This may be the case when delayed
  // garbage collection is disabled, or when the collector has decided
  // it is time to do a check.
  static int GiveReference(vtkObjectBase* obj);

  // Description:
  // Called by Register method of an object that supports garbage
  // collection.  The Register may not actually increment the
  // reference count if it can take a reference previously handed to
  // the garbage collector.  If a reference can be taken, this method
  // hands it back to the caller by returning 1.  If no reference is
  // available, returns 0.
  static int TakeReference(vtkObjectBase* obj);

  // Description:
  // Called by the ReportReferences method of objects in a reference
  // graph to report an outgoing connection.  The first argument
  // should point to the reported reference is made.  The second
  // argument should be a brief description of how the reference is
  // made for use in debugging reference loops.
  void ReportReference(vtkObjectBase*, const char*);

  // Description:
  // Set/Get global garbage collection debugging flag.  When set to 1,
  // all garbage collection checks will produce debugging information.
  static void SetGlobalDebugFlag(int flag);
  static int GetGlobalDebugFlag();
protected:
  vtkGarbageCollector(vtkGarbageCollectorInternals*);
  ~vtkGarbageCollector();

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void Register(vtkObjectBase*);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void UnRegister(vtkObjectBase*);

  // Forward call to given object.
  void ForwardReportReferences(vtkObjectBase*);
  static void ForwardRemoveReferences(vtkObjectBase*);
  static void ForwardGarbageCollectionStarting(vtkObjectBase*);
  static void ForwardGarbageCollectionFinishing(vtkObjectBase*);

  // Forward call to the internal implementation.
  void CheckReferenceLoops(vtkObjectBase* root);

private:
  // Internal implementation details.
  vtkGarbageCollectorInternals* Internal;

  // Singleton management functions.
  static void ClassInitialize();
  static void ClassFinalize();

  //BTX
  friend class vtkGarbageCollectorManager;
  //ETX

  //BTX
  friend class vtkGarbageCollectorInternals;
  //ETX
private:
  vtkGarbageCollector(const vtkGarbageCollector&);  // Not implemented.
  void operator=(const vtkGarbageCollector&);  // Not implemented.
};

#endif
