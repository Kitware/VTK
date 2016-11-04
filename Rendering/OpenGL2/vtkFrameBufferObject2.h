/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObject2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFrameBufferObject2
 * @brief   Interface to OpenGL framebuffer object.
 *
 * Deprecated class, use vtkFrameBufferObject instead
 * The functionality and API of this class has been
 * moved into vtkFrameBufferObject
 *
 * @sa
 * vtkFrameBufferObject
*/

#ifndef vtkFrameBufferObject2_h
#define vtkFrameBufferObject2_h

#include "vtkFrameBufferObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class VTKRENDERINGOPENGL2_EXPORT vtkFrameBufferObject2 : public vtkFrameBufferObject
{
public:
  static vtkFrameBufferObject2* New();
  vtkTypeMacro(vtkFrameBufferObject2, vtkFrameBufferObject);

protected:
  vtkFrameBufferObject2();
  ~vtkFrameBufferObject2();

private:
  vtkFrameBufferObject2(const vtkFrameBufferObject2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFrameBufferObject2&) VTK_DELETE_FUNCTION;
};

#endif
