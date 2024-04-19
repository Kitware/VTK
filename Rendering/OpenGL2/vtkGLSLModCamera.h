// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModCamera
 * @brief   Implement model-view-projection transforms in the OpenGL renderer.
 */

#ifndef vtkGLSLModCamera_h
#define vtkGLSLModCamera_h

#include "vtkGLSLModifierBase.h"

#include "vtkMatrix3x3.h"              // for ivar
#include "vtkMatrix4x4.h"              // for ivar
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkSmartPointer.h"           // for ivar

#include <array> // for array

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkInformationObjectBaseKey;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModCamera : public vtkGLSLModifierBase
{
public:
  static vtkGLSLModCamera* New();
  vtkTypeMacro(vtkGLSLModCamera, vtkGLSLModifierBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void EnableShiftScale(bool coordShiftAndScaleInUse, vtkMatrix4x4* ssMatrix);
  void DisableShiftScale();

  // vtkGLSLModifierBase virtuals:
  bool ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
    std::string& tessControlShader, std::string& tessEvalShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor) override;
  bool SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
    vtkAbstractMapper* mapper, vtkActor* actor, vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  bool IsUpToDate(vtkOpenGLRenderer* vtkNotUsed(renderer), vtkAbstractMapper* vtkNotUsed(mapper),
    vtkActor* vtkNotUsed(actor)) override
  {
    // no replacements which depend upon any of renderer/mapper/actor were performed.
    // shader is always up-to-date, as far as this mod is concerned.
    return true;
  }

protected:
  vtkGLSLModCamera();
  ~vtkGLSLModCamera() override;

  vtkNew<vtkMatrix3x3> TempMatrix3;
  vtkNew<vtkMatrix4x4> TempMatrix4;

  bool CoordinateShiftAndScaleInUse;
  bool ApplyShiftAndScaleFromShader;
  vtkSmartPointer<vtkMatrix4x4> SSMatrix;

private:
  vtkGLSLModCamera(const vtkGLSLModCamera&) = delete;
  void operator=(const vtkGLSLModCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
