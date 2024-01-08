// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVersion
 * @brief   Versioning class for vtk
 *
 * Holds methods for defining/determining the current vtk version
 * (major, minor, build).
 *
 * @warning
 * This file will change frequently to update the VTKSourceVersion which
 * timestamps a particular source release.
 */

#ifndef vtkVersion_h
#define vtkVersion_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVersionMacros.h" // For version macros

#define GetVTKVersion VTK_ABI_NAMESPACE_MANGLE(GetVTKVersion)

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkVersion : public vtkObject
{
public:
  static vtkVersion* New();
  vtkTypeMacro(vtkVersion, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the version of vtk this object is a part of.
   * A variety of methods are included. GetVTKSourceVersion returns a string
   * with an identifier which timestamps a particular source tree.
   */
  static const char* GetVTKVersion() { return VTK_VERSION; }
  static const char* GetVTKVersionFull();
  static int GetVTKMajorVersion() { return VTK_MAJOR_VERSION; }
  static int GetVTKMinorVersion() { return VTK_MINOR_VERSION; }
  static int GetVTKBuildVersion() { return VTK_BUILD_VERSION; }
  static const char* GetVTKSourceVersion() { return VTK_SOURCE_VERSION; }

protected:
  vtkVersion() = default; // ensure constructor/destructor protected
  ~vtkVersion() override = default;

private:
  vtkVersion(const vtkVersion&) = delete;
  void operator=(const vtkVersion&) = delete;
};

VTK_ABI_NAMESPACE_END

extern "C"
{
  VTKCOMMONCORE_EXPORT const char* GetVTKVersion();
}

#endif
