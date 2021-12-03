/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRRenderer
 * @brief   OpenVR renderer
 *
 * vtkOpenVRRenderer is a concrete implementation of the abstract class
 * vtkVRRenderer.
 */

#ifndef vtkOpenVRRenderer_h
#define vtkOpenVRRenderer_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRRenderer.h"

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderer : public vtkVRRenderer
{
public:
  static vtkOpenVRRenderer* New();
  vtkTypeMacro(vtkOpenVRRenderer, vtkVRRenderer);

  /**
   * Create a new Camera suitable for use with this type of Renderer.
   */
  VTK_NEWINSTANCE vtkCamera* MakeCamera() override;

protected:
  vtkOpenVRRenderer() = default;
  ~vtkOpenVRRenderer() override = default;

private:
  vtkOpenVRRenderer(const vtkOpenVRRenderer&) = delete;
  void operator=(const vtkOpenVRRenderer&) = delete;
};

#endif
