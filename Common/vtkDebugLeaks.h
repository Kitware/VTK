/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
#include "vtkToolkits.h"

class vtkDebugLeaksHashTable;

class VTK_COMMON_EXPORT vtkDebugLeaks : public vtkObject
{
public: 
  static vtkDebugLeaks *New();
  vtkTypeRevisionMacro(vtkDebugLeaks,vtkObject);

  // Description:
  // Call this when creating a class of a given name.
  static void ConstructClass(const char* classname);

  // Description:
  // Call this when deleting a class of a given name.
  static void DestructClass(const char* classname);

  // Description:
  // Print all the values in the table.
  static void PrintCurrentLeaks();

  // Description:
  // Clean up the table memory.
  static void DeleteTable();

  // Description:
  // Turn prompt at exit on/off.
  static void PromptUserOn();
  static void PromptUserOff();

protected:
  vtkDebugLeaks(){}; 
  virtual ~vtkDebugLeaks(){}; 

  static int PromptUser;

private:
  static vtkDebugLeaksHashTable* MemoryTable;
private:
  vtkDebugLeaks(const vtkDebugLeaks&);  // Not implemented.
  void operator=(const vtkDebugLeaks&);  // Not implemented.
};

#endif // __vtkDebugLeaks_h
