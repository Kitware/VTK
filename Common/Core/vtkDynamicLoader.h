// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include <vtksys/DynamicLoader.hxx> // Implementation

typedef vtksys::DynamicLoader::LibraryHandle vtkLibHandle;
typedef vtksys::DynamicLoader::SymbolPointer vtkSymbolPointer;

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDynamicLoader : public vtkObject
{
public:
  static vtkDynamicLoader* New();
  vtkTypeMacro(vtkDynamicLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Load a dynamic library into the current process.
   * The returned vtkLibHandle can be used to access the symbols in the
   * library.
   */
  static vtkLibHandle OpenLibrary(VTK_FILEPATH const char*);
  static vtkLibHandle OpenLibrary(VTK_FILEPATH const char*, int);

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
  vtkDynamicLoader() = default;
  ~vtkDynamicLoader() override = default;

private:
  vtkDynamicLoader(const vtkDynamicLoader&) = delete;
  void operator=(const vtkDynamicLoader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
