/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglVertexArrayObject_h
#define __vtkglVertexArrayObject_h

#include "vtkRenderingOpenGL2Module.h"
#include <string> // For API.

class vtkShaderProgram;

namespace vtkgl
{
class BufferObject;

/**
 * @brief The VertexArrayObject class uses, or emulates, vertex array objects.
 * These are extremely useful for setup/tear down of vertex attributes, and can
 * offer significant performance benefits when the hardware supports them.
 *
 * It should be noted that this object is very lightweight, and it assumes the
 * objects being used are correctly set up. Even without support for VAOs this
 * class caches the array locations, types, etc and avoids repeated look ups. It
 * it bound to a single ShaderProgram object.
 */

class VTKRENDERINGOPENGL2_EXPORT VertexArrayObject
{
public:
  VertexArrayObject();
  ~VertexArrayObject();

  void Bind();

  void Release();

  void ReleaseGraphicsResources();

  void ShaderProgramChanged();

  bool AddAttributeArray(vtkShaderProgram *program, BufferObject &buffer,
                         const std::string &name, int offset, size_t stride,
                         int elementType, int elementTupleSize, bool normalize);

  bool RemoveAttributeArray(const std::string &name);

private:
  class Private;
  Private *d;
};

} // End of vtkgl namespace

#endif // __vtkglVertexArrayObject_h

// VTK-HeaderTest-Exclude: vtkglVertexArrayObject.h
