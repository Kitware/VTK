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

#include "vtkRenderingOpenGL2Module.h" // for export macro

#include "vtkglBufferObject.h"
#include "vtkglVertexArrayObject.h"

#include "vtk_glew.h" // used for struct ivars
#include <vector> // used for struct ivars
#include "vtkTimeStamp.h" // used for struct ivars

class vtkCellArray;
class vtkPoints;
class vtkDataArray;
class vtkPolyData;
class vtkOpenGLShaderCache;
class vtkWindow;

namespace vtkgl
{

// Process the string, and return a version with replacements.
std::string VTKRENDERINGOPENGL2_EXPORT replace(std::string source, const std::string &search,
                    const std::string replace, bool all = true);

// used to create an IBO for triangle primatives
size_t CreateTriangleIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                                 vtkPoints *points, std::vector<unsigned int> &cellPointMap);

// used to create an IBO for point primatives
size_t CreatePointIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer);

// used to create an IBO for line strips and triangle strips
size_t CreateMultiIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                              std::vector<GLintptr> &memoryOffsetArray,
                              std::vector<unsigned int> &elementCountArray,
                              bool wireframeTriStrips);

// special index buffer for polys wireframe with edge visibilityflags
size_t CreateEdgeFlagIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                                 vtkDataArray *edgeflags);

// Store the shaders, program, and ibo in a common struct.
class VTKRENDERINGOPENGL2_EXPORT CellBO
{
public:
  vtkShaderProgram *Program;
  BufferObject ibo;
  VertexArrayObject vao;
  vtkTimeStamp ShaderSourceTime;

  size_t indexCount;
  // These are client side objects for multi draw where IBOs are not used.
  std::vector<GLintptr> offsetArray;
  std::vector<unsigned int> elementsArray;

  vtkTimeStamp attributeUpdateTime;

  CellBO() {this->Program = NULL; };
  void ReleaseGraphicsResources(vtkWindow *win);
};

// Sizes/offsets are all in bytes as OpenGL API expects them.
struct VBOLayout
{
  size_t VertexCount; // Number of vertices in the VBO
  int Stride;       // The size of a complete vertex + attributes
  int VertexOffset; // Offset of the vertex
  int NormalOffset; // Offset of the normal
  int TCoordOffset; // Offset of the texture coordinates
  int TCoordComponents; // Number of texture dimensions
  int ColorOffset;  // Offset of the color
  int ColorComponents; // Number of color components
};

// Take the points, and pack them into the VBO object supplied. This currently
// takes whatever the input type might be and packs them into a VBO using
// floats for the vertices and normals, and unsigned char for the colors (if
// the array is non-null).
VBOLayout CreateVBO(vtkPoints *points, unsigned int numPoints, vtkDataArray *normals,
                    vtkDataArray *tcoords,
                    unsigned char *colors, int colorComponents,
                    BufferObject &vertexBuffer, unsigned int *cellPointMap, unsigned int *pointCellMap,
                    bool cellScalars, bool cellNormals);


// used to create an IBO for stripped primatives such as lines and triangle strips
void CreateCellSupportArrays(vtkPolyData *poly, vtkCellArray *[4],
                             std::vector<unsigned int> &cellPointMap,
                             std::vector<unsigned int> &pointCellMap);

} // End namespace

#endif // __vtkGLVBOHelpher_h

// VTK-HeaderTest-Exclude: vtkglVBOHelper.h
