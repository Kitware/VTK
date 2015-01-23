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
// .NAME vtkOpenGLImageMapper - 2D image display support for OpenGL
// .SECTION Description
// vtkOpenGLImageMapper is a concrete subclass of vtkImageMapper that
// renders images under OpenGL

// .SECTION Caveats
// vtkOpenGLImageMapper does not support vtkBitArray, you have to convert the array first
// to vtkUnsignedCharArray (for example)
//
// .SECTION See Also
// vtkImageMapper

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
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Handle the render method.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
    { this->RenderStart(viewport, actor); }

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data,
                  vtkActor2D* actor);

protected:
  vtkOpenGLImageMapper();
  ~vtkOpenGLImageMapper();

private:
  vtkOpenGLImageMapper(const vtkOpenGLImageMapper&);  // Not implemented.
  void operator=(const vtkOpenGLImageMapper&);  // Not implemented.
};

#endif
