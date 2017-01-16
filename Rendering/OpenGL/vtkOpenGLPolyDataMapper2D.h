/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLPolyDataMapper2D
 * @brief   2D PolyData support for OpenGL
 *
 * vtkOpenGLPolyDataMapper2D provides 2D PolyData annotation support for
 * vtk under OpenGL.  Normally the user should use vtkPolyDataMapper2D
 * which in turn will use this class.
 *
 * @sa
 * vtkPolyDataMapper2D
*/

#ifndef vtkOpenGLPolyDataMapper2D_h
#define vtkOpenGLPolyDataMapper2D_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataMapper2D.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLPolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkOpenGLPolyDataMapper2D, vtkPolyDataMapper2D);
  static vtkOpenGLPolyDataMapper2D *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Actually draw the poly data.
   */
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) VTK_OVERRIDE;

protected:
  vtkOpenGLPolyDataMapper2D() {}
  ~vtkOpenGLPolyDataMapper2D() VTK_OVERRIDE {}

private:
  vtkOpenGLPolyDataMapper2D(const vtkOpenGLPolyDataMapper2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLPolyDataMapper2D&) VTK_DELETE_FUNCTION;
};

#endif
