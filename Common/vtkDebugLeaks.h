/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkDebugLeaks,vtkObject);

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
