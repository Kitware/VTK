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

#include "vtkglBufferObject.h"
#include <vector>

namespace vtkgl
{

// Process the string, and return a version with replacements.
std::string replace(std::string source, const std::string &search,
                    const std::string replace, bool all = true);

size_t CreateIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                         int num);

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
template<typename T>
VBOLayout CreateTriangleVBO(T* points, T* normals, vtkIdType numPts,
                            unsigned char *colors, int colorComponents,
                            BufferObject &vertexBuffer)
{
  VBOLayout layout;
  // Figure out how big each block will be, currently 6 or 7 floats.
  int blockSize = 3;
  layout.VertexOffset = 0;
  if (normals)
    {
    blockSize += 3;
    layout.NormalOffset = sizeof(float) * 3;
    }
  else
    {
    layout.NormalOffset = 0;
    }

  if (colors)
    {
    ++blockSize;
    layout.ColorComponents = colorComponents;
    layout.ColorOffset = sizeof(float) * 6;
    }
  else
    {
    layout.ColorComponents = 0;
    layout.ColorOffset = 0;
    }
  layout.Stride = sizeof(float) * blockSize;

  // Create a buffer, and copy the data over.
  std::vector<float> packedVBO;
  packedVBO.resize(blockSize * numPts);
  std::vector<float>::iterator it = packedVBO.begin();

  for (vtkIdType i = 0; i < numPts; ++i)
    {
    // Vertices
    *(it++) = *(points++);
    *(it++) = *(points++);
    *(it++) = *(points++);
    if (normals)
      {
      *(it++) = *(normals++);
      *(it++) = *(normals++);
      *(it++) = *(normals++);
      }
    if (colors)
      {
      if (colorComponents == 4)
        {
        *(it++) = *reinterpret_cast<float *>(colors);
        colors += 4;
        }
      else
        {
        unsigned char c[4] = { *(colors++), *(colors++), *(colors++), 255 };
        *(it++) = *reinterpret_cast<float *>(c);
        }
      }
    }
  vertexBuffer.upload(packedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.VertexCount = numPts;
  return layout;
}


} // End namespace

#endif // __vtkGLVBOHelpher_h
