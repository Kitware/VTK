// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModLight
 * @brief   Implement light kit support in the OpenGL renderer for vtkCellGrid.
 */

#ifndef vtkGLSLModLight_h
#define vtkGLSLModLight_h

#include "vtkGLSLModifierBase.h"

#include "vtkOpenGLRenderer.h"         // for ivar
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkWeakPointer.h"            // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkInformationObjectBaseKey;
class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModLight : public vtkGLSLModifierBase
{
public:
  static vtkGLSLModLight* New();
  vtkTypeMacro(vtkGLSLModLight, vtkGLSLModifierBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  struct LightStatsBasic
  {
    int Complexity = 0;
    int Count = 0;
  };

  static LightStatsBasic GetBasicLightStats(vtkOpenGLRenderer* renderer, vtkActor* actor);

  vtkSetMacro(UsePBRTextures, bool);
  vtkGetMacro(UsePBRTextures, bool);

  vtkSetMacro(UseAnisotropy, bool);
  vtkGetMacro(UseAnisotropy, bool);

  vtkSetMacro(UseClearCoat, bool);
  vtkGetMacro(UseClearCoat, bool);

  // vtkGLSLModifierBase virtuals:
  bool ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
    std::string& tessControlShader, std::string& tessEvalShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor) override;
  bool SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
    vtkAbstractMapper* mapper, vtkActor* actor, vtkOpenGLVertexArrayObject* VAO = nullptr) override;
  bool IsUpToDate(vtkOpenGLRenderer* renderer, vtkAbstractMapper* mapper, vtkActor* actor) override;

protected:
  vtkGLSLModLight();
  ~vtkGLSLModLight() override;

  /// @name Light statistics
  int LastLightComplexity = 0;
  int LastLightCount = 0;

  /// @name PBR settings
  bool UsePBRTextures = false;
  bool UseAnisotropy = false;
  bool UseClearCoat = false;

private:
  vtkGLSLModLight(const vtkGLSLModLight&) = delete;
  void operator=(const vtkGLSLModLight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
