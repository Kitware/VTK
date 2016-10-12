/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLTextActor3D
 * @brief   OpenGL2 override for vtkTextActor3D.
*/

#ifndef vtkOpenGLTextActor3D_h
#define vtkOpenGLTextActor3D_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTextActor3D.h"

class vtkOpenGLGL2PSHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextActor3D: public vtkTextActor3D
{
public:
  static vtkOpenGLTextActor3D* New();
  vtkTypeMacro(vtkOpenGLTextActor3D, vtkTextActor3D)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);

protected:
  vtkOpenGLTextActor3D();
  ~vtkOpenGLTextActor3D();

  int RenderGL2PS(vtkViewport *vp, vtkOpenGLGL2PSHelper *gl2ps);

private:
  vtkOpenGLTextActor3D(const vtkOpenGLTextActor3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLTextActor3D&) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLTextActor3D_h
