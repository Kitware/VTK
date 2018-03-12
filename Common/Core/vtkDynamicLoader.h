/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDynamicLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDynamicLoader
 * @brief   class interface to system dynamic libraries
 *
 * vtkDynamicLoader provides a portable interface to loading dynamic
 * libraries into a process.
 * @sa
 * A more portable and lightweight solution is kwsys::DynamicLoader
*/

#ifndef vtkDynamicLoader_h
#define vtkDynamicLoader_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include <vtksys/DynamicLoader.hxx>

typedef vtksys::DynamicLoader::LibraryHandle vtkLibHandle;
typedef vtksys::DynamicLoader::SymbolPointer vtkSymbolPointer;

class VTKCOMMONCORE_EXPORT vtkDynamicLoader : public vtkObject
{
public:
  static vtkDynamicLoader* New();
  vtkTypeMacro(vtkDynamicLoader,vtkObject);

  /**
   * Load a dynamic library into the current process.
   * The returned vtkLibHandle can be used to access the symbols in the
   * library.
   */
  static vtkLibHandle OpenLibrary(const char*);

  /**
   * Attempt to detach a dynamic library from the
   * process.  A value of true is returned if it is successful.
   */
  static int CloseLibrary(vtkLibHandle);

  /**
   * Find the address of the symbol in the given library
   */
  static vtkSymbolPointer GetSymbolAddress(vtkLibHandle, const char*);

  /**
   * Return the library prefix for the given architecture
   */
  static const char* LibPrefix();

  /**
   * Return the library extension for the given architecture
   */
  static const char* LibExtension();

  /**
   * Return the last error produced from a calls made on this class.
   */
  static const char* LastError();

protected:
  vtkDynamicLoader() {}
  ~vtkDynamicLoader() override {}

private:
  vtkDynamicLoader(const vtkDynamicLoader&) = delete;
  void operator=(const vtkDynamicLoader&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkDynamicLoader.h
