// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClearZPass
 * @brief   Clear the depth buffer with a given value.
 *
 * Clear the depth buffer with a given value.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkClearZPass_h
#define vtkClearZPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkClearZPass : public vtkRenderPass
{
public:
  static vtkClearZPass* New();
  vtkTypeMacro(vtkClearZPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  ///@{
  /**
   * Set/Get the depth value. Initial value is 1.0 (farthest).
   */
  vtkSetClampMacro(Depth, double, 0.0, 1.0);
  vtkGetMacro(Depth, double);
  ///@}

protected:
  /**
   * Default constructor.
   */
  vtkClearZPass();

  /**
   * Destructor.
   */
  ~vtkClearZPass() override;

  double Depth;

private:
  vtkClearZPass(const vtkClearZPass&) = delete;
  void operator=(const vtkClearZPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
