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
// reference counting loops (such as Algorithm <-> Executive).  It
// detects strongly connected components of the reference graph that
// have been leaked deletes them.  The garbage collector uses the
// ReportReferences method to search the reference graph and construct
// a net reference count for each connected component.  If the net
// reference count is zero the entire set of objects is deleted.
// Deleting each component may leak other components, which are then
// collected recursively.
//
// To enable garbage collection for a class, add these members:
//
//  public:
//   virtual void Register(vtkObjectBase* o)
//     {
//     this->RegisterInternal(o, 1);
//     }
//   virtual void UnRegister(vtkObjectBase* o)
//     {
//     this->UnRegisterInternal(o, 1);
//     }
//
//  protected:
//
//   virtual void ReportReferences(vtkGarbageCollector* collector)
//     {
//     // Report references held by this object that may be in a loop.
//     this->Superclass::ReportReferences(collector);
//     vtkGarbageCollectorReport(collector, this->OtherObject, "Other Object");
//     }
//
// It is important that the reference be reported using the real
// pointer or smart pointer instance that holds the reference.  When
// collecting the garbage collector will actually set this pointer to
// NULL.  The destructor of the class should be written to deal with
// this.
//
// The implementations should be in the .cxx file in practice.
//
// If subclassing from a class that already supports garbage
// collection, one need only provide the ReportReferences method.

#ifndef __vtkGarbageCollector_h
#define __vtkGarbageCollector_h

#include "vtkObject.h"
#include "vtkGarbageCollectorManager.h" // Needed for singleton initialization.

// This function is a friend of the collector so that it can call the
// internal report method.
void VTK_COMMON_EXPORT
vtkGarbageCollectorReportInternal(vtkGarbageCollector*,
                                  vtkObjectBase*, void*,
                                  const char*);

class VTK_COMMON_EXPORT vtkGarbageCollector : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkGarbageCollector,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Collect immediately and include the given object in the reference
  // graph walk.  This also accounts for any deferred collection
  // checks.  Strongly connected components in the reference graph are
  // identified.  Those with a net reference count of zero are
  // deleted.  When a component is deleted it may remove references to
  // other components that are not part of the same reference loop but
  // are held by objects in the original component.  These removed
  // references are handled as any other and their corresponding
  // checks may be deferred.  This method keeps collecting until no
  // deferred collection checks remain.
  static void Collect();

  // Description:
  // Collect immediately and include the given object in the reference
  // graph walk.  This also accounts for any deferred collection
  // checks.  Strongly connected components in the reference graph are
  // identified.  Those with a net reference count of zero are
  // deleted.  When a component is deleted it may remove references to
  // other components that are not part of the same reference loop but
  // are held by objects in the original component.  These removed
  // references are handled as any other and their corresponding
  // checks may be deferred.  This method does continue collecting in
  // this case.
  static void Collect(vtkObjectBase* root);

  // Description:
  // Get/Set the maximum number of deferred collection checks.  The
  // default is zero, which means collection always occurs immediately
  // when a reference is removed from an object.  Decreasing the
  // number may result in an immediate collection if the number of
  // deferred checks is larger than the new value.  Setting to a
  // negative number will allow an unlimited number of checks to be
  // deferred.
  static int GetDeferredCollectionLimit();
  static void SetDeferredCollectionLimit(int limit);

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
  // Set/Get global garbage collection debugging flag.  When set to 1,
  // all garbage collection checks will produce debugging information.
  static void SetGlobalDebugFlag(int flag);
  static int GetGlobalDebugFlag();

protected:
  vtkGarbageCollector();
  ~vtkGarbageCollector();

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void Register(vtkObjectBase*);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void UnRegister(vtkObjectBase*);

private:

  // Singleton management functions.
  static void ClassInitialize();
  static void ClassFinalize();

  //BTX
  friend class vtkGarbageCollectorManager;
  //ETX

  // Internal report callback and friend function that calls it.
  virtual void Report(vtkObjectBase* obj, void* ptr, const char* desc)=0;
  friend void VTK_COMMON_EXPORT
  vtkGarbageCollectorReportInternal(vtkGarbageCollector*,
                                    vtkObjectBase*, void*,
                                    const char*);

private:
  vtkGarbageCollector(const vtkGarbageCollector&);  // Not implemented.
  void operator=(const vtkGarbageCollector&);  // Not implemented.
};

//BTX
class vtkSmartPointerBase;

// Description:
// Function to report a reference held by a smart pointer to a collector.
void VTK_COMMON_EXPORT
vtkGarbageCollectorReport(vtkGarbageCollector* collector,
                          vtkSmartPointerBase& ptr,
                          const char* desc);

// Description:
// Function to report a reference held by a raw pointer to a collector.
template <class T>
void vtkGarbageCollectorReport(vtkGarbageCollector* collector, T*& ptr,
                               const char* desc)
{
  vtkGarbageCollectorReportInternal(collector, ptr, &ptr, desc);
}
//ETX

#endif
