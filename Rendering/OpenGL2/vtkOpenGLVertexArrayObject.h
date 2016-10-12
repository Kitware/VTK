/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGLVertexArrayObject_h
#define vtkOpenGLVertexArrayObject_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkObject.h"
#include <string> // For API.

class vtkShaderProgram;
class vtkOpenGLBufferObject;

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

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexArrayObject : public vtkObject
{
public:
  static vtkOpenGLVertexArrayObject* New();
  vtkTypeMacro(vtkOpenGLVertexArrayObject, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  void Bind();

  void Release();

  void ReleaseGraphicsResources();

  void ShaderProgramChanged();

  bool AddAttributeArray(vtkShaderProgram *program,
                         vtkOpenGLBufferObject *buffer,
                         const std::string &name, int offset, size_t stride,
                         int elementType, int elementTupleSize, bool normalize)
  {
    return this->AddAttributeArrayWithDivisor(program, buffer, name,
      offset,stride,elementType, elementTupleSize, normalize, 0, false);
  }

  bool AddAttributeArrayWithDivisor(vtkShaderProgram *program,
                         vtkOpenGLBufferObject *buffer,
                         const std::string &name, int offset, size_t stride,
                         int elementType, int elementTupleSize, bool normalize,
                         int divisor, bool isMatrix);

  bool AddAttributeMatrixWithDivisor(vtkShaderProgram *program,
                         vtkOpenGLBufferObject *buffer,
                         const std::string &name, int offset, size_t stride,
                         int elementType, int elementTupleSize, bool normalize,
                         int divisor);

  bool RemoveAttributeArray(const std::string &name);

  // Force this VAO to emulate a vertex aray object even if
  // the system supports VAOs. This can be useful in cases where
  // the vertex array object does not handle all extensions.
  void SetForceEmulation(bool val);

protected:
  vtkOpenGLVertexArrayObject();
  ~vtkOpenGLVertexArrayObject();

private:
  vtkOpenGLVertexArrayObject(
    const vtkOpenGLVertexArrayObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLVertexArrayObject&) VTK_DELETE_FUNCTION;
  class Private;
  Private *Internal;
};

#endif // vtkOpenGLVertexArrayObject_h
