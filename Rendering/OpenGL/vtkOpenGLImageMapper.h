/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLImageMapper
 * @brief   2D image display support for OpenGL
 *
 * vtkOpenGLImageMapper is a concrete subclass of vtkImageMapper that
 * renders images under OpenGL
 *
 * @warning
 * vtkOpenGLImageMapper does not support vtkBitArray, you have to convert the array first
 * to vtkUnsignedCharArray (for example)
 *
 * @sa
 * vtkImageMapper
*/

#ifndef vtkOpenGLImageMapper_h
#define vtkOpenGLImageMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkImageMapper.h"

class vtkActor2D;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLImageMapper : public vtkImageMapper
{
public:
  static vtkOpenGLImageMapper *New();
  vtkTypeMacro(vtkOpenGLImageMapper, vtkImageMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Handle the render method.
   */
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) VTK_OVERRIDE
    { this->RenderStart(viewport, actor); }

  /**
   * Called by the Render function in vtkImageMapper.  Actually draws
   * the image to the screen.
   */
  void RenderData(vtkViewport* viewport, vtkImageData* data,
                  vtkActor2D* actor) VTK_OVERRIDE;

protected:
  vtkOpenGLImageMapper();
  ~vtkOpenGLImageMapper() VTK_OVERRIDE;

private:
  vtkOpenGLImageMapper(const vtkOpenGLImageMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLImageMapper&) VTK_DELETE_FUNCTION;
};

#endif
