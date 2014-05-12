/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"

#include "vtkglVBOHelper.h"

namespace vtkgl {

// internal function called by CreateVBO
template<typename T, typename T2>
VBOLayout TemplatedCreateVBO2(T* points, T2* normals, vtkIdType numPts,
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
  vertexBuffer.Upload(packedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.VertexCount = numPts;
  return layout;
}


//----------------------------------------------------------------------------
template<typename T>
VBOLayout TemplatedCreateVBO(T* points, vtkDataArray *normals, vtkIdType numPts,
                    unsigned char *colors, int colorComponents,
                    BufferObject &vertexBuffer)
{
  if (normals)
    {
    switch(normals->GetDataType())
      {
      vtkTemplateMacro(
        return
          TemplatedCreateVBO2(points,
                    static_cast<VTK_TT*>(normals->GetVoidPointer(0)),
                    numPts,
                    colors,
                    colorComponents,
                    vertexBuffer));
      }
    }
  else
    {
    return
      TemplatedCreateVBO2(points,
                          (float *)NULL,
                          numPts,
                          colors,
                          colorComponents,
                          vertexBuffer);
    }
  return VBOLayout();
}


// Take the points, and pack them into the VBO object supplied. This currently
// takes whatever the input type might be and packs them into a VBO using
// floats for the vertices and normals, and unsigned char for the colors (if
// the array is non-null).
VBOLayout CreateVBO(vtkPoints *points, vtkDataArray *normals,
                    unsigned char *colors, int colorComponents,
                    BufferObject &vertexBuffer)
{
  switch(points->GetDataType())
    {
    vtkTemplateMacro(
      return
        TemplatedCreateVBO(static_cast<VTK_TT*>(points->GetVoidPointer(0)),
                  normals,
                  points->GetNumberOfPoints(),
                  colors,
                  colorComponents,
                  vertexBuffer));
    }
  return VBOLayout();
}

// Process the string, and return a version with replacements.
std::string replace(std::string source, const std::string &search,
                    const std::string replace, bool all)
{
  std::string::size_type pos = 0;
  bool first = true;
  while ((pos = source.find(search, 0)) != std::string::npos)
    {
    source.replace(pos, search.length(), replace);
    pos += search.length();
    if (first)
      {
      first = false;
      if (!all)
        {
        return source;
        }
      }
    }
  return source;
}


// used to create an IBO for triangle primatives
size_t CreateTriangleIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                                 vtkPoints *points)
{
  std::vector<unsigned int> indexArray;
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  indexArray.reserve(cells->GetNumberOfCells() * 3);
  vtkPolygon *polygon = NULL;
  vtkIdList *tris = NULL;

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    if (npts < 3)
      {
      exit(-1); // assert(points >= 3);
      }

    // triangulate needed
    if (npts > 3)
      {
      // special case for quads which VTK uses a lot
      if (npts == 4)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[1]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        }
      else
        {
        if (!polygon)
          {
          polygon = vtkPolygon::New();
          tris = vtkIdList::New();
          }
        polygon->Initialize(npts, indices, points);
        polygon->Triangulate(tris);
        for (int j = 0; j < tris->GetNumberOfIds(); ++j)
          {
          indexArray.push_back(static_cast<unsigned int>(indices[tris->GetId(j)]));
          }
        }
      }
    else
      {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      }
    }
  if (polygon)
    {
    polygon->Delete();
    tris->Delete();
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for point primatives
size_t CreatePointIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer)
{
  std::vector<unsigned int> indexArray;
  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  indexArray.reserve(cells->GetNumberOfCells());

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
    {
    indexArray.push_back(static_cast<unsigned int>(*(indices++)));
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

// used to create an IBO for stripped primatives such as lines and triangle strips
size_t CreateMultiIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                              std::vector<GLintptr> &memoryOffsetArray,
                              std::vector<unsigned int> &elementCountArray)
{
  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;
  std::vector<unsigned int> indexArray;
  unsigned int count = 0;
  indexArray.reserve(cells->GetData()->GetSize());
  for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
    {
    memoryOffsetArray.push_back(count*sizeof(unsigned int));
    elementCountArray.push_back(npts);
    for (int j = 0; j < npts; ++j)
      {
      indexArray.push_back(static_cast<unsigned int>(pts[j]));
      count++;
      }
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

}
