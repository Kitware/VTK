/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderCodeLibrary.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShaderCodeLibrary - Library for Hardware Shaders.
// .SECTION Description
// This class provides the hardware shader code.
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkShaderCodeLibrary_h
#define __vtkShaderCodeLibrary_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGCORE_EXPORT vtkShaderCodeLibrary : public vtkObject
{
public:
  static vtkShaderCodeLibrary* New();
  vtkTypeMacro(vtkShaderCodeLibrary, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Obtain the code for the shader with given name.
  // Note that Cg shader names are prefixed with CG and
  // GLSL shader names are prefixed with GLSL.
  // This method allocates memory. It's the responsibility
  // of the caller to free this memory.
  static char* GetShaderCode(const char* name);

  // Description:
  // Returns an array of pointers to char strings that are
  // the names of the shader codes provided by the library.
  // The end of the array is marked by a null pointer.
  static const char** GetListOfShaderCodeNames();

  // Description:
  // Provides for registering shader code. This overrides the compiled in shader
  // codes.
  static void RegisterShaderCode(const char* name, const char* code);
//BTX
protected:
  vtkShaderCodeLibrary();
  ~vtkShaderCodeLibrary();

private:
  vtkShaderCodeLibrary(const vtkShaderCodeLibrary&); // Not implemented.
  void operator=(const vtkShaderCodeLibrary&); // Not implemented.

  // vtkInternalCleanup is used to destroy Internal ptr when the application
  // exits.
  class vtkInternalCleanup
    {
  public:
    vtkInternalCleanup() {}
    ~vtkInternalCleanup();
    };

  friend class vtkInternalCleanup;
  static vtkInternalCleanup Cleanup;

  // vtkInternal is used to maintain user registered shader codes.
  class vtkInternal;
  static vtkInternal* Internal;
//ETX
};

#endif
