// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkResourceFileLocator
 * @brief utility to locate resource files.
 *
 * VTK based application often need to locate resource files, such configuration
 * files, Python modules, etc. vtkResourceFileLocator provides methods that can
 * be used to locate such resource files at runtime.
 *
 * Using `Locate`, one can locate files relative to an
 * anchor directory such as the executable directory, or the library directory.
 *
 * `GetLibraryPathForSymbolUnix` and `GetLibraryPathForSymbolWin32` methods can
 * be used to locate the library that provides a particular symbol. For example,
 * this is used by `vtkPythonInterpreter` to ensure that the `vtk` Python package
 * is located relative the VTK libraries, irrespective of the application location.
 */

#ifndef vtkResourceFileLocator_h
#define vtkResourceFileLocator_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkDeprecation.h"      // For VTK_DEPRECATED_IN_9_6_0
#include "vtkObject.h"

#include <string> // needed for std::string
#include <vector> // needed for std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMISC_EXPORT vtkResourceFileLocator : public vtkObject
{
public:
  static vtkResourceFileLocator* New();
  vtkTypeMacro(vtkResourceFileLocator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The log verbosity to use when logging information about the resource
   * searching. Default is `vtkLogger::VERBOSITY_TRACE`.
   */
  vtkSetMacro(LogVerbosity, int);
  vtkGetMacro(LogVerbosity, int);
  ///@}

  /**
   * Given a starting anchor directory, look for the landmark file relative to
   * the anchor. If found return the anchor. If not found, go one directory up
   * and then look the landmark file again.
   */
  virtual std::string Locate(const std::string& anchor, const std::string& landmark,
    const std::string& defaultDir = std::string());

  /**
   * This variant is used to look for landmark relative to the anchor using
   * additional prefixes for the landmark file. For example, if you're looking for
   * `vtk/__init__.py`, but it can be placed relative to your anchor location
   * (let's say the executable directory), under "lib" or "lib/python", then
   * use this variant with "lib", and "lib/python" passed in as the landmark
   * prefixes. On success, the returned value will be anchor + matching prefix.
   */
  virtual std::string Locate(const std::string& anchor,
    const std::vector<std::string>& landmark_prefixes, const std::string& landmark,
    const std::string& defaultDir = std::string());

  /**
   * Return the path to the library containing the given pointer.
   */
  static VTK_FILEPATH std::string GetLibraryPathForAddress(const void* ptr);

  /**
   * Return the path to the current executable.
   */
  static VTK_FILEPATH std::string GetCurrentExecutablePath();

  ///@{
  /**
   * Returns the name of the library providing the symbol. For example, if you
   * want to locate where the VTK libraries located call
   * `GetLibraryPathForSymbolUnix("GetVTKVersion")` on Unixes and
   * `GetLibraryPathForSymbolWin32(GetVTKVersion)` on Windows. Alternatively, you
   * can simply use the `vtkGetLibraryPathForSymbol(GetVTKVersion)` macro
   * that makes the appropriate call as per the current platform.
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetLibraryPathForAddress() instead")
  static VTK_FILEPATH std::string GetLibraryPathForSymbolUnix(const char* symbolname);
  VTK_DEPRECATED_IN_9_6_0("Use GetLibraryPathForAddress() instead")
  static VTK_FILEPATH std::string GetLibraryPathForSymbolWin32(const void* fptr);
  ///@}

protected:
  vtkResourceFileLocator();
  ~vtkResourceFileLocator() override;

private:
  vtkResourceFileLocator(const vtkResourceFileLocator&) = delete;
  void operator=(const vtkResourceFileLocator&) = delete;

  int LogVerbosity;
};

#define vtkGetLibraryPathForSymbol(function)                                                       \
  vtkResourceFileLocator::GetLibraryPathForAddress(reinterpret_cast<const void*>(&function))

VTK_ABI_NAMESPACE_END
#endif
