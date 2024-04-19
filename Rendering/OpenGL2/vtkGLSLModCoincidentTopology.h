// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModCoincidentTopology
 * @brief   Handle coincident topology
 */

#ifndef vtkGLSLModCoincidentTopology_h
#define vtkGLSLModCoincidentTopology_h

#include "vtkGLSLModifierBase.h"

#include "vtkMatrix3x3.h"              // for ivar
#include "vtkMatrix4x4.h"              // for ivar
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkWeakPointer.h"            // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkInformationObjectBaseKey;
class vtkMapper;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModCoincidentTopology : public vtkGLSLModifierBase
{
public:
  static vtkGLSLModCoincidentTopology* New();
  vtkTypeMacro(vtkGLSLModCoincidentTopology, vtkGLSLModifierBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // vtkGLSLModifierBase virtuals:
  bool ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
    std::string& tessControlShader, std::string& tessEvalShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor) override;
  bool SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
    vtkAbstractMapper* mapper, vtkActor* actor, vtkOpenGLVertexArrayObject* VAO = nullptr) override;
  bool IsUpToDate(vtkOpenGLRenderer* vtkNotUsed(renderer), vtkAbstractMapper* vtkNotUsed(mapper),
    vtkActor* vtkNotUsed(actor)) override
  {
    // always up to date, because once replacements were handled, there is no need to remove them
    return true;
  }

protected:
  vtkGLSLModCoincidentTopology();
  ~vtkGLSLModCoincidentTopology() override;

  void GetCoincidentParameters(
    vtkOpenGLRenderer* ren, vtkMapper* mapper, vtkActor* actor, float& factor, float& offset);

  bool ReplacementsDone = false;

private:
  vtkGLSLModCoincidentTopology(const vtkGLSLModCoincidentTopology&) = delete;
  void operator=(const vtkGLSLModCoincidentTopology&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
