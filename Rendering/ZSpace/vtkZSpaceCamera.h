// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceCamera
 * @brief   Extends vtkOpenGLCamera to use custom view and projection matrix given by zSpace SDK.
 *
 * This is needed to change the view / projection matrix during a render(), depending on the
 * vtkCamera::LeftEye value (support for stereo).
 */

#ifndef vtkZSpaceCamera_h
#define vtkZSpaceCamera_h

#include "vtkOpenGLCamera.h"
#include "vtkRenderingZSpaceModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceCamera : public vtkOpenGLCamera
{
public:
  static vtkZSpaceCamera* New();
  vtkTypeMacro(vtkZSpaceCamera, vtkOpenGLCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the model view matrix of model view transform given by zSpace SDK.
   */
  vtkMatrix4x4* GetModelViewTransformMatrix() override;

  /**
   * Return the projection transform matrix given by zSpace SDK.
   */
  vtkMatrix4x4* GetProjectionTransformMatrix(double aspect, double nearz, double farz) override;

protected:
  vtkZSpaceCamera() = default;
  ~vtkZSpaceCamera() override = default;

private:
  vtkZSpaceCamera(const vtkZSpaceCamera&) = delete;
  void operator=(const vtkZSpaceCamera&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
