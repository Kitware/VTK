/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkObjectFactory.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkCellArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_glew.h"

#include <set>

vtkStandardNewMacro(vtkOpenGLIndexBufferObject)

vtkOpenGLIndexBufferObject::vtkOpenGLIndexBufferObject()
{
  this->IndexCount = 0;
  this->SetType(vtkOpenGLIndexBufferObject::ElementArrayBuffer);
}

vtkOpenGLIndexBufferObject::~vtkOpenGLIndexBufferObject()
{
}

namespace
{
// A worker functor. The calculation is implemented in the function template
// for operator().
struct AppendTrianglesWorker
{
  std::vector<unsigned int> *indexArray;
  vtkCellArray *cells;
  vtkIdType vOffset;

  // AoS fast path
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType> *src)
  {
    vtkIdType *idPtr = cells->GetPointer();
    vtkIdType *idEnd = idPtr + cells->GetNumberOfConnectivityEntries();

    ValueType *points = src->Begin();
    while (idPtr < idEnd)
    {
      if (*idPtr >= 3)
      {
        vtkIdType id1 = *(idPtr+1);
        ValueType* p1 = points + id1*3;
        for (int i = 2; i < *idPtr; i++)
        {
          vtkIdType id2 = *(idPtr+i);
          vtkIdType id3 = *(idPtr+i+1);
          ValueType* p2 = points + id2*3;
          ValueType* p3 = points + id3*3;
          if ((p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2]) &&
            (p3[0] != p2[0] || p3[1] != p2[1] || p3[2] != p2[2]) &&
            (p3[0] != p1[0] || p3[1] != p1[1] || p3[2] != p1[2]))
          {
            indexArray->push_back(static_cast<unsigned int>(id1+vOffset));
            indexArray->push_back(static_cast<unsigned int>(id2+vOffset));
            indexArray->push_back(static_cast<unsigned int>(id3+vOffset));
          }
        }
      }
    idPtr += (*idPtr + 1);
    }
  }

  // Generic API, on VS13 Rel this is about 80% slower than
  // the AOS template above.
  template <typename PointArray>
  void operator()(PointArray *points)
  {
    // This allows the compiler to optimize for the AOS array stride.
    VTK_ASSUME(points->GetNumberOfComponents() == 3);

    // These allow this single worker function to be used with both
    // the vtkDataArray 'double' API and the more efficient
    // vtkGenericDataArray APIs, depending on the template parameters:
    vtkDataArrayAccessor<PointArray> pt(points);

    vtkIdType *idPtr = cells->GetPointer();
    vtkIdType *idEnd = idPtr + cells->GetNumberOfConnectivityEntries();

    while (idPtr < idEnd)
    {
      if (*idPtr >= 3)
      {
        vtkIdType id1 = *(idPtr+1);
        for (int i = 2; i < *idPtr; i++)
        {
          vtkIdType id2 = *(idPtr+i);
          vtkIdType id3 = *(idPtr+i+1);
          if (
            (pt.Get(id1, 0) != pt.Get(id2, 0) ||
              pt.Get(id1, 1) != pt.Get(id2, 1) ||
              pt.Get(id1, 2) != pt.Get(id2, 2)) &&
            (pt.Get(id1, 0) != pt.Get(id3, 0) ||
              pt.Get(id1, 1) != pt.Get(id3, 1) ||
              pt.Get(id1, 2) != pt.Get(id3, 2)) &&
            (pt.Get(id3, 0) != pt.Get(id2, 0) ||
              pt.Get(id3, 1) != pt.Get(id2, 1) ||
              pt.Get(id3, 2) != pt.Get(id2, 2)))
          {
            indexArray->push_back(static_cast<unsigned int>(id1+vOffset));
            indexArray->push_back(static_cast<unsigned int>(id2+vOffset));
            indexArray->push_back(static_cast<unsigned int>(id3+vOffset));
          }
        }
      }
    idPtr += (*idPtr + 1);
    }
  }
};

} // end anon namespace


// used to create an IBO for triangle primitives
void vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkPoints *points,
  vtkIdType vOffset)
{
  if (cells->GetNumberOfConnectivityEntries() >
      cells->GetNumberOfCells()*3)
  {
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
  }

  // Create our worker functor:
  AppendTrianglesWorker worker;
  worker.indexArray = &indexArray;
  worker.cells = cells;
  worker.vOffset = vOffset;

  // Define our dispatcher on float/double
  typedef vtkArrayDispatch::DispatchByValueType
    <
      vtkArrayDispatch::Reals
    > Dispatcher;

  // Execute the dispatcher:
  if (!Dispatcher::Execute(points->GetData(), worker))
    {
    // If Execute() fails, it means the dispatch failed due to an
    // unsupported array type this falls back to using the
    // vtkDataArray double API:
    worker(points->GetData());
    }
}

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreateTriangleIndexBuffer(
  vtkCellArray *cells,
  vtkPoints *points)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendTriangleIndexBuffer(indexArray, cells, points, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for point primitives
void vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
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

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreatePointIndexBuffer(vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendPointIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}


// used to create an IBO for primitives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
void vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
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

// used to create an IBO for primitives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
size_t vtkOpenGLIndexBufferObject::CreateTriangleLineIndexBuffer(
  vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendTriangleLineIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for primitives as lines.  This method treats each
// line segment as independent.  So for a line strip you would get multiple
// line segments out
void vtkOpenGLIndexBufferObject::AppendLineIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);

  // possibly adjust size
  if (cells->GetNumberOfConnectivityEntries() >
      2*cells->GetNumberOfCells())
  {
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

// used to create an IBO for primitives as lines.  This method treats each
// line segment as independent.  So for a line strip you would get multiple
// line segments out
size_t vtkOpenGLIndexBufferObject::CreateLineIndexBuffer(vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendLineIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for triangle strips
size_t vtkOpenGLIndexBufferObject::CreateStripIndexBuffer(
  vtkCellArray *cells,
  bool wireframeTriStrips)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendStripIndexBuffer(indexArray, cells, 0, wireframeTriStrips);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

void vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset, bool wireframeTriStrips)
{
  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;

  size_t triCount = cells->GetNumberOfConnectivityEntries()
    - 3*cells->GetNumberOfCells();
  size_t targetSize = wireframeTriStrips ? 2*(triCount*2+1)
   : triCount*3;
  indexArray.reserve(targetSize);

  if (wireframeTriStrips)
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
    {
      indexArray.push_back(static_cast<unsigned int>(pts[0]+vOffset));
      indexArray.push_back(static_cast<unsigned int>(pts[1]+vOffset));
      for (int j = 0; j < npts-2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j+2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j+2]+vOffset));
      }
    }
  }
  else
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
    {
      for (int j = 0; j < npts-2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1+j%2]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j+1+(j+1)%2]+vOffset));
      }
    }
  }
}

// Create supporting arays that are needed when rendering cell data
// Some VTK cells have to be broken into smaller cells for OpenGL
// When we have cell data we have to map cell attributes from the VTK
// cell number to the actual OpenGL cell
// The following code fills in
//
//   cellCellMap which maps a openGL cell id to the VTK cell it came from
//
void vtkOpenGLIndexBufferObject::CreateCellSupportArrays(
  vtkCellArray *prims[4],
  std::vector<vtkIdType> &cellCellMap,
  int representation,
  vtkPoints *points)
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
  vtkIdType vtkCellCount = 0;

  // points
  for (prims[0]->InitTraversal(); prims[0]->GetNextCell(npts, indices); )
  {
    for (vtkIdType i=0; i < npts; ++i)
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
        for (vtkIdType i=0; i < npts; ++i)
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
      for (vtkIdType i = 0; i < npts-1; ++i)
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
        for (vtkIdType i = 0; i < npts; ++i)
        {
          cellCellMap.push_back(vtkCellCount);
        }
        vtkCellCount++;
      } // for cell

      // strips
      for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices); )
      {
        cellCellMap.push_back(vtkCellCount);
        for (vtkIdType i = 2; i < npts; ++i)
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
          for (vtkIdType i = 2; i < npts; i++)
          {
            double p1[3];
            points->GetPoint(indices[0],p1);
            double p2[3];
            points->GetPoint(indices[i-1],p2);
            double p3[3];
            points->GetPoint(indices[i],p3);
            if ((p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2]) &&
                (p3[0] != p2[0] || p3[1] != p2[1] || p3[2] != p2[2]) &&
                (p3[0] != p1[0] || p3[1] != p1[1] || p3[2] != p1[2]))
            {
              cellCellMap.push_back(vtkCellCount);
            }
          }
        }
        vtkCellCount++;
      } // for cell

      // strips
      for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices); )
      {
        for (vtkIdType i = 2; i < npts; ++i)
        {
          cellCellMap.push_back(vtkCellCount);
        }
        vtkCellCount++;
      } // for cell
    }
  }

}

// used to create an IBO for polys in wireframe with edge flags
void vtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkIdType vOffset,
  vtkDataArray *ef)
{
  vtkIdType* pts(NULL);
  vtkIdType npts(0);

  unsigned char *ucef = vtkArrayDownCast<vtkUnsignedCharArray>(ef)->GetPointer(0);

  // possibly adjust size
  if (cells->GetNumberOfConnectivityEntries() >
      2*cells->GetNumberOfCells())
  {
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
  }
  for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
  {
    for (int j = 0; j < npts; ++j)
    {
      if (ucef[pts[j]] && npts > 1) // draw this edge and poly is not degenerate
      {
        // determine the ending vertex
        vtkIdType nextVert = (j == npts-1) ? pts[0] : pts[j+1];
        indexArray.push_back(static_cast<unsigned int>(pts[j]+vOffset));
        indexArray.push_back(static_cast<unsigned int>(nextVert + vOffset));
      }
    }
  }
}

// used to create an IBO for polys in wireframe with edge flags
size_t vtkOpenGLIndexBufferObject::CreateEdgeFlagIndexBuffer(
  vtkCellArray *cells,
  vtkDataArray *ef)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendEdgeFlagIndexBuffer(indexArray, cells, 0, ef);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for point primitives
void vtkOpenGLIndexBufferObject::AppendVertexIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray **cells,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);

  // we use a set to make them unique
  std::set<vtkIdType> vertsUsed;
  for (int j = 0; j < 4; j++)
  {
    for (cells[j]->InitTraversal(); cells[j]->GetNextCell(npts, indices); )
    {
      for (int i = 0; i < npts; ++i)
      {
        vertsUsed.insert(static_cast<unsigned int>(*(indices++)+vOffset));
      }
    }
  }

  // now put them into the vector
  size_t targetSize = indexArray.size() +
    vertsUsed.size();
  if (targetSize > indexArray.capacity())
  {
    if (targetSize < indexArray.capacity()*1.5)
    {
      targetSize = indexArray.capacity()*1.5;
    }
    indexArray.reserve(targetSize);
  }

  for (std::set<vtkIdType>::const_iterator i = vertsUsed.begin(); i != vertsUsed.end(); ++i)
  {
    indexArray.push_back(*i);
  }

}

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreateVertexIndexBuffer(vtkCellArray **cells)
{
  unsigned long totalCells = 0;
  for (int i = 0; i < 4; i++)
  {
    totalCells += cells[i]->GetNumberOfCells();
  }

  if (!totalCells)
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendVertexIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

//-----------------------------------------------------------------------------
void vtkOpenGLIndexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
