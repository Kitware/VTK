// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClearRGBPass
 * @brief   Paint in the color buffer.
 *
 * Clear the color buffer to the specified color.
 *
 * @sa
 * vtkValuePasses
 */

#ifndef vtkClearRGBPass_h
#define vtkClearRGBPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkClearRGBPass : public vtkRenderPass
{
public:
  static vtkClearRGBPass* New();
  vtkTypeMacro(vtkClearRGBPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  void Render(const vtkRenderState* s) override;

  ///@{
  /**
   * Set/Get the background color of the rendering screen using an rgb color
   * specification.
   */
  vtkSetVector3Macro(Background, double);
  vtkGetVector3Macro(Background, double);
  ///@}

protected:
  /**
   * Default constructor.
   */
  vtkClearRGBPass();

  /**
   * Destructor.
   */
  ~vtkClearRGBPass() override;

  double Background[3];

private:
  vtkClearRGBPass(const vtkClearRGBPass&) = delete;
  void operator=(const vtkClearRGBPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
