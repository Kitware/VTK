/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkToolkits.h" // Needed for VTK_DEBUG_LEAKS macro setting.
#include "vtkDebugLeaksManager.h" // Needed for proper singleton initialization

class vtkDebugLeaksHashTable;
class vtkSimpleCriticalSection;
class vtkDebugLeaksObserver;

class VTKCOMMONCORE_EXPORT vtkDebugLeaks : public vtkObject
{
public:
  static vtkDebugLeaks *New();
  vtkTypeMacro(vtkDebugLeaks,vtkObject);

  /**
   * Call this when creating a class of a given name.
   */
  static void ConstructClass(const char* classname);

  /**
   * Call this when deleting a class of a given name.
   */
  static void DestructClass(const char* classname);

  /**
   * Print all the values in the table.  Returns non-zero if there
   * were leaks.
   */
  static int PrintCurrentLeaks();

  //@{
  /**
   * Get/Set flag for exiting with an error when leaks are present.
   * Default is on when VTK_DEBUG_LEAKS is on and off otherwise.
   */
  static int GetExitError();
  static void SetExitError(int);
  //@}

  static void SetDebugLeaksObserver(vtkDebugLeaksObserver* observer);
  static vtkDebugLeaksObserver* GetDebugLeaksObserver();

protected:
  vtkDebugLeaks(){}
  ~vtkDebugLeaks() override{}

  static int DisplayMessageBox(const char*);

  static void ClassInitialize();
  static void ClassFinalize();

  static void ConstructingObject(vtkObjectBase* object);
  static void DestructingObject(vtkObjectBase* object);

  friend class vtkDebugLeaksManager;
  friend class vtkObjectBase;

private:
  static vtkDebugLeaksHashTable* MemoryTable;
  static vtkSimpleCriticalSection* CriticalSection;
  static vtkDebugLeaksObserver* Observer;
  static int ExitError;

  vtkDebugLeaks(const vtkDebugLeaks&) = delete;
  void operator=(const vtkDebugLeaks&) = delete;
};

// This class defines callbacks for debugging tools. The callbacks are not for general use.
// The objects passed as arguments to the callbacks are in partially constructed or destructed
// state and accessing them may cause undefined behavior.
class VTKCOMMONCORE_EXPORT vtkDebugLeaksObserver {
public:
  virtual ~vtkDebugLeaksObserver() {}
  virtual void ConstructingObject(vtkObjectBase*) = 0;
  virtual void DestructingObject(vtkObjectBase*) = 0;
};

#endif // vtkDebugLeaks_h
// VTK-HeaderTest-Exclude: vtkDebugLeaks.h
