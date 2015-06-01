/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGLVertexBufferObject_h
#define vtkOpenGLVertexBufferObject_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkOpenGLBufferObject.h"


/**
 * @brief OpenGL vertex buffer object
 *
 * OpenGL buffer object to store geometry and/or attribute data on the
 * GPU.
 */


// useful union for stuffing colors into a float
union vtkucfloat
{
  unsigned char c[4];
  float f;
};

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexBufferObject :
  public vtkOpenGLBufferObject
{
public:
  static vtkOpenGLVertexBufferObject *New();
  vtkTypeMacro(vtkOpenGLVertexBufferObject, vtkOpenGLBufferObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Take the points, and pack them into this VBO. This currently
  // takes whatever the input type might be and packs them into a VBO using
  // floats for the vertices and normals, and unsigned char for the colors (if
  // the array is non-null).
  void CreateVBO(vtkPoints *points, unsigned int numPoints,
      vtkDataArray *normals,
      vtkDataArray *tcoords,
      unsigned char *colors, int colorComponents);

  void AppendVBO(vtkPoints *points, unsigned int numPoints,
      vtkDataArray *normals,
      vtkDataArray *tcoords,
      unsigned char *colors, int colorComponents);

  // Sizes/offsets are all in bytes as OpenGL API expects them.
  size_t VertexCount; // Number of vertices in the VBO
  int Stride;       // The size of a complete vertex + attributes
  int VertexOffset; // Offset of the vertex
  int NormalOffset; // Offset of the normal
  int TCoordOffset; // Offset of the texture coordinates
  int TCoordComponents; // Number of texture dimensions
  int ColorOffset;  // Offset of the color
  int ColorComponents; // Number of color components
  std::vector<float> PackedVBO; // the data

protected:
  vtkOpenGLVertexBufferObject();
  ~vtkOpenGLVertexBufferObject();

private:
  vtkOpenGLVertexBufferObject(const vtkOpenGLVertexBufferObject&); // Not implemented
  void operator=(const vtkOpenGLVertexBufferObject&); // Not implemented
};

#endif
