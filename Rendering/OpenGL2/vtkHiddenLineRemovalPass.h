// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkHiddenLineRemovalPass
 * @brief   RenderPass for HLR.
 *
 *
 * This render pass renders wireframe polydata such that only the front
 * wireframe surface is drawn.
 */

#ifndef vtkHiddenLineRemovalPass_h
#define vtkHiddenLineRemovalPass_h

#include "vtkOpenGLRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <vector> // For std::vector!

VTK_ABI_NAMESPACE_BEGIN
class vtkProp;
class vtkViewport;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkHiddenLineRemovalPass
  : public vtkOpenGLRenderPass
{
public:
  static vtkHiddenLineRemovalPass* New();
  vtkTypeMacro(vtkHiddenLineRemovalPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* s) override;

  /**
   * Returns true if any of the nProps in propArray are rendered as wireframe.
   */
  static bool WireframePropsExist(vtkProp** propArray, int nProps);

protected:
  vtkHiddenLineRemovalPass();
  ~vtkHiddenLineRemovalPass() override;

  void SetRepresentation(std::vector<vtkProp*>& props, int repr);
  int RenderProps(std::vector<vtkProp*>& props, vtkViewport* vp);

private:
  vtkHiddenLineRemovalPass(const vtkHiddenLineRemovalPass&) = delete;
  void operator=(const vtkHiddenLineRemovalPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHiddenLineRemovalPass_h
