/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLHelper_h
#define vtkOpenGLHelper_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkTimeStamp.h"

class vtkOpenGLIndexBufferObject;
class vtkOpenGLShaderCache;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkWindow;

// Store the shaders, program, and ibo in a common place
// as they are used together frequently.  This is just
// a convenience class.
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLHelper
{
public:
  vtkShaderProgram *Program;
  vtkTimeStamp ShaderSourceTime;
  vtkOpenGLVertexArrayObject *VAO;
  vtkTimeStamp AttributeUpdateTime;

  vtkOpenGLIndexBufferObject *IBO;

  vtkOpenGLHelper();
  ~vtkOpenGLHelper();
  void ReleaseGraphicsResources(vtkWindow *win);
};

#endif // vtkOpenGLHelper_h

// VTK-HeaderTest-Exclude: vtkOpenGLHelper.h
