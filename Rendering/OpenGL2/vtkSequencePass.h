// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSequencePass
 * @brief   Execute render passes sequentially.
 *
 * vtkSequencePass executes a list of render passes sequentially.
 * This class allows to define a sequence of render passes at run time.
 * The other solution to write a sequence of render passes is to write an
 * effective subclass of vtkRenderPass.
 *
 * As vtkSequencePass is a vtkRenderPass itself, it is possible to have a
 * hierarchy of render passes built at runtime.
 * @sa
 * vtkRenderPass
 */

#ifndef vtkSequencePass_h
#define vtkSequencePass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderPassCollection;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkSequencePass : public vtkRenderPass
{
public:
  static vtkSequencePass* New();
  vtkTypeMacro(vtkSequencePass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * The ordered list of render passes to execute sequentially.
   * If the pointer is NULL or the list is empty, it is silently ignored.
   * There is no warning.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(Passes, vtkRenderPassCollection);
  virtual void SetPasses(vtkRenderPassCollection* passes);
  ///@}

protected:
  vtkRenderPassCollection* Passes;

  vtkSequencePass();
  ~vtkSequencePass() override;

private:
  vtkSequencePass(const vtkSequencePass&) = delete;
  void operator=(const vtkSequencePass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
