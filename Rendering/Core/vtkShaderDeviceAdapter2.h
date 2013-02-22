/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderDeviceAdapter2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShaderDeviceAdapter2 - an adapter to pass generic vertex attributes
// to the rendering pipeline.
// .SECTION
// This class is an adapter used to pass generic vertex attributes to the
// rendering pipeline. Since this changes based on the shading language used,
// this class merely defines the API and subclasses provide implementations for
// Cg and GL.

#ifndef __vtkShaderDeviceAdapter2_h
#define __vtkShaderDeviceAdapter2_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkShaderProgram2;

class VTKRENDERINGCORE_EXPORT vtkShaderDeviceAdapter2 : public vtkObject
{
public:
  vtkTypeMacro(vtkShaderDeviceAdapter2, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Sends a single attribute to the graphics card.
  // The attrname parameter identifies the name of attribute.
  // The components parameter gives the number of
  // components in the attribute.  In general, components must be between
  // 1-4, but a rendering system may impose even more constraints.  The
  // type parameter is a VTK type enumeration (VTK_FLOAT, VTK_INT, etc.).
  // Again, a rendering system may not support all types for all
  // attributes.  The attribute parameter is the actual data for the
  // attribute.
  // If offset is specified, it is added to attribute pointer \c after
  // it has been casted to the proper type.
  virtual void SendAttribute(const char* attrname, int components, int type,
                             const void* attribute,
                             unsigned long offset = 0) = 0;

//BTX
  // Description:
  // Set the shader program which is being updated by this device adapter.
  // The shader program is not reference counted to avoid reference loops.
  void SetShaderProgram(vtkShaderProgram2* program)
    { this->ShaderProgram = program; }
  vtkGetObjectMacro(ShaderProgram, vtkShaderProgram2)

  // Descrition:
  // This method is called before rendering. This gives the shader device
  // adapter an opportunity to collect information, such as attribute indices
  // that it will need while rendering.
  virtual void PrepareForRender() = 0;

protected:
  vtkShaderDeviceAdapter2();
  ~vtkShaderDeviceAdapter2();

  vtkShaderProgram2* ShaderProgram;

private:
  vtkShaderDeviceAdapter2(const vtkShaderDeviceAdapter2&); // Not implemented
  void operator=(const vtkShaderDeviceAdapter2&); // Not implemented
//ETX
};

#endif
