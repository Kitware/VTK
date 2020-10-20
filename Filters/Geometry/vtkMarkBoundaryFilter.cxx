/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkBoundaryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_0_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkMarkBoundaryFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkPentagonalPrism.h"
#include "vtkPointData.h"
#include "vtkPyramid.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkTetra.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridCellIterator.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

vtkStandardNewMacro(vtkMarkBoundaryFilter);

//------------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkMarkBoundaryFilter::vtkMarkBoundaryFilter()
{
  this->GenerateBoundaryFaces = false;

  this->BoundaryPointsName = nullptr;
  this->SetBoundaryPointsName("BoundaryPoints");
  this->BoundaryCellsName = nullptr;
  this->SetBoundaryCellsName("BoundaryCells");
  this->BoundaryFacesName = nullptr;
  this->SetBoundaryFacesName("BoundaryFaces");
}

//------------------------------------------------------------------------------
vtkMarkBoundaryFilter::~vtkMarkBoundaryFilter()
{
  delete[] this->BoundaryPointsName;
  delete[] this->BoundaryCellsName;
  delete[] this->BoundaryFacesName;
}

//------------------------------------------------------------------------------
int vtkMarkBoundaryFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMarkBoundaryFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
void vtkMarkBoundaryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Boundary Faces: " << (this->GenerateBoundaryFaces ? "On\n" : "Off\n");

  os << indent << "Boundary Points Name: " << this->GetBoundaryPointsName() << endl;
  os << indent << "Boundary Cells Name: " << this->GetBoundaryCellsName() << endl;
  os << indent << "Boundary Faces Name: " << this->GetBoundaryFacesName() << endl;
}

//------------------------------------------------------------------------------
// Core algorithms for marking boundary cells
//
namespace // anonymous
{
void InitializeBoundaryArrays(
  vtkUnsignedCharArray* bPoints, vtkUnsignedCharArray* bCells, vtkIdTypeArray* bFaces)
{
  vtkIdType numPts = bPoints->GetNumberOfTuples();
  vtkIdType numCells = bCells->GetNumberOfTuples();

  unsigned char* bPointsPtr = bPoints->GetPointer(0);
  std::fill_n(bPointsPtr, numPts, 0);

  unsigned char* bCellsPtr = bCells->GetPointer(0);
  std::fill_n(bCellsPtr, numCells, 0);

  if (bFaces != nullptr)
  {
    vtkIdType* bFacesPtr = bFaces->GetPointer(0);
    std::fill_n(bFacesPtr, numCells, 0);
  }
}

// Superclass for marking boundary information. Derived classes
// as specific to dataset type.
struct MarkCellBoundary
{
  const unsigned char* CellGhosts;
  unsigned char* PtMarks;
  unsigned char* CellMarks;
  vtkIdType* FaceMarks;

  MarkCellBoundary(const unsigned char* ghosts, unsigned char* ptMarks, unsigned char* cellMarks,
    vtkIdType* faceMarks)
    : CellGhosts(ghosts)
    , PtMarks(ptMarks)
    , CellMarks(cellMarks)
    , FaceMarks(faceMarks)
  {
  }

  // Threaded method. The cell info is being written to by only one
  // thread. The point info may be written to by multiple threads, but the
  // info is always set to the same value (=1).
  void MarkCell(vtkIdType cellId, vtkIdType faceNum, vtkIdType npts, const vtkIdType* pts)
  {
    this->CellMarks[cellId] = 1;
    if (this->FaceMarks != nullptr && faceNum < (int)sizeof(vtkIdType))
    {
      this->FaceMarks[cellId] |= (static_cast<vtkIdType>(1) << faceNum);
    }
    for (auto i = 0; i < npts; ++i)
    {
      this->PtMarks[pts[i]] = 1;
    }
  }

  // Specialized method for structured data.
  void MarkStructuredCell(vtkIdType cellId, vtkIdType faceMark, vtkIdList* ptIds)
  {
    this->CellMarks[cellId] = 1;
    if (this->FaceMarks)
    {
      this->FaceMarks[cellId] = faceMark;
    }
    vtkIdType numBPts = ptIds->GetNumberOfIds();
    for (vtkIdType i = 0; i < numBPts; ++i)
    {
      this->PtMarks[ptIds->GetId(i)] = 1;
    }
  }
};

struct MarkPolys : MarkCellBoundary
{
  vtkPolyData* Mesh;
  vtkIdType Offset;
  vtkCellArray* Polys;
  vtkStaticCellLinksTemplate<vtkIdType>* Links;
  // Working objects to avoid repeated allocation
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIter;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> Neighbors;

  MarkPolys(vtkPolyData* mesh, const unsigned char* ghosts, vtkIdType offset, vtkCellArray* polys,
    vtkStaticCellLinksTemplate<vtkIdType>* links, unsigned char* ptMarks, unsigned char* cellMarks,
    vtkIdType* faceMarks)
    : MarkCellBoundary(ghosts, ptMarks, cellMarks, faceMarks)
    , Mesh(mesh)
    , Offset(offset)
    , Polys(polys)
    , Links(links)
  {
  }

  void Initialize()
  {
    this->CellIter.Local().TakeReference(
      static_cast<vtkCellArrayIterator*>(this->Polys->NewIterator()));
    this->Neighbors.Local().TakeReference(vtkIdList::New());
    this->Neighbors.Local()->Allocate(2);
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& cellIter = this->CellIter.Local();
    auto& neighbors = this->Neighbors.Local();
    vtkIdType npts, edgePts[2];
    const vtkIdType* pts;

    for (cellId = 0; cellId < endCellId; ++cellId)
    {
      // Handle ghost cells here.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // Mark boundary polygons. A boundary polygon has an edge used by only the boundary polygon.
      cellIter->GetCellAtId(cellId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        edgePts[0] = pts[i];
        edgePts[1] = pts[(i + 1) % npts];
        this->Links->GetCells(2, edgePts, neighbors);
        if (neighbors->GetNumberOfIds() < 2)
        {
          this->MarkCell(cellId, i, 2, edgePts);
        }
      }
    } // for all cells in this batch
  }

  void Reduce() {}
};

int PolyDataExecute(vtkDataSet* dsInput, const unsigned char* ghosts, unsigned char* bPoints,
  unsigned char* bCells, vtkIdType* bFaces)
{
  vtkPolyData* input = static_cast<vtkPolyData*>(dsInput);
  vtkIdType numPts = input->GetNumberOfPoints();

  // To simplify and accelerate marking, traverse each of the four
  // cell arrays that make up vtkPolyData separately. For now, do not
  // process triangle strips.
  vtkIdType cellId = 0;
  vtkCellArray* verts = input->GetVerts();
  vtkIdType numVerts = verts->GetNumberOfCells();
  vtkCellArray* lines = input->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();
  vtkCellArray* polys = input->GetPolys();
  vtkIdType numPolys = polys->GetNumberOfCells();
  vtkCellArray* strips = input->GetStrips();
  vtkIdType numStrips = strips->GetNumberOfCells();
  if (numStrips > 0)
  {
    vtkLog(ERROR, "Triangle strips not supported.");
  }

  // All verts are considered on the boundary. Process in serial since
  // it's an uncommon workflow.
  vtkIdType npts;
  const vtkIdType* pts;
  if (numVerts > 0)
  {
    MarkCellBoundary marker(ghosts, bPoints, bCells, bFaces);
    auto iter = vtk::TakeSmartPointer(verts->NewIterator());
    for (cellId = 0; cellId < numVerts; ++cellId)
    {
      iter->GetCellAtId(cellId, npts, pts);
      marker.MarkCell(cellId, 0, npts, pts);
    }
  }

  // Lines at the end of linked chains of lines are considered boundary.
  // This is done in serial since it's an uncommon workflow.
  if (numLines > 0)
  {
    MarkCellBoundary marker(ghosts, bPoints, bCells, bFaces);
    auto iter = vtk::TakeSmartPointer(lines->NewIterator());
    vtkStaticCellLinksTemplate<vtkIdType> links;
    links.ThreadedBuildLinks(numPts, numLines, lines);
    for (cellId = 0; cellId < numLines; ++cellId)
    {
      iter->GetCellAtId(cellId, npts, pts);
      // Only first and last point of line/polyline need be checked.
      // A line/polyline can have at most two boundary "faces".
      if (links.GetNcells(pts[0]) < 2)
      {
        marker.MarkCell(cellId, 0, 1, pts);
      }
      if (links.GetNcells(pts[npts - 1]) < 2)
      {
        marker.MarkCell(cellId, 1, 1, pts + npts - 1);
      }
    }
  }

  // Perform the threaded boundary marking of boundary polygons, and possibly
  // polygon edges (i.e., the 1D faces of polygons).
  if (numPolys > 0)
  {
    vtkStaticCellLinksTemplate<vtkIdType> links;
    links.ThreadedBuildLinks(numPts, numPolys, polys);
    MarkPolys mark(input, ghosts, (numVerts + numLines), polys, &links, bPoints, bCells, bFaces);
    vtkSMPTools::For(0, numPolys, mark);
  }

  return 1;
}

//--------------------------------------------------------------------------
// Given a cell, mark boundary features from the cell.  This method works
// with unstructured grids.
void MarkUGCell(vtkUnstructuredGrid* input, vtkIdType cellId, int cellType, vtkIdType npts,
  const vtkIdType* pts, vtkUnstructuredGridCellIterator* cellIter, vtkGenericCell* cell,
  MarkCellBoundary* marker)
{
  vtkIdType faceId, numEdgePts, numFacePts;
  const int MAX_FACE_POINTS = 32;
  vtkIdType ptIds[MAX_FACE_POINTS]; // cell face point ids
  const vtkIdType* faceVerts;
  bool insertEdge, insertFace;
  static const int pixelConvert[4] = { 0, 1, 3, 2 };
  vtkIdType edgePts[2];

  switch (cellType)
  {
    case VTK_EMPTY_CELL:
      break;

    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      // All verts are considered boundary
      marker->MarkCell(cellId, 0, npts, pts);
      break;

    case VTK_LINE:
    case VTK_POLY_LINE:
      // The end points, used by one line, are boundary
      if (input->IsCellBoundary(cellId, 1, pts))
      {
        marker->MarkCell(cellId, 0, 1, pts);
      }
      if (input->IsCellBoundary(cellId, 1, pts + npts - 1))
      {
        marker->MarkCell(cellId, 1, 1, pts + npts - 1);
      }
      break;

    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_POLYGON:
      // Polygons with boundary edges are boundary cells
      for (auto i = 0; i < npts; ++i)
      {
        edgePts[0] = pts[i];
        edgePts[1] = pts[(i + 1) % npts];
        if (input->IsCellBoundary(cellId, 2, edgePts))
        {
          marker->MarkCell(cellId, i, 2, edgePts);
        }
      }
      break;

    case VTK_TRIANGLE_STRIP:
      // Currently not supported. Internal edges are a pain;
      // this could be fixed if needed.
      vtkLog(ERROR, "Triangle strips not supported.");
      break;

    case VTK_PIXEL:
      // Polygons with boundary edges are boundary cells
      for (auto i = 0; i < npts; ++i)
      {
        edgePts[0] = pts[pixelConvert[i]];
        edgePts[1] = pts[pixelConvert[(i + 1) % npts]];
        if (input->IsCellBoundary(cellId, 2, edgePts))
        {
          marker->MarkCell(cellId, i, 2, edgePts);
        }
      }
      break;

    case VTK_TETRA:
      numFacePts = 3;
      for (faceId = 0; faceId < 4; faceId++)
      {
        faceVerts = vtkTetra::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_VOXEL:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkVoxel::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[pixelConvert[0]]];
        ptIds[1] = pts[faceVerts[pixelConvert[1]]];
        ptIds[2] = pts[faceVerts[pixelConvert[2]]];
        ptIds[3] = pts[faceVerts[pixelConvert[3]]];
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_HEXAHEDRON:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkHexahedron::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_WEDGE:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkWedge::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        numFacePts = 3;
        if (faceVerts[3] >= 0)
        {
          ptIds[3] = pts[faceVerts[3]];
          numFacePts = 4;
        }
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_PYRAMID:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkPyramid::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        numFacePts = 3;
        if (faceVerts[3] >= 0)
        {
          ptIds[3] = pts[faceVerts[3]];
          numFacePts = 4;
        }
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_HEXAGONAL_PRISM:
      for (faceId = 0; faceId < 8; faceId++)
      {
        faceVerts = vtkHexagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        numFacePts = 4;
        if (faceVerts[4] >= 0)
        {
          ptIds[4] = pts[faceVerts[4]];
          ptIds[5] = pts[faceVerts[5]];
          numFacePts = 6;
        }
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    case VTK_PENTAGONAL_PRISM:
      for (faceId = 0; faceId < 7; faceId++)
      {
        faceVerts = vtkPentagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        numFacePts = 4;
        if (faceVerts[4] >= 0)
        {
          ptIds[4] = pts[faceVerts[4]];
          numFacePts = 5;
        }
        insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        if (insertFace)
        {
          marker->MarkCell(cellId, faceId, numFacePts, ptIds);
        }
      }
      break;

    default:
      // Other types of 3D cells.
      cellIter->GetCell(cell);
      if (cell->GetCellDimension() == 3)
      {
        int numFaces = cell->GetNumberOfFaces();
        for (auto j = 0; j < numFaces; j++)
        {
          vtkCell* face = cell->GetFace(j);
          numFacePts = face->PointIds->GetNumberOfIds();
          insertFace = input->IsCellBoundary(cellId, numFacePts, face->PointIds->GetPointer(0));
          if (insertFace)
          {
            marker->MarkCell(cellId, j, numFacePts, face->PointIds->GetPointer(0));
          }
        } // for all cell faces
      }   // if 3D
      else if (cell->GetCellDimension() == 2)
      {
        int numEdges = cell->GetNumberOfEdges();
        for (auto j = 0; j < numEdges; j++)
        {
          vtkCell* edge = cell->GetEdge(j);
          numEdgePts = edge->PointIds->GetNumberOfIds();
          insertEdge = input->IsCellBoundary(cellId, numEdgePts, edge->PointIds->GetPointer(0));
          if (insertEdge)
          {
            marker->MarkCell(cellId, j, numEdgePts, edge->PointIds->GetPointer(0));
          }
        }  // for all cell edges
      }    // if 2D
      else // should never happen
      {
        vtkLog(ERROR, "Unsupported cell type.");
      }
  } // switch
} // MarkUGCell()

struct MarkUGrid : MarkCellBoundary
{
  vtkUnstructuredGrid* Grid;
  // Working objects to avoid repeated allocation
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkUnstructuredGridCellIterator>> CellIter;

  MarkUGrid(vtkUnstructuredGrid* grid, const unsigned char* ghosts, unsigned char* ptMarks,
    unsigned char* cellMarks, vtkIdType* faceMarks)
    : MarkCellBoundary(ghosts, ptMarks, cellMarks, faceMarks)
    , Grid(grid)
  {
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->CellIter.Local().TakeReference(
      static_cast<vtkUnstructuredGridCellIterator*>(this->Grid->NewCellIterator()));
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& cell = this->Cell.Local();
    auto& cellIter = this->CellIter.Local();

    for (cellIter->GoToCell(cellId); cellId < endCellId; ++cellId, cellIter->GoToNextCell())
    {
      // Handle ghost cells here.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      int type = cellIter->GetCellType();
      vtkIdList* pointIdList = cellIter->GetPointIds();
      vtkIdType npts = pointIdList->GetNumberOfIds();
      vtkIdType* pts = pointIdList->GetPointer(0);

      MarkUGCell(this->Grid, cellId, type, npts, pts, cellIter, cell, this);
    } // for all cells in this batch
  }

  void Reduce() {}
};

// Mark unstructured grids
int UnstructuredGridExecute(vtkDataSet* dsInput, const unsigned char* ghosts,
  unsigned char* bPoints, unsigned char* bCells, vtkIdType* bFaces)
{
  vtkUnstructuredGrid* input = static_cast<vtkUnstructuredGrid*>(dsInput);
  vtkCellArray* connectivity = input->GetCells();

  if (connectivity == nullptr)
  {
    return 0;
  }

  vtkIdType numCells = input->GetNumberOfCells();

  // Make sure links are built since link building is not thread safe.
  input->BuildLinks();

  // Perform the threaded boundary marking.
  MarkUGrid mark(input, ghosts, bPoints, bCells, bFaces);
  vtkSMPTools::For(0, numCells, mark);

  return 1;
}

struct MarkStructured : public MarkCellBoundary
{
  vtkDataSet* Input; // Input data
  vtkIdType* Extent; // Data extent
  int Dims[3];       // Grid dimensions
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> PtIds;

  MarkStructured(vtkDataSet* ds, vtkIdType ext[6], const unsigned char* ghosts,
    unsigned char* bPoints, unsigned char* bCells, vtkIdType* bFaces)
    : MarkCellBoundary(ghosts, bPoints, bCells, bFaces)
    , Input(ds)
    , Extent(ext)
  {
    this->Dims[0] = this->Extent[1] - this->Extent[0] + 1;
    this->Dims[1] = this->Extent[3] - this->Extent[2] + 1;
    this->Dims[2] = this->Extent[5] - this->Extent[4] + 1;
  }

  // Determine whether to process the structured cell at location ijk[3]
  // with the cellId given. Return a faceMark indicating what faces are
  // boundary. A faceMark==0 means no faces are boundary. Also, the point
  // ids of the face(s) on the boundary are returned.
  vtkIdType ProcessCell(vtkIdType cellId, int ijk[3], vtkIdList* ptIds)
  {
    // Are we on the boundary of the structured dataset? If not, just return.
    if (ijk[0] != 0 && ijk[0] != (this->Dims[0] - 2) && ijk[1] != 0 &&
      ijk[1] != (this->Dims[1] - 2) && ijk[2] != 0 && ijk[2] != (this->Dims[2] - 2))
    {
      return 0;
    }

    // Okay one or more faces and points are on the boundary. Need to figure out which
    // is which.
    char ptUses[8] = { 0 };
    vtkIdType faceMark = 0;
    vtkStructuredData::GetCellPoints(cellId, ptIds, VTK_XYZ_GRID, this->Dims);
    vtkIdType tmpPtIds[8];
    std::copy_n(ptIds->GetPointer(0), 8, tmpPtIds);

    if (ijk[0] == 0)
    {
      faceMark |= (1 << 0);
      ptUses[0] = ptUses[2] = ptUses[4] = ptUses[6] = 1;
    }
    if (ijk[0] == (this->Dims[0] - 2))
    {
      faceMark |= (1 << 1);
      ptUses[1] = ptUses[3] = ptUses[5] = ptUses[7] = 1;
    }
    if (ijk[1] == 0)
    {
      faceMark |= (1 << 2);
      ptUses[0] = ptUses[1] = ptUses[4] = ptUses[5] = 1;
    }
    if (ijk[1] == (this->Dims[1] - 2))
    {
      faceMark |= (1 << 3);
      ptUses[2] = ptUses[3] = ptUses[6] = ptUses[7] = 1;
    }
    if (ijk[2] == 0)
    {
      faceMark |= (1 << 4);
      ptUses[0] = ptUses[1] = ptUses[2] = ptUses[3] = 1;
    }
    if (ijk[2] == (this->Dims[2] - 2))
    {
      faceMark |= (1 << 5);
      ptUses[4] = ptUses[5] = ptUses[6] = ptUses[7] = 1;
    }

    // Mark the points
    ptIds->Reset();
    for (int i = 0; i < 8; ++i)
    {
      if (ptUses[i])
      {
        ptIds->InsertNextId(tmpPtIds[i]);
      }
    }

    return faceMark;
  }

  void Initialize() { this->PtIds.Local().TakeReference(vtkIdList::New()); }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& ptIds = this->PtIds.Local();
    for (; cellId < endCellId; ++cellId)
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // Get the ijk to see if this cell is on the boundary of the structured data.
      vtkIdType faceMark;
      int ijk[3];
      vtkStructuredData::ComputeCellStructuredCoords(cellId, this->Dims, ijk);
      if ((faceMark = this->ProcessCell(cellId, ijk, ptIds)) > 0)
      { // on boundary
        this->MarkStructuredCell(cellId, faceMark, ptIds);
      }
    } // for all cells in this batch
  }   // operator()

  void Reduce() {}
};

// Mark 3D structured grids
int StructuredExecute(vtkDataSet* input, const unsigned char* ghosts, unsigned char* bPoints,
  unsigned char* bCells, vtkIdType* bFaces)
{
  vtkIdType numCells = input->GetNumberOfCells();

  // Setup processing
  vtkIdType ext[6];
  int* tmpext;
  switch (input->GetDataObjectType())
  {
    case VTK_STRUCTURED_GRID:
    {
      vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(input);
      tmpext = grid->GetExtent();
      break;
    }
    case VTK_RECTILINEAR_GRID:
    {
      vtkRectilinearGrid* grid = vtkRectilinearGrid::SafeDownCast(input);
      tmpext = grid->GetExtent();
      break;
    }
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
    {
      vtkImageData* image = vtkImageData::SafeDownCast(input);
      tmpext = image->GetExtent();
      break;
    }
    default:
      return 0;
  }

  // Update the extent
  ext[0] = tmpext[0];
  ext[1] = tmpext[1];
  ext[2] = tmpext[2];
  ext[3] = tmpext[3];
  ext[4] = tmpext[4];
  ext[5] = tmpext[5];

  // Perform the threaded boundary marking.
  MarkStructured mark(input, ext, ghosts, bPoints, bCells, bFaces);
  vtkSMPTools::For(0, numCells, mark);

  return 1;
}

// Process general datasets
struct MarkDataSet : MarkCellBoundary
{
  vtkDataSet* DataSet;
  // Working objects to avoid repeated allocation
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> IPts;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> CellIds;

  MarkDataSet(vtkDataSet* ds, const unsigned char* ghosts, unsigned char* ptMarks,
    unsigned char* cellMarks, vtkIdType* faceMarks)
    : MarkCellBoundary(ghosts, ptMarks, cellMarks, faceMarks)
    , DataSet(ds)
  {
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->IPts.Local().TakeReference(vtkIdList::New());
    this->CellIds.Local().TakeReference(vtkIdList::New());
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& cell = this->Cell.Local();
    auto& cellIds = this->CellIds.Local();
    auto& ptIds = this->IPts.Local();

    for (; cellId < endCellId; ++cellId)
    {
      // Handle ghost cells here.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      this->DataSet->GetCell(cellId, cell);

      // Process cells differently depending on dimension
      if (cell->GetCellDimension() == 0)
      {
        vtkIdType npts = cell->PointIds->GetNumberOfIds();
        this->MarkCell(cellId, 0, npts, ptIds->GetPointer(0));
      } // if 0D

      else if (cell->GetCellDimension() == 1)
      {
        // End points of 1D cells may be boundary
        vtkIdType npts = cell->PointIds->GetNumberOfIds();
        ptIds->SetNumberOfIds(1);
        ptIds->SetId(0, cell->PointIds->GetId(0));
        this->DataSet->GetCellNeighbors(cellId, ptIds, cellIds);
        if (cellIds->GetNumberOfIds() <= 0)
        {
          this->MarkCell(cellId, 0, 1, ptIds->GetPointer(0));
        }
        ptIds->SetId(0, cell->PointIds->GetId(npts - 1));
        this->DataSet->GetCellNeighbors(cellId, ptIds, cellIds);
        if (cellIds->GetNumberOfIds() <= 0)
        {
          this->MarkCell(cellId, 1, 1, ptIds->GetPointer(npts - 1));
        }
      } // if 1D

      else if (cell->GetCellDimension() == 2)
      {
        // Boundary edges used only once are boundary
        int numEdges = cell->GetNumberOfEdges();
        for (auto j = 0; j < numEdges; j++)
        {
          vtkCell* edge = cell->GetEdge(j);
          vtkIdType numEdgePts = edge->PointIds->GetNumberOfIds();
          this->DataSet->GetCellNeighbors(cellId, edge->PointIds, cellIds);
          if (cellIds->GetNumberOfIds() <= 0)
          {
            this->MarkCell(cellId, j, numEdgePts, edge->PointIds->GetPointer(0));
          }
        } // for all cell edges
      }   // if 2D

      else if (cell->GetCellDimension() == 3)
      {
        int numFaces = cell->GetNumberOfFaces();
        for (auto j = 0; j < numFaces; j++)
        {
          vtkCell* face = cell->GetFace(j);
          vtkIdType numFacePts = face->PointIds->GetNumberOfIds();
          this->DataSet->GetCellNeighbors(cellId, face->PointIds, cellIds);
          if (cellIds->GetNumberOfIds() <= 0)
          {
            this->MarkCell(cellId, j, numFacePts, face->PointIds->GetPointer(0));
          }
        } // for all cell faces
      }   // if 3D

      else // should never happen
      {
        vtkLog(ERROR, "Unsupported cell type.");
      }
    } // for all cells in this batch
  }

  void Reduce() {}
};

// Fallback for other dataset types.
int DataSetExecute(vtkDataSet* input, const unsigned char* ghosts, unsigned char* bPoints,
  unsigned char* bCells, vtkIdType* bFaces)
{
  // Perform the threaded boundary marking.
  vtkIdType numCells = input->GetNumberOfCells();
  MarkDataSet mark(input, ghosts, bPoints, bCells, bFaces);
  vtkSMPTools::For(0, numCells, mark);

  return 1;
}

} // anonymous namespace

//----------------------------------------------------------------------------
int vtkMarkBoundaryFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // The output structure is the same as the input. Input point and cell data is
  // copied through as well.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Create the required output arrays indicating boundary points, cells,
  // mand optional faces.
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  vtkNew<vtkUnsignedCharArray> bPoints;
  bPoints->SetNumberOfTuples(numPts);
  bPoints->SetName(this->BoundaryPointsName);
  output->GetPointData()->AddArray(bPoints);

  vtkNew<vtkUnsignedCharArray> bCells;
  bCells->SetNumberOfTuples(numCells);
  bCells->SetName(this->BoundaryCellsName);
  output->GetCellData()->AddArray(bCells);

  vtkSmartPointer<vtkIdTypeArray> bFaces;
  if (this->GenerateBoundaryFaces)
  {
    bFaces.TakeReference(vtkIdTypeArray::New());
    bFaces->SetNumberOfTuples(numCells);
    bFaces->SetName(this->BoundaryFacesName);
    output->GetCellData()->AddArray(bFaces);
  }

  // Initially, nothing is marked on the boundary
  InitializeBoundaryArrays(bPoints, bCells, bFaces);
  unsigned char* bPtsPtr = bPoints->GetPointer(0);
  unsigned char* bCellsPtr = bCells->GetPointer(0);
  vtkIdType* bFacesPtr = (bFaces.Get() != nullptr ? bFaces->GetPointer(0) : nullptr);

  if (numCells == 0)
  {
    return 1;
  }

  // Grab ghost levels if needed
  unsigned char* cellGhosts = nullptr;
  vtkDataArray* temp = nullptr;
  temp = input->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName());
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  // Now visit different dataset types, marking those points, cells, and
  // optional faces on the boundary.
  int dataDim = 0;
  switch (input->GetDataObjectType())
  {
    case VTK_POLY_DATA:
    {
      return PolyDataExecute(input, cellGhosts, bPtsPtr, bCellsPtr, bFacesPtr);
    }

    case VTK_UNSTRUCTURED_GRID:
    case VTK_UNSTRUCTURED_GRID_BASE:
    {
      return UnstructuredGridExecute(input, cellGhosts, bPtsPtr, bCellsPtr, bFacesPtr);
    }

    // Structured dataset types
    case VTK_RECTILINEAR_GRID:
      dataDim = vtkRectilinearGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_STRUCTURED_GRID:
      dataDim = vtkStructuredGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_UNIFORM_GRID:
      dataDim = vtkUniformGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_STRUCTURED_POINTS:
      dataDim = vtkStructuredPoints::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_IMAGE_DATA:
      dataDim = vtkImageData::SafeDownCast(input)->GetDataDimension();
      break;
  }

  // Delegate to the faster structured processing if possible. It simplifies
  // things if we only consider 3D structured datasets. Otherwise the
  // general DataSetExecute will handle it just fine.
  if (dataDim == 3)
  {
    return StructuredExecute(input, cellGhosts, bPtsPtr, bCellsPtr, bFacesPtr);
  }

  // Use the general case for 1D/2D images, or for other dataset types
  return DataSetExecute(input, cellGhosts, bPtsPtr, bCellsPtr, bFacesPtr);
}
