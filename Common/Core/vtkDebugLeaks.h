// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDebugLeaks
 * @brief identify memory leaks at program termination
 * vtkDebugLeaks is used to report memory leaks at the exit of the program. It
 * uses vtkObjectBase::InitializeObjectBase() (called via vtkObjectFactory
 * macros) to intercept the construction of all VTK objects. It uses the
 * UnRegisterInternal method of vtkObjectBase to intercept the destruction of
 * all objects.
 *
 * If not using the vtkObjectFactory macros to implement New(), be sure to call
 * vtkObjectBase::InitializeObjectBase() explicitly on the constructed
 * instance. The rule of thumb is that wherever "new [some vtkObjectBase
 * subclass]" is called, vtkObjectBase::InitializeObjectBase() must be called
 * as well.
 *
 * There are exceptions to this:
 *
 * - vtkCommand subclasses traditionally do not fully participate in
 * vtkDebugLeaks registration, likely because they typically do not use
 * vtkTypeMacro to configure GetClassName. InitializeObjectBase should not be
 * called on vtkCommand subclasses, and all such classes will be automatically
 * registered with vtkDebugLeaks as "vtkCommand or subclass".
 *
 * - vtkInformationKey subclasses are not reference counted. They are allocated
 * statically and registered automatically with a singleton "manager" instance.
 * The manager ensures that all keys are cleaned up before exiting, and
 * registration/deregistration with vtkDebugLeaks is bypassed.
 *
 * A table of object name to number of instances is kept. At the exit of the
 * program if there are still VTK objects around it will print them out. To
 * enable this class add the flag -DVTK_DEBUG_LEAKS to the compile line, and
 * rebuild vtkObject and vtkObjectFactory.
 */

#ifndef vtkDebugLeaks_h
#define vtkDebugLeaks_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkDebug.h"             // Needed for VTK_DEBUG_LEAKS macro setting.
#include "vtkDebugLeaksManager.h" // Needed for proper singleton initialization

#include <functional> // for finalizers
#include <mutex>      // for std::mutex
#include <vector>     // for finalizers

VTK_ABI_NAMESPACE_BEGIN
class vtkDebugLeaksHashTable;
class vtkDebugLeaksTraceManager;
class vtkDebugLeaksObserver;

class VTKCOMMONCORE_EXPORT vtkDebugLeaks : public vtkObject
{
public:
  static vtkDebugLeaks* New();
  vtkTypeMacro(vtkDebugLeaks, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Call this when creating a class.
   */
  static void ConstructClass(vtkObjectBase* object);

  /**
   * Call this when creating a vtkCommand or subclasses.
   */
  static void ConstructClass(const char* className);

  /**
   * Call this when deleting a class.
   */
  static void DestructClass(vtkObjectBase* object);

  /**
   * Call this when deleting vtkCommand or a subclass.
   */
  static void DestructClass(const char* className);

  /**
   * Print all the values in the table.  Returns non-zero if there
   * were leaks.
   */
  static int PrintCurrentLeaks();

  ///@{
  /**
   * Get/Set flag for exiting with an error when leaks are present.
   * Default is on when VTK_DEBUG_LEAKS is on and off otherwise.
   */
  static int GetExitError();
  static void SetExitError(int);
  ///@}

  static void SetDebugLeaksObserver(vtkDebugLeaksObserver* observer);
  static vtkDebugLeaksObserver* GetDebugLeaksObserver();

  /// Ensure that \a finalizer is invoked before debug-leaks accounting is reported.
  ///
  /// If your application holds VTK objects (i.e., instances of classes that inherit
  /// vtkObjectBase) for its duration, then adding \a finalizer function that frees
  /// them will prevent vtkDebugLeaks from reporting them as dangling references.
  /// This can occur if you declare static global variable that owns a reference
  /// to a VTK object (i.e., `static vtkNew<X> global;`). Because the order in which
  /// static variables are destroyed is not guaranteed, vtkDebugLeaks (which also
  /// depends on a static variable's destruction to report leaks at the exit of the
  /// application) may be called before these other static globals are destroyed.
  ///
  /// By adding a \a finalizer, you can release these references before leak
  /// reporting is performed.
  static void AddFinalizer(std::function<void()> finalizer);

protected:
  vtkDebugLeaks() = default;
  ~vtkDebugLeaks() override = default;

  static int DisplayMessageBox(const char*);

  static void ClassInitialize();
  static void ClassFinalize();

  static void ConstructingObject(vtkObjectBase* object);
  static void DestructingObject(vtkObjectBase* object);

  static std::vector<std::function<void()>>* Finalizers;

  friend class vtkDebugLeaksManager;
  friend class vtkObjectBase;

private:
  static vtkDebugLeaksHashTable* MemoryTable;
  static vtkDebugLeaksTraceManager* TraceManager;
  static std::mutex* CriticalSection;
  static vtkDebugLeaksObserver* Observer;
  static int ExitError;

  vtkDebugLeaks(const vtkDebugLeaks&) = delete;
  void operator=(const vtkDebugLeaks&) = delete;
};

// This class defines callbacks for debugging tools. The callbacks are not for general use.
// The objects passed as arguments to the callbacks are in partially constructed or destructed
// state and accessing them may cause undefined behavior.
class VTKCOMMONCORE_EXPORT vtkDebugLeaksObserver
{
public:
  virtual ~vtkDebugLeaksObserver() = default;
  virtual void ConstructingObject(vtkObjectBase*) = 0;
  virtual void DestructingObject(vtkObjectBase*) = 0;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDebugLeaks_h
