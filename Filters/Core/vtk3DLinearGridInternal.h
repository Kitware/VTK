/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DLinearGridInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtk3DLinearGridInternal
 * @brief   fast access and processing of 3D linear grids
 *
 * vtk3DLinearGridInternal provides fast access and processing of 3D linear cells
 * contained in a vtkUnstructuredGrid: tetrahedra, hexahedra, voxels,
 * pyramids, and/or wedges. (The cells are linear in the sense that each cell
 * edge is a straight line.)  This code is designed for high-speed,
 * specialized operation to support other algorithms. Note that accessing
 * non-3D linear cells contained in a vtkUnstructuredGrids is allowed although
 * such cells are skipped and will produce no output.
 *
 * @warning
 * This file is meant as a private include file to avoid code duplication. At
 * this time it is not meant to define a public API (the API is likely to change
 * in the future). If you write code that depends on this include, be prepared to
 * change it in the future (without complaint).
 *
 * @sa
 * vtk3DLinearGridInternalPlaneCutter vtk3DLinearGridInternalCrinkleCutter vtkContour3DLinearGrid
 */

#ifndef vtk3DLinearGridInternal_h
#define vtk3DLinearGridInternal_h

// Include appropriate cell types
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkHexahedron.h"
#include "vtkPyramid.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include <cmath>

namespace
{ // anonymous namespace

//========================= CELL MACHINARY ====================================

// Implementation note: this filter currently handles 3D linear cells. It
// could be extended to handle other 3D cell types.

// The maximum number of verts per cell (hexahedron)
#define MAX_CELL_VERTS 8

// Base class to represent cells
struct BaseCell
{
  unsigned char CellType;
  unsigned char NumVerts;
  unsigned char NumEdges;
  unsigned short* Cases;
  static unsigned char Mask[MAX_CELL_VERTS];

  BaseCell(int cellType)
    : CellType(cellType)
    , NumVerts(0)
    , NumEdges(0)
    , Cases(nullptr)
  {
  }
  virtual ~BaseCell() {}

  // Set up the case table. This is done by accessing standard VTK cells and
  // repackaging the case table for efficiency. The format of the case table
  // is as follows: a linear array, organized into two parts: 1) offsets into
  // the second part, and 2) the cases. The first 2^NumVerts entries are the
  // offsets which refer to the 2^NumVerts cases in the second part. Each
  // case is represented by the number of edges, followed by pairs of
  // vertices (v0,v1) for each edge. Note that groups of three contiguous
  // edges form a triangle.
  virtual void BuildCases() = 0;
  void BuildCases(int numCases, const vtkIdType** edges, int** cases, unsigned short* caseArray);
};
// Used to generate case mask
unsigned char BaseCell::Mask[MAX_CELL_VERTS] = { 1, 2, 4, 8, 16, 32, 64, 128 };
// Build repackaged case table and place into cases array.
void BaseCell::BuildCases(
  int numCases, const vtkIdType** edges, int** cases, unsigned short* caseArray)
{
  int caseOffset = numCases;
  for (int caseNum = 0; caseNum < numCases; ++caseNum)
  {
    caseArray[caseNum] = caseOffset;
    int* triCases = cases[caseNum];

    // Count the number of edges
    int count;
    for (count = 0; triCases[count] != (-1); ++count)
    {
    }
    caseArray[caseOffset++] = count;

    // Now populate the edges
    const vtkIdType* edge;
    for (count = 0; triCases[count] != (-1); ++count)
    {
      edge = edges[triCases[count]];
      caseArray[caseOffset++] = edge[0];
      caseArray[caseOffset++] = edge[1];
    }
  } // for all cases
}

// Contour tetrahedral cell------------------------------------------------------
// Repackages case table for more efficient processing.
struct TetraCell : public BaseCell
{
  static unsigned short TetraCases[152];

  TetraCell()
    : BaseCell(VTK_TETRA)
  {
    this->NumVerts = 4;
    this->NumEdges = 6;
    this->BuildCases();
    this->Cases = this->TetraCases;
  }
  ~TetraCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at initialization. The lengtth of the
// array is determined from the equation length=(2*NumCases + 3*2*NumTris).
unsigned short TetraCell::TetraCases[152] = { 0 };
// Load and transform vtkTetra case table. The case tables are repackaged for
// efficiency (e.g., support the GetCase() method).
void TetraCell::BuildCases()
{
  const vtkIdType** edges = new const vtkIdType*[this->NumEdges];
  int numCases = std::pow(2, this->NumVerts);
  int** cases = new int*[numCases];
  for (int i = 0; i < this->NumEdges; ++i)
  {
    edges[i] = vtkTetra::GetEdgeArray(i);
  }
  for (int i = 0; i < numCases; ++i)
  {
    cases[i] = vtkTetra::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->TetraCases);

  delete[] edges;
  delete[] cases;
}

// Contour hexahedral cell------------------------------------------------------
struct HexahedronCell : public BaseCell
{
  static unsigned short HexahedronCases[5432];

  HexahedronCell()
    : BaseCell(VTK_HEXAHEDRON)
  {
    this->NumVerts = 8;
    this->NumEdges = 12;
    this->BuildCases();
    this->Cases = this->HexahedronCases;
  }
  ~HexahedronCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short HexahedronCell::HexahedronCases[5432] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method).
void HexahedronCell::BuildCases()
{
  const vtkIdType** edges = new const vtkIdType*[this->NumEdges];
  int numCases = std::pow(2, this->NumVerts);
  int** cases = new int*[numCases];
  for (int i = 0; i < this->NumEdges; ++i)
  {
    edges[i] = vtkHexahedron::GetEdgeArray(i);
  }
  for (int i = 0; i < numCases; ++i)
  {
    cases[i] = vtkHexahedron::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->HexahedronCases);

  delete[] edges;
  delete[] cases;
}

// Contour wedge cell ------------------------------------------------------
struct WedgeCell : public BaseCell
{
  static unsigned short WedgeCases[968];

  WedgeCell()
    : BaseCell(VTK_WEDGE)
  {
    this->NumVerts = 6;
    this->NumEdges = 9;
    this->BuildCases();
    this->Cases = this->WedgeCases;
  }
  ~WedgeCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short WedgeCell::WedgeCases[968] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method).
void WedgeCell::BuildCases()
{
  const vtkIdType** edges = new const vtkIdType*[this->NumEdges];
  int numCases = std::pow(2, this->NumVerts);
  int** cases = new int*[numCases];
  for (int i = 0; i < this->NumEdges; ++i)
  {
    edges[i] = vtkWedge::GetEdgeArray(i);
  }
  for (int i = 0; i < numCases; ++i)
  {
    cases[i] = vtkWedge::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->WedgeCases);

  delete[] edges;
  delete[] cases;
}

// Contour pyramid cell------------------------------------------------------
struct PyramidCell : public BaseCell
{
  static unsigned short PyramidCases[448];

  PyramidCell()
    : BaseCell(VTK_PYRAMID)
  {
    this->NumVerts = 5;
    this->NumEdges = 8;
    this->BuildCases();
    this->Cases = this->PyramidCases;
  }
  ~PyramidCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short PyramidCell::PyramidCases[448] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method).
void PyramidCell::BuildCases()
{
  const vtkIdType** edges = new const vtkIdType*[this->NumEdges];
  int numCases = std::pow(2, this->NumVerts);
  int** cases = new int*[numCases];
  for (int i = 0; i < this->NumEdges; ++i)
  {
    edges[i] = vtkPyramid::GetEdgeArray(i);
  }
  for (int i = 0; i < numCases; ++i)
  {
    cases[i] = vtkPyramid::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->PyramidCases);

  delete[] edges;
  delete[] cases;
}

// Contour voxel cell------------------------------------------------------
struct VoxelCell : public BaseCell
{
  static unsigned short VoxCases[5432];

  VoxelCell()
    : BaseCell(VTK_VOXEL)
  {
    this->NumVerts = 8;
    this->NumEdges = 12;
    this->BuildCases();
    this->Cases = this->VoxCases;
  }
  ~VoxelCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short VoxelCell::VoxCases[5432] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method). Note that
// the MC cases (vtkMarchingCubesTriangleCases) are specified for the
// hexahedron; voxels require a transformation to produce correct output.
void VoxelCell::BuildCases()
{
  // Map the voxel points consistent with the hex edges and cases, Basically
  // the hex points (2,3,6,7) are ordered (3,2,7,6) on the voxel.
  const vtkIdType** edges = new const vtkIdType*[this->NumEdges];
  constexpr vtkIdType voxEdges[12][2] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 }, { 4, 5 },
    { 5, 7 }, { 6, 7 }, { 4, 6 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };

  for (int i = 0; i < this->NumEdges; ++i)
  {
    edges[i] = voxEdges[i];
  }

  // Build the voxel cases. Have to shuffle them around due to different
  // vertex ordering.
  unsigned int numCases = std::pow(2, this->NumVerts);
  int** cases = new int*[numCases];
  unsigned int hexCase, voxCase;
  for (hexCase = 0; hexCase < numCases; ++hexCase)
  {
    voxCase = ((hexCase & BaseCell::Mask[0]) ? 1 : 0) << 0;
    voxCase |= ((hexCase & BaseCell::Mask[1]) ? 1 : 0) << 1;
    voxCase |= ((hexCase & BaseCell::Mask[2]) ? 1 : 0) << 3;
    voxCase |= ((hexCase & BaseCell::Mask[3]) ? 1 : 0) << 2;
    voxCase |= ((hexCase & BaseCell::Mask[4]) ? 1 : 0) << 4;
    voxCase |= ((hexCase & BaseCell::Mask[5]) ? 1 : 0) << 5;
    voxCase |= ((hexCase & BaseCell::Mask[6]) ? 1 : 0) << 7;
    voxCase |= ((hexCase & BaseCell::Mask[7]) ? 1 : 0) << 6;
    cases[voxCase] = vtkHexahedron::GetTriangleCases(hexCase);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->VoxCases);

  delete[] edges;
  delete[] cases;
}

// Contour empty cell. These cells are skipped.---------------------------------
struct EmptyCell : public BaseCell
{
  static unsigned short EmptyCases[2];

  EmptyCell()
    : BaseCell(VTK_EMPTY_CELL)
  {
    this->NumVerts = 0;
    this->NumEdges = 0;
    this->Cases = this->EmptyCases;
  }
  ~EmptyCell() override {}
  void BuildCases() override {}
};
// No triangles generated
unsigned short EmptyCell::EmptyCases[2] = { 0, 0 };

// This is a general iterator which assumes that the unstructured grid has a
// mix of cells. Any cell that is not processed by this contouring algorithm
// (i.e., not one of tet, hex, pyr, wedge, voxel) is skipped.
struct CellIter
{
  // Current active cell, and whether it is a copy (which controls
  // the destruction process).
  bool Copy;
  BaseCell* Cell;

  // The iteration state.
  unsigned char NumVerts;
  const unsigned short* Cases;

  // References to unstructured grid for cell traversal.
  vtkIdType NumCells;
  const unsigned char* Types;
  vtkSmartPointer<vtkCellArray> CellArray;
  vtkSmartPointer<vtkCellArrayIterator> ConnIter;

  // All possible cell types. The iterator switches between them when
  // processing. All unsupported cells are of type EmptyCell.
  TetraCell* Tetra;
  HexahedronCell* Hexahedron;
  PyramidCell* Pyramid;
  WedgeCell* Wedge;
  VoxelCell* Voxel;
  EmptyCell* Empty;

  CellIter()
    : Copy(true)
    , Cell(nullptr)
    , NumVerts(0)
    , Cases(nullptr)
    , NumCells(0)
    , Types(nullptr)
    , Tetra(nullptr)
    , Hexahedron(nullptr)
    , Pyramid(nullptr)
    , Wedge(nullptr)
    , Voxel(nullptr)
    , Empty(nullptr)
  {
  }

  CellIter(vtkIdType numCells, unsigned char* types, vtkCellArray* cellArray)
    : Copy(false)
    , Cell(nullptr)
    , NumVerts(0)
    , Cases(nullptr)
    , NumCells(numCells)
    , Types(types)
    , CellArray(cellArray)
    , ConnIter(vtk::TakeSmartPointer(cellArray->NewIterator()))
  {
    this->Tetra = new TetraCell;
    this->Hexahedron = new HexahedronCell;
    this->Pyramid = new PyramidCell;
    this->Wedge = new WedgeCell;
    this->Voxel = new VoxelCell;
    this->Empty = new EmptyCell;
  }

  ~CellIter()
  {
    if (!this->Copy)
    {
      delete this->Tetra;
      delete this->Hexahedron;
      delete this->Pyramid;
      delete this->Wedge;
      delete this->Voxel;
      delete this->Empty;
    }
  }

  CellIter(const CellIter&) = default; // remove compiler warnings

  // Shallow copy to avoid new/delete.
  CellIter& operator=(const CellIter& cellIter)
  {
    this->Copy = true;
    this->Cell = nullptr;

    this->NumVerts = cellIter.NumVerts;
    this->Cases = cellIter.Cases;

    this->NumCells = cellIter.NumCells;
    this->Types = cellIter.Types;
    this->CellArray = cellIter.CellArray;

    // This class is passed around by pointer and only copied deliberately
    // to create thread-local copies. Since we don't want to share state,
    // create a new iterator here:
    if (cellIter.ConnIter)
    {
      this->ConnIter = vtk::TakeSmartPointer(this->CellArray->NewIterator());
      this->ConnIter->GoToCell(cellIter.ConnIter->GetCurrentCellId());
    }
    else
    {
      this->ConnIter = nullptr;
    }

    this->Tetra = cellIter.Tetra;
    this->Hexahedron = cellIter.Hexahedron;
    this->Pyramid = cellIter.Pyramid;
    this->Wedge = cellIter.Wedge;
    this->Voxel = cellIter.Voxel;
    this->Empty = cellIter.Empty;

    return *this;
  }

  // Decode the case table. (See previous documentation of case table
  // organization.) Note that bounds/range chacking is not performed
  // for efficiency.
  const unsigned short* GetCase(unsigned char caseNum)
  {
    return (this->Cases + this->Cases[caseNum]);
  }

  // Methods for caching traversal. Initialize() sets up the traversal
  // process; Next() advances to the next cell. Note that the public data
  // members representing the iteration state (NumVerts, Cases, ConnIter) are
  // modified by these methods, and then subsequently read during iteration.
  const vtkIdType* Initialize(vtkIdType cellId)
  {
    this->Cell = this->GetCell(this->Types[cellId]);
    this->NumVerts = this->Cell->NumVerts;
    this->Cases = this->Cell->Cases;
    this->ConnIter->GoToCell(cellId);

    vtkIdType dummy;
    const vtkIdType* conn;
    this->ConnIter->GetCurrentCell(dummy, conn);
    return conn;
  }

  const vtkIdType* Next()
  {
    this->ConnIter->GoToNextCell();

    if (this->ConnIter->IsDoneWithTraversal())
    {
      return nullptr;
    }

    const vtkIdType currentCellId = this->ConnIter->GetCurrentCellId();

    // Only update information if the cell type changes. Note however that
    // empty cells may have to be treated specially.
    if (this->Cell->CellType == VTK_EMPTY_CELL ||
      this->Cell->CellType != this->Types[currentCellId])
    {
      this->Cell = this->GetCell(this->Types[currentCellId]);
      this->NumVerts = this->Cell->NumVerts;
      this->Cases = this->Cell->Cases;
    }

    vtkIdType dummy;
    const vtkIdType* conn;
    this->ConnIter->GetCurrentCell(dummy, conn);
    return conn;
  }

  // Method for random access of cell, no caching
  unsigned char GetCellType(vtkIdType cellId) { return this->Types[cellId]; }

  // Method for random access of cell, no caching
  const vtkIdType* GetCellIds(vtkIdType cellId)
  {
    this->Cell = this->GetCell(this->Types[cellId]);
    this->NumVerts = this->Cell->NumVerts;
    this->Cases = this->Cell->Cases;
    this->ConnIter->GoToCell(cellId);

    vtkIdType dummy;
    const vtkIdType* conn;
    this->ConnIter->GetCurrentCell(dummy, conn);
    return conn;
  }

  // Switch to the appropriate cell type.
  BaseCell* GetCell(int cellType)
  {
    switch (cellType)
    {
      case VTK_TETRA:
        return this->Tetra;
      case VTK_HEXAHEDRON:
        return this->Hexahedron;
      case VTK_WEDGE:
        return this->Wedge;
      case VTK_PYRAMID:
        return this->Pyramid;
      case VTK_VOXEL:
        return this->Voxel;
      default:
        return this->Empty;
    }
  }
};

} // anonymous namespace

#endif // vtk3DLinearGridInternal_h
// VTK-HeaderTest-Exclude: vtk3DLinearGridInternal.h
