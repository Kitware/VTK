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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer *ren) VTK_OVERRIDE;

  void UpdateViewport(vtkRenderer *ren) VTK_OVERRIDE;

protected:
  vtkOpenGLCamera() {}
  ~vtkOpenGLCamera() VTK_OVERRIDE {}
private:
  vtkOpenGLCamera(const vtkOpenGLCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLCamera&) VTK_DELETE_FUNCTION;
};

#endif
