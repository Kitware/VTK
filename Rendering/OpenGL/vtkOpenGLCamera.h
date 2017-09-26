/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLCamera
 * @brief   OpenGL camera
 *
 * vtkOpenGLCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkOpenGLCamera interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLCamera_h
#define vtkOpenGLCamera_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCamera.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLCamera : public vtkCamera
{
public:
  static vtkOpenGLCamera *New();
  vtkTypeMacro(vtkOpenGLCamera, vtkCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer *ren) override;

  void UpdateViewport(vtkRenderer *ren) override;

protected:
  vtkOpenGLCamera() {}
  ~vtkOpenGLCamera() override {}
private:
  vtkOpenGLCamera(const vtkOpenGLCamera&) = delete;
  void operator=(const vtkOpenGLCamera&) = delete;
};

#endif
