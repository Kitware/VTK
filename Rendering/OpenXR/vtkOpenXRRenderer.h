// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRRenderer
 * @brief   OpenXR renderer
 *
 * vtkOpenXRRenderer is a concrete implementation of the abstract class
 * vtkVRRenderer. vtkOpenXRRenderer interfaces to the OpenXR rendering library.
 */

#ifndef vtkOpenXRRenderer_h
#define vtkOpenXRRenderer_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRRenderer : public vtkVRRenderer
{
public:
  static vtkOpenXRRenderer* New();
  vtkTypeMacro(vtkOpenXRRenderer, vtkVRRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a new Camera suitable for use with this type of Renderer.
   */
  VTK_NEWINSTANCE vtkCamera* MakeCamera() override;

protected:
  vtkOpenXRRenderer();
  ~vtkOpenXRRenderer() override = default;

private:
  vtkOpenXRRenderer(const vtkOpenXRRenderer&) = delete;
  void operator=(const vtkOpenXRRenderer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
