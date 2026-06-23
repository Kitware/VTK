// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_7_0
#define VTK_DEPRECATION_LEVEL 0

#include "vtkContour3DLinearGrid.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkBatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourValues.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMarchingCellsContourCases.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkPolyhedronContour.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkSpanSpace.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility> //make_pair

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContour3DLinearGrid);
vtkCxxSetObjectMacro(vtkContour3DLinearGrid, ScalarTree, vtkScalarTree);

//------------------------------------------------------------------------------
// Classes to support threaded execution. It performs point
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
//========================= GENERAL PATH (POINT MERGING) =======================
// Use vtkStaticEdgeLocatorTemplate for edge-based point merging. Processing is
// available with and without a scalar tree.

//-----------------------------------------------------------------------------
// An Edge with its two points and a percentage value
template <typename IDType>
using SimpleEdgeType = EdgeTuple<IDType, double>;

template <typename IDType>
struct EdgeDataType
{
  double T;
  IDType EId;
};

//-----------------------------------------------------------------------------
// An Edge with its two, a percentage value, and EdgeId
template <typename IDType>
using EdgeType = EdgeTuple<IDType, EdgeDataType<IDType>>;

//-----------------------------------------------------------------------------
// Edge Locator to store and search edges
template <typename IDType>
using EdgeLocatorType = vtkStaticEdgeLocatorTemplate<IDType, EdgeDataType<IDType>>;

// Cell Max Case based on the number of points
constexpr uint8_t CellMaxCase[9] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

template <typename IDType, typename TScalarsArray, bool GenerateTriangles>
struct ExtractEdgesBase
{
  using EdgeVectorType = std::vector<SimpleEdgeType<IDType>>;

  //-----------------------------------------------------------------------------
  // Keep track of output information within each batch of cells - this
  // information is eventually rolled up into offsets into the cell
  // connectivity and offsets arrays so that separate threads know where to
  // write their data.
  struct ContourCellBatchData
  {
    // This is done to reduce memory footprint.
    EdgeVectorType Edges;
    std::vector<IDType> OriginalCellIds;
    std::vector<uint16_t> OutputPolySize;

    vtkIdType PolyOffset;
    vtkIdType PolyConnectivityOffset;

    ContourCellBatchData()
      : PolyOffset(0)
      , PolyConnectivityOffset(0)
    {
      this->Edges.reserve(3 * 64);
      this->OriginalCellIds.reserve(64);
      this->OutputPolySize.reserve(64);
    }
    ~ContourCellBatchData() = default;
    ContourCellBatchData(ContourCellBatchData&& other) noexcept
      : Edges(std::move(other.Edges))
      , OriginalCellIds(std::move(other.OriginalCellIds))
      , OutputPolySize(std::move(other.OutputPolySize))
      , PolyOffset(other.PolyOffset)
      , PolyConnectivityOffset(other.PolyConnectivityOffset)
    {
      // Move constructor - moves everything including vectors
    }
    ContourCellBatchData& operator=(ContourCellBatchData&& other) noexcept
    {
      // Move assignment - moves everything including vectors
      this->Edges = std::move(other.Edges);
      this->OriginalCellIds = std::move(other.OriginalCellIds);
      this->OutputPolySize = std::move(other.OutputPolySize);
      this->PolyOffset = other.PolyOffset;
      this->PolyConnectivityOffset = other.PolyConnectivityOffset;
      return *this;
    }
    ContourCellBatchData(const ContourCellBatchData& other)
      : PolyOffset(other.PolyOffset)
      , PolyConnectivityOffset(other.PolyConnectivityOffset)
    {
      // Copy constructor - only copies offset values
    }
    ContourCellBatchData& operator=(const ContourCellBatchData& other)
    {
      // Copy assignment - only copies offset values
      this->PolyOffset = other.PolyOffset;
      this->PolyConnectivityOffset = other.PolyConnectivityOffset;
      return *this;
    }
    ContourCellBatchData& operator+=(const ContourCellBatchData& other)
    {
      this->PolyOffset += other.PolyOffset;
      this->PolyConnectivityOffset += other.PolyConnectivityOffset;
      return *this;
    }
    ContourCellBatchData operator+(const ContourCellBatchData& other) const
    {
      ContourCellBatchData result = *this;
      result += other;
      return result;
    }
  };
  using ContourCellBatch = vtkBatch<ContourCellBatchData>;
  using ContourCellBatches = vtkBatches<ContourCellBatchData>;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    vtkSmartPointer<vtkIdList> LocalPointIds;

    vtkSmartPointer<vtkCellArray> PolyhedronFaces;
    std::vector<vtkIdType> OutputPolyhedronPolySize;
    std::vector<EdgeTuple<vtkIdType, double>> IntersectedEdges;
  };

  vtkContour3DLinearGrid* Filter;
  TScalarsArray* Scalars;
  vtkUnstructuredGrid* Input;
  double Value;
  vtkCellArray* Polys;
  vtkIdType TotalPolys;            // the total polys thus far (support multiple contours)
  vtkIdType TotalConnectivitySize; // the total number of point ids in polys thus far (support
                                   // multiple contours)

  // Keep track of generated points and triangles on a per-thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;
  ContourCellBatches CellBatches;
  int NumThreadsUsed;

  vtkIdType NumPolys;
  vtkIdType ConnectivitySize;
  std::vector<IDType>& OriginalCellIds;
  EdgeTuple<IDType, EdgeDataType<IDType>>* Edges;

  ExtractEdgesBase(vtkContour3DLinearGrid* filter, TScalarsArray* scalars,
    vtkUnstructuredGrid* input, double value, vtkCellArray* polys, vtkIdType totalPolys,
    vtkIdType totalConnectivitySize, std::vector<IDType>& originalCellIds)
    : Filter(filter)
    , Scalars(scalars)
    , Input(input)
    , Value(value)
    , Polys(polys)
    , TotalPolys(totalPolys)
    , TotalConnectivitySize(totalConnectivitySize)
    , NumPolys(0)
    , ConnectivitySize(0)
    , OriginalCellIds(originalCellIds)
    , Edges(nullptr)
  {
  }
  virtual ~ExtractEdgesBase() = default;

  // Set up the iteration process
  virtual void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalPointIds = vtkSmartPointer<vtkIdList>::New();
    localData.PolyhedronFaces = vtkSmartPointer<vtkCellArray>::New();
  }

  // operator() provided by subclass

  // Produce edges for merged points. This is basically a parallel composition
  // into the final edges array.
  struct ProduceEdges
  {
    ExtractEdgesBase* Producer;
    EdgeType<IDType>* OutEdges;
    vtkContour3DLinearGrid* Filter;
    ProduceEdges(
      ExtractEdgesBase* producer, EdgeType<IDType>* outEdges, vtkContour3DLinearGrid* filter)
      : Producer(producer)
      , OutEdges(outEdges)
      , Filter(filter)
    {
    }
    void operator()(vtkIdType batchId, vtkIdType endBatchId)
    {
      bool isFirst = vtkSMPTools::GetSingleThread();
      vtkIdType checkAbortInterval = std::min((endBatchId - batchId) / 10 + 1, (vtkIdType)1000);

      for (; batchId < endBatchId; ++batchId)
      {
        if (batchId % checkAbortInterval == 0)
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
        auto& batch = this->Producer->CellBatches[batchId];
        vtkIdType edgeNum = batch.Data.PolyConnectivityOffset;
        EdgeType<IDType>* edges = this->OutEdges + edgeNum;
        const EdgeVectorType& batchEdges = batch.Data.Edges;
        assert(batchEdges.size() > 0);
        for (auto& edge : batchEdges)
        {
          edges->V0 = edge.V0;
          edges->V1 = edge.V1;
          edges->Data.T = edge.Data;
          edges->Data.EId = edgeNum;
          ++edges;
          ++edgeNum;
        }
      } // for all threads
    }
  };

  // Produce offsets for polys using the thread-local information
  struct ProducePolyOffsetsImpl : public vtkCellArray::DispatchUtilities
  {
    template <class OffsetsT, class ConnectivityT>
    void operator()(OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType batchId,
      vtkIdType endBatchId, ExtractEdgesBase* producer)
    {
      using ValueType = GetAPIType<OffsetsT>;
      const auto& totalPolys = producer->TotalPolys;
      const auto& totalConnectivitySize = producer->TotalConnectivitySize;

      auto range = GetRange(offsets);

      for (; batchId < endBatchId; ++batchId)
      {
        auto& batch = producer->CellBatches[batchId];
        const auto& outputPolySizes = batch.Data.OutputPolySize;
        if (outputPolySizes.empty())
        {
          continue;
        }
        // Write START offsets at positions [P+0..P+K-1] for this batch's
        // K polygons. Position (P+K) -- the END of the last polygon /
        // START of the next batch's first polygon -- is filled either by
        // the next batch's first iteration, or for the very last batch
        // by SetOffset(NumPolys+TotalPolys, ...) below.
        auto offsetsIter = range.begin() + totalPolys + batch.Data.PolyOffset;
        vtkIdType currentConnPos = totalConnectivitySize + batch.Data.PolyConnectivityOffset;
        std::transform(outputPolySizes.begin(), outputPolySizes.end(), offsetsIter,
          [&](uint16_t polySize) -> ValueType
          {
            const auto offsetOfThisPoly = currentConnPos;
            currentConnPos += polySize;
            return offsetOfThisPoly;
          });
      }
    }
  };

  // Composite batch data
  virtual void Reduce()
  {
    // trim batches with 0 cells in-place
    this->CellBatches.TrimBatches(
      [](const ContourCellBatch& batch) { return batch.Data.PolyOffset == 0; });
    const auto globalSum = this->CellBatches.BuildOffsetsAndGetGlobalSum();
    this->NumPolys = globalSum.PolyOffset;
    this->ConnectivitySize = globalSum.PolyConnectivityOffset;
    this->NumThreadsUsed = static_cast<int>(this->LocalData.size());

    // create original cell ids
    this->OriginalCellIds.resize(static_cast<size_t>(this->NumPolys));
    vtkSMPTools::For(0, this->CellBatches.GetNumberOfBatches(),
      [&](vtkIdType beginBatchId, vtkIdType endBatchId)
      {
        for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
        {
          const ContourCellBatch& batch = this->CellBatches[batchId];
          const auto& localData = batch.Data;
          std::copy(localData.OriginalCellIds.begin(), localData.OriginalCellIds.end(),
            this->OriginalCellIds.begin() + batch.Data.PolyOffset);
        }
      });

    // 1. Allocate space for the combined data
    this->Polys->ResizeExact(
      this->NumPolys + this->TotalPolys, this->ConnectivitySize + this->TotalConnectivitySize);
    if constexpr (!GenerateTriangles)
    {
      // 2. Parallel Fill of offsets
      if (this->NumPolys > 0)
      {
        vtkSMPTools::For(0, this->CellBatches.GetNumberOfBatches(),
          [&](vtkIdType batchId, vtkIdType endBatchId)
          { this->Polys->Dispatch(ProducePolyOffsetsImpl{}, batchId, endBatchId, this); });
      }

      // 3. Set the VERY last offset (Index N)
      this->Polys->SetOffset(
        this->NumPolys + this->TotalPolys, this->ConnectivitySize + this->TotalConnectivitySize);
    }

    if (this->NumPolys > 0)
    {
      // Copy batch edges to composited edge array.
      this->Edges = new EdgeType<IDType>[this->ConnectivitySize];
      ProduceEdges produceEdges(this, this->Edges, this->Filter);
      EXECUTE_SMPFOR(this->Filter->GetSequentialProcessing(),
        this->CellBatches.GetNumberOfBatches(), produceEdges);
    }
  } // Reduce
}; // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without scalar tree).
template <typename IDType, typename TScalarsArray, bool GenerateTriangles>
struct ExtractEdges : public ExtractEdgesBase<IDType, TScalarsArray, GenerateTriangles>
{
  using TExtractEdgesBase = ExtractEdgesBase<IDType, TScalarsArray, GenerateTriangles>;

  ExtractEdges(vtkContour3DLinearGrid* filter, TScalarsArray* scalars, vtkUnstructuredGrid* input,
    double value, vtkCellArray* polys, vtkIdType totalPolys, vtkIdType totalConnectivitySize,
    std::vector<IDType>& originalCellIds)
    : TExtractEdgesBase(
        filter, scalars, input, value, polys, totalPolys, totalConnectivitySize, originalCellIds)
  {
    // initialize batches
    this->CellBatches.Initialize(input->GetNumberOfCells(), 1000);
  }
  ~ExtractEdges() override = default;

  // Set up the iteration process
  void Initialize() override { this->TExtractEdgesBase::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType batchId, vtkIdType endBatchId)
  {
    auto& localData = this->LocalData.Local();
    auto& lPointIds = localData.LocalPointIds;
    auto& lPolyhedronFaces = localData.PolyhedronFaces;
    auto& lIntersectedEdges = localData.IntersectedEdges;
    auto& lOutputPolyhedronPolySize = localData.OutputPolyhedronPolySize;
    vtkIdType npts, i;
    const vtkIdType* pts = nullptr;
    uint8_t isoCase;
    double s[8], value = this->Value, deltaScalar;
    double t;
    bool isFirst = vtkSMPTools::GetSingleThread();

    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endBatchId - batchId) / 10 + 1, (vtkIdType)1000);

    for (; batchId < endBatchId; ++batchId)
    {
      if (batchId % checkAbortInterval == 0)
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
      auto& batch = this->CellBatches[batchId];
      auto& batchNumberOfCells = batch.Data.PolyOffset;
      auto& batchPolyConnectivity = batch.Data.PolyConnectivityOffset;
      auto& batchEdges = batch.Data.Edges;
      auto& batchOriginalCellIds = batch.Data.OriginalCellIds;
      auto& batchOutputPolySize = batch.Data.OutputPolySize;

      for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        int cellType = this->Input->GetCellType(cellId);
        if (cellType >= VTK_TETRA && cellType <= VTK_PYRAMID)
        {
          // Compute case by repeated masking of scalar value
          this->Input->GetCellPoints(cellId, npts, pts, lPointIds);
          for (isoCase = 0, i = 0; i < npts; ++i)
          {
            s[i] = static_cast<double>(scalars[pts[i]]);
            isoCase |= (s[i] >= value) << i;
          }
          // skip cells that all points are either all in or all out
          if (isoCase == 0 || isoCase == CellMaxCase[npts])
          {
            continue;
          }
          auto cellEdges = vtkMarchingCellsContourCases::GetCellEdges(cellType);
          auto edge = GenerateTriangles
            ? vtkMarchingCellsContourCases::GetCellCase(cellType, isoCase)
            : vtkMarchingCellsContourCases::GetCellCaseWithPolygons(cellType, isoCase);
          while (*edge > -1) // for all polygons/triangles
          {
            const int numEdgePoints = GenerateTriangles ? 3 : *edge++;
            for (int edgeId = 0; edgeId < numEdgePoints; ++edgeId, ++edge)
            {
              const auto& vert = cellEdges[*edge];
              const auto& v0 = vert[0];
              const auto& v1 = vert[1];
              deltaScalar = s[v1] - s[v0];
              t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
              t = (pts[v0] < pts[v1] ? t : (1.0 - t));      // edges (v0,v1) must have v0<v1
              batchEdges.emplace_back(pts[v0], pts[v1], t); // edge constructor may swap v0<->v1
            } // for all edges in this polygon
            batchOriginalCellIds.push_back(static_cast<IDType>(cellId));
            if constexpr (!GenerateTriangles)
            {
              batchOutputPolySize.push_back(numEdgePoints);
            }
          } // for all polygons in this case
        }
        else if (cellType == VTK_POLYHEDRON)
        {
          this->Input->GetCellPoints(cellId, npts, pts, lPointIds);
          // count how many points are inside
          vtkIdType insidePoints = 0;
          for (i = 0; i < npts; ++i)
          {
            insidePoints += (scalars[pts[i]] - value) >= 0.0;
          }
          // skip cells that all points are either all in or all out
          if (insidePoints == 0 || insidePoints == npts)
          {
            continue;
          }
          this->Input->GetPolyhedronFaces(cellId, lPolyhedronFaces);
          vtkPolyhedronContour::ContourCell(npts, pts, lPolyhedronFaces.Get(), this->Scalars, value,
            GenerateTriangles, lOutputPolyhedronPolySize, lIntersectedEdges);
          size_t edgeOffset = 0;
          for (size_t polyId = 0; polyId < lOutputPolyhedronPolySize.size(); ++polyId)
          {
            const int numEdgePoints = GenerateTriangles ? 3 : lOutputPolyhedronPolySize[polyId];
            for (int edgeId = 0; edgeId < numEdgePoints; ++edgeId, ++edgeOffset)
            {
              const auto& edge = lIntersectedEdges[edgeOffset];
              batchEdges.emplace_back(edge.V0, edge.V1, edge.Data);
            } // for all edges in this polygon
            batchOriginalCellIds.push_back(static_cast<IDType>(cellId));
            if constexpr (!GenerateTriangles)
            {
              batchOutputPolySize.push_back(numEdgePoints);
            }
          } // for all polygons in this case
        }
      } // for all cells in this batch
      batchNumberOfCells = batchOriginalCellIds.size();
      batchPolyConnectivity = batchEdges.size();
    } // for all cells in this batch
  }

  // Composite local thread data
  void Reduce() override { this->TExtractEdgesBase::Reduce(); } // Reduce
}; // ExtractEdges

// Generate edges using a scalar tree.
template <typename IDType, typename TScalarsArray, bool GenerateTriangles>
struct ExtractEdgesST : public ExtractEdgesBase<IDType, TScalarsArray, GenerateTriangles>
{
  using TExtractEdgesBase = ExtractEdgesBase<IDType, TScalarsArray, GenerateTriangles>;

  vtkScalarTree* ScalarTree;
  vtkIdType NumBatches;

  ExtractEdgesST(vtkContour3DLinearGrid* filter, TScalarsArray* scalars, vtkUnstructuredGrid* input,
    double value, vtkScalarTree* st, vtkCellArray* polys, vtkIdType totalPolys,
    vtkIdType totalConnectivitySize, std::vector<IDType>& originalCellIds)
    : TExtractEdgesBase(
        filter, scalars, input, value, polys, totalPolys, totalConnectivitySize, originalCellIds)
    , ScalarTree(st)
  {
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
    if (this->NumBatches > 0)
    {
      // Match the scalar tree's batch count
      // Back-calculate batch size from numBatches and numCells
      unsigned int batchSize = static_cast<unsigned int>(
        (input->GetNumberOfCells() + this->NumBatches - 1) / this->NumBatches);
      // initialize batches
      this->CellBatches.Initialize(input->GetNumberOfCells(), batchSize);
    }
  }
  ~ExtractEdgesST() override = default;

  // Set up the iteration process
  void Initialize() override { this->TExtractEdgesBase::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType batchId, vtkIdType endBatchId)
  {
    auto& localData = this->LocalData.Local();
    auto& lPointIds = localData.LocalPointIds;
    auto& lPolyhedronFaces = localData.PolyhedronFaces;
    auto& lIntersectedEdges = localData.IntersectedEdges;
    auto& lOutputPolyhedronPolySize = localData.OutputPolyhedronPolySize;
    vtkIdType npts, i;
    const vtkIdType* pts = nullptr;
    uint8_t isoCase;
    double s[8], value = this->Value, deltaScalar;
    double t;
    const vtkIdType* cellIds;
    vtkIdType idx, numCells;
    bool isFirst = vtkSMPTools::GetSingleThread();

    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    vtkIdType checkAbortInterval = std::min((endBatchId - batchId) / 10 + 1, (vtkIdType)1000);

    for (; batchId < endBatchId; ++batchId)
    {
      if (batchId % checkAbortInterval == 0)
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
      auto& batch = this->CellBatches[batchId];
      auto& batchNumberOfCells = batch.Data.PolyOffset;
      auto& batchPolyConnectivity = batch.Data.PolyConnectivityOffset;
      auto& batchEdges = batch.Data.Edges;
      auto& batchOriginalCellIds = batch.Data.OriginalCellIds;
      auto& batchOutputPolySize = batch.Data.OutputPolySize;

      cellIds = this->ScalarTree->GetCellBatch(batchId, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        const auto& cellId = cellIds[idx];
        int cellType = this->Input->GetCellType(cellId);
        if (cellType >= VTK_TETRA && cellType <= VTK_PYRAMID)
        {
          this->Input->GetCellPoints(cellId, npts, pts, lPointIds);
          // Compute case by repeated masking of scalar value
          for (isoCase = 0, i = 0; i < npts; ++i)
          {
            s[i] = static_cast<double>(scalars[pts[i]]);
            isoCase |= (s[i] >= value) << i;
          }
          // skip cells that all points are either all in or all out
          if (isoCase == 0 || isoCase == CellMaxCase[npts])
          {
            continue;
          }
          auto cellEdges = vtkMarchingCellsContourCases::GetCellEdges(cellType);
          auto edge = GenerateTriangles
            ? vtkMarchingCellsContourCases::GetCellCase(cellType, isoCase)
            : vtkMarchingCellsContourCases::GetCellCaseWithPolygons(cellType, isoCase);
          while (*edge > -1) // for all polygons/triangles
          {
            const int numEdgePoints = GenerateTriangles ? 3 : *edge++;
            for (int edgeId = 0; edgeId < numEdgePoints; ++edgeId, ++edge)
            {
              const auto& vert = cellEdges[*edge];
              const auto& v0 = vert[0];
              const auto& v1 = vert[1];
              deltaScalar = s[v1] - s[v0];
              t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
              t = (pts[v0] < pts[v1] ? t : (1.0 - t));      // edges (v0,v1) must have v0<v1
              batchEdges.emplace_back(pts[v0], pts[v1], t); // edge constructor may swap v0<->v1
            } // for all edges in this polygon
            batchOriginalCellIds.push_back(static_cast<IDType>(cellId));
            if constexpr (!GenerateTriangles)
            {
              batchOutputPolySize.push_back(numEdgePoints);
            }
          } // for all polygons in this case
        }
        else if (cellType == VTK_POLYHEDRON)
        {
          this->Input->GetCellPoints(cellId, npts, pts, lPointIds);
          // count how many points are inside
          vtkIdType insidePoints = 0;
          for (i = 0; i < npts; ++i)
          {
            insidePoints += (scalars[pts[i]] - value) >= 0.0;
          }
          // skip cells that all points are either all in or all out
          if (insidePoints == 0 || insidePoints == npts)
          {
            continue;
          }
          this->Input->GetPolyhedronFaces(cellId, lPolyhedronFaces);
          vtkPolyhedronContour::ContourCell(npts, pts, lPolyhedronFaces.Get(), this->Scalars, value,
            GenerateTriangles, lOutputPolyhedronPolySize, lIntersectedEdges);
          size_t edgeOffset = 0;
          for (size_t polyId = 0; polyId < lOutputPolyhedronPolySize.size(); ++polyId)
          {
            const int numEdgePoints = GenerateTriangles ? 3 : lOutputPolyhedronPolySize[polyId];
            for (int edgeId = 0; edgeId < numEdgePoints; ++edgeId, ++edgeOffset)
            {
              const auto& edge = lIntersectedEdges[edgeOffset];
              batchEdges.emplace_back(edge.V0, edge.V1, edge.Data);
            } // for all edges in this polygon
            batchOriginalCellIds.push_back(static_cast<IDType>(cellId));
            if constexpr (!GenerateTriangles)
            {
              batchOutputPolySize.push_back(numEdgePoints);
            }
          } // for all polygons in this case
        }
      } // for all cells in this batch
      batchNumberOfCells = batchOriginalCellIds.size();
      batchPolyConnectivity = batchEdges.size();
    } // for all batches
  }

  // Composite local thread data
  void Reduce() override { this->TExtractEdgesBase::Reduce(); } // Reduce

}; // ExtractEdgesST

// Dispatch worker for Extract Edges. Handles template dispatching etc.
template <typename TIds>
struct ExtractEdgesWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* scalars, vtkContour3DLinearGrid* filter, vtkUnstructuredGrid* input,
    double isoValue, vtkScalarTree* st, vtkCellArray* newPolys, vtkIdType totalPolys,
    vtkIdType totalConnectivitySize, vtkIdType& numPolys, vtkIdType& connectivitySize,
    EdgeTuple<TIds, EdgeDataType<TIds>>*& mergeEdges, std::vector<TIds>& originalCellIds,
    int& numThreads)
  {
    if (st != nullptr)
    {
      if (filter->GetGenerateTriangles())
      {
        using TExtractEdgesST = ExtractEdgesST<TIds, TScalarArray, true>;
        TExtractEdgesST extractEdges(filter, scalars, input, isoValue, st, newPolys, totalPolys,
          totalConnectivitySize, originalCellIds);
        EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(),
          extractEdges.CellBatches.GetNumberOfBatches(), extractEdges, numThreads);
        numPolys = extractEdges.NumPolys;
        connectivitySize = extractEdges.ConnectivitySize;
        mergeEdges = extractEdges.Edges;
      }
      else
      {
        using TExtractEdgesST = ExtractEdgesST<TIds, TScalarArray, false>;
        TExtractEdgesST extractEdges(filter, scalars, input, isoValue, st, newPolys, totalPolys,
          totalConnectivitySize, originalCellIds);
        EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(),
          extractEdges.CellBatches.GetNumberOfBatches(), extractEdges, numThreads);
        numPolys = extractEdges.NumPolys;
        connectivitySize = extractEdges.ConnectivitySize;
        mergeEdges = extractEdges.Edges;
      }
    }
    else
    {
      if (filter->GetGenerateTriangles())
      {
        using TExtractEdges = ExtractEdges<TIds, TScalarArray, true>;
        TExtractEdges extractEdges(filter, scalars, input, isoValue, newPolys, totalPolys,
          totalConnectivitySize, originalCellIds);
        EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(),
          extractEdges.CellBatches.GetNumberOfBatches(), extractEdges, numThreads);
        numPolys = extractEdges.NumPolys;
        connectivitySize = extractEdges.ConnectivitySize;
        mergeEdges = extractEdges.Edges;
      }
      else
      {
        using TExtractEdges = ExtractEdges<TIds, TScalarArray, false>;
        TExtractEdges extractEdges(filter, scalars, input, isoValue, newPolys, totalPolys,
          totalConnectivitySize, originalCellIds);
        EXECUTE_REDUCED_SMPFOR(filter->GetSequentialProcessing(),
          extractEdges.CellBatches.GetNumberOfBatches(), extractEdges, numThreads);
        numPolys = extractEdges.NumPolys;
        connectivitySize = extractEdges.ConnectivitySize;
        mergeEdges = extractEdges.Edges;
      }
    }
  }
};

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProducePolys
{
  using MergeTupleType = EdgeType<IDType>;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  vtkCellArray* Polys;
  vtkIdType TotalPts;
  vtkIdType TotalConnectivitySize;
  int NumThreadsUsed; // placeholder
  vtkContour3DLinearGrid* Filter;

  ProducePolys(const MergeTupleType* merge, const IDType* offsets, vtkCellArray* polys,
    vtkIdType totalPts, vtkIdType totalConnectivitySize, vtkContour3DLinearGrid* filter)
    : MergeArray(merge)
    , Offsets(offsets)
    , Polys(polys)
    , TotalPts(totalPts)
    , TotalConnectivitySize(totalConnectivitySize)
    , NumThreadsUsed(1)
    , Filter(filter)
  {
  }

  struct Impl : public vtkCellArray::DispatchUtilities
  {
    template <class OffsetsT, class ConnectivityT>
    void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* conn, vtkIdType ptId,
      const vtkIdType endPtId, const vtkIdType ptOffset, const vtkIdType connOffset,
      const IDType* offsets, const MergeTupleType* mergeArray, vtkContour3DLinearGrid* filter)
    {
      using ValueType = GetAPIType<OffsetsT>;
      auto connRange = GetRange(conn);
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
          connRange[connIdx] = static_cast<ValueType>(ptId + ptOffset);
        } // for this group of coincident edges
      } // for all merged points
    }
  };

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    this->Polys->Dispatch(Impl{}, ptId, endPtId, this->TotalPts, this->TotalConnectivitySize,
      this->Offsets, this->MergeArray, this->Filter);
  }
};

// This method generates the output isosurface points. One point per
// merged edge is generated.
template <typename TInputPointsArray, typename TOutputPointsArray, typename IDType>
struct ProducePoints
{
  using MergeTupleType = EdgeTuple<IDType, EdgeDataType<IDType>>;

  vtkContour3DLinearGrid* Filter;
  TInputPointsArray* InPts;
  TOutputPointsArray* OutPts;
  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  const vtkIdType TotalPrevPoints;
  const vtkIdType TotalOutputPoints;

  ProducePoints(vtkContour3DLinearGrid* filter, TInputPointsArray* inPts,
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
    double t;
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
      if (t < 0 || t > 1)
      {
        std::swap(v0, v1);
        t = 1.0 - t;
      }
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
struct ProducePointsWorker
{
  template <typename TInputPointsArray, typename TOutputPointsArray>
  void operator()(TInputPointsArray* inputPointsArray, TOutputPointsArray* outputPointsArray,
    vtkContour3DLinearGrid* filter, const EdgeTuple<TIds, EdgeDataType<TIds>>* mergeArray,
    const TIds* offsets, vtkIdType totalPoints, vtkIdType numPts)
  {
    ProducePoints<TInputPointsArray, TOutputPointsArray, TIds> producePoints(
      filter, inputPointsArray, outputPointsArray, mergeArray, offsets, totalPoints);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, producePoints);
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
    double t;
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
      if (t < 0 || t > 1)
      {
        std::swap(v0, v1);
        t = 1.0 - t;
      }
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
  vtkIdType TotalPolys;                     // total polys / multiple contours computed previously
  vtkContour3DLinearGrid* Filter;

  ProduceCellAttributes(const std::vector<TIds>& originalCellIds, ArrayList* arrays,
    vtkIdType totalPolys, vtkContour3DLinearGrid* filter)
    : OriginalCellIds(originalCellIds)
    , Arrays(arrays)
    , TotalPolys(totalPolys)
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
      this->Arrays->Copy(this->OriginalCellIds[cellId], cellId + this->TotalPolys);
    }
  }
};

// Wrapper to handle multiple template types for merged processing
template <typename TIds>
void Process(vtkContour3DLinearGrid* filter, vtkPoints* inPts, vtkPoints* outPts,
  vtkDataArray* inScalars, vtkUnstructuredGrid* input, double isoValue, vtkScalarTree* st,
  vtkCellArray* newPolys, vtkTypeBool intAttr, vtkTypeBool computeScalars, vtkPointData* inPD,
  vtkPointData* outPD, ArrayList* pointArrays, vtkCellData* inCD, vtkCellData* outCD,
  ArrayList* cellArrays, int& numThreads, vtkIdType totalPts, vtkIdType totalPolys,
  vtkIdType totalConnectivitySize)
{
  // Extract edges that the contour intersects. Templated on type of scalars.
  // List below the explicit choice of scalars that can be processed.
  vtkIdType numPolys = 0, connectivitySize = 0;
  EdgeType<TIds>* mergeEdges = nullptr; // may need reference counting
  std::vector<TIds> originalCellIds;
  ExtractEdgesWorker<TIds> extractEdgesWorker;
  // process these scalar types, others could easily be added
  using ScalarsList = vtkTypeList::Create<unsigned int, int, float, double>;
  using DispatcherExtractEdges = vtkArrayDispatch::DispatchByValueType<ScalarsList>;
  if (!DispatcherExtractEdges::Execute(inScalars, extractEdgesWorker, filter, input, isoValue, st,
        newPolys, totalPolys, totalConnectivitySize, numPolys, connectivitySize, mergeEdges,
        originalCellIds, numThreads))
  {
    extractEdgesWorker(inScalars, filter, input, isoValue, st, newPolys, totalPolys,
      totalConnectivitySize, numPolys, connectivitySize, mergeEdges, originalCellIds, numThreads);
  }
  int nt = numThreads;

  // Make sure data was produced
  if (numPolys <= 0)
  {
    delete[] mergeEdges;
    return;
  }

  // Merge coincident edges. The Offsets refer to the single unique edge
  // from the sorted group of duplicate edges.
  vtkIdType numPts;
  EdgeLocatorType<TIds> loc;
  const TIds* offsets = loc.MergeEdges(connectivitySize, mergeEdges, numPts);

  // Generate polys.
  ProducePolys<TIds> producePolys(
    mergeEdges, offsets, newPolys, totalPts, totalConnectivitySize, filter);
  EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, producePolys);
  numThreads = nt;

  // Generate points (one per unique edge)
  outPts->GetData()->SetNumberOfTuples(numPts + totalPts);
  ProducePointsWorker<TIds> producePointsWorker;

  using DispatcherProducePoints = vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::PointArrays,
    vtkArrayDispatch::AOSPointArrays>;
  if (!DispatcherProducePoints::Execute(inPts->GetData(), outPts->GetData(), producePointsWorker,
        filter, mergeEdges, offsets, totalPts, numPts))
  {
    producePointsWorker(
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
    if (totalPolys <= 0) // first contour value generating output
    {
      outCD->CopyAllocate(inCD, numPolys);
      cellArrays->AddArrays(numPolys, inCD, outCD, 0.0, /*promote=*/false);
    }
    else
    {
      cellArrays->Realloc(totalPolys + numPolys);
    }
    ProduceCellAttributes<TIds> interpolateCell(originalCellIds, cellArrays, totalPolys, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPolys, interpolateCell);
  }

  // Clean up
  delete[] mergeEdges;
}

// Functor for computing cell normals. Could easily be templated on output
// point type but we are trying to control object size.
template <bool GenerateTriangles>
struct ComputeCellNormals
{
  vtkPoints* Points;
  vtkCellArray* Polys;
  float* CellNormals;
  vtkContour3DLinearGrid* Filter;
  vtkSMPThreadLocalObject<vtkIdList> TLPointIds;

  ComputeCellNormals(
    vtkPoints* pts, vtkCellArray* polys, float* cellNormals, vtkContour3DLinearGrid* filter)
    : Points(pts)
    , Polys(polys)
    , CellNormals(cellNormals)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType polyId, vtkIdType endPolyId)
  {
    auto& lPointIds = this->TLPointIds.Local();

    float* n = this->CellNormals + 3 * polyId;
    double nd[3];

    vtkIdType npts = 3;
    const vtkIdType* tri = nullptr;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPolyId - polyId) / 10 + 1, (vtkIdType)1000);

    for (; polyId < endPolyId; ++polyId)
    {
      if (polyId % checkAbortInterval == 0)
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
      this->Polys->GetCellAtId(polyId, npts, tri, lPointIds);
      if constexpr (GenerateTriangles)
      {
        vtkTriangle::ComputeNormal(this->Points, 3, tri, nd);
      }
      else
      {
        vtkPolygon::ComputeNormal(this->Points, npts, tri, nd);
      }
      *n++ = nd[0];
      *n++ = nd[1];
      *n++ = nd[2];
    }
  }
};

// Generate normals on output triangles
vtkSmartPointer<vtkFloatArray> GeneratePolyNormals(
  vtkPoints* pts, vtkCellArray* polys, vtkContour3DLinearGrid* filter)
{
  vtkIdType numTris = polys->GetNumberOfCells();

  auto cellNormals = vtkSmartPointer<vtkFloatArray>::New();
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numTris);
  float* n = cellNormals->GetPointer(0);

  // Execute functor over all triangles
  if (filter->GetGenerateTriangles())
  {
    ComputeCellNormals<true> computeNormals(pts, polys, n, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numTris, computeNormals);
  }
  else
  {
    ComputeCellNormals<false> computeNormals(pts, polys, n, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numTris, computeNormals);
  }

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
  this->GenerateTriangles = true;
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
void vtkContour3DLinearGrid::SetMergePointsOn()
{
  this->SetMergePoints(true);
}

//------------------------------------------------------------------------------
void vtkContour3DLinearGrid::SetMergePointsOff()
{
  this->SetMergePoints(false);
}

//------------------------------------------------------------------------------
// Specialized contouring filter to handle unstructured grids with 3D linear
// cells (tetrahedras, hexes, wedges, pyradmids, voxels, polyhedrons)
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
  if (this->GenerateTriangles)
  {
    newPolys->UseFixedSizeDefaultStorage(3);
  }

  // Process all contour values
  vtkIdType totalPts = 0;
  vtkIdType totalPolys = 0;
  vtkIdType totalConnectivitySize = 0;

  // Now produce the output
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
    if (this->LargeIds)
    {
      Process<vtkIdType>(this, inPts, outPts, inScalars, input, value, stree, newPolys,
        this->InterpolateAttributes, this->ComputeScalars, inPD, outPD, &pointArrays, inCD, outCD,
        &cellArrays, this->NumberOfThreadsUsed, totalPts, totalPolys, totalConnectivitySize);
    }
    else
    {
      Process<int>(this, inPts, outPts, inScalars, input, value, stree, newPolys,
        this->InterpolateAttributes, this->ComputeScalars, inPD, outPD, &pointArrays, inCD, outCD,
        &cellArrays, this->NumberOfThreadsUsed, totalPts, totalPolys, totalConnectivitySize);
    }

    // Multiple contour values require accumulating points & triangles
    totalPts = outPts->GetNumberOfPoints();
    totalPolys = newPolys->GetNumberOfCells();
    totalConnectivitySize = newPolys->GetNumberOfConnectivityIds();
  } // for all contour values

  // If requested, compute normals. Basically triangle normals are averaged
  // on each merged point. Requires building static CellLinks so it is a
  // relatively expensive operation. (This block of code is separate to
  // control .obj object bloat.)
  if (this->ComputeNormals)
  {
    vtkSmartPointer<vtkFloatArray> triNormals = GeneratePolyNormals(outPts, newPolys, this);
    if (this->LargeIds)
    {
      GeneratePointNormals<vtkIdType>(outPts, newPolys, triNormals, outPD, this);
    }
    else
    {
      GeneratePointNormals<int>(outPts, newPolys, triNormals, outPD, this);
    }
  }

  // Report the results of execution
  vtkDebugMacro(<< "Created: " << outPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // Restore the original active point scalars on the output. The contour
  // pipeline temporarily sets inScalars (the contouring array) as active so
  // that interpolation works, then removes it from the output if
  // ComputeScalars is off. Without this restore, output point data has no
  // active scalars even though oldScalars (e.g. the original SDF) is still
  // present as a regular array, which causes downstream mappers to fall
  // through to cell scalars instead of the expected point field.
  if (oldScalars && outPD->HasArray(oldScalars->GetName()))
  {
    outPD->SetActiveScalars(oldScalars->GetName());
  }

  // Clean up
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
      vtkLog(TRACE, "Scalar array is null");
      return true;
    }

    int aType = array->GetDataType();
    if (aType != VTK_UNSIGNED_INT && aType != VTK_INT && aType != VTK_FLOAT && aType != VTK_DOUBLE)
    {
      vtkLog(TRACE, "Invalid scalar array type");
      return false;
    }

    // Get list of cell types in the unstructured grid
    if (vtkUnsignedCharArray* cellTypes = ug->GetDistinctCellTypesArray())
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        if (cellType != VTK_EMPTY_CELL && cellType != VTK_VOXEL && cellType != VTK_TETRA &&
          cellType != VTK_HEXAHEDRON && cellType != VTK_WEDGE && cellType != VTK_PYRAMID &&
          cellType != VTK_POLYHEDRON)
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
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "true\n" : "false\n");
  os << indent << "Generate Triangles: " << (this->GenerateTriangles ? "true\n" : "false\n");

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
