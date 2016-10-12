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
/**
 * @class   vtkGLSLShaderDeviceAdapter2
 * @brief   adapter to pass generic vertex
 * attributes to the rendering pipeline to be used in a vtkShaderProgram2.
 *
 * vtkShaderDeviceAdapter subclass for vtkShaderProgram2.
*/

#ifndef vtkGLSLShaderDeviceAdapter2_h
#define vtkGLSLShaderDeviceAdapter2_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkShaderDeviceAdapter2.h"

class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkGLSLShaderDeviceAdapter2
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

  /**
   * Sends a single attribute to the graphics card.
   * The attrname parameter identifies the name of attribute.
   * The components parameter gives the number of
   * components in the attribute.  In general, components must be between
   * 1-4, but a rendering system may impose even more constraints.  The
   * type parameter is a VTK type enumeration (VTK_FLOAT, VTK_INT, etc.).
   * Again, a rendering system may not support all types for all
   * attributes.  The attribute parameter is the actual data for the
   * attribute.
   * If offset is specified, it is added to attribute pointer \c after
   * it has been casted to the proper type.
   * If attribute is NULL, the OpenGL ID for the attribute will simply be
   * cached.
   */
  virtual void SendAttribute(const char* attrname,
                             int components,
                             int type,
                             const void *attribute,
                             unsigned long offset=0);

protected:
  vtkGLSLShaderDeviceAdapter2();
  ~vtkGLSLShaderDeviceAdapter2();

  int GetAttributeLocation(const char* attrName);

private:
  vtkGLSLShaderDeviceAdapter2(const vtkGLSLShaderDeviceAdapter2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGLSLShaderDeviceAdapter2&) VTK_DELETE_FUNCTION;

  class vtkInternal;
  vtkInternal* Internal;

};

#endif
