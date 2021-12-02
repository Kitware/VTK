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

#endif
