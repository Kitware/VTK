// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkObjectBase
 * @brief   abstract base class for most VTK objects
 *
 * vtkObjectBase is the base class for all reference counted classes
 * in the VTK. These classes include vtkCommand classes, vtkInformationKey
 * classes, and vtkObject classes.
 *
 * vtkObjectBase performs reference counting: objects that are
 * reference counted exist as long as another object uses them. Once
 * the last reference to a reference counted object is removed, the
 * object will spontaneously destruct.
 *
 * Constructor and destructor of the subclasses of vtkObjectBase
 * should be protected, so that only New() and UnRegister() actually
 * call them. Debug leaks can be used to see if there are any objects
 * left with nonzero reference count.
 *
 * @warning
 * Note: Objects of subclasses of vtkObjectBase should always be
 * created with the New() method and deleted with the Delete()
 * method. They cannot be allocated off the stack (i.e., automatic
 * objects) because the constructor is a protected method.
 *
 * @sa
 * vtkObject vtkCommand vtkInformationKey
 */

#ifndef vtkObjectBase_h
#define vtkObjectBase_h

// Semantics around vtkDebugLeaks usage has changed. Now just call
// vtkObjectBase::InitializeObjectBase() after creating an object with New().
// The object factory methods take care of this automatically.
#define VTK_HAS_INITIALIZE_OBJECT_BASE

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDeprecation.h"
#include "vtkFeatures.h" // for VTK_USE_MEMKIND
#include "vtkIndent.h"
#include "vtkSystemIncludes.h"
#include "vtkType.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <atomic> // For std::atomic
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkGarbageCollector;
class vtkGarbageCollectorToObjectBaseFriendship;
class vtkWeakPointerBase;
class vtkWeakPointerBaseToObjectBaseFriendship;

// typedefs for malloc and free compatible replacement functions
typedef void* (*vtkMallocingFunction)(size_t);
typedef void* (*vtkReallocingFunction)(void*, size_t);
typedef void (*vtkFreeingFunction)(void*);

class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkObjectBase
{
  /**
   * Return the class name as a string. This method is overridden
   * in all subclasses of vtkObjectBase with the vtkTypeMacro found
   * in vtkSetGet.h.
   */
  virtual const char* GetClassNameInternal() const { return "vtkObjectBase"; }

public:
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
// Avoid windows name mangling.
#define GetClassNameA GetClassName
#define GetClassNameW GetClassName
#endif

  /**
   * Return the class name as a string.
   */
  VTK_MARSHALGETTER(ClassName)
  const char* GetClassName() const;

  /**
   * The object description printed in messages and PrintSelf
   * output. To be used only for reporting purposes.
   */
  virtual std::string GetObjectDescription() const;

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#undef GetClassNameW
#undef GetClassNameA

  // Define possible mangled names.
  const char* GetClassNameA() const;
  const char* GetClassNameW() const;

#endif

  /**
   * Return 1 if this class type is the same type of (or a subclass of)
   * the named class. Returns 0 otherwise. This method works in
   * combination with vtkTypeMacro found in vtkSetGet.h.
   */
  static vtkTypeBool IsTypeOf(const char* name);

  /**
   * Return 1 if this class is the same type of (or a subclass of)
   * the named class. Returns 0 otherwise. This method works in
   * combination with vtkTypeMacro found in vtkSetGet.h.
   */
  virtual vtkTypeBool IsA(const char* name);

  /**
   * Given a the name of a base class of this class type, return the distance
   * of inheritance between this class type and the named class (how many
   * generations of inheritance are there between this class and the named
   * class). If the named class is not in this class's inheritance tree, return
   * a negative value. Valid responses will always be nonnegative. This method
   * works in combination with vtkTypeMacro found in vtkSetGet.h.
   */
  static vtkIdType GetNumberOfGenerationsFromBaseType(const char* name);

  /**
   * Given the name of a base class of this class type, return the distance
   * of inheritance between this class type and the named class (how many
   * generations of inheritance are there between this class and the named
   * class). If the named class is not in this class's inheritance tree, return
   * a negative value. Valid responses will always be nonnegative. This method
   * works in combination with vtkTypeMacro found in vtkSetGet.h.
   */
  virtual vtkIdType GetNumberOfGenerationsFromBase(const char* name);

  /**
   * Delete a VTK object.  This method should always be used to delete
   * an object when the New() method was used to create it. Using the
   * C++ delete method will not work with reference counting.
   */
  virtual void Delete();

  /**
   * Delete a reference to this object.  This version will not invoke
   * garbage collection and can potentially leak the object if it is
   * part of a reference loop.  Use this method only when it is known
   * that the object has another reference and would not be collected
   * if a full garbage collection check were done.
   */
  virtual void FastDelete();

  /**
   * Create an object with Debug turned off, modified time initialized
   * to zero, and reference counting on.
   */
  static vtkObjectBase* New()
  {
    vtkObjectBase* o = new vtkObjectBase;
    o->InitializeObjectBase();
    return o;
  }

  // Called by implementations of vtkObject::New(). Centralized location for
  // vtkDebugLeaks registration.
  void InitializeObjectBase();

#if defined(_WIN32) || defined(VTK_USE_MEMKIND)
  // Take control of allocation to avoid dll boundary problems or to use memkind.
  void* operator new(size_t tSize);
  void operator delete(void* p);
#endif

  /**
   * Print an object to an ostream. This is the method to call
   * when you wish to see print the internal state of an object.
   */
  void Print(ostream& os);

  ///@{
  /**
   * Methods invoked by print to print information about the object
   * including superclasses. Typically not called by the user (use
   * Print() instead) but used in the hierarchical print process to
   * combine the output of several classes.
   */
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  virtual void PrintHeader(ostream& os, vtkIndent indent);
  virtual void PrintTrailer(ostream& os, vtkIndent indent);
  ///@}

  /**
   * Increase the reference count (mark as used by another object).
   */
  void Register(vtkObjectBase* o);

  /**
   * Decrease the reference count (release by another object). This
   * has the same effect as invoking Delete() (i.e., it reduces the
   * reference count by 1).
   */
  // XXX(virtual): VTK_DEPRECATED_IN_9_2_0("Override `UsesGarbageCollector()` instead")
  virtual void UnRegister(vtkObjectBase* o);

  /// @{
  /**
   * Indicate whether the class uses `vtkGarbageCollector` or not.
   *
   * Most classes will not need to do this, but if the class participates in a
   * strongly-connected reference count cycle, participation can resolve these
   * cycles.
   *
   * If overriding this method to return true, the `ReportReferences` method
   * should be overridden to report references that may be in cycles.
   */
  virtual bool UsesGarbageCollector() const { return false; }
  /// @}

  /**
   * Return the current reference count of this object.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  int GetReferenceCount() { return this->ReferenceCount; }

  /**
   * Sets the reference count. (This is very dangerous, use with care.)
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetReferenceCount(int);

  /**
   * The name of a directory, ideally mounted -o dax, to memory map an
   * extended memory space within.
   * This must be called before any objects are constructed in the extended space.
   * It can not be changed once setup.
   */
  static void SetMemkindDirectory(const char* directoryname);

  ///@{
  /**
   * A global state flag that controls whether vtkObjects are
   * constructed in the usual way (the default) or within the extended
   * memory space.
   */
  static bool GetUsingMemkind();
  ///@}

  /**
   * A class to help modify and restore the global UsingMemkind state, like
   * SetUsingMemkind(newValue), but safer. Declare it on the stack in a function where you want to
   * make a temporary change. When the function returns it will restore the original value.
   */
  class VTKCOMMONCORE_EXPORT vtkMemkindRAII
  {
#ifdef VTK_USE_MEMKIND
    bool OriginalValue;
#endif

  public:
    vtkMemkindRAII(bool newValue);
    ~vtkMemkindRAII();
    vtkMemkindRAII(vtkMemkindRAII const&) = default;

  private:
    void Save(bool newValue);
    void Restore();
  };

  /**
   * A local state flag that remembers whether this object lives in
   * the normal or extended memory space.
   */
  bool GetIsInMemkind() const;

protected:
  vtkObjectBase();
  virtual ~vtkObjectBase();

  std::atomic<int32_t> ReferenceCount;
  vtkWeakPointerBase** WeakPointers;

  // Internal Register/UnRegister implementation that accounts for
  // possible garbage collection participation.  The second argument
  // indicates whether to participate in garbage collection.
  virtual void RegisterInternal(vtkObjectBase*, vtkTypeBool check);
  virtual void UnRegisterInternal(vtkObjectBase*, vtkTypeBool check);

  // See vtkGarbageCollector.h:
  virtual void ReportReferences(vtkGarbageCollector*);

  // Call this to call from either malloc or memkind_malloc depending on current UsingMemkind
  static vtkMallocingFunction GetCurrentMallocFunction();
  // Call this to call from either realloc or memkind_realloc depending on current UsingMemkind
  static vtkReallocingFunction GetCurrentReallocFunction();
  // Call this to call from either free or memkind_free depending on instance's IsInMemkind
  static vtkFreeingFunction GetCurrentFreeFunction();
  // Call this to unconditionally call memkind_free
  static vtkFreeingFunction GetAlternateFreeFunction();

  virtual void ObjectFinalize();

private:
  friend VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& os, vtkObjectBase& o);
  friend class vtkGarbageCollectorToObjectBaseFriendship;
  friend class vtkWeakPointerBaseToObjectBaseFriendship;

  friend class vtkMemkindRAII;
  friend class vtkTDSCMemkindRAII;
  static void SetUsingMemkind(bool);
  bool IsInMemkind;
  void SetIsInMemkind(bool);

  ///@{
  /**
   * Some classes need to clear the reference counts manually due to the way
   * they work.
   */
  friend class vtkInformationKey;
  friend class vtkGarbageCollector;
  void ClearReferenceCounts();
  ///@}

  friend class vtkDebugLeaks;
  virtual const char* GetDebugClassName() const;

protected:
  vtkObjectBase(const vtkObjectBase&) {}
  void operator=(const vtkObjectBase&) {}
};
VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkObjectBase.h
