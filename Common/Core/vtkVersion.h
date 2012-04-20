/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVersion.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVersion - Versioning class for vtk
// .SECTION Description
// Holds methods for defining/determining the current vtk version
// (major, minor, build).

// .SECTION Caveats
// This file will change frequently to update the VTKSourceVersion which
// timestamps a particular source release.


#ifndef __vtkVersion_h
#define __vtkVersion_h


#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVersionMacros.h" // For version macros

#define VTK_SOURCE_VERSION "vtk version " VTK_VERSION

class VTKCOMMONCORE_EXPORT vtkVersion : public vtkObject
{
public:
  static vtkVersion *New();
  vtkTypeMacro(vtkVersion,vtkObject);

  // Description:
  // Return the version of vtk this object is a part of.
  // A variety of methods are included. GetVTKSourceVersion returns a string
  // with an identifier which timestamps a particular source tree.
  static const char *GetVTKVersion() { return VTK_VERSION; }
  static int GetVTKMajorVersion() { return VTK_MAJOR_VERSION; }
  static int GetVTKMinorVersion() { return VTK_MINOR_VERSION; }
  static int GetVTKBuildVersion() { return VTK_BUILD_VERSION; }
  static const char *GetVTKSourceVersion() { return VTK_SOURCE_VERSION; }

protected:
  vtkVersion() {}; //insure constructor/destructor protected
  ~vtkVersion() {};
private:
  vtkVersion(const vtkVersion&);  // Not implemented.
  void operator=(const vtkVersion&);  // Not implemented.
};

#endif

// VTK-HeaderTest-Exclude: vtkVersion.h
