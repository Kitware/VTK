/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDynamicLoader.h
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
// .NAME vtkDynamicLoader - class interface to system dynamic libraries
// .SECTION Description
// vtkDynamicLoader provides a portable interface to loading dynamic 
// libraries into a process.  


#ifndef __vtkDynamicLoader_h
#define __vtkDynamicLoader_h
#include "vtkConfigure.h"

//BTX
// Ugly stuff for library handles
// They are different on several different OS's
#if defined(__hpux)
# include <dl.h>
  typedef shl_t vtkLibHandle;
#elif defined(_WIN32)
# include "vtkWin32Header.h"
  typedef HMODULE vtkLibHandle;
#else
  typedef void* vtkLibHandle;
#endif
//ETX

#include "vtkObject.h"


class VTK_COMMON_EXPORT vtkDynamicLoader : public vtkObject
{
public:
  static vtkDynamicLoader *New() {return new vtkDynamicLoader;};
  vtkTypeRevisionMacro(vtkDynamicLoader,vtkObject);

  //BTX
  // Description:
  // Load a dynamic library into the current process.
  // The returned vtkLibHandle can be used to access the symbols in the 
  // library.
  static vtkLibHandle OpenLibrary(const char*);

  // Description:
  // Attempt to detach a dynamic library from the
  // process.  A value of true is returned if it is successful.
  static int CloseLibrary(vtkLibHandle);
  //ETX
  
  // Description:
  // Find the address of the symbol in the given library
  static void* GetSymbolAddress(vtkLibHandle, const char*);

  // Description:
  // Return the library prefix for the given architecture
  static const char* LibPrefix();

  // Description:
  // Return the library extension for the given architecture
  static const char* LibExtension();

  // Description:
  // Return the last error produced from a calls made on this class.
  static const char* LastError();
  
protected:
  vtkDynamicLoader() {};
  ~vtkDynamicLoader() {};

  
private:
  vtkDynamicLoader(const vtkDynamicLoader&);  // Not implemented.
  void operator=(const vtkDynamicLoader&);  // Not implemented.
};

#endif
