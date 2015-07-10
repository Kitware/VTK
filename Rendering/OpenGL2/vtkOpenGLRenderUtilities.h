/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLRenderUtilities - OpenGL rendering utility functions
// .SECTION Description
// vtkOpenGLRenderUtilities provides functions to help render primitives.

#ifndef vtkOpenGLRenderUtilities_h
#define vtkOpenGLRenderUtilities_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include "vtk_glew.h" // Needed for GLuint.

class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkOpenGLRenderUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Helper function that draws a quad on the screen
  // at the specified vertex coordinates and if
  // tcoords are not NULL with the specified
  // texture coordinates.
  static void RenderQuad(
    float *verts, float *tcoords,
    vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao);
  static void RenderTriangles(
    float *verts, unsigned int numVerts,
    GLuint *indices, unsigned int numIndices,
    float *tcoords,
    vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao);

protected:
  vtkOpenGLRenderUtilities();
  ~vtkOpenGLRenderUtilities();

private:
  vtkOpenGLRenderUtilities(const vtkOpenGLRenderUtilities&);  // Not implemented.
  void operator=(const vtkOpenGLRenderUtilities&);  // Not implemented.
};

#endif
