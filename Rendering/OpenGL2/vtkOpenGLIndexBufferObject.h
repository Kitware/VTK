/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGLIndexBufferObject_h
#define vtkOpenGLIndexBufferObject_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkOpenGLBufferObject.h"


/**
 * @brief OpenGL vertex buffer object
 *
 * OpenGL buffer object to store geometry and/or attribute data on the
 * GPU.
 */

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLIndexBufferObject :
  public vtkOpenGLBufferObject
{
public:
  static vtkOpenGLIndexBufferObject *New();
  vtkTypeMacro(vtkOpenGLIndexBufferObject, vtkOpenGLBufferObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Sizes/offsets are all in bytes as OpenGL API expects them.
  size_t IndexCount; // Number of indices in the VBO

  // Description:
  // used to create an IBO for triangle primatives
  size_t CreateTriangleIndexBuffer(vtkCellArray *cells,
     vtkPoints *points);

  // Description:
  // used to create an IBO for triangle primatives
  static void AppendTriangleIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkPoints *points,
    vtkIdType vertexOffset);

  // Description:
  // create a IBO for wireframe polys/tris
  size_t CreateTriangleLineIndexBuffer(vtkCellArray *cells);

  // Description:
  // used to create an IBO for line primatives
  static void AppendLineIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkIdType vertexOffset);

  // Description:
  // create a IBO for wireframe polys/tris
  size_t CreateLineIndexBuffer(vtkCellArray *cells);

  // Description:
  // create a IBO for wireframe polys/tris
  static void AppendTriangleLineIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkIdType vertexOffset);

  // Description:
  // used to create an IBO for primatives as points
  size_t CreatePointIndexBuffer(vtkCellArray *cells);

  // Description:
  // used to create an IBO for primatives as points
  static void AppendPointIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkIdType vertexOffset);

  // Description:
  // used to create an IBO for line strips and triangle strips
  size_t CreateStripIndexBuffer(
    vtkCellArray *cells, bool wireframeTriStrips);

  static void AppendStripIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkIdType vertexOffset,  bool wireframeTriStrips);

  // Description:
  // special index buffer for polys wireframe with edge visibilityflags
  static void AppendEdgeFlagIndexBuffer(
    std::vector<unsigned int> &indexArray,
    vtkCellArray *cells,
    vtkIdType vertexOffset,  vtkDataArray *edgeflags);

  size_t CreateEdgeFlagIndexBuffer(
    vtkCellArray *cells, vtkDataArray *edgeflags);

  // Create supporting arays that are needed when rendering cell data
  // Some VTK cells have to be broken into smaller cells for OpenGL
  // When we have cell data we have to map cell attributes from the VTK
  // cell number to the actual OpenGL cell
  // The following code fills in
  //
  //   cellCellMap which maps a openGL cell id to the VTK cell it came from
  //
  static void CreateCellSupportArrays(
    vtkCellArray *[4],
    std::vector<unsigned int> &cellCellMap,
    int representation,
    vtkPoints *points);

protected:
  vtkOpenGLIndexBufferObject();
  ~vtkOpenGLIndexBufferObject();

private:
  vtkOpenGLIndexBufferObject(const vtkOpenGLIndexBufferObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLIndexBufferObject&) VTK_DELETE_FUNCTION;
};

#endif
