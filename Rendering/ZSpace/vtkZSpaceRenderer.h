// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceRenderer
 * @brief   VR renderer
 *
 * vtkZSpaceRenderer is a vtkRenderer subclass that is meant to be used with the ZSpace hardware.
 * It redefines in particular the ResetCamera function, that uses the zSpace SDK to retrieve
 * the "comfort zone" of the stereo frustum and fit bounding box in it.
 */

#ifndef vtkZSpaceRenderer_h
#define vtkZSpaceRenderer_h

#include "vtkNew.h" // For ivar
#include "vtkOpenGLRenderer.h"
#include "vtkRenderingZSpaceModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkZSpaceCamera;

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceRenderer : public vtkOpenGLRenderer
{
public:
  static vtkZSpaceRenderer* New();
  vtkTypeMacro(vtkZSpaceRenderer, vtkOpenGLRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and the
   * bounding box fits in the comfort zone (coupled zone) of the ZSpace
   * camera frustum.
   */
  void ResetCamera(const double bounds[6]) override;

  /**
   * @brief Create a new vtkZSpaceCamera.
   *
   * @return vtkZSpaceCamera*
   */
  vtkCamera* MakeCamera() override;

protected:
  vtkZSpaceRenderer();
  ~vtkZSpaceRenderer() override = default;

private:
  vtkZSpaceRenderer(const vtkZSpaceRenderer&) = delete;
  void operator=(const vtkZSpaceRenderer&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
