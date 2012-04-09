/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderDeviceAdapter2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGLSLShaderDeviceAdapter2 - adapter to pass generic vertex 
// attributes to the rendering pipeline to be used in a vtkShaderProgram2.
// .SECTION Description
// vtkShaderDeviceAdapter subclass for vtkShaderProgram2.

#ifndef __vtkGLSLShaderDeviceAdapter2_h
#define __vtkGLSLShaderDeviceAdapter2_h

#include "vtkShaderDeviceAdapter2.h"

class vtkShaderProgram2;

class VTK_RENDERING_EXPORT vtkGLSLShaderDeviceAdapter2
  : public vtkShaderDeviceAdapter2
{
public:
  vtkTypeMacro(vtkGLSLShaderDeviceAdapter2, vtkShaderDeviceAdapter2);
  static vtkGLSLShaderDeviceAdapter2 *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Descrition:
  // This method is called before rendering. This gives the shader device
  // adapter an opportunity to collect information, such as attribute indices
  // that it will need while rendering.
  virtual void PrepareForRender();

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
  // If attribute is NULL, the OpenGL ID for the attribute will simply be
  // cached.
  virtual void SendAttribute(const char* attrname,
                             int components,
                             int type,
                             const void *attribute,
                             unsigned long offset=0);
   
//BTX  
  // Description:
  // Set the shader program which is being updated by this device adapter.
  // The shader program is not reference counted to avoid reference loops.
  void SetShaderProgram(vtkShaderProgram2 *program);
  vtkGetObjectMacro(ShaderProgram, vtkShaderProgram2);
  
protected:
  vtkGLSLShaderDeviceAdapter2();
  ~vtkGLSLShaderDeviceAdapter2();

  // Description:
  int GetAttributeLocation(const char* attrName);

  vtkShaderProgram2 *ShaderProgram;
  
private:
  vtkGLSLShaderDeviceAdapter2(const vtkGLSLShaderDeviceAdapter2&);
  // Not implemented
  void operator=(const vtkGLSLShaderDeviceAdapter2&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
