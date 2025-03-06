// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGarbageCollector
 * @brief   Detect and break reference loops
 *
 * vtkGarbageCollector is used by VTK classes that may be involved in
 * reference counting loops (such as Algorithm <-> Executive).  It
 * detects strongly connected components of the reference graph that
 * have been leaked deletes them.  The garbage collector uses the
 * ReportReferences method to search the reference graph and construct
 * a net reference count for each connected component.  If the net
 * reference count is zero the entire set of objects is deleted.
 * Deleting each component may leak other components, which are then
 * collected recursively.
 *
 * To enable garbage collection for a class, add these members:
 *
 * \code
 *
 *  public:
 *   bool UsesGarbageCollector() const override { return true; }
 *
 *  protected:
 *
 *   void ReportReferences(vtkGarbageCollector* collector) override
 *   {
 *     // Report references held by this object that may be in a loop.
 *     this->Superclass::ReportReferences(collector);
 *     vtkGarbageCollectorReport(collector, this->OtherObject, "Other Object");
 *   }
 * \endcode
 *
 * The implementations should be in the .cxx file in practice.
 * It is important that the reference be reported using the real
 * pointer or smart pointer instance that holds the reference.  When
 * collecting the garbage collector will actually set this pointer to
 * nullptr.  The destructor of the class should be written to deal with
 * this.  It is also expected that an invariant is maintained for any
 * reference that is reported.  The variable holding the reference
 * must always either be nullptr or refer to a fully constructed valid
 * object.  Therefore code like "this->Object->UnRegister(this)" must
 * be avoided if "this->Object" is a reported reference because it
 * is possible that the object is deleted before UnRegister returns
 * but then "this->Object" will be left as a dangling pointer.  Instead
 * use code like
 *
 * \code
 *   vtkObjectBase* obj = this->Object;
 *   this->Object = 0;
 *   obj->UnRegister(this);
 * \endcode
 *
 * so that the reported reference maintains the invariant.
 *
 * If subclassing from a class that already supports garbage
 * collection, one need only provide the ReportReferences method.
 */

#ifndef vtkGarbageCollector_h
#define vtkGarbageCollector_h

#include "vtkCommonCoreModule.h"        // For export macro
#include "vtkGarbageCollectorManager.h" // Needed for singleton initialization.
#include "vtkObject.h"

// This function is a friend of the collector so that it can call the
// internal report method.
VTK_ABI_NAMESPACE_BEGIN
void VTKCOMMONCORE_EXPORT vtkGarbageCollectorReportInternal(
  vtkGarbageCollector*, vtkObjectBase*, void*, const char*);

// This forward ref allows us to define methods with vtkNew.
template <class T>
class vtkNew;

// This allows vtkObjectBase to get at the methods it needs.
class vtkObjectBaseToGarbageCollectorFriendship;

class VTKCOMMONCORE_EXPORT vtkGarbageCollector : public vtkObject
{
public:
  vtkTypeMacro(vtkGarbageCollector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkGarbageCollector* New();

  /**
   * Collect immediately using any objects whose collection was
   * previously deferred as a root for the reference graph walk.
   * Strongly connected components in the reference graph are
   * identified.  Those with a net reference count of zero are
   * deleted.  When a component is deleted it may remove references to
   * other components that are not part of the same reference loop but
   * are held by objects in the original component.  These removed
   * references are handled as any other and their corresponding
   * checks may be deferred.  This method keeps collecting until no
   * deferred collection checks remain.
   */
  static void Collect();

  /**
   * Collect immediately using the given object as the root for a
   * reference graph walk.  Strongly connected components in the
   * reference graph are identified.  Those with a net reference count
   * of zero are deleted.  When a component is deleted it may remove
   * references to other components that are not part of the same
   * reference loop but are held by objects in the original component.
   * These removed references are handled as any other and their
   * corresponding checks may be deferred.  This method does continue
   * collecting in this case.
   */
  static void Collect(vtkObjectBase* root);

  ///@{
  /**
   * Push/Pop whether to do deferred collection.  Whenever the total
   * number of pushes exceeds the total number of pops collection will
   * be deferred.  Code can call the Collect method directly to force
   * collection.
   */
  static void DeferredCollectionPush();
  static void DeferredCollectionPop();
  ///@}

  ///@{
  /**
   * Set/Get global garbage collection debugging flag.  When set to true,
   * all garbage collection checks will produce debugging information.
   */
  static void SetGlobalDebugFlag(bool flag);
  static bool GetGlobalDebugFlag();
  ///@}

protected:
  vtkGarbageCollector();
  ~vtkGarbageCollector() override;

private:
  /**
   * Called by UnRegister method of an object that supports garbage
   * collection.  The UnRegister may not actually decrement the
   * reference count, but instead hands the reference to the garbage
   * collector.  If a reference can be given, this method accepts it
   * from the caller by returning 1.  If the reference cannot be
   * accepted then it returns 0.  This may be the case when delayed
   * garbage collection is disabled, or when the collector has decided
   * it is time to do a check.
   */
  static vtkTypeBool GiveReference(vtkObjectBase* obj);

  /**
   * Called by Register method of an object that supports garbage
   * collection.  The Register may not actually increment the
   * reference count if it can take a reference previously handed to
   * the garbage collector.  If a reference can be taken, this method
   * hands it back to the caller by returning 1.  If no reference is
   * available, returns 0.
   */
  static vtkTypeBool TakeReference(vtkObjectBase* obj);

  // Singleton management functions.
  static void ClassInitialize();
  static void ClassFinalize();

  friend class vtkGarbageCollectorManager;
  friend class vtkObjectBaseToGarbageCollectorFriendship;

  // Internal report callback and friend function that calls it.
  virtual void Report(vtkObjectBase* obj, void* ptr, const char* desc);
  friend void VTKCOMMONCORE_EXPORT vtkGarbageCollectorReportInternal(
    vtkGarbageCollector*, vtkObjectBase*, void*, const char*);

  vtkGarbageCollector(const vtkGarbageCollector&) = delete;
  void operator=(const vtkGarbageCollector&) = delete;
};

class vtkSmartPointerBase;

/**
 * Function to report a reference held by a smart pointer to a collector.
 */
void VTKCOMMONCORE_EXPORT vtkGarbageCollectorReport(
  vtkGarbageCollector* collector, vtkSmartPointerBase& ptr, const char* desc);

/**
 * Function to report a reference held by a vtkNew to a collector.
 */
template <class T>
void vtkGarbageCollectorReport(vtkGarbageCollector* collector, vtkNew<T>& ptr, const char* desc)
{
  vtkGarbageCollectorReportInternal(collector, ptr.Object, &ptr.Object, desc);
}

/**
 * Function to report a reference held by a raw pointer to a collector.
 */
template <class T>
void vtkGarbageCollectorReport(vtkGarbageCollector* collector, T*& ptr, const char* desc)
{
  vtkGarbageCollectorReportInternal(collector, ptr, &ptr, desc);
}

VTK_ABI_NAMESPACE_END
#endif
