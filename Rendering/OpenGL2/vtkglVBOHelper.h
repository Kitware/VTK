/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkGLVBOHelpher_h
#define __vtkGLVBOHelpher_h

#include <GL/glew.h>

#include "vtkglBufferObject.h"
#include "vtkglShader.h"
#include "vtkglShaderProgram.h"
#include <vector>

namespace vtkgl
{

// Process the string, and return a version with replacements.
std::string replace(std::string source, const std::string &search,
                    const std::string replace, bool all = true);

// used to create an IBO for triangle primatives
size_t CreateTriangleIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                                 vtkPoints *points);

// used to create an IBO for point primatives
size_t CreatePointIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer);

// used to create an IBO for line strips and triangle strips
size_t CreateMultiIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                              std::vector<GLintptr> &memoryOffsetArray,
                              std::vector<unsigned int> &elementCountArray);

// Store the shaders, program, and ibo in a common struct.
struct CellBO
{
  Shader vs;
  Shader fs;
  const char* vsFile;
  const char* fsFile;
  ShaderProgram program;
  BufferObject ibo;
  size_t indexCount;
  // These are client side objects for multi draw where IBOs are not used.
  std::vector<ptrdiff_t> offsetArray;
  std::vector<unsigned int> elementsArray;
  vtkTimeStamp buildTime;
};

// Sizes/offsets are all in bytes as OpenGL API expects them.
struct VBOLayout
{
  size_t VertexCount; // Number of vertices in the VBO
  int Stride;       // The size of a complete vertex + attributes
  int VertexOffset; // Offset of the vertex
  int NormalOffset; // Offset of the normal
  int ColorOffset;  // Offset of the color
  int ColorComponents; // Number of color components
};

// Take the points, and pack them into the VBO object supplied. This currently
// takes whatever the input type might be and packs them into a VBO using
// floats for the vertices and normals, and unsigned char for the colors (if
// the array is non-null).
VBOLayout CreateVBO(vtkPoints *points, vtkDataArray *normals,
                    unsigned char *colors, int colorComponents,
                    BufferObject &vertexBuffer);



} // End namespace

#endif // __vtkGLVBOHelpher_h
