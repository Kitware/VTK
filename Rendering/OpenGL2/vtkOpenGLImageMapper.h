/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2ImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGL2ImageMapper - 2D image display support for OpenGL
// .SECTION Description
// vtkOpenGL2ImageMapper is a concrete subclass of vtkImageMapper that
// renders images under OpenGL

// .SECTION Caveats
// vtkOpenGL2ImageMapper does not support vtkBitArray, you have to convert the array first
// to vtkUnsignedCharArray (for example)
//
// .SECTION See Also
// vtkImageMapper

#ifndef __vtkOpenGL2ImageMapper_h
#define __vtkOpenGL2ImageMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkImageMapper.h"

class vtkActor2D;
class vtkTexturedActor2D;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2ImageMapper : public vtkImageMapper
{
public:
  static vtkOpenGL2ImageMapper *New();
  vtkTypeMacro(vtkOpenGL2ImageMapper, vtkImageMapper);
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

  // Description:
  // draw the data once it has been converted to uchar, windowed leveled
  // used internally by the templated functions
  void DrawPixels(vtkViewport *vp, int width, int height, int numComponents, void *data);

protected:
  vtkOpenGL2ImageMapper();
  ~vtkOpenGL2ImageMapper();

  vtkTexturedActor2D *Actor;

private:
  vtkOpenGL2ImageMapper(const vtkOpenGL2ImageMapper&);  // Not implemented.
  void operator=(const vtkOpenGL2ImageMapper&);  // Not implemented.
};

#endif
