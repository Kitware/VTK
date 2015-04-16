/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkglVBOHelper.h"

#include "vtkCellArray.h"
#include "vtkProperty.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkPolyData.h"
#include "vtkShaderProgram.h"


// we only instantiate some cases to avoid template explosion
#define vtkFloatDoubleTemplateMacro(call)  \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call); \
  vtkTemplateMacroCase(VTK_FLOAT, float, call);


namespace vtkgl {

// internal function called by AppendVBO
template<typename T, typename T2, typename T3>
void TemplatedAppendVBO3(VBOLayout &layout,
  T* points, T2* normals, vtkIdType numPts,
  T3* tcoords, int textureComponents,
  unsigned char *colors, int colorComponents)
{
  // Figure out how big each block will be, currently 6 or 7 floats.
  int blockSize = 3;
  layout.VertexOffset = 0;
  layout.NormalOffset = 0;
  layout.TCoordOffset = 0;
  layout.TCoordComponents = 0;
  layout.ColorComponents = 0;
  layout.ColorOffset = 0;
  if (normals)
    {
    layout.NormalOffset = sizeof(float) * blockSize;
    blockSize += 3;
    }
  if (tcoords)
    {
    layout.TCoordOffset = sizeof(float) * blockSize;
    layout.TCoordComponents = textureComponents;
    blockSize += textureComponents;
    }
  if (colors)
    {
    layout.ColorComponents = colorComponents;
    layout.ColorOffset = sizeof(float) * blockSize;
    ++blockSize;
    }
  layout.Stride = sizeof(float) * blockSize;

  // Create a buffer, and copy the data over.
  layout.PackedVBO.resize(blockSize * (numPts + layout.VertexCount));
  std::vector<float>::iterator it = layout.PackedVBO.begin()
    + (layout.VertexCount*layout.Stride/sizeof(float));

  T *pointPtr;
  T2 *normalPtr;
  T3 *tcoordPtr;
  unsigned char *colorPtr;

  // TODO: optimize this somehow, lots of if statements in here
  for (vtkIdType i = 0; i < numPts; ++i)
    {
    pointPtr = points + i*3;
    normalPtr = normals + i*3;
    tcoordPtr = tcoords + i*textureComponents;
    colorPtr = colors + i*colorComponents;

    // Vertices
    *(it++) = *(pointPtr++);
    *(it++) = *(pointPtr++);
    *(it++) = *(pointPtr++);
    if (normals)
      {
      *(it++) = *(normalPtr++);
      *(it++) = *(normalPtr++);
      *(it++) = *(normalPtr++);
      }
    if (tcoords)
      {
      for (int j = 0; j < textureComponents; ++j)
        {
        *(it++) = *(tcoordPtr++);
        }
      }
    if (colors)
      {
      if (colorComponents == 4)
        {
        *(it++) = *reinterpret_cast<float *>(colorPtr);
        }
      else
        {
        unsigned char c[4];
        c[0] = *(colorPtr++);
        c[1] = *(colorPtr++);
        c[2] = *(colorPtr);
        c[3] =  255;
        *(it++) = *reinterpret_cast<float *>(c);
        }
      }
    }
  layout.VertexCount += numPts;
}

//----------------------------------------------------------------------------
template<typename T, typename T2>
void TemplatedAppendVBO2(VBOLayout &layout,
  T* points, T2 *normals, vtkIdType numPts,
  vtkDataArray *tcoords,
  unsigned char *colors, int colorComponents)
{
  if (tcoords)
    {
    switch(tcoords->GetDataType())
      {
      vtkFloatDoubleTemplateMacro(
        TemplatedAppendVBO3(layout, points, normals,
                  numPts,
                  static_cast<VTK_TT*>(tcoords->GetVoidPointer(0)),
                  tcoords->GetNumberOfComponents(),
                  colors, colorComponents));
      }
    }
  else
    {
    TemplatedAppendVBO3(layout, points, normals,
                        numPts, (float *)NULL, 0,
                        colors, colorComponents);
    }
}

//----------------------------------------------------------------------------
template<typename T>
void TemplatedAppendVBO(VBOLayout &layout,
  T* points, vtkDataArray *normals, vtkIdType numPts,
  vtkDataArray *tcoords,
  unsigned char *colors, int colorComponents)
{
  if (normals)
    {
    switch(normals->GetDataType())
      {
      vtkFloatDoubleTemplateMacro(
        TemplatedAppendVBO2(layout, points,
                  static_cast<VTK_TT*>(normals->GetVoidPointer(0)),
                  numPts, tcoords, colors, colorComponents));
      }
    }
  else
    {
    TemplatedAppendVBO2(layout, points,
                        (float *)NULL,
                        numPts, tcoords, colors, colorComponents);
    }
}

// Take the points, and pack them into the VBO object supplied. This currently
// takes whatever the input type might be and packs them into a VBO using
// floats for the vertices and normals, and unsigned char for the colors (if
// the array is non-null).
void AppendVBO(VBOLayout &layout,
  vtkPoints *points, unsigned int numPts,
  vtkDataArray *normals,
  vtkDataArray *tcoords,
  unsigned char *colors, int colorComponents)
{
  switch(points->GetDataType())
    {
    vtkTemplateMacro(
      TemplatedAppendVBO(layout, static_cast<VTK_TT*>(points->GetVoidPointer(0)),
                normals, numPts, tcoords, colors, colorComponents));
    }
}

// create a VBO, append the data to it, then upload it
VBOLayout CreateVBO(
  vtkPoints *points, unsigned int numPts,
  vtkDataArray *normals,
  vtkDataArray *tcoords,
  unsigned char *colors, int colorComponents,
  BufferObject &vertexBuffer)
{
  VBOLayout layout;

  // fast path
  if (!tcoords && !normals && !colors && points->GetDataType() == VTK_FLOAT)
    {
    int blockSize = 3;
    layout.VertexOffset = 0;
    layout.NormalOffset = 0;
    layout.TCoordOffset = 0;
    layout.TCoordComponents = 0;
    layout.ColorComponents = 0;
    layout.ColorOffset = 0;
    layout.Stride = sizeof(float) * blockSize;
    layout.VertexCount = numPts;
    vertexBuffer.Upload((float *)(points->GetVoidPointer(0)), numPts*3,
      vtkgl::BufferObject::ArrayBuffer);
    return layout;
    }

  // slower path
  layout.VertexCount = 0;
  AppendVBO(layout,points,numPts,normals,tcoords,colors,colorComponents);
  vertexBuffer.Upload(layout.PackedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.PackedVBO.resize(0);
  return layout;
}

// Process the string, and return a version with replacements.
std::string replace(std::string source, const std::string &search,
                    const std::string replace, bool all)
{
  std::string::size_type pos = 0;
  while ((pos = source.find(search, 0)) != std::string::npos)
    {
    source.replace(pos, search.length(), replace);
    if (!all)
      {
      return source;
      }
    pos += search.length();
    }
  return source;
}

// Process the string, and return a version with replacements.
bool substitute(std::string &source, const std::string &search,
             const std::string replace, bool all)
{
  std::string::size_type pos = 0;
  bool replaced = false;
  while ((pos = source.find(search, 0)) != std::string::npos)
    {
    source.replace(pos, search.length(), replace);
    if (!all)
      {
      return true;
      }
    pos += search.length();
    replaced = true;
    }
  return replaced;
}

// used to create an IBO for triangle primatives
void AppendTriangleIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkPoints *points,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() +
    (cells->GetNumberOfConnectivityEntries() -
     cells->GetNumberOfCells()*3)*3;
  if (targetSize > indexArray.capacity())
    {
    if (targetSize < indexArray.capacity()*1.5)
      {
      targetSize = indexArray.capacity()*1.5;
      }
    indexArray.reserve(targetSize);
    }

  // the folowing are only used if we have to triangulate a polygon
  // otherwise they just sit at NULL
  vtkPolygon *polygon = NULL;
  vtkIdList *tris = NULL;
  vtkPoints *triPoints = NULL;

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    // ignore degenerate triangles
    if (npts < 3)
      {
      continue;
      }

    // triangulate needed
    if (npts > 3)
      {
      // special case for quads, penta, hex which are common
      if (npts == 4)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[1]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        }
      else if (npts == 5)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[1]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[4]+vOffset));
        }
      else if (npts == 6)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[1]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[0]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[5]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[3]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[4]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(indices[5]+vOffset));
        }
      else // 7 sided polygon or higher, do a full smart triangulation
        {
        if (!polygon)
          {
          polygon = vtkPolygon::New();
          tris = vtkIdList::New();
          triPoints = vtkPoints::New();
          }

        vtkIdType *triIndices = new vtkIdType[npts];
        triPoints->SetNumberOfPoints(npts);
        for (int i = 0; i < npts; ++i)
          {
          int idx = indices[i];
          triPoints->SetPoint(i,points->GetPoint(idx));
          triIndices[i] = i;
          }
        polygon->Initialize(npts, triIndices, triPoints);
        polygon->Triangulate(tris);
        for (int j = 0; j < tris->GetNumberOfIds(); ++j)
          {
          indexArray.push_back(static_cast<unsigned int>(
            indices[tris->GetId(j)]+vOffset));
          }
        delete [] triIndices;
        }
      }
    else
      {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)+vOffset));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)+vOffset));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)+vOffset));
      }
    }
  if (polygon)
    {
    polygon->Delete();
    tris->Delete();
    triPoints->Delete();
    }
}

// used to create an IBO for triangle primatives
size_t CreateTriangleIndexBuffer(
  vtkCellArray *cells, BufferObject &indexBuffer,
  vtkPoints *points)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendTriangleIndexBuffer(indexArray, cells, points, 0);
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for point primatives
void AppendPointIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() +
    cells->GetNumberOfConnectivityEntries() -
    cells->GetNumberOfCells();
  if (targetSize > indexArray.capacity())
    {
    if (targetSize < indexArray.capacity()*1.5)
      {
      targetSize = indexArray.capacity()*1.5;
      }
    indexArray.reserve(targetSize);
    }

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    for (int i = 0; i < npts; ++i)
      {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)+vOffset));
      }
    }
}

// used to create an IBO for triangle primatives
size_t CreatePointIndexBuffer(
  vtkCellArray *cells, BufferObject &indexBuffer)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendPointIndexBuffer(indexArray, cells, 0);
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}


// used to create an IBO for primatives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
void AppendTriangleLineIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() + 2*(
    cells->GetNumberOfConnectivityEntries() -
    cells->GetNumberOfCells());
  if (targetSize > indexArray.capacity())
    {
    if (targetSize < indexArray.capacity()*1.5)
      {
      targetSize = indexArray.capacity()*1.5;
      }
    indexArray.reserve(targetSize);
    }

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    for (int i = 0; i < npts; ++i)
      {
      indexArray.push_back(static_cast<unsigned int>(indices[i]+vOffset));
      indexArray.push_back(static_cast<unsigned int>(
        indices[i < npts-1 ? i+1 : 0] + vOffset));
      }
    }
}

// used to create an IBO for primatives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
size_t CreateTriangleLineIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendTriangleLineIndexBuffer(indexArray, cells, 0);
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for primatives as lines.  This method treats each line segment
// as independent.  So for a line strip you would get multiple line segments out
void AppendLineIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() + 2*(
    cells->GetNumberOfConnectivityEntries()
    - 2*cells->GetNumberOfCells());
  if (targetSize > indexArray.capacity())
    {
    if (targetSize < indexArray.capacity()*1.5)
      {
      targetSize = indexArray.capacity()*1.5;
      }
    indexArray.reserve(targetSize);
    }

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    for (int i = 0; i < npts-1; ++i)
      {
      indexArray.push_back(static_cast<unsigned int>(indices[i]+vOffset));
      indexArray.push_back(static_cast<unsigned int>(indices[i+1] + vOffset));
      }
    }
}

// used to create an IBO for primatives as lines.  This method treats each line segment
// as independent.  So for a line strip you would get multiple line segments out
size_t CreateLineIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendLineIndexBuffer(indexArray, cells, 0);
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for triangle strips
size_t CreateStripIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                              bool wireframeTriStrips)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;
  std::vector<unsigned int> indexArray;

  size_t triCount = cells->GetNumberOfConnectivityEntries()
    - 3*cells->GetNumberOfCells();
  size_t targetSize = wireframeTriStrips ? 2*(triCount*2+1)
   : triCount*3;
  indexArray.reserve(targetSize);

  if (wireframeTriStrips)
    {
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
      {
      indexArray.push_back(static_cast<unsigned int>(pts[0]));
      indexArray.push_back(static_cast<unsigned int>(pts[1]));
      for (int j = 0; j < npts-2; ++j)
        {
        indexArray.push_back(static_cast<unsigned int>(pts[j]));
        indexArray.push_back(static_cast<unsigned int>(pts[j+2]));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1]));
        indexArray.push_back(static_cast<unsigned int>(pts[j+2]));
        }
      }
    }
  else
    {
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
      {
      for (int j = 0; j < npts-2; ++j)
        {
        indexArray.push_back(static_cast<unsigned int>(pts[j]));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1+j%2]));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1+(j+1)%2]));
        }
      }
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for polys in wireframe with edge flags
size_t CreateEdgeFlagIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                                 vtkDataArray *ef)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;
  std::vector<unsigned int> indexArray;
  unsigned char *ucef = NULL;
  ucef = vtkUnsignedCharArray::SafeDownCast(ef)->GetPointer(0);
  indexArray.reserve(cells->GetData()->GetSize()*2);
  for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
    {
    for (int j = 0; j < npts; ++j)
      {
      if (ucef[pts[j]] && npts > 1) // draw this edge and poly is not degenerate
        {
        // determine the ending vertex
        vtkIdType nextVert = (j == npts-1) ? pts[0] : pts[j+1];
        indexArray.push_back(static_cast<unsigned int>(pts[j]));
        indexArray.push_back(static_cast<unsigned int>(nextVert));
        }
      }
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// Create supporting arays that are needed when rendering cell data
// Some VTK cells have to be broken into smaller cells for OpenGL
// Wen we have cell data we have to map cell attributes from the VTK
// cell number to the actual OpenGL cell
// The following code fills in
//
//   cellCellMap which maps a openGL cell id to the VTK cell it came from
//
void CreateCellSupportArrays(vtkCellArray *prims[4],
                             std::vector<unsigned int> &cellCellMap,
                             int representation)
{
  // need an array to track what points to orig points
  size_t minSize = prims[0]->GetNumberOfCells() +
                   prims[1]->GetNumberOfCells() +
                   prims[2]->GetNumberOfCells() +
                   prims[3]->GetNumberOfCells();
  vtkIdType* indices(NULL);
  vtkIdType npts(0);

  // make sure we have at least minSize
  cellCellMap.reserve(minSize);
  unsigned int vtkCellCount = 0;

  // points
  for (prims[0]->InitTraversal(); prims[0]->GetNextCell(npts, indices); )
    {
    for (int i=0; i < npts; ++i)
      {
      cellCellMap.push_back(vtkCellCount);
      }
    vtkCellCount++;
    } // for cell

  if (representation == VTK_POINTS)
    {
    for (int j = 1; j < 4; j++)
      {
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices); )
        {
        for (int i=0; i < npts; ++i)
          {
          cellCellMap.push_back(vtkCellCount);
          }
        vtkCellCount++;
        } // for cell
      }
    }
  else // lines or surfaces
    {
    // lines
    for (prims[1]->InitTraversal(); prims[1]->GetNextCell(npts, indices); )
      {
      for (int i = 0; i < npts-1; ++i)
        {
        cellCellMap.push_back(vtkCellCount);
        }
      vtkCellCount++;
      } // for cell

    if (representation == VTK_WIREFRAME)
      {
      // polys
      for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices); )
        {
        for (int i = 0; i < npts; ++i)
          {
          cellCellMap.push_back(vtkCellCount);
          }
        vtkCellCount++;
        } // for cell

      // strips
      for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices); )
        {
        cellCellMap.push_back(vtkCellCount);
        for (int i = 2; i < npts; ++i)
          {
          cellCellMap.push_back(vtkCellCount);
          cellCellMap.push_back(vtkCellCount);
          }
        vtkCellCount++;
        } // for cell
      }
    else
      {
      // polys
      for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices); )
        {
        if (npts > 2)
          {
          for (int i = 2; i < npts; ++i)
            {
            cellCellMap.push_back(vtkCellCount);
            }
          vtkCellCount++;
          }
        } // for cell

      // strips
      for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices); )
        {
        for (int i = 2; i < npts; ++i)
          {
          cellCellMap.push_back(vtkCellCount);
          }
        vtkCellCount++;
        } // for cell
      }
    }

}


void CellBO::ReleaseGraphicsResources(vtkWindow * vtkNotUsed(win))
{
  if (this->Program)
    {
    // Let ShaderCache release the graphics resources as it is
    // responsible for creation and deletion.
    this->Program = 0;
    }
  this->ibo.ReleaseGraphicsResources();
  this->vao.ReleaseGraphicsResources();
}

}
