/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProgram2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShaderProgram2
// .SECTION Description
// Abstraction for a GLSL shader program used in GPGPU pipelines.
// This is not designed for reuse, one should create and use new instances for
// different shaders.

// .SECTION Caveats
// DON'T PLAY WITH IT YET.

#ifndef __vtkShaderProgram2_h
#define __vtkShaderProgram2_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkRenderWindow;
class vtkOpenGLExtensionManager;

class VTK_RENDERING_EXPORT vtkShaderProgram2 : public vtkObject
{
public:
  static vtkShaderProgram2* New();
  vtkTypeRevisionMacro(vtkShaderProgram2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the context. This does not increase the reference count of the 
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkRenderWindow*);
  vtkRenderWindow* GetContext();
//BTX
  enum KernelType
    {
    VERTEX,
    GEOMETRY,
    FRAGMENT
    };
//ETX

  // Description:
  // Add a shader kernel of the given type i.e. either Vertex of Fragment.
  // Returns the kernel index if the shader was compiled and attached 
  // successfully, -1 on error. Kernels can only be added before Bind().
  int AddKernel(KernelType type, const char* source);

  // Description:
  // Returns the OpenGL shader program id. 
  // This may be useful to set geometry shader input/output types for
  // example.
  unsigned int GetOpenGLProgramId();

  // Description:
  // Disable a kernel. This detaches the shader without deleting it.
  void DisableKernel(int index);

  // Description:
  // Enable a kernel. This attaches a already compiled kernel.
  void EnableKernel(int index);

  // Description:
  // Removes all kernels i.e. detaches all shaders from the shader program.
  void RemoveAllKernels();

  // Description:
  // Enables the shader program. If it hasn't been linked already, it will link
  // it. On failure, returns false.
  bool Bind();

  // Description:
  // Unload the shader programs.
  void UnBind();

  // Description:
  // Returns the uniform parameter location.
  // The shader must be bound before calling this.
  int GetUniformLocation(const char* name);

  // Description:
  // Returns the generic attribute location.
  // The shader must be bound before calling this.
  int GetAttributeLocation(const char* name);

  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported(vtkRenderWindow* renWin);
//BTX
protected:
  vtkShaderProgram2();
  ~vtkShaderProgram2();
  // Description:
  // Load all necessary extensions.
  bool LoadRequiredExtensions(vtkOpenGLExtensionManager*);

  bool GeometryShadersSupported;
  vtkWeakPointer<vtkRenderWindow> Context;

  vtkTimeStamp LinkTime;
private:
  vtkShaderProgram2(const vtkShaderProgram2&); // Not implemented.
  void operator=(const vtkShaderProgram2&); // Not implemented.


  bool CreateShaderProgram();
  void DestroyShader();
  void DeleteShaders();

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif


