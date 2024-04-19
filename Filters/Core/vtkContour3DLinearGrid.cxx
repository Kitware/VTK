// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContour3DLinearGrid.h"

#include "vtk3DLinearGridInternal.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourValues.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkSpanSpace.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <utility> //make_pair

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContour3DLinearGrid);
vtkCxxSetObjectMacro(vtkContour3DLinearGrid, ScalarTree, vtkScalarTree);

//------------------------------------------------------------------------------
// Classes to support threaded execution. Note that there are different
// strategies implemented here: 1) a fast path that just produces output
// triangles and points, and 2) more general approach that supports point
// merging, field interpolation, and/or normal generation. There is also some
// cell-related machinery supporting faster contouring. Finally, a scalar
// tree can be used to accelerate repeated contouring.

// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq, _num, _op)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op(0, _num);                                                                                \
    }                                                                                              \
  } while (false)

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op.Initialize();                                                                            \
      _op(0, _num);                                                                                \
      _op.Reduce();                                                                                \
    }                                                                                              \
    _nt = _op.NumThreadsUsed;                                                                      \
  } while (false)

namespace
{

//========================= FAST PATH =========================================
// Perform the contouring operation without merging coincident points. There is
// a fast path with and without a scalar tree.
template <typename TInputPointsArray, typename TOutputPointsArray, typename TScalarsArray>
struct ContourCellsBase
{
  using LocalPointsTypeArray =
    typename std::conditional<std::is_same<TOutputPointsArray, vtkDataArray>::value, vtkDoubleArray,
      TOutputPointsArray>::type;
  using TOutputPointsType = typename LocalPointsTypeArray::ValueType;
  using LocalPointsType = std::vector<TOutputPointsType>;
  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread into a
  // single vtkPolyData output.
  struct LocalDataType
  {
    LocalPointsType LocalPts;
    CellIter LocalCellIter;
    LocalDataType() { this->LocalPts.reserve(2048); }
  };

  vtkContour3DLinearGrid* Filter;
  TInputPointsArray* InPts;
  TOutputPointsArray* NewPts;
  TScalarsArray* Scalars;
  CellIter* Iter;
  double Value;
  vtkCellArray* NewPolys;

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  // Related to the compositing Reduce() method
  vtkIdType NumPts;
  vtkIdType NumTris;
  int NumThreadsUsed;
  vtkIdType TotalPts;  // the total points thus far (support multiple contours)
  vtkIdType TotalTris; // the total triangles thus far (support multiple contours)

  ContourCellsBase(vtkContour3DLinearGrid* filter, TInputPointsArray* inPts,
    TOutputPointsArray* outPts, TScalarsArray* scalars, CellIter* iter, double value,
    vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris)
    : Filter(filter)
    , InPts(inPts)
    , NewPts(outPts)
    , Scalars(scalars)
    , Iter(iter)
    , Value(value)
    , NewPolys(tris)
    , NumPts(0)
    , NumTris(0)
    , NumThreadsUsed(0)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
  {
  }
  virtual ~ContourCellsBase() = default;

  // Set up the iteration process.
  virtual void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() method implemented by subclasses (with and without scalar tree)

  // Produce points for non-merged points. This is basically a parallel copy
  // into the final VTK points array.
  struct ProducePoints
  {
    const std::vector<LocalPointsType*>& LocalPts;
    const std::vector<vtkIdType>& PtOffsets;
    TOutputPointsArray* OutPts;

    ProducePoints(const std::vector<LocalPointsType*>& lp, const std::vector<vtkIdType>& o,
      TOutputPointsArray* outPts)
      : LocalPts(lp)
      , PtOffsets(o)
      , OutPts(outPts)
    {
    }
    void operator()(vtkIdType threadId, vtkIdType endThreadId)
    {
      auto outputPoints = vtk::DataArrayTupleRange<3>(this->OutPts);
      for (; threadId < endThreadId; ++threadId)
      {
        vtkIdType ptOffset = this->PtOffsets[threadId];
        auto threadPointsCoords = *(this->LocalPts[threadId]);
        const auto numberOfCoords = static_cast<vtkIdType>(threadPointsCoords.size());
        for (vtkIdType i = 0; i < numberOfCoords;)
        {
          auto outputPoint = outputPoints[ptOffset++];
          outputPoint[0] = threadPointsCoords[i++];
          outputPoint[1] = threadPointsCoords[i++];
          outputPoint[2] = threadPointsCoords[i++];
        }
      }
    }
  };

  // Functor to build the VTK triangle list in parallel from the generated
  // points. In the fast path there are three points for every triangle. Many
  // points are typically duplicates but point merging is a significant cost
  // so is ignored in the fast path.
  struct ProduceTriangles
  {
    struct Impl
    {
      template <typename CellStateT>
      void operator()(CellStateT& state, const vtkIdType triBegin, const vtkIdType triEnd,
        const vtkIdType totalTris)
      {
        using ValueType = typename CellStateT::ValueType;
        auto* offsets = state.GetOffsets();
        auto* connectivity = state.GetConnectivity();

        const vtkIdType offsetsBegin = totalTris + triBegin;
        const vtkIdType offsetsEnd = totalTris + triEnd + 1;
        ValueType offset = static_cast<ValueType>(3 * (totalTris + triBegin - 1));
        auto offsetsRange = vtk::DataArrayValueRange<1>(offsets, offsetsBegin, offsetsEnd);
        std::generate(
          offsetsRange.begin(), offsetsRange.end(), [&]() -> ValueType { return offset += 3; });

        const vtkIdType connBegin = 3 * offsetsBegin;
        const vtkIdType connEnd = 3 * (offsetsEnd - 1);
        const ValueType startPtId = static_cast<ValueType>(3 * (totalTris + triBegin));

        auto connRange = vtk::DataArrayValueRange<1>(connectivity, connBegin, connEnd);
        std::iota(connRange.begin(), connRange.end(), startPtId);
      }
    };

    vtkIdType TotalTris;
    vtkCellArray* Tris;
    ProduceTriangles(vtkIdType totalTris, vtkCellArray* tris)
      : TotalTris(totalTris)
      , Tris(tris)
    {
    }
    void operator()(vtkIdType triId, vtkIdType endTriId)
    {
      this->Tris->Visit(Impl{}, triId, endTriId, this->TotalTris);
    }
  };

  // Composite results from each thread
  virtual void Reduce()
  {
    // Count the number of points. For fun keep track of the number of
    // threads used. Also keep track of the thread id so they can
    // be processed in parallel later (copy points in ProducePoints).
    vtkIdType numPts = 0;
    this->NumThreadsUsed = 0;
    std::vector<LocalPointsType*> localPts;
    std::vector<vtkIdType> localPtOffsets;
    for (auto& localData : this->LocalData)
    {
      localPts.push_back(&localData.LocalPts);
      localPtOffsets.push_back((this->TotalPts + numPts));
      numPts += static_cast<vtkIdType>(localData.LocalPts.size() / 3);
      this->NumThreadsUsed++;
    }

    // (Re)Allocate space for output. Multiple contours require writing into
    // the end of the arrays.
    this->NumPts = numPts;
    this->NumTris = numPts / 3;
    this->NewPts->WriteVoidPointer(0, 3 * (this->NumPts + this->TotalPts));
    this->NewPolys->ResizeExact(
      this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy points output to VTK structures. Only point coordinates are
    // copied for now; later we'll define the triangle topology.
    ProducePoints producePts(localPts, localPtOffsets, this->NewPts);
    EXECUTE_SMPFOR(this->Filter->GetSequentialProcessing(), this->NumThreadsUsed, producePts);

    // Now produce the output triangles (topology) for this contour n parallel
    ProduceTriangles produceTris(this->TotalTris, this->NewPolys);
    EXECUTE_SMPFOR(this->Filter->GetSequentialProcessing(), this->NumTris, produceTris);
  } // Reduce
};  // ContourCellsBase

// Fast path operator() without scalar tree
template <typename TInputPointsArray, typename TOutputPointsArray, typename TScalarsArray>
struct ContourCells : public ContourCellsBase<TInputPointsArray, TOutputPointsArray, TScalarsArray>
{
  using TContourCellsBase = ContourCellsBase<TInputPointsArray, TOutputPointsArray, TScalarsArray>;

  ContourCells(vtkContour3DLinearGrid* filter, TInputPointsArray* inPts, TOutputPointsArray* outPts,
    TScalarsArray* scalars, CellIter* iter, double value, vtkCellArray* tris, vtkIdType totalPts,
    vtkIdType totalTris)
    : TContourCellsBase(filter, inPts, outPts, scalars, iter, value, tris, totalPts, totalTris)
  {
  }
  ~ContourCells() override = default;

  // Set up the iteration process.
  void Initialize() override { this->TContourCellsBase::Initialize(); }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lPts = localData.LocalPts;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId);
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    bool isFirst = vtkSMPTools::GetSingleThread();

    auto inPts = vtk::DataArrayTupleRange<3>(this->InPts);
    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(scalars[c[i]]);
        isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
      }
      edges = cellIter->GetCase(isoCase);

      if (*edges > 0)
      {
        numEdges = *edges++;
        for (i = 0; i < numEdges; ++i, edges += 2)
        {
          v0 = edges[0];
          v1 = edges[1];
          const auto x0 = inPts[c[v0]];
          const auto x1 = inPts[c[v1]];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          lPts.emplace_back(x0[0] + t * (x1[0] - x0[0]));
          lPts.emplace_back(x0[1] + t * (x1[1] - x0[1]));
          lPts.emplace_back(x0[2] + t * (x1[2] - x0[2]));
        }                   // for all edges in this case
      }                     // if contour passes through this cell
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite results from each thread
  void Reduce() override { this->TContourCellsBase::Reduce(); } // Reduce
};                                                              // ContourCells

// Fast path operator() with a scalar tree
template <typename TInputPointsArray, typename TOutputPointsArray, typename TScalarsArray>
struct ContourCellsST
  : public ContourCellsBase<TInputPointsArray, TOutputPointsArray, TScalarsArray>
{
  using TContourCellsBase = ContourCellsBase<TInputPointsArray, TOutputPointsArray, TScalarsArray>;

  vtkScalarTree* ScalarTree;
  vtkIdType NumBatches;

  ContourCellsST(vtkContour3DLinearGrid* filter, TInputPointsArray* inPts,
    TOutputPointsArray* outPts, TScalarsArray* scalars, CellIter* iter, double value,
    vtkScalarTree* st, vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris)
    : TContourCellsBase(filter, inPts, outPts, scalars, iter, value, tris, totalPts, totalTris)
    , ScalarTree(st)
  {
    //    this->ScalarTree->BuildTree();
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }
  ~ContourCellsST() override = default;

  // Set up the iteration process.
  void Initialize() override { this->TContourCellsBase::Initialize(); }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle). Uses a scalar tree to accelerate operations.
  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    auto& localData = this->LocalData.Local();
    auto& lPts = localData.LocalPts;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c;
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const vtkIdType* cellIds;
    vtkIdType idx, numCells;
    bool isFirst = vtkSMPTools::GetSingleThread();

    auto inPts = vtk::DataArrayTupleRange<3>(this->InPts);
    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endBatchNum - batchNum) / 10 + 1, (vtkIdType)1000);

    for (; batchNum < endBatchNum; ++batchNum)
    {
      if (batchNum % checkAbortInterval == 0)
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
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(scalars[c[i]]);
          isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
        }
        edges = cellIter->GetCase(isoCase);

        if (*edges > 0)
        {
          numEdges = *edges++;
          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            v0 = edges[0];
            v1 = edges[1];
            const auto x0 = inPts[c[v0]];
            const auto x1 = inPts[c[v1]];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            lPts.emplace_back(x0[0] + t * (x1[0] - x0[0]));
            lPts.emplace_back(x0[1] + t * (x1[1] - x0[1]));
            lPts.emplace_back(x0[2] + t * (x1[2] - x0[2]));
          } // for all edges in this case
        }   // if contour passes through this cell
      }     // for all cells in this batch
    }       // for each batch
  }

  // Composite results from each thread
  void Reduce() override { this->TContourCellsBase::Reduce(); } // Reduce
};                                                              // ContourCellsST

// Dispatch worker for Fast path processing. Handles template dispatching etc.
struct ProcessFastPathWorker
{
  template <typename TInputPointsArray, typename TOutputPointsArray, typename TScalarsArray>
  void operator()(TInputPointsArray* inPts, TOutputPointsArray* outPts, TScalarsArray* scalars,
    vtkContour3DLinearGrid* filter, vtkIdType numCells, CellIter* cellIter, double isoValue,
    vtkScalarTree* st, vtkCellArray* tris, int& numThreads, vtkIdType totalPts, vtkIdType totalTris)
  {
    if (st != nullptr)
    {
      using TContourCellsST = ContourCellsST<TInputPointsArray, TOutputPointsArray, TScalarsArray>;
      TContourCellsST contour(
        filter, inPts, outPts, scalars, cellIter, isoValue, st, tris, totalPts, totalTris);
      EXECUTE_REDUCED_SMPFOR(
        filter->GetSequentialProcessing(), contour.NumBatches, contour, numThreads);
    }
    else
    {
      using TContourCells = ContourCells<TInputPointsArray, TOutputPointsArray, TScalarsArray>;
      TContourCells contour(
        filter, inPts, outPts, scalars, cellIter, isoValue, tris, totalPts, totalTris);
      EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(), numCells, contour, numThreads);
    }
  }
};

//========================= GENERAL PATH (POINT MERGING) =======================
// Use vtkStaticEdgeLocatorTemplate for edge-based point merging. Processing is
// available with and without a scalar tree.
template <typename IDType>
struct EdgeDataType
{
  float T;
  IDType EId;
};

template <typename IDType, typename TScalarsArray>
struct ExtractEdgesBase
{
  using EdgeVectorType = std::vector<EdgeTuple<IDType, float>>;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    EdgeVectorType LocalEdges;
    std::vector<IDType> OriginalCellIds;
    CellIter LocalCellIter;
    LocalDataType()
    {
      this->LocalEdges.reserve(2048);
      this->OriginalCellIds.reserve(2048 / 3);
    }
  };

  vtkContour3DLinearGrid* Filter;
  TScalarsArray* Scalars;
  CellIter* Iter;
  double Value;
  vtkCellArray* Tris;
  vtkIdType TotalTris; // the total triangles thus far (support multiple contours)
  std::vector<IDType>& OriginalCellIds;

  // Keep track of generated points and triangles on a per-thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;
  int NumThreadsUsed;
  vtkIdType NumTris;
  EdgeTuple<IDType, EdgeDataType<IDType>>* Edges;

  ExtractEdgesBase(vtkContour3DLinearGrid* filter, TScalarsArray* scalars, CellIter* iter,
    double value, vtkCellArray* tris, vtkIdType totalTris, std::vector<IDType>& originalCellIds)
    : Filter(filter)
    , Scalars(scalars)
    , Iter(iter)
    , Value(value)
    , Tris(tris)
    , TotalTris(totalTris)
    , OriginalCellIds(originalCellIds)
    , NumThreadsUsed(0)
    , NumTris(0)
    , Edges(nullptr)
  {
  }
  virtual ~ExtractEdgesBase() = default;

  // Set up the iteration process
  virtual void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() provided by subclass

  // Produce edges for merged points. This is basically a parallel composition
  // into the final edges array.
  template <typename IDT>
  struct ProduceEdges
  {
    const std::vector<EdgeVectorType*>& LocalEdges;
    const std::vector<vtkIdType>& TriOffsets;
    EdgeTuple<IDT, EdgeDataType<IDT>>* OutEdges;
    vtkContour3DLinearGrid* Filter;
    ProduceEdges(const std::vector<EdgeVectorType*>& le, const std::vector<vtkIdType>& o,
      EdgeTuple<IDT, EdgeDataType<IDT>>* outEdges, vtkContour3DLinearGrid* filter)
      : LocalEdges(le)
      , TriOffsets(o)
      , OutEdges(outEdges)
      , Filter(filter)
    {
    }
    void operator()(vtkIdType threadId, vtkIdType endThreadId)
    {
      vtkIdType triOffset, edgeNum;
      const EdgeVectorType* lEdges;
      EdgeTuple<IDT, EdgeDataType<IDT>>* edges;
      bool isFirst = vtkSMPTools::GetSingleThread();
      vtkIdType checkAbortInterval = std::min((endThreadId - threadId) / 10 + 1, (vtkIdType)1000);

      for (; threadId < endThreadId; ++threadId)
      {
        if (threadId % checkAbortInterval == 0)
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
        triOffset = this->TriOffsets[threadId];
        edgeNum = 3 * triOffset;
        edges = this->OutEdges + edgeNum;
        lEdges = this->LocalEdges[threadId];
        for (auto& edge : *lEdges)
        {
          edges->V0 = edge.V0;
          edges->V1 = edge.V1;
          edges->Data.T = edge.Data;
          edges->Data.EId = edgeNum;
          edges++;
          edgeNum++;
        }
      } // for all threads
    }
  };

  // Composite local thread data
  virtual void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    vtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    std::vector<EdgeVectorType*> localEdges;
    std::vector<vtkIdType> localTriOffsets;
    for (auto& localData : this->LocalData)
    {
      localEdges.push_back(&localData.LocalEdges);
      localTriOffsets.push_back(numTris);
      numTris +=
        static_cast<vtkIdType>(localData.LocalEdges.size() / 3); // three edges per triangle
      this->NumThreadsUsed++;
    }
    this->OriginalCellIds.reserve(static_cast<size_t>(numTris));
    for (auto& localData : this->LocalData)
    {
      this->OriginalCellIds.insert(this->OriginalCellIds.end(), localData.OriginalCellIds.begin(),
        localData.OriginalCellIds.end());
    }

    // Allocate space for VTK triangle output. Take into account previous
    // contours.
    this->NumTris = numTris;
    this->Tris->ResizeExact(this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy local edges to composited edge array.
    this->Edges =
      new EdgeTuple<IDType, EdgeDataType<IDType>>[3 * this->NumTris]; // three edges per triangle
    ProduceEdges<IDType> produceEdges(localEdges, localTriOffsets, this->Edges, this->Filter);
    EXECUTE_SMPFOR(this->Filter->GetSequentialProcessing(), this->NumThreadsUsed, produceEdges);
  } // Reduce
};  // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without scalar tree).
template <typename IDType, typename TScalarsArray>
struct ExtractEdges : public ExtractEdgesBase<IDType, TScalarsArray>
{
  using TExtractEdgesBase = ExtractEdgesBase<IDType, TScalarsArray>;

  ExtractEdges(vtkContour3DLinearGrid* filter, TScalarsArray* scalars, CellIter* iter, double value,
    vtkCellArray* tris, vtkIdType totalTris, std::vector<IDType>& originalCellIds)
    : TExtractEdgesBase(filter, scalars, iter, value, tris, totalTris, originalCellIds)
  {
  }
  ~ExtractEdges() override = default;

  // Set up the iteration process
  void Initialize() override { this->TExtractEdgesBase::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    auto& lOriginalCellIds = localData.OriginalCellIds;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    bool isFirst = vtkSMPTools::GetSingleThread();
    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(scalars[c[i]]);
        isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
      }
      edges = cellIter->GetCase(isoCase);

      if (*edges > 0)
      {
        numEdges = *edges++;
        const int numberOfProducedTriangles = numEdges / 3;
        for (i = 0; i < numberOfProducedTriangles; ++i)
        {
          lOriginalCellIds.push_back(static_cast<IDType>(cellId));
        }
        for (i = 0; i < numEdges; ++i, edges += 2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
          lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
        }                                       // for all edges in this case
      }                                         // if contour passes through this cell
      c = cellIter->Next();                     // move to the next cell
    }                                           // for all cells in this batch
  }

  // Composite local thread data
  void Reduce() override { this->TExtractEdgesBase::Reduce(); } // Reduce
};                                                              // ExtractEdges

// Generate edges using a scalar tree.
template <typename IDType, typename TScalarsArray>
struct ExtractEdgesST : public ExtractEdgesBase<IDType, TScalarsArray>
{
  using TExtractEdgesBase = ExtractEdgesBase<IDType, TScalarsArray>;

  vtkScalarTree* ScalarTree;
  vtkIdType NumBatches;

  ExtractEdgesST(vtkContour3DLinearGrid* filter, TScalarsArray* scalars, CellIter* iter,
    double value, vtkScalarTree* st, vtkCellArray* tris, vtkIdType totalTris,
    std::vector<IDType>& originalCellIds)
    : TExtractEdgesBase(filter, scalars, iter, value, tris, totalTris, originalCellIds)
    , ScalarTree(st)
  {
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }
  ~ExtractEdgesST() override = default;

  // Set up the iteration process
  void Initialize() override { this->TExtractEdgesBase::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    auto& lOriginalCellIds = localData.OriginalCellIds;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c;
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const vtkIdType* cellIds;
    vtkIdType idx, numCells, cellId;
    bool isFirst = vtkSMPTools::GetSingleThread();

    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endBatchNum - batchNum) / 10 + 1, (vtkIdType)1000);

    for (; batchNum < endBatchNum; ++batchNum)
    {
      if (batchNum % checkAbortInterval == 0)
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
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        cellId = cellIds[idx];
        c = cellIter->GetCellIds(cellId);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(scalars[c[i]]);
          isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
        }
        edges = cellIter->GetCase(isoCase);

        if (*edges > 0)
        {
          numEdges = *edges++;
          const int numberOfProducedTriangles = numEdges / 3;
          for (i = 0; i < numberOfProducedTriangles; ++i)
          {
            lOriginalCellIds.push_back(static_cast<IDType>(cellId));
          }
          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
            lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
          }                                       // for all edges in this case
        }                                         // if contour passes through this cell
      }                                           // for all cells in this batch
    }                                             // for all batches
  }

  // Composite local thread data
  void Reduce() override { this->TExtractEdgesBase::Reduce(); } // Reduce

}; // ExtractEdgesST

// Dispatch worker for Extract Edges. Handles template dispatching etc.
template <typename TIds>
struct ExtractEdgesWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* scalars, vtkContour3DLinearGrid* filter, vtkIdType numCells,
    CellIter* cellIter, double isoValue, vtkScalarTree* st, vtkCellArray* newPolys,
    vtkIdType totalTris, vtkIdType& numTris, EdgeTuple<TIds, EdgeDataType<TIds>>*& mergeEdges,
    std::vector<TIds>& originalCellIds, int& numThreads)
  {
    if (st != nullptr)
    {
      using TExtractEdgesST = ExtractEdgesST<TIds, TScalarArray>;
      TExtractEdgesST extractEdges(
        filter, scalars, cellIter, isoValue, st, newPolys, totalTris, originalCellIds);
      EXECUTE_REDUCED_SMPFOR(
        filter->GetSequentialProcessing(), extractEdges.NumBatches, extractEdges, numThreads);
      numTris = extractEdges.NumTris;
      mergeEdges = extractEdges.Edges;
    }
    else
    {
      using TExtractEdges = ExtractEdges<TIds, TScalarArray>;
      TExtractEdges extractEdges(
        filter, scalars, cellIter, isoValue, newPolys, totalTris, originalCellIds);
      EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(), numCells, extractEdges, numThreads);
      numTris = extractEdges.NumTris;
      mergeEdges = extractEdges.Edges;
    }
  }
};

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProduceMergedTriangles
{
  typedef EdgeTuple<IDType, EdgeDataType<IDType>> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  vtkIdType NumTris;
  vtkCellArray* Tris;
  vtkIdType TotalPts;
  vtkIdType TotalTris;
  int NumThreadsUsed; // placeholder
  vtkContour3DLinearGrid* Filter;

  ProduceMergedTriangles(const MergeTupleType* merge, const IDType* offsets, vtkIdType numTris,
    vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris, vtkContour3DLinearGrid* filter)
    : MergeArray(merge)
    , Offsets(offsets)
    , NumTris(numTris)
    , Tris(tris)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
    , NumThreadsUsed(1)
    , Filter(filter)
  {
  }

  void Initialize()
  {
    // without this method Reduce() is not called
  }

  struct Impl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, vtkIdType ptId, const vtkIdType endPtId,
      const vtkIdType ptOffset, const vtkIdType connOffset, const IDType* offsets,
      const MergeTupleType* mergeArray, vtkContour3DLinearGrid* filter)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* conn = state.GetConnectivity();
      bool isFirst = vtkSMPTools::GetSingleThread();
      vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

      for (; ptId < endPtId; ++ptId)
      {
        if (ptId % checkAbortInterval == 0)
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
        const IDType numPtsInGroup = offsets[ptId + 1] - offsets[ptId];
        for (IDType i = 0; i < numPtsInGroup; ++i)
        {
          const IDType connIdx = mergeArray[offsets[ptId] + i].Data.EId + connOffset;
          conn->SetValue(connIdx, static_cast<ValueType>(ptId + ptOffset));
        } // for this group of coincident edges
      }   // for all merged points
    }
  };

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    this->Tris->Visit(Impl{}, ptId, endPtId, this->TotalPts, 3 * this->TotalTris, this->Offsets,
      this->MergeArray, this->Filter);
  }

  struct ReduceImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, const vtkIdType totalTris, const vtkIdType nTris)
    {
      using ValueType = typename CellStateT::ValueType;

      auto offsets =
        vtk::DataArrayValueRange<1>(state.GetOffsets(), totalTris, totalTris + nTris + 1);
      ValueType offset = 3 * (totalTris - 1); // +=3 on first access
      std::generate(offsets.begin(), offsets.end(), [&]() -> ValueType { return offset += 3; });
    }
  };

  // Update the triangle connectivity (numPts for each triangle. This could
  // be done in parallel but it's probably not faster.
  void Reduce() { this->Tris->Visit(ReduceImpl{}, this->TotalTris, this->NumTris); }
};

// This method generates the output isosurface points. One point per
// merged edge is generated.
template <typename TInputPointsArray, typename TOutputPointsArray, typename IDType>
struct ProduceMergedPoints
{
  using MergeTupleType = EdgeTuple<IDType, EdgeDataType<IDType>>;

  vtkContour3DLinearGrid* Filter;
  TInputPointsArray* InPts;
  TOutputPointsArray* OutPts;
  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  const vtkIdType TotalPrevPoints;
  const vtkIdType TotalOutputPoints;

  ProduceMergedPoints(vtkContour3DLinearGrid* filter, TInputPointsArray* inPts,
    TOutputPointsArray* outPts, const MergeTupleType* merge, const IDType* offsets,
    vtkIdType totalPts)
    : Filter(filter)
    , InPts(inPts)
    , OutPts(outPts)
    , MergeArray(merge)
    , Offsets(offsets)
    , TotalPrevPoints(totalPts)
    , TotalOutputPoints(this->OutPts->GetNumberOfTuples())
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTupleType* mergeTuple;
    IDType v0, v1;
    float t;
    bool isFirst = vtkSMPTools::GetSingleThread();
    auto inPoints = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPoints =
      vtk::DataArrayTupleRange<3>(this->OutPts, this->TotalPrevPoints, this->TotalOutputPoints);
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
      mergeTuple = this->MergeArray + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->Data.T;
      const auto x0 = inPoints[v0];
      const auto x1 = inPoints[v1];
      auto x = outPoints[ptId];
      x[0] = x0[0] + t * (x1[0] - x0[0]);
      x[1] = x0[1] + t * (x1[1] - x0[1]);
      x[2] = x0[2] + t * (x1[2] - x0[2]);
    }
  }
};

template <typename TIds>
struct ProduceMergedPointsWorker
{
  template <typename TInputPointsArray, typename TOutputPointsArray>
  void operator()(TInputPointsArray* inputPointsArray, TOutputPointsArray* outputPointsArray,
    vtkContour3DLinearGrid* filter, const EdgeTuple<TIds, EdgeDataType<TIds>>* mergeArray,
    const TIds* offsets, vtkIdType totalPoints, vtkIdType numPts)
  {
    ProduceMergedPoints<TInputPointsArray, TOutputPointsArray, TIds> produceMergedPoints(
      filter, inputPointsArray, outputPointsArray, mergeArray, offsets, totalPoints);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, produceMergedPoints);
  }
};

// If requested, interpolate point data attributes. The merge tuple contains an
// interpolation value t for the merged edge.
template <typename TIds>
struct ProducePointAttributes
{
  const EdgeTuple<TIds, EdgeDataType<TIds>>* Edges; // all edges, sorted into groups of merged edges
  const TIds* Offsets;                              // refer to single, unique, merged edge
  ArrayList* Arrays;                                // carry list of attributes to interpolate
  vtkIdType TotalPts; // total points / multiple contours computed previously
  vtkContour3DLinearGrid* Filter;

  ProducePointAttributes(const EdgeTuple<TIds, EdgeDataType<TIds>>* mt, const TIds* offsets,
    ArrayList* arrays, vtkIdType totalPts, vtkContour3DLinearGrid* filter)
    : Edges(mt)
    , Offsets(offsets)
    , Arrays(arrays)
    , TotalPts(totalPts)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const EdgeTuple<TIds, EdgeDataType<TIds>>* mergeTuple;
    TIds v0, v1;
    float t;
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
      mergeTuple = this->Edges + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->Data.T;
      this->Arrays->InterpolateEdge(v0, v1, t, ptId + this->TotalPts);
    }
  }
};

// If requested, interpolate cell data attributes.
template <typename TIds>
struct ProduceCellAttributes
{
  const std::vector<TIds>& OriginalCellIds; // original cell ids
  ArrayList* Arrays;                        // carry list of attributes to interpolate
  vtkIdType TotalTris; // total triangles / multiple contours computed previously
  vtkContour3DLinearGrid* Filter;

  ProduceCellAttributes(const std::vector<TIds>& originalCellIds, ArrayList* arrays,
    vtkIdType totalTris, vtkContour3DLinearGrid* filter)
    : OriginalCellIds(originalCellIds)
    , Arrays(arrays)
    , TotalTris(totalTris)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();

    vtkIdType checkAbortInterval = std::min((endCellId - beginCellId) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      this->Arrays->Copy(this->OriginalCellIds[cellId], cellId + this->TotalTris);
    }
  }
};

// Wrapper to handle multiple template types for merged processing
template <typename TIds>
int ProcessMerged(vtkContour3DLinearGrid* filter, vtkPoints* inPts, vtkPoints* outPts,
  vtkDataArray* inScalars, vtkIdType numCells, CellIter* cellIter, double isoValue,
  vtkScalarTree* st, vtkCellArray* newPolys, vtkTypeBool intAttr, vtkTypeBool computeScalars,
  vtkPointData* inPD, vtkPointData* outPD, ArrayList* pointArrays, vtkCellData* inCD,
  vtkCellData* outCD, ArrayList* cellArrays, int& numThreads, vtkIdType totalPts,
  vtkIdType totalTris)
{
  // Extract edges that the contour intersects. Templated on type of scalars.
  // List below the explicit choice of scalars that can be processed.
  vtkIdType numTris = 0;
  EdgeTuple<TIds, EdgeDataType<TIds>>* mergeEdges = nullptr; // may need reference counting
  std::vector<TIds> originalCellIds;
  ExtractEdgesWorker<TIds> extractEdgesWorker;
  // process these scalar types, others could easily be added
  using ScalarsList = vtkTypeList::Create<unsigned int, int, float, double>;
  using DispatcherExtractEdges = vtkArrayDispatch::DispatchByValueType<ScalarsList>;
  if (!DispatcherExtractEdges::Execute(inScalars, extractEdgesWorker, filter, numCells, cellIter,
        isoValue, st, newPolys, totalTris, numTris, mergeEdges, originalCellIds, numThreads))
  {
    extractEdgesWorker(inScalars, filter, numCells, cellIter, isoValue, st, newPolys, totalTris,
      numTris, mergeEdges, originalCellIds, numThreads);
  }
  int nt = numThreads;

  // Make sure data was produced
  if (numTris <= 0)
  {
    delete[] mergeEdges;
    return 1;
  }

  // Merge coincident edges. The Offsets refer to the single unique edge
  // from the sorted group of duplicate edges.
  vtkIdType numPts;
  vtkStaticEdgeLocatorTemplate<TIds, EdgeDataType<TIds>> loc;
  const TIds* offsets = loc.MergeEdges(3 * numTris, mergeEdges, numPts);

  // Generate triangles.
  ProduceMergedTriangles<TIds> produceTris(
    mergeEdges, offsets, numTris, newPolys, totalPts, totalTris, filter);
  EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(), numPts, produceTris, numThreads);
  numThreads = nt;

  // Generate points (one per unique edge)
  outPts->GetData()->WriteVoidPointer(0, 3 * (numPts + totalPts));
  ProduceMergedPointsWorker<TIds> produceMergedPointsWorker;

  using DispatcherProducePoints =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!DispatcherProducePoints::Execute(inPts->GetData(), outPts->GetData(),
        produceMergedPointsWorker, filter, mergeEdges, offsets, totalPts, numPts))
  {
    produceMergedPointsWorker(
      inPts->GetData(), outPts->GetData(), filter, mergeEdges, offsets, totalPts, numPts);
  }

  // Now process point data attributes if requested
  if (intAttr)
  {
    // interpolate point data
    if (totalPts <= 0) // first contour value generating output
    {
      outPD->InterpolateAllocate(inPD, numPts);
      if (!computeScalars)
      {
        pointArrays->ExcludeArray(inScalars);
      }
      pointArrays->AddArrays(numPts, inPD, outPD, 0.0, /*promote=*/false);
      if (!computeScalars)
      {
        outPD->RemoveArray(inScalars->GetName());
      }
    }
    else
    {
      pointArrays->Realloc(totalPts + numPts);
    }
    ProducePointAttributes<TIds> interpolate(mergeEdges, offsets, pointArrays, totalPts, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, interpolate);

    // interpolate cell data
    if (totalTris <= 0) // first contour value generating output
    {
      outCD->CopyAllocate(inCD, numTris);
      cellArrays->AddArrays(numTris, inCD, outCD, 0.0, /*promote=*/false);
    }
    else
    {
      cellArrays->Realloc(totalTris + numTris);
    }
    ProduceCellAttributes<TIds> interpolateCell(originalCellIds, cellArrays, totalTris, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numTris, interpolateCell);
  }

  // Clean up
  delete[] mergeEdges;
  return 1;
}

// Functor for computing cell normals. Could easily be templated on output
// point type but we are trying to control object size.
struct ComputeCellNormals
{
  vtkPoints* Points;
  vtkCellArray* Tris;
  float* CellNormals;
  vtkContour3DLinearGrid* Filter;

  ComputeCellNormals(
    vtkPoints* pts, vtkCellArray* tris, float* cellNormals, vtkContour3DLinearGrid* filter)
    : Points(pts)
    , Tris(tris)
    , CellNormals(cellNormals)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    auto cellIt = vtk::TakeSmartPointer(this->Tris->NewIterator());

    float* n = this->CellNormals + 3 * triId;
    double nd[3];

    vtkIdType unused = 3;
    const vtkIdType* tri = nullptr;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endTriId - triId) / 10 + 1, (vtkIdType)1000);

    for (cellIt->GoToCell(triId); cellIt->GetCurrentCellId() < endTriId; cellIt->GoToNextCell())
    {
      if (triId % checkAbortInterval == 0)
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
      triId++;
      cellIt->GetCurrentCell(unused, tri);
      vtkTriangle::ComputeNormal(this->Points, 3, tri, nd);
      *n++ = nd[0];
      *n++ = nd[1];
      *n++ = nd[2];
    }
  }
};

// Generate normals on output triangles
vtkSmartPointer<vtkFloatArray> GenerateTriNormals(
  vtkPoints* pts, vtkCellArray* tris, vtkContour3DLinearGrid* filter)
{
  vtkIdType numTris = tris->GetNumberOfCells();

  auto cellNormals = vtkSmartPointer<vtkFloatArray>::New();
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numTris);
  float* n = cellNormals->GetPointer(0);

  // Execute functor over all triangles
  ComputeCellNormals computeNormals(pts, tris, n, filter);
  EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numTris, computeNormals);

  return cellNormals;
}

// Functor for averaging normals at each merged point.
template <typename TId>
struct AverageNormals
{
  vtkStaticCellLinksTemplate<TId>* Links;
  const float* CellNormals;
  float* PointNormals;
  vtkContour3DLinearGrid* Filter;

  AverageNormals(vtkStaticCellLinksTemplate<TId>* links, float* cellNormals, float* ptNormals,
    vtkContour3DLinearGrid* filter)
    : Links(links)
    , CellNormals(cellNormals)
    , PointNormals(ptNormals)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    TId i, numTris;
    const TId* tris;
    const float* nc;
    float* n = this->PointNormals + 3 * ptId;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId, n += 3)
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
      numTris = this->Links->GetNumberOfCells(ptId);
      tris = this->Links->GetCells(ptId);
      n[0] = n[1] = n[2] = 0.0;
      for (i = 0; i < numTris; ++i)
      {
        nc = this->CellNormals + 3 * tris[i];
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
void GeneratePointNormals(vtkPoints* pts, vtkCellArray* tris, vtkFloatArray* cellNormals,
  vtkPointData* pd, vtkContour3DLinearGrid* filter)
{
  vtkIdType numPts = pts->GetNumberOfPoints();

  vtkNew<vtkFloatArray> ptNormals;
  ptNormals->SetName("Normals");
  ptNormals->SetNumberOfComponents(3);
  ptNormals->SetNumberOfTuples(numPts);
  float* ptN = ptNormals->GetPointer(0);

  // Grab the computed triangle normals
  float* triN = cellNormals->GetPointer(0);

  // Build cell links
  vtkNew<vtkPolyData> dummy;
  dummy->SetPoints(pts);
  dummy->SetPolys(tris);
  vtkStaticCellLinksTemplate<TId> links;
  links.BuildLinks(dummy);

  // Process all points, averaging normals
  AverageNormals<TId> average(&links, triN, ptN, filter);
  EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, average);

  // Clean up and get out
  pd->SetNormals(ptNormals);
}

} // anonymous namespace

// Map scalar trees to input datasets. Necessary due to potential composite
// data set input types, where each piece may have a different scalar tree.
struct vtkScalarTreeMap : public std::map<vtkUnstructuredGrid*, vtkScalarTree*>
{
};

//------------------------------------------------------------------------------
// Construct an instance of the class.
vtkContour3DLinearGrid::vtkContour3DLinearGrid()
{
  this->ContourValues = vtkContourValues::New();

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // By default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  this->MergePoints = false;
  this->InterpolateAttributes = false;
  this->ComputeNormals = false;
  this->ComputeScalars = false;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;
  this->ScalarTreeMap = new vtkScalarTreeMap;
}

//------------------------------------------------------------------------------
vtkContour3DLinearGrid::~vtkContour3DLinearGrid()
{
  this->ContourValues->Delete();

  // Need to free scalar trees associated with each dataset. There is a
  // special case where the stree cannot be deleted because is has been
  // specified by the user.
  vtkScalarTree* stree;
  vtkScalarTreeMap::iterator iter;
  for (iter = this->ScalarTreeMap->begin(); iter != this->ScalarTreeMap->end(); ++iter)
  {
    stree = iter->second;
    if (stree != nullptr && stree != this->ScalarTree)
    {
      stree->Delete();
    }
  }
  delete this->ScalarTreeMap;

  if (this->ScalarTree)
  {
    this->ScalarTree->Delete();
    this->ScalarTree = nullptr;
  }
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkContour3DLinearGrid::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
// Specialized contouring filter to handle unstructured grids with 3D linear
// cells (tetrahedras, hexes, wedges, pyradmids, voxels).
//
void vtkContour3DLinearGrid::ProcessPiece(
  vtkUnstructuredGrid* input, vtkDataArray* inScalars, vtkPolyData* output)
{
  // Make sure there is data to process
  vtkIdType numPts = input->GetNumberOfPoints(), numCells = input->GetNumberOfCells();
  if (numCells < 1)
  {
    vtkDebugMacro(<< "No data in this piece");
    return;
  }

  // Get the contour values.
  const vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double value, *values = this->ContourValues->GetValues();

  // Check the input point type. Only real types are supported.
  vtkPoints* inPts = input->GetPoints();
  int inPtsType = inPts->GetDataType();
  if ((inPtsType != VTK_FLOAT && inPtsType != VTK_DOUBLE))
  {
    vtkErrorMacro(<< "Input point type must be float or double");
    return;
  }
  // Create the output points. Only real types are supported.
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

  // Compute the scalar array range difference between min and max is 0.0, do not use
  // a scalar tree (no contour will be generated anyway).
  double scalarRange[2];
  input->GetPointData()->GetRange(inScalars->GetName(), scalarRange);
  double rangeDiff = scalarRange[1] - scalarRange[0];

  // If a scalar tree is requested, retrieve previous or if not found,
  // create a default or clone the factory.
  vtkScalarTree* stree = nullptr;
  if (this->UseScalarTree && rangeDiff > 0.0)
  {
    auto mapIter = this->ScalarTreeMap->find(input);
    if (mapIter == this->ScalarTreeMap->end())
    {
      if (this->ScalarTree)
      {
        stree = this->ScalarTree->NewInstance();
        stree->ShallowCopy(this->ScalarTree);
      }
      else
      {
        stree = vtkSpanSpace::New(); // default type if not provided
      }
      this->ScalarTreeMap->insert(std::make_pair(input, stree));
    }
    else
    {
      stree = mapIter->second;
    }
    // These will not cause a Modified() if the values haven't changed.
    stree->SetDataSet(input);
    stree->SetScalars(inScalars);
  }

  // Output triangles go here.
  vtkNew<vtkCellArray> newPolys;

  // Process all contour values
  vtkIdType totalPts = 0;
  vtkIdType totalTris = 0;

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  auto cellTypes = vtkUnsignedCharArray::SafeDownCast(input->GetCellTypesArray())->GetPointer(0);
  CellIter* cellIter = new CellIter(numCells, cellTypes, input->GetCells());

  // Now produce the output: fast path or general path
  bool mergePoints = this->MergePoints || this->ComputeNormals || this->InterpolateAttributes;
  if (!mergePoints)
  { // fast path
    // Generate all of the points at once (for multiple contours) and then produce the triangles.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      // process these scalar types, others could easily be added
      using ScalarsList = vtkTypeList::Create<unsigned int, int, float, double>;
      using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<vtkArrayDispatch::Reals,
        vtkArrayDispatch::Reals, ScalarsList>;

      ProcessFastPathWorker worker;
      if (!Dispatcher::Execute(inPts->GetData(), outPts->GetData(), inScalars, worker, this,
            numCells, cellIter, value, stree, newPolys.Get(), this->NumberOfThreadsUsed, totalPts,
            totalTris))
      {
        worker(inPts->GetData(), outPts->GetData(), inScalars, this, numCells, cellIter, value,
          stree, newPolys.Get(), this->NumberOfThreadsUsed, totalPts, totalTris);
      }

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    } // for all contours
  }

  else // Need to merge points, and possibly perform attribute interpolation
       // and generate normals. Hence use the slower path.
  {
    vtkPointData* inPDOriginal = input->GetPointData();
    // We don't want to change the active scalars in the input, but we
    // need to set the active scalars to match the input array to
    // process so that the point data copying works as expected. Create
    // a shallow copy of point data so that we can do this without
    // changing the input.
    vtkNew<vtkPointData> inPD;
    inPD->ShallowCopy(inPDOriginal);
    // Keep track of the old active scalars because when we set the new
    // scalars, the old scalars are removed from the point data entirely
    // and we have to add them back.
    vtkAbstractArray* oldScalars = inPD->GetScalars();
    inPD->SetScalars(inScalars);
    if (oldScalars)
    {
      inPD->AddArray(oldScalars);
    }
    vtkPointData* outPD = output->GetPointData();
    vtkCellData* inCD = input->GetCellData();
    vtkCellData* outCD = output->GetCellData();
    ArrayList pointArrays;
    ArrayList cellArrays;

    // Determine the size/type of point and cell ids needed to index points
    // and cells. Using smaller ids results in a greatly reduced memory footprint
    // and faster processing.
    this->LargeIds = numPts >= VTK_INT_MAX || numCells >= VTK_INT_MAX;

    // Generate all the merged points and triangles at once (for multiple
    // contours) and then produce the normals if requested.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      if (!this->LargeIds)
      {
        if (!ProcessMerged<int>(this, inPts, outPts, inScalars, numCells, cellIter, value, stree,
              newPolys, this->InterpolateAttributes, this->ComputeScalars, inPD, outPD,
              &pointArrays, inCD, outCD, &cellArrays, this->NumberOfThreadsUsed, totalPts,
              totalTris))
        {
          return;
        }
      }
      else
      {
        if (!ProcessMerged<vtkIdType>(this, inPts, outPts, inScalars, numCells, cellIter, value,
              stree, newPolys, this->InterpolateAttributes, this->ComputeScalars, inPD, outPD,
              &pointArrays, inCD, outCD, &cellArrays, this->NumberOfThreadsUsed, totalPts,
              totalTris))
        {
          return;
        }
      }

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    } // for all contour values

    // If requested, compute normals. Basically triangle normals are averaged
    // on each merged point. Requires building static CellLinks so it is a
    // relatively expensive operation. (This block of code is separate to
    // control .obj object bloat.)
    if (this->ComputeNormals)
    {
      vtkSmartPointer<vtkFloatArray> triNormals = GenerateTriNormals(outPts, newPolys, this);
      if (this->LargeIds)
      {
        GeneratePointNormals<vtkIdType>(outPts, newPolys, triNormals, outPD, this);
      }
      else
      {
        GeneratePointNormals<int>(outPts, newPolys, triNormals, outPD, this);
      }
    }
  } // slower path requires point merging

  // Report the results of execution
  vtkDebugMacro(<< "Created: " << outPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // Clean up
  delete cellIter;
  output->SetPoints(outPts);
  output->SetPolys(newPolys);
}

//------------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int vtkContour3DLinearGrid::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  int outputType = -1;
  if (vtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    outputType = VTK_POLY_DATA;
  }
  else if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    outputType = inputDO->GetDataObjectType();
  }
  else
  {
    vtkErrorMacro("Unsupported input type: " << inputDO->GetClassName());
    return 0;
  }

  return vtkDataObjectAlgorithm::SetOutputDataObject(
           outputType, outputVector->GetInformationObject(0), /*exact*/ true)
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
// RequestData checks the input, manages composite data, and handles the
// (optional) scalar tree. For each input vtkUnstructuredGrid, it produces an
// output vtkPolyData piece by performing contouring on the input dataset.
//
int vtkContour3DLinearGrid::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkUnstructuredGrid* inputGrid = vtkUnstructuredGrid::GetData(inputVector[0]);
  vtkPolyData* outputPD = vtkPolyData::GetData(outputVector);

  vtkCompositeDataSet* inputCDS = vtkCompositeDataSet::GetData(inputVector[0]);
  vtkCompositeDataSet* outputCDS = vtkCompositeDataSet::GetData(outputVector);

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputPD == nullptr) &&
    (inputCDS == nullptr || outputCDS == nullptr))
  {
    return 0;
  }

  // Get the contour values.
  int numContours = this->ContourValues->GetNumberOfContours();
  if (numContours < 1)
  {
    vtkLog(TRACE, "No contour values defined");
    return 1;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output vtkPolyData.
  vtkDataArray* inScalars;
  if (inputGrid)
  {
    // Get the scalars to process
    inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!inScalars)
    {
      vtkLog(TRACE, "No scalars available");
      return 1;
    }
    this->ProcessPiece(inputGrid, inScalars, outputPD);
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a vtkPolyData that is added to
  // the output multiblock dataset.
  else
  {
    outputCDS->CopyStructure(inputCDS);
    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (auto grid = vtkUnstructuredGrid::SafeDownCast(inIter->GetCurrentDataObject()))
      {
        int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
        inScalars = this->GetInputArrayToProcess(0, grid, association);
        if (!inScalars)
        {
          vtkLog(TRACE, "No scalars available");
          continue;
        }
        vtkNew<vtkPolyData> polydata;
        this->ProcessPiece(grid, inScalars, polydata);
        outputCDS->SetDataSet(inIter, polydata);
      }
      else
      {
        vtkDebugMacro(<< "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkContour3DLinearGrid::CanFullyProcessDataObject(
  vtkDataObject* object, const char* scalarArrayName)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    vtkDataArray* array = ug->GetPointData()->HasArray(scalarArrayName) == 1
      ? ug->GetPointData()->GetArray(scalarArrayName)
      : ug->GetPointData()->GetScalars();
    if (!array)
    {
      vtkLog(INFO, "Scalar array is null");
      return true;
    }

    int aType = array->GetDataType();
    if (aType != VTK_UNSIGNED_INT && aType != VTK_INT && aType != VTK_FLOAT && aType != VTK_DOUBLE)
    {
      vtkLog(INFO, "Invalid scalar array type");
      return false;
    }

    // Get list of cell types in the unstructured grid
    if (vtkUnsignedCharArray* cellTypes = ug->GetDistinctCellTypesArray())
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        if (cellType != VTK_EMPTY_CELL && cellType != VTK_VOXEL && cellType != VTK_TETRA &&
          cellType != VTK_HEXAHEDRON && cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
        {
          // Unsupported cell type, can't process data
          return false;
        }
      }
    }

    // All cell types are supported, can process data.
    return true;
  }
  else if (cd)
  {
    bool supported = true;
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto leafDS = iter->GetCurrentDataObject();
      if (!CanFullyProcessDataObject(leafDS, scalarArrayName))
      {
        supported = false;
        break;
      }
    }
    return supported;
  }

  return false; // not a vtkUnstructuredGrid nor a composite dataset
}

//------------------------------------------------------------------------------
int vtkContour3DLinearGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkContour3DLinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Merge Points: " << (this->MergePoints ? "true\n" : "false\n");
  os << indent
     << "Interpolate Attributes: " << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "true\n" : "false\n");

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: " << (this->LargeIds ? "true\n" : "false\n");

  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");
  if (this->ScalarTree)
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
VTK_ABI_NAMESPACE_END
