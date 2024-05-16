// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpaquePass
 * @brief   Render the opaque geometry with property key
 * filtering.
 *
 * vtkOpaquePass renders the opaque geometry of all the props that have the
 * keys contained in vtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
 */

#ifndef vtkOpaquePass_h
#define vtkOpaquePass_h

#include "vtkDefaultPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpaquePass : public vtkDefaultPass
{
public:
  static vtkOpaquePass* New();
  vtkTypeMacro(vtkOpaquePass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  vtkOpaquePass();

  /**
   * Destructor.
   */
  ~vtkOpaquePass() override;

private:
  vtkOpaquePass(const vtkOpaquePass&) = delete;
  void operator=(const vtkOpaquePass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
