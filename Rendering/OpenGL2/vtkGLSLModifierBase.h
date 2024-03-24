// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModifierBase
 * @brief   Abstract class that helps you develop modifier for VTK GLSL shaders
 */

#ifndef vtkGLSLModifierBase_h
#define vtkGLSLModifierBase_h

#include "vtkObject.h"

#include "vtkRenderingOpenGL2Module.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractMapper;
class vtkActor;
class vtkInformationObjectBaseKey;
class vtkOpenGLVertexArrayObject;
class vtkOpenGLRenderer;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModifierBase : public vtkObject
{
public:
  vtkTypeMacro(vtkGLSLModifierBase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// The renderer will set primitive type.
  void SetPrimitiveType(unsigned int primType) { this->PrimitiveType = primType; }

  /// Abstract interfaces to replace shader values and apply parameters as uniform values.
  virtual bool ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
    std::string& tessControlShader, std::string& tessEvalShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor) = 0;
  virtual bool SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
    vtkAbstractMapper* mapper, vtkActor* actor, vtkOpenGLVertexArrayObject* VAO = nullptr) = 0;

  /// Whether mod needs to be re-applied.
  /// Return true if your mod's ReplaceShaderValues must be run again.
  /// Typically, you'd want to perform the shader substitutions again if
  /// properties fundamental to the shader construction have been modified.
  virtual bool IsUpToDate(
    vtkOpenGLRenderer* renderer, vtkAbstractMapper* mapper, vtkActor* actor) = 0;

  /**
   * if this key exists on the \a ShaderMods of \a vtkDrawTexturedElements, that mod will be
   * applied before rendering.
   */
  static vtkInformationObjectBaseKey* GLSL_MODIFIERS();

protected:
  vtkGLSLModifierBase();
  ~vtkGLSLModifierBase() override;

  unsigned int PrimitiveType = 0;

private:
  vtkGLSLModifierBase(const vtkGLSLModifierBase&) = delete;
  void operator=(const vtkGLSLModifierBase&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
