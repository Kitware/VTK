// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkOpenGLVertexArrayObject_h
#define vtkOpenGLVertexArrayObject_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include <string>                      // For API.

VTK_ABI_NAMESPACE_BEGIN
class vtkShaderProgram;
class vtkOpenGLBufferObject;
class vtkOpenGLVertexBufferObject;

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
  vtkTypeMacro(vtkOpenGLVertexArrayObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Bind();

  void Release();

  void ReleaseGraphicsResources();

  void ShaderProgramChanged();

  bool AddAttributeArray(vtkShaderProgram* program, vtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize)
  {
    return this->AddAttributeArrayWithDivisor(
      program, buffer, name, offset, stride, elementType, elementTupleSize, normalize, 0, false);
  }

  bool AddAttributeArray(vtkShaderProgram* program, vtkOpenGLVertexBufferObject* buffer,
    const std::string& name, int offset, bool normalize);

  bool AddAttributeArrayWithDivisor(vtkShaderProgram* program, vtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize, int divisor, bool isMatrix);

  bool AddAttributeMatrixWithDivisor(vtkShaderProgram* program, vtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize, int divisor, int tupleOffset);

  bool RemoveAttributeArray(const std::string& name);

  // Force this VAO to emulate a vertex array object even if
  // the system supports VAOs. This can be useful in cases where
  // the vertex array object does not handle all extensions.
  void SetForceEmulation(bool val);

protected:
  vtkOpenGLVertexArrayObject();
  ~vtkOpenGLVertexArrayObject() override;

private:
  vtkOpenGLVertexArrayObject(const vtkOpenGLVertexArrayObject&) = delete;
  void operator=(const vtkOpenGLVertexArrayObject&) = delete;
  class Private;
  Private* Internal;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVertexArrayObject_h
