/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPlaneClipper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataPlaneClipper.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinks.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkObjectFactoryNewMacro(vtkPolyDataPlaneClipper);

//------------------------------------------------------------------------------
// Here is the VTK class proper.
vtkPolyDataPlaneClipper::vtkPolyDataPlaneClipper()
{
  this->Plane = nullptr;
  this->ClippingLoops = true;
  this->Capping = true;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->BatchSize = 10000;

  this->SetNumberOfOutputPorts(2);
  vtkPolyData* output2 = vtkPolyData::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
}

//------------------------------------------------------------------------------
vtkPolyDataPlaneClipper::~vtkPolyDataPlaneClipper() = default;

//------------------------------------------------------------------------------
// Get the output dataset representing the capped loops.
vtkPolyData* vtkPolyDataPlaneClipper::GetCap()
{
  return this->GetOutput(1);
}

//------------------------------------------------------------------------------
// Specify the plane (an implicit function) used to clip the input vtkPolyData.
void vtkPolyDataPlaneClipper::SetPlane(vtkPlane* plane)
{
  if (plane != this->Plane)
  {
    this->Plane = plane;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtkPolyDataPlaneClipper::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Plane != nullptr)
  {
    vtkMTimeType mTime2 = this->Plane->GetMTime();
    return (mTime2 > mTime ? mTime2 : mTime);
  }
  else
  {
    return mTime;
  }
}

//------------------------------------------------------------------------------
// A high level overview of the algorithms is as follows. All steps but the
// loop construction and triangulation are performed in parallel. 1) Points
// are evaluated against the plane equation and marked as in/out the
// plane. 2) Since many input points are retained (i.e., kept) a point map is
// generated that maps input points to output points. 3) A traversal of the
// cells is made in order to configure and prepare for generating the
// output. 4) A second traversal of the cells is made, collecting kept
// edges and those clipped by the plane. 5) Duplicates clipped edges may be
// generated, so a sort of edges is required to collect duplicated clip edges
// into groups.  Each group is assigned a new point id (i.e., a new clip
// point). 6) The edges, when inserted, had an original edge id which is used
// to update the new clip edges (and associated point id) to the new point id
// (in the output connectivity array). 7) Since the clipped input polygons
// are convex, each clipped cell will have two clipped edges which form a
// line segment which is inserted into the second output if requested. 8)
// Clip edges are processed to produce the xyz coordinates of the clip points
// and inserted into the output points. 9) Point and cell attributes are
// generated and interpolated as necessary. 10) If capping is enabled, the
// line segments are joined into loops and triangulated and the triangulation
// is sent to the second output.

namespace // begin anonymous namespace
{

// Evaluate the plane equation for each input point. Develop a point map from
// the input points to output points.
template <typename TP>
struct EvaluatePoints
{
  TP* Points;
  double Origin[3];
  double Normal[3];
  vtkIdType* PtMap;
  vtkIdType NumberOfKeptPoints;

  EvaluatePoints(TP* pts, vtkPlane* plane)
    : Points(pts)
  {
    plane->GetOrigin(Origin);
    plane->GetNormal(Normal);
    vtkMath::Normalize(Normal);
    this->PtMap = new vtkIdType[pts->GetNumberOfTuples()];
    this->NumberOfKeptPoints = 0;
  }

  void Initialize() {}

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto pts = vtk::DataArrayTupleRange<3>(this->Points);
    double p[3], *n = this->Normal, *o = this->Origin;
    vtkIdType* map = this->PtMap + ptId;

    for (; ptId < endPtId; ++ptId)
    {
      auto pt = pts[ptId];
      p[0] = pt[0];
      p[1] = pt[1];
      p[2] = pt[2];

      // Outside points are marked with number <0.
      *map++ = (vtkPlane::Evaluate(n, o, p) > 0.0 ? 1 : -1);
    }
  }

  void Reduce()
  {
    // Prefix sum to create point map of kept (i.e., retained) points.
    vtkIdType numInputPts = this->Points->GetNumberOfTuples();
    vtkIdType npts = 0;
    for (auto id = 0; id < numInputPts; ++id)
    {
      if (this->PtMap[id] > 0)
      {
        this->PtMap[id] = npts++;
      }
    }

    this->NumberOfKeptPoints = npts;
  }
};

// Support point-type-based dispatching.
struct EvaluatePointsWorker
{
  vtkIdType* KeptPtMap;
  vtkIdType NumberOfKeptPoints;
  EvaluatePointsWorker()
    : KeptPtMap(nullptr)
  {
  }
  ~EvaluatePointsWorker() { delete[] this->KeptPtMap; }

  template <typename DataT>
  void operator()(DataT* pts, vtkPlane* plane)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();

    EvaluatePoints<DataT> ep(pts, plane);
    vtkSMPTools::For(0, numPts, ep);
    this->KeptPtMap = ep.PtMap;
    this->NumberOfKeptPoints = ep.NumberOfKeptPoints;
  }
};

// Keep track of output information within each batch of cells - this
// information is eventually rolled up into offsets into the cell
// connectivity and offsets arrays so that separate threads know where to
// write their data. We need to know how many total cells are created, the
// number of lines generated (which is equal to the number of clipped cells),
// and the connectivity size of the output cells and lines.
struct Batch
{
  // These are accumulated in EvaluateCells::operator().
  vtkIdType NumKeptCells;
  vtkIdType NumClippedCells;
  vtkIdType CellsConnSize;

  // These are assigned via prefix sum in EvaluateCells::Reduce(). This
  // information is used to instantiate the output cell arrays,
  vtkIdType CellsOffset;
  vtkIdType CellsConnOffset;
  vtkIdType LinesOffset;
  vtkIdType LinesConnOffset;

  Batch()
    : NumKeptCells(0)
    , NumClippedCells(0)
    , CellsConnSize(0)
    , CellsOffset(0)
    , CellsConnOffset(0)
    , LinesOffset(0)
    , LinesConnOffset(0)
  {
  }
};
struct vtkBatchInfo
{
  int BatchSize;
  int NumBatches;
  Batch* Batches;
  vtkBatchInfo()
    : Batches(nullptr)
  {
  }
  ~vtkBatchInfo() { delete[] this->Batches; }
};

// Compute the cases for convex cells. The case is one of either (0,N>0). If
// 0, then the entire cell is discarded. If N==npts (i.e., the number of
// input points equals the number of output points) then the entire cell is
// retained/kept. If (0<N<npts) then the cell is clipped and a convex cell of
// (N+2) pts is produced.
struct CellCases
{
  static vtkIdType ComputeCase(vtkIdType npts, const vtkIdType* cell, const vtkIdType* ptMap)
  {
    vtkIdType count = 0;
    for (auto i = 0; i < npts; ++i)
    {
      if (ptMap[cell[i]] >= 0)
        count++;
    }
    return count;
  }
};

// Gather information on the size of the output. Note that a cell map is
// created, it sets one of three values (-1,0,1). Zero means the entire cell
// is discarded; -1 means the cell is clipped; 1 means the entire cell is
// kept. Later on this cell map is transformed into a proper cell map which
// maps an input cell id to an output cell id.
struct EvaluateCells
{
  const vtkIdType* PtMap;
  vtkCellArray* Cells;
  vtkIdType NumCells;
  vtkBatchInfo BatchInfo;
  vtkIdType* CellMap;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkIdType NumberOfKeptCells;
  vtkIdType NumberOfClippedCells;
  vtkIdType CellsConnSize;

  EvaluateCells(vtkIdType* ptMap, vtkCellArray* cells, int batchSize)
    : PtMap(ptMap)
    , Cells(cells)
  {
    this->NumCells = this->Cells->GetNumberOfCells();
    this->BatchInfo.BatchSize = batchSize;
    this->BatchInfo.NumBatches = ((this->NumCells - 1) / batchSize) + 1;
    this->BatchInfo.Batches = new Batch[this->BatchInfo.NumBatches];
    this->CellMap = new vtkIdType[this->NumCells];
  }
  ~EvaluateCells() { delete[] this->CellMap; }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Cells->NewIterator()); }

  void operator()(vtkIdType batchId, vtkIdType endBatchId)
  {
    vtkIdType npts;
    const vtkIdType* cell;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();

    for (; batchId < endBatchId; ++batchId)
    {
      Batch* batch = this->BatchInfo.Batches + batchId;
      vtkIdType cellId = batchId * this->BatchInfo.BatchSize;
      vtkIdType endCellId =
        (cellId + this->BatchInfo.BatchSize > this->NumCells ? this->NumCells
                                                             : cellId + this->BatchInfo.BatchSize);
      vtkIdType* cellMap = this->CellMap + cellId;
      vtkIdType numKeptPts;

      for (; cellId < endCellId; ++cellId)
      {
        cellIter->GetCellAtId(cellId, npts, cell);
        numKeptPts = CellCases::ComputeCase(npts, cell, this->PtMap);
        if (numKeptPts == 0) // Cell discarded
        {
          *cellMap++ = 0;
        }
        else // An output cell is produced, either clipped or kept.
        {
          if (numKeptPts < npts) // Cell clipped by plane
          {
            *cellMap++ = -1; // cell clipped
            // The property of a convex cell is that two extra clipped points
            // will be generated.
            batch->CellsConnSize += (numKeptPts + 2);
            batch->NumClippedCells++;
          }
          else // Entire cell kept, no new clipped points will be generated.
          {
            *cellMap++ = 1; // cell kept
            batch->CellsConnSize += numKeptPts;
            batch->NumKeptCells++;
          }
        }
      } // for each cell in this batch
    }   // for each batch of cells
  }

  // Reduce() basically builds offsets and such so that the output can be
  // generated in the next pass.
  void Reduce()
  {
    this->NumberOfKeptCells = 0;
    this->NumberOfClippedCells = 0;
    this->CellsConnSize = 0;
    vtkIdType cellsConnSize = 0, cellsOffset = 0, linesOffset = 0;

    // Prefix sum over the batches to roll up total output.
    for (auto batchNum = 0; batchNum < this->BatchInfo.NumBatches; ++batchNum)
    {
      Batch* batch = this->BatchInfo.Batches + batchNum;
      batch->CellsOffset = cellsOffset;
      batch->CellsConnOffset = cellsConnSize;
      batch->LinesOffset = linesOffset;
      batch->LinesConnOffset = 2 * linesOffset;

      cellsOffset += batch->NumKeptCells + batch->NumClippedCells;
      cellsConnSize += batch->CellsConnSize;
      linesOffset += batch->NumClippedCells;

      this->NumberOfKeptCells += batch->NumKeptCells;
      this->NumberOfClippedCells += batch->NumClippedCells;
    }
    this->CellsConnSize = cellsConnSize;
  } // Reduce

  void Execute() { vtkSMPTools::For(0, this->BatchInfo.NumBatches, *this); }
};

// Represent clip edges. A clip edge has two values: (V0,V1) defining the
// edge, plus the edge data CIdx and LIdx, which are output locations in the cell
// and line connectivity arrays respectively. After sorting via MergeEdges(),
// these indices are used to update the cell and line connectivity arrays to
// use the newly generated point ids. The struct below is used in conjunction
// with vtkStaticEdgeLocatorTemplate to associate data with the edges.
struct IdxType
{
  vtkIdType CIdx;
  vtkIdType LIdx;
};
using EdgeTupleType = EdgeTuple<vtkIdType, IdxType>;
using EdgeLocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, IdxType>;

// Extract the cells (offsets and connectivity, plus clipped edges).
// Also copy cell data.
struct ExtractCells
{
  const vtkBatchInfo& BatchInfo;
  const vtkIdType* PtMap;
  vtkCellArray* Cells;
  vtkIdType NumCells;
  vtkIdType* CellMap;
  vtkIdType* CellConn;
  vtkIdType* CellOffsets;
  vtkIdType* LineConn;
  vtkIdType* LineOffsets;
  EdgeTupleType* Edges;
  ArrayList* Arrays;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  ExtractCells(const vtkBatchInfo& binfo, const vtkIdType* ptMap, vtkCellArray* cells,
    vtkIdType* cellMap, vtkIdTypeArray* cellConn, vtkIdTypeArray* cellOffsets,
    vtkIdTypeArray* lineConn, vtkIdTypeArray* lineOffsets, EdgeTupleType* e, ArrayList* arrays)
    : BatchInfo(binfo)
    , PtMap(ptMap)
    , Cells(cells)
    , CellMap(cellMap)
    , Edges(e)
    , Arrays(arrays)
  {
    this->NumCells = this->Cells->GetNumberOfCells();
    this->CellConn = cellConn->GetPointer(0);
    this->CellOffsets = cellOffsets->GetPointer(0);
    this->LineConn = lineConn->GetPointer(0);
    this->LineOffsets = lineOffsets->GetPointer(0);
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Cells->NewIterator()); }

  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    vtkIdType npts;
    const vtkIdType* cell;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    const vtkIdType* ptMap = this->PtMap;

    for (; batchNum < endBatchNum; ++batchNum)
    {
      Batch* batch = this->BatchInfo.Batches + batchNum;
      vtkIdType cellId = batchNum * this->BatchInfo.BatchSize;
      vtkIdType endCellId =
        (cellId + this->BatchInfo.BatchSize > this->NumCells ? this->NumCells
                                                             : cellId + this->BatchInfo.BatchSize);
      vtkIdType* cellMap = this->CellMap + cellId;
      vtkIdType cellConnIdx = batch->CellsConnOffset;
      vtkIdType* cellOffsets = this->CellOffsets + batch->CellsOffset;
      vtkIdType newCellId = batch->CellsOffset;
      vtkIdType cellOffset = batch->CellsConnOffset;
      vtkIdType lineConnIdx = batch->LinesConnOffset;
      vtkIdType* lineOffsets = this->LineOffsets + batch->LinesOffset;
      vtkIdType lineOffset = batch->LinesConnOffset;
      EdgeTupleType* edge = this->Edges + batch->LinesConnOffset;
      ArrayList* arrays = this->Arrays;

      for (; cellId < endCellId; ++cellId, ++cellMap)
      {
        if (*cellMap != 0) // if the cell is clipped or kept
        {
          cellIter->GetCellAtId(cellId, npts, cell);
          vtkIdType numEdgeClips = 0;
          vtkIdType numCellPts = 0;
          for (auto i = 0; i < npts; ++i) // loop over all cell points and edges
          {
            vtkIdType ptId = cell[i];
            vtkIdType nextId = cell[(i + 1) % npts];
            vtkIdType newPtId = ptMap[ptId];
            vtkIdType newNextId = ptMap[nextId];
            // Insert point if it is kept
            if (newPtId >= 0)
            {
              numCellPts++;
              this->CellConn[cellConnIdx++] = newPtId;
            }
            // Insert edge if it is clipped. (Clipping occurs on an edge when
            // one of the edge end points is kept, and the other is
            // discarded. We clamp the number of total number of edge
            // intersections in a cell to two, if more than two intersections
            // then the cell is concave. This clamping forces the cell to
            // behave as if it were convex and prevents crashes. Of course the
            // result is incorrect but the input to the filter specifies
            // convex cells.
            if (((newPtId >= 0 && newNextId < 0) || (newPtId < 0 && newNextId >= 0)) &&
              numEdgeClips < 2)
            {
              numEdgeClips++;
              numCellPts++;
              edge->Define(ptId, nextId);
              edge->Data.CIdx = cellConnIdx++;
              edge->Data.LIdx = lineConnIdx++;
              ++edge;
            } // if clipped edge
          }   // for all cell points and edges

          // Update the cell array offsets
          if (*cellMap < 0) // i.e. the cell has been clipped
          {
            *lineOffsets++ = lineOffset;
            lineOffset += 2;
          }
          *cellOffsets++ = cellOffset;
          cellOffset += numCellPts;
          *cellMap = newCellId;
          arrays->Copy(cellId, newCellId++);
        } // if cell is clipped or kept
      }   // for each cell in this batch
    }     // for each batch of cells
  }

  void Reduce() {}

  void Execute() { vtkSMPTools::For(0, this->BatchInfo.NumBatches, *this); }
};

// Write the points to the output. There are two parts to this: first copy
// retained input points to the output; then second, generate clip points and
// copy them to the output. Also copy / interpolate point data to the output.
struct OutputPointsWorker
{
  template <typename InPtsT, typename OutPtsT>
  void operator()(InPtsT* inPts, OutPtsT* outPts, vtkIdType* ptMap, vtkIdType numNewPts,
    const EdgeTupleType* mergeEdges, const vtkIdType* mergeOffsets, vtkPlane* plane,
    ArrayList* arrays)
  {
    vtkIdType numInPts = inPts->GetNumberOfTuples();
    vtkIdType numOutPts = outPts->GetNumberOfTuples();

    // Copy kept points to output.
    vtkSMPTools::For(
      0, numInPts, [&, inPts, outPts, ptMap, arrays](vtkIdType ptId, vtkIdType endPtId) {
        const auto in = vtk::DataArrayTupleRange<3>(inPts);
        auto out = vtk::DataArrayTupleRange<3>(outPts);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptMap[ptId] >= 0)
          {
            auto xin = in[ptId];
            auto xout = out[ptMap[ptId]];
            xout[0] = xin[0];
            xout[1] = xin[1];
            xout[2] = xin[2];
            arrays->Copy(ptId, ptMap[ptId]);
          }
        }
      }); // end lambda

    // Interpolate new points on clip edges. Since we are going to the trouble of
    // computing the t parametric coordinate along the edge, also interpolate the
    // point attributes at the same time.
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);
    vtkMath::Normalize(normal);
    vtkIdType numKeptPts = numOutPts - numNewPts;
    vtkSMPTools::For(0, numNewPts,
      [&, numKeptPts, outPts, mergeEdges, mergeOffsets, arrays](
        vtkIdType newPtId, vtkIdType endNewPtId) {
        const auto in = vtk::DataArrayTupleRange<3>(inPts);
        auto out = vtk::DataArrayTupleRange<3>(outPts);
        double x0[3], x1[3];

        for (; newPtId < endNewPtId; ++newPtId)
        {
          const EdgeTupleType* edge = mergeEdges + mergeOffsets[newPtId];
          const auto x0T = in[edge->V0];
          x0[0] = x0T[0];
          x0[1] = x0T[1];
          x0[2] = x0T[2];
          const auto x1T = in[edge->V1];
          x1[0] = x1T[0];
          x1[1] = x1T[1];
          x1[2] = x1T[2];
          double v0 = vtkPlane::Evaluate(normal, origin, x0);
          double v1 = vtkPlane::Evaluate(normal, origin, x1);
          double delta = (v1 - v0);
          double t = (delta == 0.0 ? 0.0 : (-v0 / delta));
          auto xout = out[newPtId + numKeptPts];
          xout[0] = x0[0] + t * (x1[0] - x0[0]);
          xout[1] = x0[1] + t * (x1[1] - x0[1]);
          xout[2] = x0[2] + t * (x1[2] - x0[2]);
          arrays->InterpolateEdge(edge->V0, edge->V1, t, newPtId + numKeptPts);
        }
      }); // end lambda
  }
};

// Update the cell connectivity
struct OutputCells
{
  vtkIdType NumKeptPts;
  vtkIdType NumNewPts;
  const EdgeTupleType* MergeEdges;
  const vtkIdType* MergeOffsets;
  vtkIdType* OutCellsConn;
  vtkIdType* OutLinesConn;

  OutputCells(vtkIdType numKeptPts, vtkIdType numNewPts, const EdgeTupleType* mergeEdges,
    const vtkIdType* mergeOffsets, vtkIdTypeArray* outCells, vtkIdTypeArray* outLines)
    : NumKeptPts(numKeptPts)
    , NumNewPts(numNewPts)
    , MergeEdges(mergeEdges)
    , MergeOffsets(mergeOffsets)
    , OutCellsConn(outCells->GetPointer(0))
    , OutLinesConn(outLines->GetPointer(0))
  {
  }

  void Execute()
  {
    vtkIdType numKeptPts = this->NumKeptPts;
    vtkIdType numNewPts = this->NumNewPts;
    const EdgeTupleType* edges = this->MergeEdges;
    const vtkIdType* offsets = this->MergeOffsets;
    vtkIdType* cellsConn = this->OutCellsConn;
    vtkIdType* linesConn = this->OutLinesConn;

    vtkSMPTools::For(0, numNewPts,
      [&, numKeptPts, edges, offsets, cellsConn, linesConn](
        vtkIdType newPtId, vtkIdType endNewPtId) {
        const EdgeTupleType* edge;

        for (; newPtId < endNewPtId; ++newPtId)
        {
          vtkIdType numEdges = offsets[newPtId + 1] - offsets[newPtId];
          vtkIdType updatedId = newPtId + numKeptPts;
          for (auto i = 0; i < numEdges; ++i)
          {
            edge = edges + offsets[newPtId] + i;
            cellsConn[edge->Data.CIdx] = updatedId;
            linesConn[edge->Data.LIdx] = updatedId;
          }
        }
      }); // end lambda
  }
};

// A fast helper class to access line connectivity. Each generated (clip)
// line is defined by two points.
void GetLine(vtkIdType lineId, vtkIdType* lineConn, vtkIdType& npts, const vtkIdType*& pts)
{
  npts = 2;
  pts = lineConn + 2 * lineId;
}

// Generate a cap for the loops. Create one or more loops, and then
// triangulate each closed loop.
void GenerateCap(vtkCellArray* lines, vtkPolyData* pd)
{
  // Make sure there are input lines
  vtkIdType numPts = pd->GetNumberOfPoints();
  vtkPoints* inPts = pd->GetPoints();
  vtkIdType numLines = lines->GetNumberOfCells();
  if (numPts < 1 || numLines < 3)
  {
    return; // must form a loop
  }

  // Prepare for processing
  vtkStaticCellLinksTemplate<vtkIdType> links;
  links.ThreadedBuildLinks(numPts, numLines, lines);
  vtkNew<vtkCellArray> polys;
  vtkNew<vtkPolygon> polygon;
  vtkIdType* lineConn =
    reinterpret_cast<vtkIdTypeArray*>(lines->GetConnectivityArray())->GetPointer(0);

  // Keep track of what lines are visited. This is needed to
  // form potentially multiple loops.
  char* visited = new char[numLines];
  std::fill_n(visited, numLines, 0);

  // Run around loop until returning to the beginning
  vtkIdType lineId, npts, startPt, currentPt, nextPt;
  vtkIdType numLoopPts, currentLine;
  const vtkIdType* pts;
  vtkNew<vtkIdList> outTris;

  // Run across all lines, seeking those that have not been visited. An
  // unvisited line is part of a new loop.
  vtkIdType totTris = 0;
  vtkNew<vtkIdTypeArray> outConn; // collect the output triangles
  vtkNew<vtkIdTypeArray> outOffsets;
  for (lineId = 0; lineId < numLines; ++lineId)
  {
    if (!visited[lineId]) // start next loop
    {
      currentLine = lineId;
      visited[lineId] = 1;
      numLoopPts = 0;
      GetLine(lineId, lineConn, npts, pts);
      startPt = currentPt = pts[0];
      nextPt = pts[1];

      // Traverse loop
      bool closed = true;
      polygon->PointIds->Reset();
      polygon->Points->Reset();
      while (closed && nextPt != startPt)
      {
        // By inserting the points at the beginning of this while() loop, the
        // last point is not inserted, which is what we want for a polygon (i.e.,
        // the first and last point should not be duplicated).
        polygon->PointIds->InsertId(numLoopPts, currentPt);
        polygon->Points->InsertPoint(numLoopPts++, inPts->GetPoint(currentPt));
        if (links.GetNumberOfCells(nextPt) < 2)
        {
          closed = false;
          break;
        }
        vtkIdType* cells = links.GetCells(nextPt);
        currentLine = (cells[0] != currentLine ? cells[0] : cells[1]);
        visited[currentLine] = 1;
        GetLine(currentLine, lineConn, npts, pts);
        currentPt = nextPt;
        nextPt = (pts[0] != nextPt ? pts[0] : pts[1]);
      }

      // If the last loop is closed, triangulate the polygon and then feed
      // the resulting triangles into the filter output.
      if (closed)
      {
        // The vtkPolygon triangulation creates the connectivity array. It
        // is necessary to also create the offsets array.
        if (polygon->Triangulate(outTris))
        {
          vtkIdType* ids = polygon->PointIds->GetPointer(0);
          vtkIdType numTris = outTris->GetNumberOfIds() / 3;
          vtkIdType* outTrisPtr = outTris->GetPointer(0);

          outConn->WritePointer(0, 3 * (numTris + totTris));
          outOffsets->WritePointer(0, totTris + numTris + 1);
          vtkIdType* outConnPtr = outConn->GetPointer(0);
          vtkIdType* outOffsetsPtr = outOffsets->GetPointer(0);

          vtkSMPTools::For(0, numTris,
            [&, totTris, ids, outTrisPtr, outConnPtr, outOffsetsPtr](
              vtkIdType triId, vtkIdType endTriId) {
              for (; triId < endTriId; ++triId)
              {
                vtkIdType tID = triId + totTris;
                vtkIdType* triIn = outTrisPtr + 3 * triId;
                vtkIdType* triOut = outConnPtr + 3 * tID;
                triOut[0] = ids[triIn[0]];
                triOut[1] = ids[triIn[1]];
                triOut[2] = ids[triIn[2]];
                outOffsetsPtr[tID] = 3 * tID;
              }
            });
          totTris += numTris;
        } // successful triangulation
      }   // if closed loop
    }     // if not visited
  }

  // If some triangles were produced, send them to the output
  if (totTris > 0)
  {
    outOffsets->SetComponent(totTris, 0, 3 * totTris);
    polys->SetData(outOffsets, outConn);
    pd->SetPolys(polys);
  }

  // Clean up
  delete[] visited;
}

} // anonymous namespace

//------------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
int vtkPolyDataPlaneClipper::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing vtkPolyData plane clipper");

  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* outInfo2 = outputVector->GetInformationObject(1);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output2 = vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure there is input
  vtkIdType numPts, numCells;
  vtkCellArray* cells = input->GetPolys();
  numCells = cells->GetNumberOfCells();
  if ((numPts = input->GetNumberOfPoints()) < 1 || (numCells = input->GetNumberOfCells()) < 1 ||
    this->Plane == nullptr)
  {
    return 1;
  }

  // Evaluate the plane equation across all points.
  vtkPoints* inPts = input->GetPoints();
  using EvaluatePointsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  EvaluatePointsWorker epWorker;
  if (!EvaluatePointsDispatch::Execute(inPts->GetData(), epWorker, this->Plane))
  {
    epWorker(inPts->GetData(), this->Plane);
  }
  vtkIdType numKeptPts = epWorker.NumberOfKeptPoints;

  // Return quickly in two special cases: 1) when all points are discarded;
  // 2) when all points are kept.
  if (epWorker.NumberOfKeptPoints == 0)
  { // return empty
    return 1;
  }
  else if (numKeptPts == numPts)
  { // return input
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  // Now process the convex cells to determine the size of the output. We
  // are going to process cells in batches, and to avoid thread local
  // storage and to facilitate threading, keep track of information regarding
  // the size of the output. This requires multiple passes to first determine
  // what's output, and then to actually create the output.
  EvaluateCells ec(epWorker.KeptPtMap, cells, this->BatchSize);
  ec.Execute();
  vtkBatchInfo& batchInfo = ec.BatchInfo;
  vtkIdType numOutCells = ec.NumberOfKeptCells + ec.NumberOfClippedCells;

  // Build the cell arrays for the output cells and lines. This means
  // creating connectivity and offset arrays. Clipe edges are merged because there
  // are duplicates, and they are used to generate new points.
  EdgeLocatorType edgeLocator;
  vtkIdType numEdges = ec.NumberOfClippedCells;
  EdgeTupleType* mergeEdges = new EdgeTupleType[2 * numEdges];
  vtkNew<vtkIdTypeArray> cellConn;
  cellConn->SetNumberOfTuples(ec.CellsConnSize);
  vtkNew<vtkIdTypeArray> cellOffsets;
  cellOffsets->SetNumberOfTuples(numOutCells + 1);
  vtkNew<vtkIdTypeArray> lineConn;
  lineConn->SetNumberOfTuples(2 * numEdges);
  vtkNew<vtkIdTypeArray> lineOffsets;
  lineOffsets->SetNumberOfTuples(numEdges + 1);

  ArrayList cellArrays;
  output->GetCellData()->InterpolateAllocate(input->GetCellData(), numOutCells);
  cellArrays.AddArrays(numOutCells, input->GetCellData(), output->GetCellData());

  ExtractCells ext(batchInfo, ec.PtMap, cells, ec.CellMap, cellConn, cellOffsets, lineConn,
    lineOffsets, mergeEdges, &cellArrays);
  ext.Execute();
  cellOffsets->SetComponent(numOutCells, 0, ec.CellsConnSize);
  lineOffsets->SetComponent(ec.NumberOfClippedCells, 0, 2 * numEdges);

  // New points are generated from groups of duplicate edges. The groups are
  // formed via sorting.
  vtkIdType numNewPts;
  const vtkIdType* mergeOffsets = edgeLocator.MergeEdges(2 * numEdges, mergeEdges, numNewPts);

  // By merging edges, we've identified the new clip points (i.e., each set of
  // duplicate edges generates one new clip point). At this point we need to
  // update the cell and line connectivity arrays to their new point ids.
  vtkNew<vtkCellArray> outCells;
  vtkNew<vtkCellArray> outLines;
  OutputCells oc(numKeptPts, numNewPts, mergeEdges, mergeOffsets, cellConn, lineConn);
  oc.Execute();
  outCells->SetData(cellOffsets, cellConn);
  outLines->SetData(lineOffsets, lineConn);

  // Now output the points. There is a combination of kept points from
  // the input, plus new points generated from the clipping operation.
  vtkIdType numOutPts = numKeptPts + numNewPts;
  vtkNew<vtkPoints> outPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  outPts->SetNumberOfPoints(numOutPts);
  output->SetPoints(outPts);
  if (this->ClippingLoops || this->Capping)
  {
    output2->SetPoints(outPts);
  }

  // Prepare to copy / interpolate point data
  ArrayList ptArrays;
  output->GetPointData()->InterpolateAllocate(input->GetPointData(), numOutPts);
  ptArrays.AddArrays(numOutPts, input->GetPointData(), output->GetPointData());

  // Generate new points, a combination of kept points and interpolated points.
  using OutputPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  OutputPointsWorker opWorker;
  if (!OutputPointsDispatch::Execute(inPts->GetData(), outPts->GetData(), opWorker,
        epWorker.KeptPtMap, numNewPts, mergeEdges, mergeOffsets, this->Plane, &ptArrays))
  {
    opWorker(inPts->GetData(), outPts->GetData(), epWorker.KeptPtMap, numNewPts, mergeEdges,
      mergeOffsets, this->Plane, &ptArrays);
  }
  delete[] mergeEdges;

  // Now output the cells and optionally the clip lines. The cells
  // are a combination of the kept cells, plus the new convex cells due
  // to clipping.
  output->SetPolys(outCells);

  // If clip loops are requested, send to the output.
  if (this->ClippingLoops)
  {
    output2->SetLines(outLines);
  }

  // Finally, if capping is enabled, then a triangulation of the clipping loops
  // is required.
  if (this->Capping)
  {
    GenerateCap(outLines, output2);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyDataPlaneClipper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Batch Size: " << this->BatchSize << "\n";
}
