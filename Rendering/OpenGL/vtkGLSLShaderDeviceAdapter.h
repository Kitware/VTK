/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderDeviceAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGLSLShaderDeviceAdapter - adapter to pass generic vertex 
// attributes to the rendering pipeline to be used in a GLSL shader.
// .SECTION Description
// vtkShaderDeviceAdapter subclass for GLSL.
// .SECTION Thanks
// Support for generic vertex attributes in VTK was contributed in
// collaboration with Stephane Ploix at EDF.

#ifndef __vtkGLSLShaderDeviceAdapter_h
#define __vtkGLSLShaderDeviceAdapter_h

#include "vtkShaderDeviceAdapter.h"

class VTK_RENDERING_EXPORT vtkGLSLShaderDeviceAdapter :
  public vtkShaderDeviceAdapter
{
public:
  vtkTypeMacro(vtkGLSLShaderDeviceAdapter, vtkShaderDeviceAdapter);
  static vtkGLSLShaderDeviceAdapter *New();
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
protected:
  vtkGLSLShaderDeviceAdapter();
  ~vtkGLSLShaderDeviceAdapter();

  // Description:
  int GetAttributeLocation(const char* attrName);

  friend class vtkGLSLShaderProgram;
  
private:
  vtkGLSLShaderDeviceAdapter(const vtkGLSLShaderDeviceAdapter&);
  // Not implemented
  void operator=(const vtkGLSLShaderDeviceAdapter&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
