/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContour3DLinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContour3DLinearGrid.h"

#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"
#include "vtkVoxel.h"
#include "vtkTriangle.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkStaticPointLocator.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkStaticCellLinksTemplate.h"
#include "vtkSpanSpace.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

#include <set>

vtkStandardNewMacro(vtkContour3DLinearGrid);
vtkCxxSetObjectMacro(vtkContour3DLinearGrid,ScalarTree,vtkScalarTree);

//-----------------------------------------------------------------------------
// Classes to support threaded execution. Note that there are different
// strategies implemented here: 1) a fast path that just produces output
// triangles and points, and 2) more general approach that supports point
// merging, field interpolation, and/or normal generation. There is also some
// cell-related machinery supporting faster contouring.

// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq,_num,_op) \
  if ( !_seq) \
  {\
    vtkSMPTools::For(0,_num,_op); \
  }\
  else \
  {\
  _op(0,_num);\
  }

#define EXECUTE_REDUCED_SMPFOR(_seq,_num,_op,_nt)  \
  if ( !_seq) \
  {\
    vtkSMPTools::For(0,_num,_op); \
  }\
  else \
  {\
  _op.Initialize();\
  _op(0,_num);\
  _op.Reduce();\
  }\
  _nt = _op.NumThreadsUsed;


namespace {

//========================= CELL MACHINERY ====================================

// Implementation note: this filter currently handles 3D linear cells. It
// could be extended to handle other 3D cell types if the contouring
// operation can be expressed as transformation of scalar values from cell
// vertices to a case table of triangles.

// The maximum number of verts per cell
#define MAX_CELL_VERTS 8

// Base class to represent cells
struct BaseCell
{
  unsigned char CellType;
  unsigned char NumVerts;
  unsigned char NumEdges;
  unsigned short *Cases;
  static unsigned char Mask[MAX_CELL_VERTS];

  BaseCell(int cellType) : CellType(cellType), NumVerts(0), NumEdges(0), Cases(nullptr) {}
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
  void BuildCases(int numCases, int **edges, int **cases, unsigned short *caseArray);
};
// Used to generate case mask
unsigned char BaseCell::Mask[MAX_CELL_VERTS] = {1,2,4,8,16,32,64,128};
// Build repackaged case table and place into cases array.
void BaseCell::BuildCases(int numCases, int **edges, int **cases,
                          unsigned short *caseArray)
{
  int caseOffset = numCases;
  for (int caseNum=0; caseNum < numCases; ++caseNum)
  {
    caseArray[caseNum] = caseOffset;
    int *triCases = cases[caseNum];

    // Count the number of edges
    int count;
    for (count=0; triCases[count] != (-1); ++count) {}
    caseArray[caseOffset++] = count;

    // Now populate the edges
    int *edge;
    for (count=0; triCases[count] != (-1); ++count)
    {
      edge = edges[triCases[count]];
      caseArray[caseOffset++] = edge[0];
      caseArray[caseOffset++] = edge[1];
    }
  }//for all cases
}

// Contour tetrahedral cell------------------------------------------------------
// Repackages case table for more efficient processing.
struct TetCell : public BaseCell
{
  static unsigned short TetCases[152];

  TetCell() : BaseCell(VTK_TETRA)
  {
    this->NumVerts = 4;
    this->NumEdges = 6;
    this->BuildCases();
    this->Cases = this->TetCases;
  }
  ~TetCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at initialization. The lengtth of the
// array is determined from the equation length=(2*NumCases + 3*2*NumTris).
unsigned short TetCell::TetCases[152] = { 0 };
// Load and transform vtkTetra case table. The case tables are repackaged for
// efficiency (e.g., support the GetCase() method).
void TetCell::BuildCases()
{
  int **edges = new int*[this->NumEdges];
  int numCases = std::pow(2,this->NumVerts);
  int **cases = new int*[numCases];
  for ( int i=0; i<this->NumEdges; ++i)
  {
    edges[i] = vtkTetra::GetEdgeArray(i);
  }
  for ( int i=0; i < numCases; ++i)
  {
    cases[i] = vtkTetra::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->TetCases);

  delete [] edges;
  delete [] cases;
}

// Contour hexahedral cell------------------------------------------------------
struct HexCell : public BaseCell
{
  static unsigned short HexCases[5432];

  HexCell() : BaseCell(VTK_HEXAHEDRON)
  {
    this->NumVerts = 8;
    this->NumEdges = 12;
    this->BuildCases();
    this->Cases = this->HexCases;
  }
  ~HexCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short HexCell::HexCases[5432] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method).
void HexCell::BuildCases()
{
  int **edges = new int*[this->NumEdges];
  int numCases = std::pow(2,this->NumVerts);
  int **cases = new int*[numCases];
  for ( int i=0; i<this->NumEdges; ++i)
  {
    edges[i] = vtkHexahedron::GetEdgeArray(i);
  }
  for ( int i=0; i < numCases; ++i)
  {
    cases[i] = vtkHexahedron::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->HexCases);

  delete [] edges;
  delete [] cases;
}

// Contour wedge cell ------------------------------------------------------
struct WedgeCell : public BaseCell
{
  static unsigned short WedgeCases[968];

  WedgeCell() : BaseCell(VTK_WEDGE)
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
  int **edges = new int*[this->NumEdges];
  int numCases = std::pow(2,this->NumVerts);
  int **cases = new int*[numCases];
  for ( int i=0; i<this->NumEdges; ++i)
  {
    edges[i] = vtkWedge::GetEdgeArray(i);
  }
  for ( int i=0; i < numCases; ++i)
  {
    cases[i] = vtkWedge::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->WedgeCases);

  delete [] edges;
  delete [] cases;
}

// Contour pyramid cell------------------------------------------------------
struct PyrCell : public BaseCell
{
  static unsigned short PyrCases[448];

  PyrCell() : BaseCell(VTK_PYRAMID)
  {
    this->NumVerts = 5;
    this->NumEdges = 8;
    this->BuildCases();
    this->Cases = this->PyrCases;
  }
  ~PyrCell() override {}
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short PyrCell::PyrCases[448] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method).
void PyrCell::BuildCases()
{
  int **edges = new int*[this->NumEdges];
  int numCases = std::pow(2,this->NumVerts);
  int **cases = new int*[numCases];
  for ( int i=0; i<this->NumEdges; ++i)
  {
    edges[i] = vtkPyramid::GetEdgeArray(i);
  }
  for ( int i=0; i < numCases; ++i)
  {
    cases[i] = vtkPyramid::GetTriangleCases(i);
  }

  BaseCell::BuildCases(numCases, edges, cases, this->PyrCases);

  delete [] edges;
  delete [] cases;
}

// Contour voxel cell------------------------------------------------------
struct VoxCell : public BaseCell
{
  static unsigned short VoxCases[5432];

  VoxCell() : BaseCell(VTK_VOXEL)
  {
    this->NumVerts = 8;
    this->NumEdges = 12;
    this->BuildCases();
    this->Cases = this->VoxCases;
  }
  ~VoxCell() override {};
  void BuildCases() override;
};
// Dummy initialization filled in later at instantiation
unsigned short VoxCell::VoxCases[5432] = { 0 };
// Load and transform marching cubes case table. The case tables are
// repackaged for efficiency (e.g., support the GetCase() method). Note that
// the MC cases (vtkMarchingCubesTriangleCases) are specified for the
// hexahedron; voxels require a transformation to produce correct output.
void VoxCell::BuildCases()
{
  // Map the voxel points consistent with the hex edges and cases, Basically
  // the hex points (2,3,6,7) are ordered (3,2,7,6) on the voxel.
  int **edges = new int*[this->NumEdges];
  int voxEdges[12][2] = { {0,1}, {1,3}, {2,3}, {0,2}, {4,5}, {5,7},
                          {6,7}, {4,6}, {0,4}, {1,5}, {2,6}, {3,7} };

  for ( int i=0; i<this->NumEdges; ++i)
  {
    edges[i] = voxEdges[i];
  }

  // Build the voxel cases. Have to shuffle them around due to different
  // vertex ordering.
  unsigned int numCases = std::pow(2,this->NumVerts);
  int **cases = new int*[numCases];
  unsigned int hexCase, voxCase;
  for ( hexCase=0; hexCase < numCases; ++hexCase)
  {
    voxCase  = ((hexCase & BaseCell::Mask[0]) ? 1 : 0) << 0;
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

  delete [] edges;
  delete [] cases;
}

// Contour empty cell. These cells are skipped.---------------------------------
struct EmptyCell : public BaseCell
{
  static unsigned short EmptyCases[2];

  EmptyCell() : BaseCell(VTK_EMPTY_CELL)
  {
    this->NumVerts = 0;
    this->NumEdges = 0;
    this->Cases = this->EmptyCases;
  }
  ~EmptyCell() override {}
  void BuildCases() override {}
};
// No triangles generated
unsigned short EmptyCell::EmptyCases[2] = { 0,0 };


// This is a general iterator which assumes that the unstructured grid has a
// mix of cells. Any cell that is not processed by this contouring algorithm
// (i.e., not one of tet, hex, pyr, wedge, voxel) is skipped.
struct CellIter
{
  // Current active cell, and whether it is a copy (which controls
  // the destruction process).
  bool Copy;
  BaseCell *Cell;

  // The iteration state.
  unsigned char NumVerts;
  const unsigned short *Cases;
  vtkIdType Incr;

  // References to unstructured grid for cell traversal.
  vtkIdType NumCells;
  const unsigned char *Types;
  const vtkIdType *Conn;
  const vtkIdType *Locs;

  // All possible cell types. The iterator switches between them when
  // processing. All unsupported cells are of type EmptyCell.
  TetCell *Tet;
  HexCell *Hex;
  PyrCell *Pyr;
  WedgeCell *Wedge;
  VoxCell *Vox;
  EmptyCell *Empty;

  CellIter() : Copy(true), Cell(nullptr), NumVerts(0), Cases(nullptr), Incr(0),
               NumCells(0), Types(nullptr), Conn(nullptr), Locs(nullptr), Tet(nullptr),
               Hex(nullptr), Pyr(nullptr), Wedge(nullptr), Vox(nullptr), Empty(nullptr)
  {}

  CellIter(vtkIdType numCells, unsigned char *types, vtkIdType *conn, vtkIdType *locs) :
    Copy(false), Cell(nullptr), NumVerts(0), Cases(nullptr), Incr(0),
    NumCells(numCells), Types(types), Conn(conn), Locs(locs)
  {
    this->Tet = new TetCell;
    this->Hex = new HexCell;
    this->Pyr = new PyrCell;
    this->Wedge = new WedgeCell;
    this->Vox = new VoxCell;
    this->Empty = new EmptyCell;
  }

  ~CellIter()
  {
    if ( ! this->Copy )
    {
      delete this->Tet;
      delete this->Hex;
      delete this->Pyr;
      delete this->Wedge;
      delete this->Vox;
      delete this->Empty;
    }
  }

  CellIter(const CellIter &) = default; //remove compiler warnings

  // Shallow copy to avoid new/delete.
  CellIter& operator=(const CellIter& cellIter)
  {
    this->Copy = true;
    this->Cell = nullptr;

    this->NumVerts = cellIter.NumVerts;
    this->Cases = cellIter.Cases;
    this->Incr = cellIter.Incr;

    this->NumCells = cellIter.NumCells;
    this->Types = cellIter.Types;
    this->Conn = cellIter.Conn;
    this->Locs = cellIter.Locs;

    this->Tet = cellIter.Tet;
    this->Hex = cellIter.Hex;
    this->Pyr = cellIter.Pyr;
    this->Wedge = cellIter.Wedge;
    this->Vox = cellIter.Vox;
    this->Empty = cellIter.Empty;

    return *this;
  }

  // Decode the case table. (See previous documentation of case table
  // organization.) Note that bounds/range chacking is not performed
  // for efficiency.
  const unsigned short *GetCase(unsigned char caseNum)
  {
    return (this->Cases + this->Cases[caseNum]);
  }

  // Methods for caching traversal. Initialize() sets up the traversal
  // process; Next() advances to the next cell. Note that the public data
  // members representing the iteration state (NumVerts, Cases, Incr) are
  // modified by these methods, and then subsequently read during iteration.
  const vtkIdType* Initialize(vtkIdType cellId)
  {
    this->Cell = this->GetCell(this->Types[cellId]);
    this->NumVerts = this->Cell->NumVerts;
    this->Cases = this->Cell->Cases;

    if ( this->Cell->CellType != VTK_EMPTY_CELL )
    {
      this->Incr = this->NumVerts + 1;
    }
    else // Else have to update the increment differently
    {
      this->Incr = (cellId >= (this->NumCells-1) ? 0 :
                    this->Locs[cellId+1] - this->Locs[cellId]);
    }

    return (this->Conn + this->Locs[cellId] + 1);
  }

  vtkIdType Next(vtkIdType cellId)
  {
    // Guard against end of array condition; only update information if the
    // cell type changes. Note however that empty cells may have to be
    // treated specially.
    if ( cellId >= (this->NumCells-1) ||
         (this->Cell->CellType != VTK_EMPTY_CELL &&
          this->Cell->CellType == this->Types[cellId+1]) )
    {
      return this->Incr;
    }

    // Need to look up new information as cell type has changed.
    vtkIdType incr = this->Incr;
    this->Cell = this->GetCell(this->Types[cellId+1]);
    this->NumVerts = this->Cell->NumVerts;
    this->Cases = this->Cell->Cases;

    if ( this->Cell->CellType != VTK_EMPTY_CELL )
    {
      this->Incr = this->NumVerts + 1;
    }
    else // Else have to update the increment differently
    {
      this->Incr = this->Locs[cellId+2] - this->Locs[cellId+1];
    }

    return incr;
  }

  // Method for random access of cell, no caching
  const vtkIdType* GetCellIds(vtkIdType cellId)
  {
    this->Cell = this->GetCell(this->Types[cellId]);
    this->NumVerts = this->Cell->NumVerts;
    this->Cases = this->Cell->Cases;
    return (this->Conn + this->Locs[cellId] + 1);
  }

  // Switch to the appropriate cell type.
  BaseCell *GetCell(int cellType)
  {
    switch ( cellType )
    {
      case VTK_TETRA:
        return this->Tet;
      case VTK_HEXAHEDRON:
        return this->Hex;
      case VTK_WEDGE:
        return this->Wedge;
      case VTK_PYRAMID:
        return this->Pyr;
      case VTK_VOXEL:
        return this->Vox;
     default:
       return this->Empty;
    }
  }
};


//========================= FAST PATH =========================================
// Perform the contouring operation without merging coincident points. There is
// a fast path with and without a scalar tree.
template <typename TIP, typename TOP, typename TS>
struct ContourCellsBase
{
  typedef std::vector<TOP> LocalPtsType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread into a
  // single vtkPolyData output.
  struct LocalDataType
  {
    LocalPtsType LocalPts;
    CellIter LocalCellIter;

    LocalDataType()
    {
      this->LocalPts.reserve(2048);
    }
  };

  CellIter *Iter;
  const TIP *InPts;
  const TS *Scalars;
  double  Value;
  vtkPoints *NewPts;
  vtkCellArray *NewPolys;

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  // Results from the compositing Reduce() method
  vtkIdType NumPts;
  vtkIdType NumTris;
  int NumThreadsUsed;
  vtkIdType TotalPts; //the total points thus far (support multiple contours)
  vtkIdType TotalTris; //the total triangles thus far (support multiple contours)

  ContourCellsBase(TIP *inPts, CellIter *iter, TS *s, double value,
                   vtkPoints* outPts, vtkCellArray *tris, vtkIdType totalPts,
                   vtkIdType totalTris) :
    Iter(iter), InPts(inPts), Scalars(s), Value(value), NewPts(outPts),
    NewPolys(tris), NumPts(0), NumTris(0), NumThreadsUsed(0),
    TotalPts(totalPts), TotalTris(totalTris)
  {
  }

  // Set up the iteration process.
  void Initialize()
  {
    auto & localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() method implemented by subclasses (with and without scalar tree)

  // Composite results from each thread
  void Reduce()
  {
    // Count the number of points. For fun keep track of the number of
    // threads used.
    vtkIdType numPts = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    for ( auto ldItr=this->LocalData.begin(); ldItr != ldEnd; ++ldItr )
    {
      numPts += static_cast<vtkIdType>(((*ldItr).LocalPts.size() / 3)); //x-y-z components
      this->NumThreadsUsed++;
    }

    // (Re)Allocate space for output. Multiple contours require writing into
    // the middle of arrays.
    this->NumPts = numPts;
    this->NumTris = numPts / 3;
    this->NewPts->GetData()->WriteVoidPointer(0,3*(this->NumPts+this->TotalPts));
    TOP *pts = static_cast<TOP*>(this->NewPts->GetVoidPointer(this->TotalPts*3));
    this->NewPolys->WritePointer((this->NumTris+this->TotalTris),
                                 4*(this->NumTris+this->TotalTris));

    // Copy points output to VTK structures. Only point coordinates are
    // copied for now; later we'll define the triangle topology.
    for ( auto ldItr=this->LocalData.begin(); ldItr != ldEnd; ++ldItr )
    {
      auto pEnd = (*ldItr).LocalPts.end();
      for ( auto pItr = (*ldItr).LocalPts.begin(); pItr != pEnd; )
      {
        *pts++ = *pItr++;
      }
    }//For all threads
  }//Reduce
};//ContourCellsBase

// Fast path operator() without scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCells : public ContourCellsBase<TIP,TOP,TS>
{
  ContourCells(TIP *inPts, CellIter *iter, TS *s, double value,
               vtkPoints* outPts, vtkCellArray *tris, vtkIdType totalPts,
               vtkIdType totalTris) :
    ContourCellsBase<TIP,TOP,TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris)
  {
  }

  // Set up the iteration process.
  void Initialize()
  {
    this->ContourCellsBase<TIP,TOP,TS>::Initialize();
  }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto & localData = this->LocalData.Local();
    auto & lPts = localData.LocalPts;
    CellIter *cellIter = &localData.LocalCellIter;
    const vtkIdType *c = cellIter->Initialize(cellId);
    unsigned short isoCase, numEdges, i;
    const unsigned short *edges;
    double s[MAX_CELL_VERTS], value=this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const TIP *x[MAX_CELL_VERTS];

    for ( ; cellId < endCellId; ++cellId )
    {
      // Compute case by repeated masking of scalar value
      for ( isoCase=0, i=0; i < cellIter->NumVerts; ++i )
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
        isoCase |= ( s[i] >= value ? BaseCell::Mask[i] : 0 );
      }
      edges = cellIter->GetCase(isoCase);

      if ( *edges > 0 )
      {
        numEdges = *edges++;
        for ( i=0; i < cellIter->NumVerts; ++i )
        {
          x[i] = this->InPts + 3*c[i];
        }

        for (i=0; i<numEdges; ++i, edges+=2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          lPts.emplace_back(x[v0][0] + t*(x[v1][0] - x[v0][0]));
          lPts.emplace_back(x[v0][1] + t*(x[v1][1] - x[v0][1]));
          lPts.emplace_back(x[v0][2] + t*(x[v1][2] - x[v0][2]));
        }//for all edges in this case
      }//if contour passes through this cell
      c += cellIter->Next(cellId); //move to the next cell
    }//for all cells in this batch
  }

  // Composite results from each thread
  void Reduce()
  {
    this->ContourCellsBase<TIP,TOP,TS>::Reduce();
  }//Reduce
};//ContourCells

// Fast path operator() with a scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCellsST : public ContourCellsBase<TIP,TOP,TS>
{
  vtkScalarTree *ScalarTree;
  vtkIdType NumBatches;

  ContourCellsST(TIP *inPts, CellIter *iter, TS *s, double value,
                 vtkScalarTree *st, vtkPoints* outPts, vtkCellArray *tris,
                 vtkIdType totalPts, vtkIdType totalTris) :
    ContourCellsBase<TIP,TOP,TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris),
    ScalarTree(st)
  {
    this->ScalarTree->InitTraversal(this->Value);
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches();
  }

  // Set up the iteration process.
  void Initialize()
  {
    this->ContourCellsBase<TIP,TOP,TS>::Initialize();
  }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle). Uses a scalar tree to accelerate operations.
  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    auto & localData = this->LocalData.Local();
    auto & lPts = localData.LocalPts;
    CellIter *cellIter = &localData.LocalCellIter;
    const vtkIdType *c;
    unsigned short isoCase, numEdges, i;
    const unsigned short *edges;
    double s[MAX_CELL_VERTS], value=this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const TIP *x[MAX_CELL_VERTS];
    const vtkIdType *cellIds;
    vtkIdType numCells;

    for ( ; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum,numCells);
      for (vtkIdType idx=0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for ( isoCase=0, i=0; i < cellIter->NumVerts; ++i )
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
          isoCase |= ( s[i] >= value ? BaseCell::Mask[i] : 0 );
        }
        edges = cellIter->GetCase(isoCase);

        if ( *edges > 0 )
        {
          numEdges = *edges++;
          for ( i=0; i < cellIter->NumVerts; ++i )
          {
            x[i] = this->InPts + 3*c[i];
          }

          for (i=0; i<numEdges; ++i, edges+=2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            lPts.emplace_back(x[v0][0] + t*(x[v1][0] - x[v0][0]));
            lPts.emplace_back(x[v0][1] + t*(x[v1][1] - x[v0][1]));
            lPts.emplace_back(x[v0][2] + t*(x[v1][2] - x[v0][2]));
          }//for all edges in this case
        }//if contour passes through this cell
      }//for all cells in this batch
    }//for each batch
  }

  // Composite results from each thread
  void Reduce()
  {
    this->ContourCellsBase<TIP,TOP,TS>::Reduce();
  }//Reduce
};//ContourCellsST


// Dispatch method for Fast path processing. Handles template dispatching etc.
template <typename TS>
void ProcessFastPath(vtkIdType numCells, vtkPoints *inPts, CellIter *cellIter,
                     TS *s, double isoValue, vtkScalarTree *st, vtkPoints *outPts,
                     vtkCellArray *tris, vtkTypeBool seqProcessing,
                     int &numThreads, vtkIdType totalPts, vtkIdType totalTris)
{
  double val = static_cast<double>(isoValue);
  int inPtsType = inPts->GetDataType();
  void *inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  if ( inPtsType == VTK_FLOAT && outPtsType == VTK_FLOAT )
  {
    if ( st != nullptr )
    {
      ContourCellsST<float,float,TS> contour((float*)inPtsPtr,cellIter,(TS*)s,val,
                                             st,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,contour.NumBatches,contour,numThreads);
    }
    else
    {
      ContourCells<float,float,TS> contour((float*)inPtsPtr,cellIter,
                                           (TS*)s,val,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,numCells,contour,numThreads);
    }
  }
  else if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_DOUBLE )
  {
    if ( st != nullptr )
    {
      ContourCellsST<double,double,TS> contour((double*)inPtsPtr,cellIter,(TS*)s,val,
                                               st,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,contour.NumBatches,contour,numThreads);
    }
    else
    {
      ContourCells<double,double,TS> contour((double*)inPtsPtr,cellIter,
                                             (TS*)s,val,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,numCells,contour,numThreads);
    }
  }
  else if ( inPtsType == VTK_FLOAT && outPtsType == VTK_DOUBLE )
  {
    if ( st != nullptr )
    {
      ContourCellsST<float,double,TS> contour((float*)inPtsPtr,cellIter,(TS*)s,val,
                                              st,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,contour.NumBatches,contour,numThreads);
    }
    else
    {
      ContourCells<float,double,TS> contour((float*)inPtsPtr,cellIter,
                                            (TS*)s,val,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,numCells,contour,numThreads);
    }
  }
  else //if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_FLOAT )
  {
    if ( st != nullptr )
    {
      ContourCellsST<double,float,TS> contour((double*)inPtsPtr,cellIter,(TS*)s,val,
                                              st,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,contour.NumBatches,contour,numThreads);
    }
    else
    {
      ContourCells<double,float,TS> contour((double*)inPtsPtr,cellIter,
                                            (TS*)s,val,outPts,tris,totalPts,totalTris);
      EXECUTE_REDUCED_SMPFOR(seqProcessing,numCells,contour,numThreads);
    }
  }
};

// Produce triangles for non-merged points-------------------------
struct ProduceTriangles
{
  vtkIdType *Tris;

  ProduceTriangles(vtkIdType *tris) : Tris(tris)
  {}

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    vtkIdType *tris = this->Tris + 4*triId;
    vtkIdType id = 3*triId;
    for ( ; triId < endTriId; ++triId )
    {
      *tris++ = 3;
      *tris++ = id++;
      *tris++ = id++;
      *tris++ = id++;
    }
  }
};


//========================= GENERAL PATH (POINT MERGING) =======================
// Use vtkStaticEdgeLocatorTemplate for edge-based point merging. Processing is
// available with and without a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesBase
{
  typedef std::vector<EdgeTuple<IDType,float>> EdgeVectorType;
  typedef std::vector<MergeTuple<IDType,float>> MergeVectorType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    EdgeVectorType LocalEdges;
    CellIter LocalCellIter;

    LocalDataType()
    {
      this->LocalEdges.reserve(2048);
    }
  };

  CellIter *Iter;
  const TS *Scalars;
  double  Value;
  MergeTuple<IDType,float> *Edges;
  vtkCellArray *Tris;
  vtkIdType NumTris;
  int NumThreadsUsed;
  vtkIdType TotalTris; //the total triangles thus far (support multiple contours)

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractEdgesBase(CellIter *c, TS *s, double value, vtkCellArray *tris,
                   vtkIdType totalTris) :
    Iter(c), Scalars(s), Value(value), Edges(nullptr), Tris(tris),
    NumTris(0), NumThreadsUsed(0), TotalTris(totalTris)
  {}

  // Set up the iteration process
  void Initialize()
  {
    auto & localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() provided by subclass

  // Composite local thread data
  void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    vtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    for ( auto ldItr=this->LocalData.begin(); ldItr != ldEnd; ++ldItr )
    {
      numTris += static_cast<vtkIdType>(((*ldItr).LocalEdges.size() / 3)); //three edges per triangle
      this->NumThreadsUsed++;
    }

    // Allocate space for VTK triangle output. Take into account previous
    // contours.
    this->NumTris = numTris;
    this->Tris->WritePointer((this->NumTris+this->TotalTris),
                             4*(this->NumTris+this->TotalTris));

    // Copy local edges to global edge array. Add in the originating edge id
    // used later when merging.
    EdgeVectorType emptyVector;
    this->Edges = new MergeTuple<IDType,float>[3*this->NumTris]; //three edges per triangle
    vtkIdType edgeNum=0;
    for ( auto ldItr=this->LocalData.begin(); ldItr != ldEnd; ++ldItr )
    {
      auto eEnd = (*ldItr).LocalEdges.end();
      for ( auto eItr = (*ldItr).LocalEdges.begin(); eItr != eEnd; ++eItr )
      {
        this->Edges[edgeNum].V0 = eItr->V0;
        this->Edges[edgeNum].V1 = eItr->V1;
        this->Edges[edgeNum].T = eItr->T;
        this->Edges[edgeNum].EId = edgeNum;
        edgeNum++;
      }
      (*ldItr).LocalEdges.swap(emptyVector); //frees memory
    }//For all threads
  }//Reduce
};//ExtractEdgesBase

// Traverse all cells and extract intersected edges (without scalar tree).
template <typename IDType, typename TS>
struct ExtractEdges : public ExtractEdgesBase<IDType,TS>
{
  ExtractEdges(CellIter *c, TS *s, double value, vtkCellArray *tris,
               vtkIdType totalTris) :
    ExtractEdgesBase<IDType,TS>(c, s, value, tris, totalTris)
  {}

  // Set up the iteration process
  void Initialize()
  {
    this->ExtractEdgesBase<IDType,TS>::Initialize();
  }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto & localData = this->LocalData.Local();
    auto & lEdges = localData.LocalEdges;
    CellIter *cellIter = &localData.LocalCellIter;
    const vtkIdType *c = cellIter->Initialize(cellId); //connectivity array
    unsigned short isoCase, numEdges, i;
    const unsigned short *edges;
    double s[MAX_CELL_VERTS], value=this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;

    for ( ; cellId < endCellId; ++cellId )
    {
      // Compute case by repeated masking of scalar value
      for ( isoCase=0, i=0; i < cellIter->NumVerts; ++i )
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
        isoCase |= ( s[i] >= value ? BaseCell::Mask[i] : 0 );
      }
      edges = cellIter->GetCase(isoCase);

      if ( *edges > 0 )
      {
        numEdges = *edges++;
        for (i=0; i<numEdges; ++i, edges+=2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          t = ( c[v0] < c[v1] ? t : (1.0-t) ); //edges (v0,v1) must have v0<v1
          lEdges.emplace_back(c[v0],c[v1],t); //edge constructor may swap v0<->v1
        }//for all edges in this case
      }//if contour passes through this cell
      c += cellIter->Next(cellId); //move to the next cell
    }//for all cells in this batch
  }

  // Composite local thread data
  void Reduce()
  {
    this->ExtractEdgesBase<IDType,TS>::Reduce();
  }//Reduce
};//ExtractEdges


// Generate edges using a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesST : public ExtractEdgesBase<IDType,TS>
{
  vtkScalarTree *ScalarTree;
  vtkIdType NumBatches;

  ExtractEdgesST(CellIter *c, TS *s, double value, vtkScalarTree *st,
                 vtkCellArray *tris, vtkIdType totalTris) :
    ExtractEdgesBase<IDType,TS>(c, s, value,  tris, totalTris), ScalarTree(st)
  {
    this->ScalarTree->InitTraversal(this->Value);
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches();
  }

  // Set up the iteration process
  void Initialize()
  {
    this->ExtractEdgesBase<IDType,TS>::Initialize();
  }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    auto & localData = this->LocalData.Local();
    auto & lEdges = localData.LocalEdges;
    CellIter *cellIter = &localData.LocalCellIter;
    const vtkIdType *c;
    unsigned short isoCase, numEdges, i;
    const unsigned short *edges;
    double s[MAX_CELL_VERTS], value=this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const vtkIdType *cellIds;
    vtkIdType numCells;

    for ( ; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum,numCells);
      for (vtkIdType idx=0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for ( isoCase=0, i=0; i < cellIter->NumVerts; ++i )
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
          isoCase |= ( s[i] >= value ? BaseCell::Mask[i] : 0 );
        }
        edges = cellIter->GetCase(isoCase);

        if ( *edges > 0 )
        {
          numEdges = *edges++;
          for (i=0; i<numEdges; ++i, edges+=2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            t = ( c[v0] < c[v1] ? t : (1.0-t) ); //edges (v0,v1) must have v0<v1
            lEdges.emplace_back(c[v0],c[v1],t); //edge constructor may swap v0<->v1
          }//for all edges in this case
        }//if contour passes through this cell
      }//for all cells in this batch
    }//for all batches
  }

  // Composite local thread data
  void Reduce()
  {
    this->ExtractEdgesBase<IDType,TS>::Reduce();
  }//Reduce

};//ExtractEdgesST

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProduceMergedTriangles
{
  typedef MergeTuple<IDType,float> MergeTupleType;

  const MergeTupleType *MergeArray;
  const IDType *Offsets;
  vtkIdType NumTris;
  vtkIdType *Tris;
  vtkIdType TotalPts;
  int NumThreadsUsed; //placeholder

  ProduceMergedTriangles(const MergeTupleType *merge, const IDType *offsets,
                         vtkIdType numTris, vtkIdType *tris,
                         vtkIdType totalPts, vtkIdType totalTris) :
    MergeArray(merge), Offsets(offsets), NumTris(numTris), Tris(tris),
    TotalPts(totalPts), NumThreadsUsed(1)
  {
    this->Tris = tris + 4*totalTris;
  }

  void Initialize()
  {
    ;//without this method Reduce() is not called
  }

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTupleType *mergeArray = this->MergeArray;
    const IDType *offsets = this->Offsets;
    IDType i, numPtsInGroup, eid, triId;

    for ( ; ptId < endPtId; ++ptId )
    {
      numPtsInGroup = offsets[ptId+1] - offsets[ptId];
      for ( i=0; i<numPtsInGroup; ++i )
      {
        eid = mergeArray[offsets[ptId]+i].EId;
        triId = eid / 3;
        *(this->Tris + 4*triId + eid-(3*triId) + 1) = ptId + this->TotalPts;
      }//for this group of coincident edges
    }//for all merged points
  }

  // Update the triangle connectivity (numPts for each triangle. This could
  // be done in parallel but it's probably not faster.
  void Reduce()
  {
    vtkIdType *tris = this->Tris;
    for ( IDType triId=0; triId < this->NumTris; ++triId, tris+=4 )
    {
      *tris = 3;
    }
  }
};

// This method generates the output isosurface points. One point per
// merged edge is generated.
template <typename TIP, typename TOP, typename IDType>
struct ProduceMergedPoints
{
  typedef MergeTuple<IDType,float> MergeTupleType;

  const MergeTupleType *MergeArray;
  const IDType *Offsets;
  const TIP *InPts;
  TOP *OutPts;

  ProduceMergedPoints(const MergeTupleType *merge, const IDType *offsets,
                      TIP *inPts, TOP *outPts, vtkIdType totalPts) :
    MergeArray(merge), Offsets(offsets), InPts(inPts)
  {
    this->OutPts = outPts + 3*totalPts;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTupleType *mergeTuple;
    IDType v0, v1;
    const TIP *x0, *x1, *inPts=this->InPts;
    TOP *x, *outPts=this->OutPts;
    float t;

    for ( ; ptId < endPtId; ++ptId )
    {
      mergeTuple = this->MergeArray + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      x0 = inPts + 3*v0;
      x1 = inPts + 3*v1;
      x = outPts + 3*ptId;
      x[0] = x0[0] + t*(x1[0]-x0[0]);
      x[1] = x0[1] + t*(x1[1]-x0[1]);
      x[2] = x0[2] + t*(x1[2]-x0[2]);
    }
  }
};

// If requested, interpolate point data attributes. The merge tuple contains an
// interpolation value t for the merged edge.
template <typename TIds>
struct ProduceAttributes
{
  const MergeTuple<TIds,float> *Edges; //all edges, sorted into groups of merged edges
  const TIds *Offsets; //refer to single, unique, merged edge
  ArrayList *Arrays; //carry list of attributes to interpolate
  vtkIdType TotalPts; //total points / multiple contours computed previously

  ProduceAttributes(const MergeTuple<TIds,float> *mt, const TIds *offsets,
                    ArrayList *arrays, vtkIdType totalPts) :
    Edges(mt), Offsets(offsets), Arrays(arrays), TotalPts(totalPts)
  {}

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTuple<TIds,float> *mergeTuple;
    TIds v0, v1;
    float t;

    for ( ; ptId < endPtId; ++ptId )
    {
      mergeTuple = this->Edges + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      this->Arrays->InterpolateEdge(v0,v1,t,ptId+this->TotalPts);
    }
  }
};

// Make the source code a little more readable
#define EXTRACT_MERGED(VTK_type,_type)          \
  case VTK_type: \
  { \
    if ( st == nullptr ) \
    { \
      ExtractEdges<TIds,_type> extractEdges(cellIter,(_type*)s,isoValue,newPolys,totalTris); \
      EXECUTE_REDUCED_SMPFOR(seqProcessing,numCells,extractEdges,numThreads); \
      numTris = extractEdges.NumTris;  \
      tris = newPolys->GetPointer();   \
      mergeEdges = extractEdges.Edges; \
    } \
    else                                       \
    { \
      ExtractEdgesST<TIds,_type> extractEdges(cellIter,(_type*)s,isoValue,st,newPolys,totalTris); \
      EXECUTE_REDUCED_SMPFOR(seqProcessing,extractEdges.NumBatches,extractEdges,numThreads); \
      numTris = extractEdges.NumTris;  \
      tris = newPolys->GetPointer();   \
      mergeEdges = extractEdges.Edges; \
    } \
  } \
  break;

// Wrapper to handle multiple template types for merged processing
template <typename TIds>
int ProcessMerged(vtkIdType numCells, vtkPoints *inPts, CellIter *cellIter,
                  int sType, void *s, double isoValue, vtkPoints *outPts,
                  vtkCellArray *newPolys, vtkTypeBool intAttr, vtkDataArray *inScalars,
                  vtkPointData *inPD, vtkPointData *outPD, vtkScalarTree *st, vtkTypeBool seqProcessing,
                  int &numThreads, vtkIdType totalPts, vtkIdType totalTris)
{
  // Extract edges that the contour intersects. Templated on type of scalars.
  // List below the explicit choice of scalars that can be processed.
  vtkIdType numTris=0, *tris=nullptr;
  MergeTuple<TIds,float> *mergeEdges=nullptr; //may need reference counting
  switch ( sType ) //process these scalar types, others could easily be added
  {
    EXTRACT_MERGED(VTK_UNSIGNED_INT,unsigned int);
    EXTRACT_MERGED(VTK_INT,int);
    EXTRACT_MERGED(VTK_FLOAT,float);
    EXTRACT_MERGED(VTK_DOUBLE,double);
    default:
      vtkGenericWarningMacro(<<"Scalar type not supported");
      return 0;
  };
  int nt = numThreads;

  // Make sure data was produced
  if ( numTris <= 0 )
  {
    outPts->SetNumberOfPoints(0);
    delete [] mergeEdges;
    return 1;
  }

  // Merge coincident edges. The Offsets refer to the single unique edge
  // from the sorted group of duplicate edges.
  vtkIdType numPts;
  vtkStaticEdgeLocatorTemplate<TIds,float> loc;
  const TIds *offsets = loc.MergeEdges(3*numTris,mergeEdges,numPts);

  // Generate triangles.
  ProduceMergedTriangles<TIds> produceTris(mergeEdges,offsets,numTris,tris,
                                           totalPts,totalTris);
  EXECUTE_REDUCED_SMPFOR(seqProcessing,numPts,produceTris,numThreads);
  numThreads = nt;

  // Generate points (one per unique edge)
  outPts->GetData()->WriteVoidPointer(0,3*(numPts+totalPts));
  int inPtsType = inPts->GetDataType();
  void *inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  void *outPtsPtr = outPts->GetVoidPointer(0);

  // Only handle combinations of real types
  if ( inPtsType == VTK_FLOAT && outPtsType == VTK_FLOAT )
  {
    ProduceMergedPoints<float,float,TIds>
      producePts(mergeEdges, offsets, (float*)inPtsPtr, (float*)outPtsPtr, totalPts );
    EXECUTE_SMPFOR(seqProcessing,numPts,producePts);
  }
  else if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_DOUBLE )
  {
    ProduceMergedPoints<double,double,TIds>
      producePts(mergeEdges, offsets, (double*)inPtsPtr, (double*)outPtsPtr, totalPts );
    EXECUTE_SMPFOR(seqProcessing,numPts,producePts);
  }
  else if ( inPtsType == VTK_FLOAT && outPtsType == VTK_DOUBLE )
  {
    ProduceMergedPoints<float,double,TIds>
      producePts(mergeEdges, offsets, (float*)inPtsPtr, (double*)outPtsPtr, totalPts );
    EXECUTE_SMPFOR(seqProcessing,numPts,producePts);
  }
  else //if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_FLOAT )
  {
    ProduceMergedPoints<double,float,TIds>
      producePts(mergeEdges, offsets, (double*)inPtsPtr, (float*)outPtsPtr, totalPts );
    EXECUTE_SMPFOR(seqProcessing,numPts,producePts);
  }

  // Now process point data attributes if requested
  if ( intAttr )
  {
    ArrayList arrays;
    if ( totalPts <= 0 ) //first contour value generating output
    {
      outPD->InterpolateAllocate(inPD,numPts);
      outPD->RemoveArray(inScalars->GetName());
      arrays.ExcludeArray(inScalars);
      arrays.AddArrays(numPts,inPD,outPD);
    }
    else
    {
      arrays.Realloc(totalPts+numPts);
    }
    ProduceAttributes<TIds> interpolate(mergeEdges,offsets,&arrays,totalPts);
    EXECUTE_SMPFOR(seqProcessing,numPts,interpolate);
  }

  // Clean up
  delete [] mergeEdges;
  return 1;
};
#undef EXTRACT_MERGED

// Functor for computing cell normals. Could easily be templated on output
// point type but we are trying to control object size.
struct ComputeCellNormals
{
  vtkPoints *Points;
  vtkIdType *Tris;
  float *CellNormals;

  ComputeCellNormals(vtkPoints *pts, vtkIdType *tris, float *cellNormals) :
    Points(pts), Tris(tris), CellNormals(cellNormals)
  {}

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    vtkIdType *tri = this->Tris + 4*triId;
    tri++; //move the pointer to the begging of triangle connectivity
    float *n = this->CellNormals + 3*triId;
    double nd[3];

    for ( ; triId < endTriId; ++triId, tri+=4 )
    {
      vtkTriangle::ComputeNormal(this->Points, 3, tri, nd);
      *n++ = nd[0];
      *n++ = nd[1];
      *n++ = nd[2];
    }
  }
};

// Generate normals on output triangles
vtkFloatArray *GenerateTriNormals(vtkTypeBool seqProcessing, vtkPoints *pts, vtkCellArray *tris)
{
  vtkIdType numTris = tris->GetNumberOfCells();

  vtkFloatArray *cellNormals = vtkFloatArray::New();
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numTris);
  float *n = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Execute functor over all triangles
  ComputeCellNormals computeNormals(pts,tris->GetPointer(),n);
  EXECUTE_SMPFOR(seqProcessing, numTris, computeNormals);

  return cellNormals;
}

// Functor for averaging normals at each merged point.
template <typename TId>
struct AverageNormals
{
  vtkStaticCellLinksTemplate<TId> *Links;
  const float *CellNormals;
  float *PointNormals;

  AverageNormals(vtkStaticCellLinksTemplate<TId> *links, float *cellNormals, float *ptNormals) :
   Links(links), CellNormals(cellNormals), PointNormals(ptNormals)
  {}

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    TId i, numTris;
    const TId *tris;
    const float *nc;
    float *n = this->PointNormals + 3*ptId;

    for ( ; ptId < endPtId; ++ptId, n+=3 )
    {
      numTris = this->Links->GetNumberOfCells(ptId);
      tris = this->Links->GetCells(ptId);
      n[0] = n[1] = n[2] = 0.0;
      for ( i=0; i < numTris; ++i)
      {
        nc = this->CellNormals + 3*tris[i];
        n[0] += nc[0];
        n[1] += nc[1];
        n[2] += nc[2];
      }
      vtkMath::Normalize(n);
    }
  }
};

// Generate normals on merged points. Average cell normals at each point.
template <typename TId>
void GeneratePointNormals(vtkTypeBool seqProcessing, vtkPoints *pts, vtkCellArray *tris,
                          vtkFloatArray *cellNormals, vtkPointData *pd)
{
  vtkIdType numPts = pts->GetNumberOfPoints();

  vtkFloatArray *ptNormals = vtkFloatArray::New();
  ptNormals->SetName("Normals");
  ptNormals->SetNumberOfComponents(3);
  ptNormals->SetNumberOfTuples(numPts);
  float *ptN = static_cast<float*>(ptNormals->GetVoidPointer(0));

  // Grab the computed triangle normals
  float *triN = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Build cell links
  vtkPolyData *dummy = vtkPolyData::New();
  dummy->SetPoints(pts);
  dummy->SetPolys(tris);
  vtkStaticCellLinksTemplate<TId> links;
  links.BuildLinks(dummy);

  // Process all points, averaging normals
  AverageNormals<TId> average(&links, triN, ptN);
  EXECUTE_SMPFOR(seqProcessing,numPts,average);

  // Clean up and get out
  dummy->Delete();
  pd->SetNormals(ptNormals);
  cellNormals->Delete();
  ptNormals->Delete();
};

}//anonymous namespace


//-----------------------------------------------------------------------------
// Construct an instance of the class.
vtkContour3DLinearGrid::vtkContour3DLinearGrid()
{
  this->ContourValues = vtkContourValues::New();

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // By default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  this->MergePoints = false;
  this->InterpolateAttributes = false;
  this->ComputeNormals = false;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;
}

//-----------------------------------------------------------------------------
vtkContour3DLinearGrid::~vtkContour3DLinearGrid()
{
  this->ContourValues->Delete();

  if ( this->ScalarTree )
  {
    this->ScalarTree->Delete();
    this->ScalarTree = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkContour3DLinearGrid::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

// Make code more readable
#define EXTRACT_FAST_PATH(VTK_SCALAR_type,_type) \
  case VTK_SCALAR_type: \
  ProcessFastPath<_type>(numCells, inPts, cellIter, \
    (_type*)sPtr, value, stree, outPts, newPolys, \
    this->SequentialProcessing,this->NumberOfThreadsUsed, \
    totalPts, totalTris); \
    break;

//-----------------------------------------------------------------------------
// Specialized contouring filter to handle unstructured grids with 3D linear
// cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtkContour3DLinearGrid::
RequestData(vtkInformation*, vtkInformationVector** inputVector,
            vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input || !output)
  {
    return 0;
  }

  // Make sure there is data to process
  vtkCellArray *cells = input->GetCells();
  vtkIdType numPts, numTris, numCells = cells->GetNumberOfCells();
  vtkDataArray *inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars)
  {
    return 1;
  }
  int sType = inScalars->GetDataType();
  void *sPtr = inScalars->GetVoidPointer(0);

  // Get the contour values.
  int numContours = this->ContourValues->GetNumberOfContours();
  double value, *values=this->ContourValues->GetValues();
  if ( numContours < 1 )
  {
    vtkDebugMacro(<<"No contour values defined");
    return 1;
  }

  // Check the input point type. Only real types are supported.
  vtkPoints *inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();
  int inPtsType = inPts->GetDataType();
  if ( (inPtsType != VTK_FLOAT && inPtsType != VTK_DOUBLE) )
  {
    vtkErrorMacro(<<"Input point type not supported");
    return 0;
  }

  // Create the output points. Only real types are supported.
  vtkPoints *outPts = vtkPoints::New();
  if ( this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION )
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }

  // Output triangles go here.
  vtkCellArray *newPolys = vtkCellArray::New();

  // Process all contour values
  vtkIdType totalPts = 0;
  vtkIdType totalTris = 0;

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  vtkIdType *conn = cells->GetPointer();
  unsigned char *cellTypes = static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  vtkIdType *locs = static_cast<vtkIdType*>(input->GetCellLocationsArray()->GetVoidPointer(0));
  CellIter *cellIter = new CellIter(numCells,cellTypes,conn,locs);

  // See whether scalar tree has been provided.
  vtkScalarTree *stree=nullptr;
  if ( this->UseScalarTree )
  {
    if ( this->ScalarTree == nullptr )
    {
      vtkSpanSpace *spanSpace = vtkSpanSpace::New();
      spanSpace->SetResolution(sqrt(numCells)/20); //clamped to 100 minimum
      spanSpace->SetBatchSize(100);
      this->ScalarTree = spanSpace;
    }
    stree = this->ScalarTree;
    this->ScalarTree->SetDataSet(input);
    this->ScalarTree->SetScalars(inScalars);
   }

  // Now produce the output: fast path or general path
  int mergePoints = this->MergePoints | this->ComputeNormals | this->InterpolateAttributes;
  if ( ! mergePoints )
  {//fast path
    // Generate all of the points at once (for multiple contours) and then produce the triangles.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      switch ( sType ) //process these scalar types, others could easily be added
      {
        EXTRACT_FAST_PATH(VTK_UNSIGNED_INT,unsigned int);
        EXTRACT_FAST_PATH(VTK_INT,int);
        EXTRACT_FAST_PATH(VTK_FLOAT,float);
        EXTRACT_FAST_PATH(VTK_DOUBLE,double);
        default:
          vtkGenericWarningMacro(<<"Scalar type not supported");
          return 0;
      };

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    }//for all contours

    // From the points create the output triangles. In the fast path there
    // are three points for every triangle. Many points are typically
    // duplicates but point merging is a significant cost so ignored in the
    // fast path.
    numTris = newPolys->GetNumberOfCells();
    vtkIdType *tris = newPolys->GetPointer();
    ProduceTriangles produceTris(tris);
    EXECUTE_SMPFOR(this->SequentialProcessing,numTris,produceTris)
  }

  else //Need to merge points, and possibly perform attribute interpolation
       //and generate normals. Hence use the slower path.
  {
    vtkPointData *inPD = input->GetPointData();
    vtkPointData *outPD = output->GetPointData();

    // Determine the size/type of point and cell ids needed to index points
    // and cells. Using smaller ids results in a greatly reduced memory footprint
    // and faster processing.
    this->LargeIds = ( numPts >= VTK_INT_MAX || numCells >= VTK_INT_MAX ? true : false );

    // Generate all of the merged points and triangles at once (for multiple
    // contours) and then produce the normals if requested.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      if ( this->LargeIds == false )
      {
        if ( ! ProcessMerged<int>(numCells, inPts, cellIter, sType, sPtr, value,
                                  outPts, newPolys, this->InterpolateAttributes,
                                  inScalars, inPD, outPD, stree, this->SequentialProcessing,
                                  this->NumberOfThreadsUsed, totalPts, totalTris) )
        {
          return 0;
        }
      }
      else
      {
        if ( ! ProcessMerged<vtkIdType>(numCells, inPts, cellIter, sType, sPtr, value,
                                        outPts, newPolys, this->InterpolateAttributes,
                                        inScalars, inPD, outPD, stree, this->SequentialProcessing,
                                        this->NumberOfThreadsUsed, totalPts, totalTris) )
        {
          return 0;
        }
      }

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    }//for all contour values

    // If requested, compute normals. Basically triangle normals are averaged
    // on each merged point. Requires building static CellLinks so it is a
    // relatively expensive operation. (This block of code is separate to
    // control .obj object bloat.)
    if ( this->ComputeNormals )
    {
      vtkFloatArray *triNormals =
        GenerateTriNormals(this->SequentialProcessing, outPts,newPolys);
      if ( this->LargeIds )
      {
        GeneratePointNormals<vtkIdType>(this->SequentialProcessing,
                                        outPts,newPolys,triNormals,outPD);
      }
      else
      {
        GeneratePointNormals<int>(this->SequentialProcessing,
                                  outPts,newPolys,triNormals,outPD);
      }
    }
  }// slower path requires point merging

  // Report the results of execution
  vtkDebugMacro(<<"Created: " << outPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // Clean up
  delete cellIter;
  output->SetPoints(outPts);
  outPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkContour3DLinearGrid::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkContour3DLinearGrid::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
int vtkContour3DLinearGrid::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkContour3DLinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Precision of the output points: "
     << this->OutputPointsPrecision << "\n";

  os << indent << "Merge Points: "
     << (this->MergePoints ? "true\n" : "false\n");
  os << indent << "Interpolate Attributes: "
     << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: "
     << (this->ComputeNormals ? "true\n" : "false\n");

  os << indent << "Sequential Processing: "
     << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: "
     << (this->LargeIds ? "true\n" : "false\n");

  os << indent << "Use Scalar Tree: "
     << (this->UseScalarTree ? "On\n" : "Off\n");
  if ( this->ScalarTree )
  {
    os << indent << "Scalar Tree: " << this->ScalarTree << "\n";
  }
  else
  {
    os << indent << "Scalar Tree: (none)\n";
  }

}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
#undef EXTRACT_MERGED
#undef EXTRACT_FAST_PATH
