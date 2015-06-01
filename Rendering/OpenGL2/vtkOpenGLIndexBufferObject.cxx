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

#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenGLIndexBufferObject)

vtkOpenGLIndexBufferObject::vtkOpenGLIndexBufferObject()
{
  this->IndexCount = 0;
  this->SetType(vtkOpenGLIndexBufferObject::ElementArrayBuffer);
}

vtkOpenGLIndexBufferObject::~vtkOpenGLIndexBufferObject()
{
}

// used to create an IBO for triangle primatives
void vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
  std::vector<unsigned int> &indexArray,
  vtkCellArray *cells,
  vtkPoints *points,
  vtkIdType vOffset)
{
  vtkIdType* indices(NULL);
  vtkIdType npts(0);

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
size_t vtkOpenGLIndexBufferObject::CreateTriangleIndexBuffer(
  vtkCellArray *cells,
  vtkPoints *points)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendTriangleIndexBuffer(indexArray, cells, points, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for point primatives
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

// used to create an IBO for triangle primatives
size_t vtkOpenGLIndexBufferObject::CreatePointIndexBuffer(vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendPointIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}


// used to create an IBO for primatives as lines.  This method treats each line segment
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

// used to create an IBO for primatives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
size_t vtkOpenGLIndexBufferObject::CreateTriangleLineIndexBuffer(
  vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
    {
    return 0;
    }
  std::vector<unsigned int> indexArray;
  AppendTriangleLineIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for primatives as lines.  This method treats each
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

// used to create an IBO for primatives as lines.  This method treats each
// line segment as independent.  So for a line strip you would get multiple
// line segments out
size_t vtkOpenGLIndexBufferObject::CreateLineIndexBuffer(vtkCellArray *cells)
{
  if (!cells->GetNumberOfCells())
    {
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
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
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
          }
        vtkCellCount++;
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

// used to create an IBO for polys in wireframe with edge flags
size_t vtkOpenGLIndexBufferObject::CreateEdgeFlagIndexBuffer(
  vtkCellArray *cells,
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
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

//-----------------------------------------------------------------------------
void vtkOpenGLIndexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
