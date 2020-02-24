/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCellToVTKCellMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLCellToVTKCellMap.h"

#include "vtkCellArray.h"
#include "vtkOpenGLState.h"
#include "vtkPoints.h"
#include "vtkProperty.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLCellToVTKCellMap);

// ----------------------------------------------------------------------------
vtkOpenGLCellToVTKCellMap::vtkOpenGLCellToVTKCellMap()
{
  this->PrimitiveOffsets[0] = 0;
  this->PrimitiveOffsets[1] = 0;
  this->PrimitiveOffsets[2] = 0;
  this->PrimitiveOffsets[3] = 0;

  this->CellMapSizes[0] = 0;
  this->CellMapSizes[1] = 0;
  this->CellMapSizes[2] = 0;
  this->CellMapSizes[3] = 0;
}

// ----------------------------------------------------------------------------
vtkOpenGLCellToVTKCellMap::~vtkOpenGLCellToVTKCellMap() = default;

void vtkOpenGLCellToVTKCellMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLCellToVTKCellMap::SetStartOffset(vtkIdType start)
{
  if (this->StartOffset == start)
  {
    return;
  }

  // adjust PrimitiveOffsets in case they are already calculated
  this->PrimitiveOffsets[0] = this->PrimitiveOffsets[0] - this->StartOffset + start;
  this->PrimitiveOffsets[1] = this->PrimitiveOffsets[1] - this->StartOffset + start;
  this->PrimitiveOffsets[2] = this->PrimitiveOffsets[2] - this->StartOffset + start;
  this->PrimitiveOffsets[3] = this->PrimitiveOffsets[3] - this->StartOffset + start;

  this->StartOffset = start;
}

void vtkOpenGLCellToVTKCellMap::BuildPrimitiveOffsetsIfNeeded(
  vtkCellArray* prims[4], int representation, vtkPoints* points)
{
  // if the users created a full cell cell map AND it is still valid then
  // the values will be computed as part of that and we should use them
  if (this->CellCellMap.size())
  {
    this->TempState.Clear();
    this->TempState.Append(prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0, "verts");
    this->TempState.Append(prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0, "lines");
    this->TempState.Append(prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0, "polys");
    this->TempState.Append(prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0, "strips");
    this->TempState.Append(representation, "representation");
    this->TempState.Append(points ? points->GetMTime() : 0, "points");

    if (this->MapBuildState != this->TempState)
    {
      this->CellCellMap.clear();
    }
    else
    {
      return;
    }
  }

  // otherwise compute some conservative values
  // verts
  this->PrimitiveOffsets[0] = this->StartOffset;
  this->CellMapSizes[0] = prims[0]->GetNumberOfConnectivityIds();

  // point rep is easy for all prims
  if (representation == VTK_POINTS)
  {
    for (int j = 1; j < 4; j++)
    {
      this->CellMapSizes[j] = prims[j]->GetNumberOfConnectivityIds();
      this->PrimitiveOffsets[j] = this->PrimitiveOffsets[j - 1] + this->CellMapSizes[j - 1];
    }
    return;
  }

  // lines
  this->CellMapSizes[1] = prims[1]->GetNumberOfConnectivityIds() - prims[1]->GetNumberOfCells();
  this->PrimitiveOffsets[1] = this->PrimitiveOffsets[0] + this->CellMapSizes[0];

  if (representation == VTK_WIREFRAME)
  {
    // polys
    this->CellMapSizes[2] = prims[2]->GetNumberOfConnectivityIds();
    this->PrimitiveOffsets[2] = this->PrimitiveOffsets[1] + this->CellMapSizes[1];

    // strips
    this->CellMapSizes[3] =
      2 * prims[3]->GetNumberOfConnectivityIds() - 3 * prims[3]->GetNumberOfCells();
    this->PrimitiveOffsets[3] = this->PrimitiveOffsets[2] + this->CellMapSizes[2];

    return;
  }

  // otherwise surface rep

  // polys
  this->CellMapSizes[2] = prims[2]->GetNumberOfConnectivityIds() - 2 * prims[2]->GetNumberOfCells();
  this->PrimitiveOffsets[2] = this->PrimitiveOffsets[1] + this->CellMapSizes[1];

  // strips
  this->CellMapSizes[3] = prims[3]->GetNumberOfConnectivityIds() - 2 * prims[3]->GetNumberOfCells();
  this->PrimitiveOffsets[3] = this->PrimitiveOffsets[2] + this->CellMapSizes[2];
}

// Create supporting arrays that are needed when rendering cell data
// Some VTK cells have to be broken into smaller cells for OpenGL
// When we have cell data we have to map cell attributes from the VTK
// cell number to the actual OpenGL cell
// The following code fills in
//
//   cellCellMap which maps a openGL cell id to the VTK cell it came from
//
void vtkOpenGLCellToVTKCellMap::BuildCellSupportArrays(
  vtkCellArray* prims[4], int representation, vtkPoints* points)
{
  // need an array to track what points to orig points
  size_t minSize = prims[0]->GetNumberOfCells() + prims[1]->GetNumberOfCells() +
    prims[2]->GetNumberOfCells() + prims[3]->GetNumberOfCells();
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);

  // make sure we have at least minSize
  this->CellCellMap.clear();
  this->CellCellMap.reserve(minSize);
  vtkIdType vtkCellCount = 0;
  vtkIdType cumulativeSize = 0;
  this->BuildRepresentation = representation;

  // points
  this->PrimitiveOffsets[0] = this->StartOffset;
  for (prims[0]->InitTraversal(); prims[0]->GetNextCell(npts, indices);)
  {
    for (vtkIdType i = 0; i < npts; ++i)
    {
      this->CellCellMap.push_back(vtkCellCount);
    }
    vtkCellCount++;
  } // for cell

  this->CellMapSizes[0] = static_cast<vtkIdType>(this->CellCellMap.size());
  cumulativeSize = this->CellMapSizes[0];

  if (representation == VTK_POINTS)
  {
    for (int j = 1; j < 4; j++)
    {
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices);)
      {
        for (vtkIdType i = 0; i < npts; ++i)
        {
          this->CellCellMap.push_back(vtkCellCount);
        }
        vtkCellCount++;
      } // for cell
      this->PrimitiveOffsets[j] = this->PrimitiveOffsets[j - 1] + this->CellMapSizes[j - 1];
      this->CellMapSizes[j] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
      cumulativeSize = static_cast<vtkIdType>(this->CellCellMap.size());
    }
    return;
  }

  // lines
  for (prims[1]->InitTraversal(); prims[1]->GetNextCell(npts, indices);)
  {
    for (vtkIdType i = 0; i < npts - 1; ++i)
    {
      this->CellCellMap.push_back(vtkCellCount);
    }
    vtkCellCount++;
  } // for cell

  this->PrimitiveOffsets[1] = this->PrimitiveOffsets[0] + this->CellMapSizes[0];
  this->CellMapSizes[1] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
  cumulativeSize = static_cast<vtkIdType>(this->CellCellMap.size());

  if (representation == VTK_WIREFRAME)
  {
    // polys
    for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices);)
    {
      for (vtkIdType i = 0; i < npts; ++i)
      {
        this->CellCellMap.push_back(vtkCellCount);
      }
      vtkCellCount++;
    } // for cell

    this->PrimitiveOffsets[2] = this->PrimitiveOffsets[1] + this->CellMapSizes[1];
    this->CellMapSizes[2] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
    cumulativeSize = static_cast<vtkIdType>(this->CellCellMap.size());

    // strips
    for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices);)
    {
      this->CellCellMap.push_back(vtkCellCount);
      for (vtkIdType i = 2; i < npts; ++i)
      {
        this->CellCellMap.push_back(vtkCellCount);
        this->CellCellMap.push_back(vtkCellCount);
      }
      vtkCellCount++;
    } // for cell

    this->PrimitiveOffsets[3] = this->PrimitiveOffsets[2] + this->CellMapSizes[2];
    this->CellMapSizes[3] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
    return;
  }

  // polys
  for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices);)
  {
    if (npts > 2)
    {
      for (vtkIdType i = 2; i < npts; i++)
      {
        double p1[3];
        points->GetPoint(indices[0], p1);
        double p2[3];
        points->GetPoint(indices[i - 1], p2);
        double p3[3];
        points->GetPoint(indices[i], p3);
        if ((p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2]) &&
          (p3[0] != p2[0] || p3[1] != p2[1] || p3[2] != p2[2]) &&
          (p3[0] != p1[0] || p3[1] != p1[1] || p3[2] != p1[2]))
        {
          this->CellCellMap.push_back(vtkCellCount);
        }
      }
    }
    vtkCellCount++;
  } // for cell
  this->PrimitiveOffsets[2] = this->PrimitiveOffsets[1] + this->CellMapSizes[1];
  this->CellMapSizes[2] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
  cumulativeSize = static_cast<vtkIdType>(this->CellCellMap.size());

  // strips
  for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices);)
  {
    for (vtkIdType i = 2; i < npts; ++i)
    {
      this->CellCellMap.push_back(vtkCellCount);
    }
    vtkCellCount++;
  } // for cell

  this->PrimitiveOffsets[3] = this->PrimitiveOffsets[2] + this->CellMapSizes[2];
  this->CellMapSizes[3] = static_cast<vtkIdType>(this->CellCellMap.size()) - cumulativeSize;
}

void vtkOpenGLCellToVTKCellMap::Update(vtkCellArray** prims, int representation, vtkPoints* points)
{
  this->TempState.Clear();
  this->TempState.Append(prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0, "verts");
  this->TempState.Append(prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0, "lines");
  this->TempState.Append(prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0, "polys");
  this->TempState.Append(prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0, "strips");
  this->TempState.Append(representation, "representation");
  this->TempState.Append(points ? points->GetMTime() : 0, "points");

  if (this->MapBuildState != this->TempState)
  {
    this->MapBuildState = this->TempState;
    this->BuildCellSupportArrays(prims, representation, points);
  }
}

vtkIdType vtkOpenGLCellToVTKCellMap::ConvertOpenGLCellIdToVTKCellId(
  bool pointPicking, vtkIdType openGLId)
{
  // start with the vert offset and remove that
  vtkIdType result = openGLId - this->PrimitiveOffsets[0];

  // check if we really are a vert
  if (result < this->CellMapSizes[0])
  {
    return this->CellCellMap[result];
  }

  // OK we are a line maybe?
  result = openGLId - this->PrimitiveOffsets[1];
  if (pointPicking && this->BuildRepresentation != VTK_POINTS)
  {
    result /= 2;
  }
  if (result < this->CellMapSizes[1])
  {
    return this->CellCellMap[result + this->CellMapSizes[0]];
  }

  // OK maybe a triangle
  result = openGLId - this->PrimitiveOffsets[2];
  if (pointPicking && this->BuildRepresentation == VTK_WIREFRAME)
  {
    result /= 2;
  }
  if (pointPicking && this->BuildRepresentation == VTK_SURFACE)
  {
    result /= 3;
  }
  if (result < this->CellMapSizes[2])
  {
    return this->CellCellMap[result + this->CellMapSizes[1] + this->CellMapSizes[0]];
  }

  // must be a strip
  result = openGLId - this->PrimitiveOffsets[3];
  if (pointPicking && this->BuildRepresentation == VTK_WIREFRAME)
  {
    result /= 2;
  }
  if (pointPicking && this->BuildRepresentation == VTK_SURFACE)
  {
    result /= 3;
  }
  if (result < this->CellMapSizes[3])
  {
    return this
      ->CellCellMap[result + this->CellMapSizes[2] + this->CellMapSizes[1] + this->CellMapSizes[0]];
  }

  // error
  return 0;
}
