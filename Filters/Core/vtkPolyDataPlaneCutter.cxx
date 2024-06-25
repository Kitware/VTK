// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataPlaneCutter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkBatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
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
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkPolyDataPlaneCutter);
VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
// A high level overview of the algorithms is as follows. All steps are
// performed in parallel. 1) Points are evaluated against the plane equation
// and marked as in/out the plane. 2) A traversal of the cells is made in
// order to configure and prepare for generating the output. 3) A second
// traversal of the cells is made, producing line segments. 4) The end points
// of the line segments are determined by plane / cell edge intersection.
// These end points are collected into a edge locator, and sorted to identify
// duplicate points. 5) Output point ids are assigned to the end points, and
// used to update the line connectivity array. 6) Cut edges are processed to
// produce the xyz coordinates of the cut/line end points and inserted into the
// array of output points. 7) Point and cell attributes are generated and
// interpolated as necessary.

namespace // begin anonymous namespace
{

// Evaluate the plane equation for each input point. Mark points as to whether
// they are above or below the plane.
template <typename TP>
struct EvaluatePoints
{
  TP* Points;                        // Input points to the filter
  double Origin[3];                  // Plane origin
  double Normal[3];                  // Plane normal
  std::vector<unsigned char>& PtMap; // 0/1 values indicating below and above plane
  vtkPolyDataPlaneCutter* Filter;
  bool Intersects;

  // These are used to determine whether the plane
  // intersects the polydata - enables a quick cull of the input.
  vtkSMPThreadLocal<unsigned char> BelowPlane;
  vtkSMPThreadLocal<unsigned char> AbovePlane;

  EvaluatePoints(
    TP* pts, vtkPlane* plane, std::vector<unsigned char>& ptMap, vtkPolyDataPlaneCutter* filter)
    : Points(pts)
    , PtMap(ptMap)
    , Filter(filter)
  {
    plane->GetOrigin(Origin);
    plane->GetNormal(Normal);
    vtkMath::Normalize(Normal);
  }

  void Initialize()
  {
    this->BelowPlane.Local() = 0;
    this->AbovePlane.Local() = 0;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto pts = vtk::DataArrayTupleRange<3>(this->Points);
    double p[3], *n = this->Normal, *o = this->Origin;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId)
    {
      if (ptId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }

      auto pt = pts[ptId];
      p[0] = pt[0];
      p[1] = pt[1];
      p[2] = pt[2];

      // Points above the plane are marked 1; 0 otherwise
      auto val = vtkPlane::Evaluate(n, o, p);
      if (val > 0.0)
      {
        this->PtMap[ptId] = 1;
        this->AbovePlane.Local() = 1;
      }
      else
      {
        this->PtMap[ptId] = 0;
        this->BelowPlane.Local() = 1;
      }
    }
  }

  // Determine if there are intersections with any cell.
  void Reduce()
  {
    bool belowPlane = false, abovePlane = false;
    this->Intersects = false;

    for (auto bItr = this->BelowPlane.begin(); bItr != this->BelowPlane.end(); ++bItr)
    {
      if (*bItr > 0)
      {
        belowPlane = true;
      }
    }
    for (auto aItr = this->AbovePlane.begin(); aItr != this->AbovePlane.end(); ++aItr)
    {
      if (*aItr > 0)
      {
        abovePlane = true;
      }
    }

    this->Intersects = (belowPlane && abovePlane ? true : false);
  }
}; // EvaluatePoints

// Support point-type-based dispatching.
struct EvaluatePointsWorker
{
  bool Intersects;

  template <typename DataT>
  void operator()(
    DataT* pts, vtkPlane* plane, std::vector<unsigned char>& ptMap, vtkPolyDataPlaneCutter* filter)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();
    EvaluatePoints<DataT> ep(pts, plane, ptMap, filter);
    vtkSMPTools::For(0, numPts, ep);
    this->Intersects = ep.Intersects;
  }
}; // EvaluatePointsWorker

// Determine whether a cell is cut by the plane. This requires at least one
// point above the plane, and at least one point below the plane.
bool CellIntersectsPlane(
  vtkIdType npts, const vtkIdType* cell, const std::vector<unsigned char>& ptMap)
{
  bool belowPlane = false, abovePlane = false;
  for (auto i = 0; i < npts; ++i)
  {
    unsigned char val = ptMap[cell[i]];
    if (val > 0)
    {
      abovePlane = true;
    }
    else
    {
      belowPlane = true;
    }
  }

  return (belowPlane && abovePlane);
} // CellIntersectsPlane

struct PolyCutterBatchData
{
  vtkIdType LinesOffset;

  PolyCutterBatchData()
    : LinesOffset(0)
  {
  }
  ~PolyCutterBatchData() = default;
  PolyCutterBatchData& operator+=(const PolyCutterBatchData& other)
  {
    this->LinesOffset += other.LinesOffset;
    return *this;
  }
  PolyCutterBatchData operator+(const PolyCutterBatchData& other) const
  {
    PolyCutterBatchData result = *this;
    result += other;
    return result;
  }
};
using PolyCutterBatch = vtkBatch<PolyCutterBatchData>;
using PolyCutterBatches = vtkBatches<PolyCutterBatchData>;

// Gather information on the size of the output. Basically, count the
// number of line segments created in each batch. Then roll up these
// counts to create offsets which are later used to generate the output
// lines and points.
struct EvaluateCells
{
  const std::vector<unsigned char>& PtMap;
  vtkCellArray* Cells;
  vtkIdType NumCells;
  vtkPolyDataPlaneCutter* Filter;
  PolyCutterBatches Batches;
  std::vector<unsigned char> CellMap;
  vtkIdType NumLines;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  EvaluateCells(const std::vector<unsigned char>& ptMap, vtkCellArray* cells, int batchSize,
    vtkPolyDataPlaneCutter* filter)
    : PtMap(ptMap)
    , Cells(cells)
    , Filter(filter)
  {
    this->NumCells = this->Cells->GetNumberOfCells();
    this->Batches.Initialize(this->NumCells, batchSize);
    this->CellMap.resize(this->NumCells);
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Cells->NewIterator()); }

  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    vtkIdType npts;
    const vtkIdType* cell;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();

    // Over batches of cells
    for (; batchNum < endBatchNum; ++batchNum)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      auto& batch = this->Batches[batchNum];
      auto& batchNumberOfLines = batch.Data.LinesOffset;
      vtkIdType numLines = 0;

      // For all cells making up this batch
      for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        cellIter->GetCellAtId(cellId, npts, cell);

        // If cell is intersected, count and mark
        if (CellIntersectsPlane(npts, cell, this->PtMap))
        {
          numLines++;
          this->CellMap[cellId] = 1;
        }
        else
        {
          this->CellMap[cellId] = 0;
        }
      } // for each cell in this batch

      // Keep track of the number of cell intersections for each batch. A
      // subsequent prefix sum will produce offsets from this information.
      batchNumberOfLines = numLines;
    } // for each batch of cells
  }

  // Reduce() basically builds offsets and such so that the output can be
  // generated in the next pass.
  void Reduce()
  {
    // trim batches that have no intersections
    this->Batches.TrimBatches(
      [](const PolyCutterBatch& batch) { return batch.Data.LinesOffset == 0; });

    // Prefix sum over the batches to roll up total output.
    const auto globalSum = this->Batches.BuildOffsetsAndGetGlobalSum();
    this->NumLines = globalSum.LinesOffset;
  } // Reduce

  void Execute() { vtkSMPTools::For(0, this->Batches.GetNumberOfBatches(), *this); }
}; // EvaluateCells

// Represent cut edges. A cut edge has two values: (V0,V1) defining the
// edge, plus the edge data LIdx, which is the output location (an index)
// into the line connectivity array. After sorting via MergeEdges(), this
// index is used to update line connectivity arrays to use the newly
// generated point ids. The struct below is used in conjunction with
// vtkStaticEdgeLocatorTemplate to associate data with the edges.
struct IdxType
{
  vtkIdType LIdx;
};
using EdgeTupleType = EdgeTuple<vtkIdType, IdxType>;
using EdgeLocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, IdxType>;

// Extract the lines. Also copy cell data.
struct ExtractLines
{
  const EvaluateCells& EC;
  const std::vector<unsigned char>& PtMap;
  vtkCellArray* Cells;
  vtkIdType NumCells;
  const std::vector<unsigned char>& CellMap;
  vtkIdType* LineConn;
  vtkIdType* LineOffsets;
  std::vector<EdgeTupleType>& Edges;
  ArrayList* Arrays;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkPolyDataPlaneCutter* Filter;

  ExtractLines(EvaluateCells& ec, const std::vector<unsigned char>& ptMap, vtkCellArray* cells,
    std::vector<unsigned char>& cellMap, vtkIdTypeArray* lineConn, vtkIdTypeArray* lineOffsets,
    std::vector<EdgeTupleType>& e, ArrayList* arrays, vtkPolyDataPlaneCutter* filter)
    : EC(ec)
    , PtMap(ptMap)
    , Cells(cells)
    , CellMap(cellMap)
    , Edges(e)
    , Arrays(arrays)
    , Filter(filter)
  {
    this->NumCells = this->Cells->GetNumberOfCells();
    this->LineConn = lineConn->GetPointer(0);
    this->LineOffsets = lineOffsets->GetPointer(0);
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Cells->NewIterator()); }

  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    vtkIdType npts;
    const vtkIdType* cell;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    const std::vector<unsigned char>& ptMap = this->PtMap;
    const std::vector<unsigned char>& cellMap = this->CellMap;
    std::vector<EdgeTupleType>& edges = this->Edges;
    ArrayList* arrays = this->Arrays;
    bool isFirst = vtkSMPTools::GetSingleThread();

    // For each batch, process the intersected cells in the batch.
    for (; batchNum < endBatchNum; ++batchNum)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      auto& batch = this->EC.Batches[batchNum];

      // What's going on here is the line offsets are being updated, and
      // the merged edges are being created with indices into the line
      // connectivity array. Later, after edge sorting and when the final
      // point ids are defined, the edge connectivity array is updated with
      // the output point ids.
      vtkIdType lineNum = batch.Data.LinesOffset;
      vtkIdType lineConnIdx = 2 * lineNum;
      vtkIdType* lineOffsets = this->LineOffsets + lineNum;
      vtkIdType lineOffset = 2 * lineNum;

      // For all cells in this batch
      for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        if (cellMap[cellId] != 0) // if the cell is cut
        {
          cellIter->GetCellAtId(cellId, npts, cell);
          vtkIdType numEdgeCuts = 0;
          for (auto i = 0; i < npts && numEdgeCuts < 2; ++i) // loop over all cell points and edges
          {
            vtkIdType ptId = cell[i];
            vtkIdType nextId = cell[(i + 1) % npts];

            // If the edge points are on either side of the plane, then create
            // a cut point. Make sure that there are no more that two edges cut
            // (as a result of the convex requirement on the cell).
            if (ptMap[ptId] != ptMap[nextId])
            {
              EdgeTupleType& edge = edges[2 * lineNum + numEdgeCuts++];
              edge.Define(ptId, nextId);
              edge.Data.LIdx = lineConnIdx++;
            } // if edge cut

          } // over all cell edges, with no more than 2 cuts
          *lineOffsets++ = lineOffset;
          lineOffset += 2;
          if (arrays) // generate cell data if requested
          {
            arrays->Copy(cellId, lineNum);
          }
          lineNum++;
        } // if cell is cut
      }   // for each cell in this batch
    }     // for each batch of cells
  }

  void Reduce() {}

  void Execute() { vtkSMPTools::For(0, this->EC.Batches.GetNumberOfBatches(), *this); }
}; // ExtractLines

// Update the line connectivity with new point ids.
struct OutputLines
{
  vtkIdType NumNewPts;
  const EdgeTupleType* MergeEdges;
  const vtkIdType* MergeOffsets;
  vtkIdType* OutLinesConn;
  vtkPolyDataPlaneCutter* Filter;

  OutputLines(vtkIdType numNewPts, const EdgeTupleType* mergeEdges, const vtkIdType* mergeOffsets,
    vtkIdTypeArray* outLines, vtkPolyDataPlaneCutter* filter)
    : NumNewPts(numNewPts)
    , MergeEdges(mergeEdges)
    , MergeOffsets(mergeOffsets)
    , OutLinesConn(outLines->GetPointer(0))
    , Filter(filter)
  {
  }

  void Execute()
  {
    vtkIdType numNewPts = this->NumNewPts;
    const EdgeTupleType* edges = this->MergeEdges;
    const vtkIdType* offsets = this->MergeOffsets;
    vtkIdType* linesConn = this->OutLinesConn;

    vtkSMPTools::For(
      0, numNewPts, [&, edges, offsets, linesConn](vtkIdType newPtId, vtkIdType endNewPtId) {
        const EdgeTupleType* edge;
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endNewPtId - newPtId) / 10 + 1, (vtkIdType)1000);

        for (; newPtId < endNewPtId; ++newPtId)
        {
          if (newPtId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              this->Filter->CheckAbort();
            }
            if (this->Filter->GetAbortOutput())
            {
              break;
            }
          }

          vtkIdType numEdges = offsets[newPtId + 1] - offsets[newPtId];
          vtkIdType updatedId = newPtId;
          for (auto i = 0; i < numEdges; ++i)
          {
            edge = edges + offsets[newPtId] + i;
            linesConn[edge->Data.LIdx] = updatedId;
          }
        }
      }); // end lambda
  }
}; // OutputLines

// Interpolate and write the points to the output. Also copy / interpolate
// point data to the filter output.
struct OutputPointsWorker
{
  template <typename InPtsT, typename OutPtsT>
  void operator()(InPtsT* inPts, OutPtsT* outPts, vtkIdType numNewPts,
    const EdgeTupleType* mergeEdges, const vtkIdType* mergeOffsets, vtkPlane* plane,
    ArrayList* arrays, vtkPolyDataPlaneCutter* filter)
  {
    // Interpolate new points on cut edges. Since we are going to the trouble of
    // computing the t parametric coordinate along the edge, also interpolate the
    // point attributes at the same time.
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);
    vtkMath::Normalize(normal);
    vtkSMPTools::For(0, numNewPts,
      [&, outPts, mergeEdges, mergeOffsets, arrays](vtkIdType newPtId, vtkIdType endNewPtId) {
        const auto in = vtk::DataArrayTupleRange<3>(inPts);
        auto out = vtk::DataArrayTupleRange<3>(outPts);
        double x0[3], x1[3];
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endNewPtId - newPtId) / 10 + 1, (vtkIdType)1000);

        for (; newPtId < endNewPtId; ++newPtId)
        {
          if (newPtId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
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
          auto xout = out[newPtId];
          xout[0] = x0[0] + t * (x1[0] - x0[0]);
          xout[1] = x0[1] + t * (x1[1] - x0[1]);
          xout[2] = x0[2] + t * (x1[2] - x0[2]);
          if (arrays) // if interpolate attributes
          {
            arrays->InterpolateEdge(edge->V0, edge->V1, t, newPtId);
          }
        }
      }); // end lambda
  }
}; // OutputPointsWorker

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//==============================================================================
//------------------------------------------------------------------------------
// Here is the VTK class proper.
vtkPolyDataPlaneCutter::vtkPolyDataPlaneCutter()
{
  this->Plane = nullptr;
  this->ComputeNormals = false;
  this->InterpolateAttributes = true;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->BatchSize = 10000;
}

//------------------------------------------------------------------------------
vtkPolyDataPlaneCutter::~vtkPolyDataPlaneCutter() = default;

//------------------------------------------------------------------------------
// Specify the plane (an implicit function) used to cut the input vtkPolyData.
void vtkPolyDataPlaneCutter::SetPlane(vtkPlane* plane)
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
vtkMTimeType vtkPolyDataPlaneCutter::GetMTime()
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
// This method drives the various threaded functors to implement the
// plane cutting algorithm.
int vtkPolyDataPlaneCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
  std::vector<unsigned char> ptMap(numPts);
  if (!EvaluatePointsDispatch::Execute(inPts->GetData(), epWorker, this->Plane, ptMap, this))
  {
    epWorker(inPts->GetData(), this->Plane, ptMap, this);
  }

  // Return quickly when no cells are cut.
  if (!epWorker.Intersects)
  { // return empty
    return 1;
  }

  // Now process the convex cells to determine the size of the output (i.e.,
  // the number of output line segments). We are going to process cells in
  // batches, and to avoid the use of thread local storage and to facilitate
  // threading, keep track of information regarding the size of the output.
  EvaluateCells evalCells(ptMap, cells, this->BatchSize, this);
  evalCells.Execute();

  // Build the cell array for the output lines. Also generate cut edges (and
  // associated intersection points), which are edges cut by the plane, which
  // are eventually merged. This avoids using the relatively slow point locator.
  vtkIdType numLines = evalCells.NumLines;
  std::vector<EdgeTupleType> mergeEdges(2 * numLines);
  vtkNew<vtkIdTypeArray> lineConn;
  lineConn->SetNumberOfTuples(2 * numLines);
  vtkNew<vtkIdTypeArray> lineOffsets;
  lineOffsets->SetNumberOfTuples(numLines + 1);

  // If requested, each line segment has cell data copied from the
  // intersected cell.
  ArrayList cellArrays;
  if (this->InterpolateAttributes)
  {
    output->GetCellData()->InterpolateAllocate(input->GetCellData(), numLines);
    cellArrays.AddArrays(
      numLines, input->GetCellData(), output->GetCellData(), /*nullValue*/ 0.0, /*promote*/ false);
  }

  // Extract the line segments.
  ExtractLines extLines(evalCells, ptMap, cells, evalCells.CellMap, lineConn, lineOffsets,
    mergeEdges, (this->InterpolateAttributes ? &cellArrays : nullptr), this);
  extLines.Execute();
  lineOffsets->SetComponent(numLines, 0, 2 * numLines);

  // New points are generated from groups of duplicate edges. The groups are
  // formed via sorting. The number of points in a group represents the number
  // of duplicate points on that edge.
  vtkIdType numOutPts;
  EdgeLocatorType edgeLocator;
  const vtkIdType* mergeOffsets =
    edgeLocator.MergeEdges(2 * numLines, mergeEdges.data(), numOutPts);

  // By merging edges into groups, we've identified the new cut points (i.e.,
  // each group of duplicate edges generates one new cut point). Now update
  // the line connectivity array with new point ids.
  vtkNew<vtkCellArray> outLines;
  OutputLines ol(numOutPts, mergeEdges.data(), mergeOffsets, lineConn, this);
  ol.Execute();
  outLines->SetData(lineOffsets, lineConn);

  // Now output the cut lines.
  output->SetLines(outLines);

  // Create and initialize the generated/interpolated cut points.
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

  // Prepare to copy / interpolate point data (if requested).
  ArrayList ptArrays;
  if (this->InterpolateAttributes)
  {
    output->GetPointData()->InterpolateAllocate(input->GetPointData(), numOutPts);
    ptArrays.AddArrays(numOutPts, input->GetPointData(), output->GetPointData(), /*nullValue*/ 0.0,
      /*promote*/ false);
  }

  // Generate the new points coordinates, and interpolate point data.
  using OutputPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  OutputPointsWorker opWorker;
  if (!OutputPointsDispatch::Execute(inPts->GetData(), outPts->GetData(), opWorker, numOutPts,
        mergeEdges.data(), mergeOffsets, this->Plane,
        (this->InterpolateAttributes ? &ptArrays : nullptr), this))
  {
    opWorker(inPts->GetData(), outPts->GetData(), numOutPts, mergeEdges.data(), mergeOffsets,
      this->Plane, (this->InterpolateAttributes ? &ptArrays : nullptr), this);
  }

  // If normals requested, then create an array of point normals.
  if (this->ComputeNormals)
  {
    vtkNew<vtkFloatArray> normals; // don't really need a lot of precision here
    normals->SetNumberOfComponents(3);
    normals->SetName("Normals");
    normals->SetNumberOfTuples(numOutPts);
    double planeNormal[3];
    this->Plane->GetNormal(planeNormal);
    vtkSMPTools::For(0, numOutPts, [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType i = begin; i < end; ++i)
      {
        normals->SetTuple(i, planeNormal);
      }
    });
    output->GetPointData()->AddArray(normals);
  }

  return 1;
}
VTK_ABI_NAMESPACE_END

// Support convexity check on input
namespace // begin anonymous namespace
{

struct CheckConvex
{
  vtkPoints* Points;
  vtkCellArray* Polys;
  vtkIdType NumPolys;
  unsigned char IsConvex; // final, reduced result

  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> PolyIterator;
  vtkSMPThreadLocal<unsigned char> isConvex; // per thread result

  CheckConvex(vtkPoints* pts, vtkCellArray* ca)
    : Points(pts)
    , Polys(ca)
    , IsConvex(1)
  {
    this->NumPolys = ca->GetNumberOfCells();
  }

  void Initialize()
  {
    this->PolyIterator.Local().TakeReference(this->Polys->NewIterator());
    this->isConvex.Local() = 1;
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    vtkCellArrayIterator* polyIter = this->PolyIterator.Local();
    vtkPoints* p = this->Points;

    for (; cellId < endCellId && this->isConvex.Local(); ++cellId)
    {
      polyIter->GetCellAtId(cellId, npts, pts);
      if (!vtkPolygon::IsConvex(p, npts, pts))
      {
        this->isConvex.Local() = 0;
      }
    }
  }

  void Reduce()
  {
    this->IsConvex = 1;
    for (auto cItr = this->isConvex.begin(); cItr != this->isConvex.end(); ++cItr)
    {
      if (!*cItr)
      {
        this->IsConvex = 0;
      }
    }
  }

  void Execute() { vtkSMPTools::For(0, this->NumPolys, *this); }

}; // CheckConvex

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Assess whether the specified data object can be processed by this filter. The
// input data must be of type vtkPolyData, and contain only convex polygons.
bool vtkPolyDataPlaneCutter::CanFullyProcessDataObject(vtkDataObject* object)
{
  // Perform some quick checks
  auto pdata = vtkPolyData::SafeDownCast(object);
  if (!pdata || pdata->GetVerts()->GetNumberOfCells() > 0 ||
    pdata->GetLines()->GetNumberOfCells() > 0 || pdata->GetStrips()->GetNumberOfCells() > 0)
  {
    return false;
  }

  // If this is all triangles, then they are convex. This check is a bit of a
  // hack, since we are looking for a connectivity array of size 3*numCells -
  // there are cases where the data is degenerate when this might not hold
  // (e.g., the polygons are lines and/or points, mixed together with quads
  // etc). Of course in a degenerate case the cutting process will likely
  // fail no matter what plane cutter is used.
  vtkIdType numCells = pdata->GetPolys()->GetNumberOfCells();
  vtkIdType numConnIds = pdata->GetPolys()->GetNumberOfConnectivityIds();
  if (numConnIds == (3 * numCells))
  {
    return true;
  }

  // Okay, need to process cell-by-cell to determine if they are convex.
  CheckConvex checkConvex(pdata->GetPoints(), pdata->GetPolys());
  checkConvex.Execute();
  if (checkConvex.IsConvex)
  {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkPolyDataPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Batch Size: " << this->BatchSize << "\n";
}
VTK_ABI_NAMESPACE_END
