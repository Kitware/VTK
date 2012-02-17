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
// .NAME vtkDebugLeaks - identify memory leaks at program termination
// .SECTION Description
// vtkDebugLeaks is used to report memory leaks at the exit of the program.
// It uses the vtkObjectFactory to intercept the construction of all VTK
// objects. It uses the UnRegister method of vtkObject to intercept the
// destruction of all objects. A table of object name to number of instances
// is kept. At the exit of the program if there are still VTK objects around
// it will print them out.  To enable this class add the flag
// -DVTK_DEBUG_LEAKS to the compile line, and rebuild vtkObject and
// vtkObjectFactory.

#ifndef __vtkDebugLeaks_h
#define __vtkDebugLeaks_h

#include "vtkObject.h"

#include "vtkToolkits.h" // Needed for VTK_DEBUG_LEAKS macro setting.
#include "vtkDebugLeaksManager.h" // Needed for proper singleton initialization

class vtkDebugLeaksHashTable;
class vtkSimpleCriticalSection;

class VTK_COMMON_EXPORT vtkDebugLeaks : public vtkObject
{
public: 
  static vtkDebugLeaks *New();
  vtkTypeMacro(vtkDebugLeaks,vtkObject);

  // Description:
  // Call this when creating a class of a given name.
  static void ConstructClass(const char* classname);

  // Description:
  // Call this when deleting a class of a given name.
  static void DestructClass(const char* classname);

  // Description:
  // Print all the values in the table.  Returns non-zero if there
  // were leaks.
  static int PrintCurrentLeaks();

  // Description:
  // @deprecated Turn prompt at exit on/off (this setting is deprecated 
  // and will be ignored).
  VTK_LEGACY(static void PromptUserOn());
  VTK_LEGACY(static void PromptUserOff());

  // Description:
  // Get/Set flag for exiting with an error when leaks are present.
  // Default is on when VTK_DEBUG_LEAKS is on and off otherwise.
  static int GetExitError();
  static void SetExitError(int);

protected:
  vtkDebugLeaks(){}; 
  virtual ~vtkDebugLeaks(){}; 

  static int DisplayMessageBox(const char*);

  static void ClassInitialize();
  static void ClassFinalize();

  //BTX
  friend class vtkDebugLeaksManager;
  //ETX

private:
  static vtkDebugLeaksHashTable* MemoryTable;
  static vtkSimpleCriticalSection* CriticalSection;
  static int ExitError;

  vtkDebugLeaks(const vtkDebugLeaks&);  // Not implemented.
  void operator=(const vtkDebugLeaks&);  // Not implemented.
};

#endif // __vtkDebugLeaks_h
