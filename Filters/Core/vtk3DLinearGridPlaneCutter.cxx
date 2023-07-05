// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk3DLinearGridPlaneCutter.h"

#include "vtk3DLinearGridInternal.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <algorithm>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtk3DLinearGridPlaneCutter);
vtkCxxSetObjectMacro(vtk3DLinearGridPlaneCutter, Plane, vtkPlane);

//------------------------------------------------------------------------------
// Classes to support threaded execution. Note that there is only one
// strategy at this time: a path that pre-computes plane function values and
// uses these to cull non-intersected cells. Sphere trees may be supported in
// the future.

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

//========================= Quick plane cut culling ===========================
// Compute an array that classifies each point with respect to the current
// plane (i.e. above the plane(=2), below the plane(=1), on the plane(=0)).
// InOutArray is allocated here and should be deleted by the invoking
// code. InOutArray is an unsigned char array to simplify bit fiddling later
// on (i.e., PlaneIntersects() method).
//
// The reason we compute this unsigned char array as compared to an array of
// function values is to reduce the amount of memory used, and the written to
// memory, since these are significant costs for large data.

// Templated for explicit point representations of real type
template <typename TP>
struct ClassifyPoints;

struct Classify
{
  unsigned char* InOutArray;
  double* DistanceArray;
  double Origin[3];
  double Normal[3];
  vtk3DLinearGridPlaneCutter* Filter;

  Classify(vtkPoints* pts, vtkPlane* plane, vtk3DLinearGridPlaneCutter* filter)
  {
    this->InOutArray = new unsigned char[pts->GetNumberOfPoints()];
    this->DistanceArray = new double[pts->GetNumberOfPoints()];
    plane->GetOrigin(this->Origin);
    plane->GetNormal(this->Normal);
    this->Filter = filter;
  }

  // Check if a list of points intersects the plane
  static bool PlaneIntersects(const unsigned char* inout, vtkIdType npts, const vtkIdType* pts)
  {
    unsigned char onOneSideOfPlane = inout[pts[0]];
    for (vtkIdType i = 1; onOneSideOfPlane && i < npts; ++i)
    {
      onOneSideOfPlane &= inout[pts[i]];
    }
    return (!onOneSideOfPlane);
  }
};

template <typename TP>
struct ClassifyPoints : public Classify
{
  TP* Points;

  ClassifyPoints(vtkPoints* pts, vtkPlane* plane, vtk3DLinearGridPlaneCutter* filter)
    : Classify(pts, plane, filter)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double p[3];
    double *n = this->Normal, *o = this->Origin;
    TP* pts = this->Points + 3 * ptId;
    unsigned char* ioa = this->InOutArray + ptId;
    double* dist = this->DistanceArray + ptId;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);
    for (; ptId < endPtId; ++ptId, ++dist)
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
      // Access each point
      p[0] = static_cast<double>(*pts);
      ++pts;
      p[1] = static_cast<double>(*pts);
      ++pts;
      p[2] = static_cast<double>(*pts);
      ++pts;

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      *dist = vtkPlane::Evaluate(n, o, p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *ioa++ = (*dist > 0.0 ? 2 : (*dist < 0.0 ? 1 : 0));
    }
  }
};

//========================= Compute edge intersections ========================
// Use vtkStaticEdgeLocatorTemplate for edge-based point merging.
template <typename IDType>
struct EdgeDataType
{
  float T;
  IDType EId;
};

template <typename IDType, typename TIP>
struct ExtractEdgesBase
{
  using EdgeTupleType = EdgeTuple<IDType, float>;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    std::vector<EdgeTupleType> LocalEdges;
    std::vector<IDType> LocalCells; // size is LocalEdges.size / 3
    CellIter LocalCellIter;

    LocalDataType() { this->LocalEdges.reserve(2048); }
  };

  const TIP* InPts;
  CellIter* Iter;
  EdgeTuple<IDType, EdgeDataType<IDType>>* Edges;
  const bool ComputeCells;
  IDType* Cells;
  vtkCellArray* Tris;
  vtkIdType NumTris;
  int NumThreadsUsed;
  double Origin[3];
  double Normal[3];
  vtk3DLinearGridPlaneCutter* Filter;

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractEdgesBase(TIP* inPts, CellIter* c, vtkPlane* plane, vtkCellArray* tris, bool computeCells,
    vtk3DLinearGridPlaneCutter* filter)
    : InPts(inPts)
    , Iter(c)
    , Edges(nullptr)
    , ComputeCells(computeCells)
    , Cells(nullptr)
    , Tris(tris)
    , NumTris(0)
    , NumThreadsUsed(0)
    , Filter(filter)
  {
    plane->GetNormal(this->Normal);
    plane->GetOrigin(this->Origin);
  }

  // Set up the iteration process
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() provided by subclass

  // Composite local thread data
  void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    vtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    for (const auto& ld : this->LocalData)
    {
      numTris += static_cast<vtkIdType>(ld.LocalEdges.size() / 3); // three edges per triangle
      this->NumThreadsUsed++;
    }

    // Allocate space for VTK triangle output.
    this->NumTris = numTris;
    this->Tris->ResizeExact(this->NumTris, 3 * this->NumTris);

    // Copy local edges to global edge array. Add in the originating edge id
    // and the original cell id used later when merging.
    const IDType edgeSize = this->NumTris * 3;
    const IDType cellSize = this->ComputeCells ? this->NumTris : 0;
    this->Edges = new EdgeTuple<IDType, EdgeDataType<IDType>>[edgeSize]; // three edges per triangle
    if (cellSize > 0)
    {
      this->Cells = new IDType[cellSize];
    }

    vtkIdType edgeNum = 0;
    for (auto& ld : this->LocalData)
    {
      vtkIdType cellNum = edgeNum / 3;
      for (const auto& lc : ld.LocalCells)
      {
        this->Cells[cellNum] = lc;
        cellNum++;
      }
      for (const auto& le : ld.LocalEdges)
      {
        this->Edges[edgeNum].V0 = le.V0;
        this->Edges[edgeNum].V1 = le.V1;
        this->Edges[edgeNum].Data.T = le.Data;
        this->Edges[edgeNum].Data.EId = edgeNum;
        edgeNum++;
      }
      std::vector<IDType>().swap(ld.LocalCells); // frees memory
      std::vector<EdgeTupleType>().swap(ld.LocalEdges);
    } // For all threads
  }   // Reduce
};    // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without a sphere tree).
template <typename IDType, typename TIP>
struct ExtractEdges : public ExtractEdgesBase<IDType, TIP>
{
  const unsigned char* InOut;
  const double* Distance;

  ExtractEdges(TIP* inPts, CellIter* c, vtkPlane* plane, unsigned char* inout, double* distance,
    vtkCellArray* tris, bool computeCells, vtk3DLinearGridPlaneCutter* filter)
    : ExtractEdgesBase<IDType, TIP>(inPts, c, plane, tris, computeCells, filter)
    , InOut(inout)
    , Distance(distance)
  {
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TIP>::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    auto& lCells = localData.LocalCells;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    const unsigned short* edges;
    double s[MAX_CELL_VERTS];
    bool isFirst = vtkSMPTools::GetSingleThread();
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
      // Does the plane cut this cell?
      if (Classify::PlaneIntersects(this->InOut, cellIter->NumVerts, c))
      {
        unsigned short isoCase;
        vtkIdType i;
        // Compute case by repeated masking with function value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = this->Distance[c[i]];
          isoCase |= (s[i] >= 0.0 ? BaseCell::Mask[i] : 0);
        }

        edges = cellIter->GetCase(isoCase);
        if (*edges > 0)
        {
          const unsigned short numEdges = *(edges++);
          // three-edges form a triangles corresponding to a cell
          assert(numEdges % 3 == 0);

          // edge info
          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            unsigned char v0 = edges[0];
            unsigned char v1 = edges[1];
            double deltaScalar = s[v1] - s[v0];
            // the t here is computed for each edges of each cell
            // so it is computed twice for most edges.
            // This could be improved by deferring the computation
            // of t to the last moment (when we are producing points / attributes)
            // This way, we should be able to compute t only once per output edge.
            double t = (deltaScalar == 0.0 ? 0.0 : (-s[v0] / deltaScalar));
            t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
            lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
          }                                       // for all edges in this case

          // cell info
          if (this->ComputeCells)
          {
            for (i = 0; i < numEdges; i += 3)
            {
              lCells.emplace_back(cellId);
            }
          }
        }                   // if contour passes through this cell
      }                     // if plane intersects
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite local thread data
  void Reduce() { this->ExtractEdgesBase<IDType, TIP>::Reduce(); } // Reduce
};                                                                 // ExtractEdges

// Produce points for non-merged points from input edge tuples. Every edge
// produces one point; three edges in a row form a triangle. The merge edges
// contain an interpolation parameter t used to interpolate point coordinates.
// into the final VTK points array. The template parameters correspond to the
// type of input and output points.
template <typename TIP, typename TOP, typename IDType>
struct ProducePoints
{
  typedef EdgeTuple<IDType, EdgeDataType<IDType>> MergeTupleType;

  const MergeTupleType* Edges;
  const TIP* InPts;
  TOP* OutPts;
  const double* Distance;
  const double* Normal;
  vtk3DLinearGridPlaneCutter* Filter;

  ProducePoints(const MergeTupleType* mt, const TIP* inPts, TOP* outPts, double* distance,
    vtkPlane* plane, vtk3DLinearGridPlaneCutter* filter)
    : Edges(mt)
    , InPts(inPts)
    , OutPts(outPts)
    , Distance(distance)
    , Normal(plane->GetNormal())
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
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
      const MergeTupleType& mergeTuple = this->Edges[ptId];
      const TIP* x0 = this->InPts + 3 * mergeTuple.V0;
      const TIP* x1 = this->InPts + 3 * mergeTuple.V1;
      double d0 = this->Distance[mergeTuple.V0];
      double d1 = this->Distance[mergeTuple.V1];
      TOP* x = this->OutPts + 3 * ptId;

      TIP p0[3], p1[3];

      // project x0 and x1 on plane to p0 and p1 respectively
      p0[0] = x0[0] - d0 * this->Normal[0];
      p0[1] = x0[1] - d0 * this->Normal[1];
      p0[2] = x0[2] - d0 * this->Normal[2];
      p1[0] = x1[0] - d1 * this->Normal[0];
      p1[1] = x1[1] - d1 * this->Normal[1];
      p1[2] = x1[2] - d1 * this->Normal[2];

      // compute intersection position based on p0 and p1
      x[0] = p0[0] + mergeTuple.Data.T * (p1[0] - p0[0]);
      x[1] = p0[1] + mergeTuple.Data.T * (p1[1] - p0[1]);
      x[2] = p0[2] + mergeTuple.Data.T * (p1[2] - p0[2]);
    }
  }
};

// Functor to build the VTK triangle list in parallel from the generated,
// non-merged edges. Every three edges represents one triangle.
struct ProduceTriangles
{
  vtkCellArray* Tris;
  vtk3DLinearGridPlaneCutter* Filter;

  ProduceTriangles(vtkCellArray* tris, vtk3DLinearGridPlaneCutter* filter)
    : Tris(tris)
    , Filter(filter)
  {
  }

  struct Impl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, vtkIdType triId, vtkIdType endTriId)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      auto offsetRange = vtk::DataArrayValueRange<1>(offsets, triId, endTriId + 1);
      ValueType offset = 3 * (triId - 1); // Incremented before first use
      std::generate(
        offsetRange.begin(), offsetRange.end(), [&]() -> ValueType { return offset += 3; });

      auto connRange = vtk::DataArrayValueRange<1>(conn, 3 * triId, 3 * endTriId);
      vtkIdType ptId = 3 * triId;
      std::iota(connRange.begin(), connRange.end(), ptId);
    }
  };

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    this->Tris->Visit(Impl{}, triId, endTriId);
  }
};

// If requested, interpolate point data attributes from non-merged
// points. The merge tuple contains an interpolation value t for the merged
// edge. Templated on type of id.
template <typename TIds>
struct ProducePDAttributes
{
  const EdgeTuple<TIds, EdgeDataType<TIds>>* Edges; // all edges
  ArrayList* Arrays;                                // the list of attributes to interpolate
  vtk3DLinearGridPlaneCutter* Filter;

  ProducePDAttributes(const EdgeTuple<TIds, EdgeDataType<TIds>>* mt, ArrayList* arrays,
    vtk3DLinearGridPlaneCutter* filter)
    : Edges(mt)
    , Arrays(arrays)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
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
      const auto& mergeTuple = this->Edges[ptId];
      TIds v0 = mergeTuple.V0;
      TIds v1 = mergeTuple.V1;
      float t = mergeTuple.Data.T;
      this->Arrays->InterpolateEdge(v0, v1, t, ptId);
    }
  }
};

// If requested, retrieve cell data attributes.
template <typename TIds>
struct ProduceCDAttributes
{
  const TIds* Cells; // original cell ids
  ArrayList* Arrays; // the list of attributes to interpolate
  vtk3DLinearGridPlaneCutter* Filter;

  ProduceCDAttributes(const TIds* c, ArrayList* arrays, vtk3DLinearGridPlaneCutter* filter)
    : Cells(c)
    , Arrays(arrays)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
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
      // retrieve CellData for the corresponding cell
      this->Arrays->Copy(this->Cells[cellId], cellId);
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
  int NumThreadsUsed; // placeholder
  vtk3DLinearGridPlaneCutter* Filter;

  ProduceMergedTriangles(const MergeTupleType* merge, const IDType* offsets, vtkIdType numTris,
    vtkCellArray* tris, vtk3DLinearGridPlaneCutter* filter)
    : MergeArray(merge)
    , Offsets(offsets)
    , NumTris(numTris)
    , Tris(tris)
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
      const IDType* offsets, const MergeTupleType* mergeArray, vtk3DLinearGridPlaneCutter* filter)
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
          const IDType connIdx = mergeArray[offsets[ptId] + i].Data.EId;
          conn->SetValue(connIdx, static_cast<ValueType>(ptId));
        } // for this group of coincident edges
      }   // for all merged points
    }
  };

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    this->Tris->Visit(Impl{}, ptId, endPtId, this->Offsets, this->MergeArray, this->Filter);
  }

  struct ReduceImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, const vtkIdType numTris)
    {
      using ValueType = typename CellStateT::ValueType;

      auto offsets = vtk::DataArrayValueRange<1>(state.GetOffsets(), 0, numTris + 1);
      ValueType offset = -3; // +=3 on first access
      std::generate(offsets.begin(), offsets.end(), [&]() -> ValueType { return offset += 3; });
    }
  };

  // Update the triangle connectivity (numPts for each triangle. This could
  // be done in parallel but it's probably not faster.
  void Reduce() { this->Tris->Visit(ReduceImpl{}, this->NumTris); }
};

// This method generates the output isosurface points. One point per
// merged edge is generated.
template <typename TIP, typename TOP, typename IDType>
struct ProduceMergedPoints
{
  typedef EdgeTuple<IDType, EdgeDataType<IDType>> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  const TIP* InPts;
  TOP* OutPts;
  const double* Distance;
  const double* Normal;
  vtk3DLinearGridPlaneCutter* Filter;

  ProduceMergedPoints(const MergeTupleType* merge, const IDType* offsets, TIP* inPts, TOP* outPts,
    double* distance, vtkPlane* plane, vtk3DLinearGridPlaneCutter* filter)
    : MergeArray(merge)
    , Offsets(offsets)
    , InPts(inPts)
    , OutPts(outPts)
    , Distance(distance)
    , Normal(plane->GetNormal())
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
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
      const MergeTupleType* mergeTuple = this->MergeArray + this->Offsets[ptId];
      const TIP* x0 = this->InPts + 3 * mergeTuple->V0;
      const TIP* x1 = this->InPts + 3 * mergeTuple->V1;
      double d0 = this->Distance[mergeTuple->V0];
      double d1 = this->Distance[mergeTuple->V1];
      TOP* x = this->OutPts + 3 * ptId;

      TIP p0[3], p1[3];

      // project x0 and x1 on plane to p0 and p1 respectively
      p0[0] = x0[0] - d0 * this->Normal[0];
      p0[1] = x0[1] - d0 * this->Normal[1];
      p0[2] = x0[2] - d0 * this->Normal[2];
      p1[0] = x1[0] - d1 * this->Normal[0];
      p1[1] = x1[1] - d1 * this->Normal[1];
      p1[2] = x1[2] - d1 * this->Normal[2];

      // compute intersection position based on p0 and p1
      x[0] = p0[0] + mergeTuple->Data.T * (p1[0] - p0[0]);
      x[1] = p0[1] + mergeTuple->Data.T * (p1[1] - p0[1]);
      x[2] = p0[2] + mergeTuple->Data.T * (p1[2] - p0[2]);
    }
  }
};

// If requested, interpolate point data attributes. The merge tuple contains an
// interpolation value t for the merged edge.
template <typename TIds>
struct ProduceMergedAttributes
{
  const EdgeTuple<TIds, EdgeDataType<TIds>>* Edges; // all edges, sorted into groups of merged edges
  const TIds* Offsets;                              // refer to single, unique, merged edge
  ArrayList* Arrays;                                // carry list of attributes to interpolate
  vtk3DLinearGridPlaneCutter* Filter;

  ProduceMergedAttributes(const EdgeTuple<TIds, EdgeDataType<TIds>>* mt, const TIds* offsets,
    ArrayList* arrays, vtk3DLinearGridPlaneCutter* filter)
    : Edges(mt)
    , Offsets(offsets)
    , Arrays(arrays)
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
      this->Arrays->InterpolateEdge(v0, v1, t, ptId);
    }
  }
};

// Wrapper to handle multiple template types for generating intersected edges
template <typename TIds>
int ProcessEdges(vtkIdType numCells, vtkPoints* inPts, CellIter* cellIter, vtkPlane* plane,
  unsigned char* inout, double* distance, vtkPoints* outPts, vtkCellArray* newPolys, bool mergePts,
  bool intAttr, bool seqProcessing, int& numThreads, vtkPointData* inPD = nullptr,
  vtkPointData* outPD = nullptr, vtkCellData* inCD = nullptr, vtkCellData* outCD = nullptr,
  vtk3DLinearGridPlaneCutter* filter = nullptr)
{
  // Extract edges that the plane intersects.
  vtkIdType numTris = 0;
  EdgeTuple<TIds, EdgeDataType<TIds>>* mergeEdges = nullptr; // may need reference counting
  TIds* originalCells = nullptr;

  const bool computeCells = (inCD != nullptr && inCD->GetNumberOfArrays() > 0);

  // Extract edges
  int ptsType = inPts->GetDataType();
  if (ptsType == VTK_FLOAT)
  {
    float* pts = static_cast<float*>(inPts->GetVoidPointer(0));
    ExtractEdges<TIds, float> extractEdges(
      pts, cellIter, plane, inout, distance, newPolys, computeCells, filter);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);
    numTris = extractEdges.NumTris;
    mergeEdges = extractEdges.Edges;
    originalCells = extractEdges.Cells;
  }
  else // if (ptsType == VTK_DOUBLE)
  {
    double* pts = static_cast<double*>(inPts->GetVoidPointer(0));
    ExtractEdges<TIds, double> extractEdges(
      pts, cellIter, plane, inout, distance, newPolys, computeCells, filter);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);
    numTris = extractEdges.NumTris;
    mergeEdges = extractEdges.Edges;
    originalCells = extractEdges.Cells;
  }
  int nt = numThreads;

  // Make sure data was produced
  if (numTris <= 0)
  {
    outPts->SetNumberOfPoints(0);
    delete[] mergeEdges;
    delete[] originalCells;
    return 1;
  }

  // There are two ways forward: do not merge coincident points; or merge
  // points. Merging typically takes longer, while the output size of
  // unmerged points is larger.
  int inPtsType = inPts->GetDataType();
  int outPtsType = outPts->GetDataType();

  if (!mergePts)
  {
    // Produce non-merged points from edges. Each edge produces one point;
    // three edges define an output triangle.
    vtkIdType numPts = 3 * numTris;
    outPts->GetData()->WriteVoidPointer(0, 3 * numPts);

    if (inPtsType == VTK_FLOAT)
    {
      float* inPtsPtr = static_cast<float*>(inPts->GetVoidPointer(0));
      if (outPtsType == VTK_FLOAT)
      {
        float* outPtsPtr = static_cast<float*>(outPts->GetVoidPointer(0));
        ProducePoints<float, float, TIds> producePoints(
          mergeEdges, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
      else // outPtsType == VTK_DOUBLE
      {
        double* outPtsPtr = static_cast<double*>(outPts->GetVoidPointer(0));
        ProducePoints<float, double, TIds> producePoints(
          mergeEdges, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
    }
    else // inPtsType == VTK_DOUBLE
    {
      double* inPtsPtr = static_cast<double*>(inPts->GetVoidPointer(0));
      if (outPtsType == VTK_FLOAT)
      {
        float* outPtsPtr = static_cast<float*>(outPts->GetVoidPointer(0));
        ProducePoints<double, float, TIds> producePoints(
          mergeEdges, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
      else // outPtsType == VTK_DOUBLE
      {
        double* outPtsPtr = static_cast<double*>(outPts->GetVoidPointer(0));
        ProducePoints<double, double, TIds> producePoints(
          mergeEdges, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
    }

    // Produce non-merged triangles from edges
    ProduceTriangles produceTris(newPolys, filter);
    EXECUTE_SMPFOR(seqProcessing, numTris, produceTris);

    // Interpolate attributes if requested
    if (intAttr)
    {
      // Point data
      if (inPD && inPD->GetNumberOfArrays() > 0)
      {
        ArrayList pointArrays;
        outPD->InterpolateAllocate(inPD, numPts);
        pointArrays.AddArrays(numPts, inPD, outPD, /*nullValue*/ 0.0, /*promote*/ false);
        ProducePDAttributes<TIds> interpolatePoints(mergeEdges, &pointArrays, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, interpolatePoints);
      }

      // Cell data
      if (inCD && inCD->GetNumberOfArrays() > 0)
      {
        ArrayList cellArrays;
        outCD->CopyAllocate(inCD, numTris);
        cellArrays.AddArrays(numTris, inCD, outCD, /*nullValue*/ 0.0, /*promote*/ false);
        ProduceCDAttributes<TIds> interpolateCells(originalCells, &cellArrays, filter);
        EXECUTE_SMPFOR(seqProcessing, numTris, interpolateCells);
      }
    }
  }
  else // generate merged output
  {
    // Merge coincident edges. The Offsets refer to the single unique edge
    // from the sorted group of duplicate edges.
    vtkIdType numPts;
    vtkStaticEdgeLocatorTemplate<TIds, EdgeDataType<TIds>> loc;
    const TIds* offsets = loc.MergeEdges(3 * numTris, mergeEdges, numPts);

    // Generate triangles from merged edges.
    ProduceMergedTriangles<TIds> produceTris(mergeEdges, offsets, numTris, newPolys, filter);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numPts, produceTris, numThreads);
    numThreads = nt;

    // Generate points (one per unique edge)
    outPts->GetData()->WriteVoidPointer(0, 3 * numPts);

    // Only handle combinations of real types
    if (inPtsType == VTK_FLOAT)
    {
      float* inPtsPtr = static_cast<float*>(inPts->GetVoidPointer(0));
      if (outPtsType == VTK_FLOAT)
      {
        float* outPtsPtr = static_cast<float*>(outPts->GetVoidPointer(0));
        ProduceMergedPoints<float, float, TIds> producePts(
          mergeEdges, offsets, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
      }
      else // outPtsType == VTK_DOUBLE
      {
        double* outPtsPtr = static_cast<double*>(outPts->GetVoidPointer(0));
        ProduceMergedPoints<float, double, TIds> producePts(
          mergeEdges, offsets, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
      }
    }
    else // inPtsType == VTK_DOUBLE
    {
      double* inPtsPtr = static_cast<double*>(inPts->GetVoidPointer(0));
      if (outPtsType == VTK_FLOAT)
      {
        float* outPtsPtr = static_cast<float*>(outPts->GetVoidPointer(0));
        ProduceMergedPoints<double, float, TIds> producePts(
          mergeEdges, offsets, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
      }
      else // outPtsType == VTK_DOUBLE
      {
        double* outPtsPtr = static_cast<double*>(outPts->GetVoidPointer(0));
        ProduceMergedPoints<double, double, TIds> producePts(
          mergeEdges, offsets, inPtsPtr, outPtsPtr, distance, plane, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
      }
    }

    // Now process point data attributes if requested
    if (intAttr)
    {
      // Point data
      if (inPD && inPD->GetNumberOfArrays() > 0)
      {
        ArrayList pointArrays;
        outPD->InterpolateAllocate(inPD, numPts);
        pointArrays.AddArrays(numPts, inPD, outPD, /*nullValue*/ 0.0, /*promote*/ false);
        ProduceMergedAttributes<TIds> interpolatePoints(mergeEdges, offsets, &pointArrays, filter);
        EXECUTE_SMPFOR(seqProcessing, numPts, interpolatePoints);
      }

      // Cell data
      if (inCD && inCD->GetNumberOfArrays() > 0)
      {
        ArrayList cellArrays;
        outCD->CopyAllocate(inCD, numTris);
        cellArrays.AddArrays(numTris, inCD, outCD, /*nullValue*/ 0.0, /*promote*/ false);
        ProduceCDAttributes<TIds> interpolateCells(originalCells, &cellArrays, filter);
        EXECUTE_SMPFOR(seqProcessing, numTris, interpolateCells);
      }
    }
  }

  // Clean up
  delete[] mergeEdges;
  delete[] originalCells;
  return 1;
}

// Functor for assigning normals at each point
struct ComputePointNormals
{
  float Normal[3];
  float* PointNormals;
  vtk3DLinearGridPlaneCutter* Filter;

  ComputePointNormals(float normal[3], float* ptNormals, vtk3DLinearGridPlaneCutter* filter)
    : PointNormals(ptNormals)
    , Filter(filter)
  {
    this->Normal[0] = normal[0];
    this->Normal[1] = normal[1];
    this->Normal[2] = normal[2];
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
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
      n[0] = this->Normal[0];
      n[1] = this->Normal[1];
      n[2] = this->Normal[2];
    }
  }

  static void Execute(bool seqProcessing, vtkPoints* pts, vtkPlane* plane, vtkPointData* pd,
    vtk3DLinearGridPlaneCutter* filter)
  {
    vtkIdType numPts = pts->GetNumberOfPoints();

    vtkFloatArray* ptNormals = vtkFloatArray::New();
    ptNormals->SetName("Normals");
    ptNormals->SetNumberOfComponents(3);
    ptNormals->SetNumberOfTuples(numPts);
    float* ptN = static_cast<float*>(ptNormals->GetVoidPointer(0));

    // Get the normal
    double dn[3];
    plane->GetNormal(dn);
    vtkMath::Normalize(dn);
    float n[3];
    n[0] = static_cast<float>(dn[0]);
    n[1] = static_cast<float>(dn[1]);
    n[2] = static_cast<float>(dn[2]);

    // Process all points, averaging normals
    ComputePointNormals compute(n, ptN, filter);
    EXECUTE_SMPFOR(seqProcessing, numPts, compute);

    // Clean up and get out
    pd->SetNormals(ptNormals);
    ptNormals->Delete();
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
// Construct an instance of the class.
vtk3DLinearGridPlaneCutter::vtk3DLinearGridPlaneCutter()
{
  this->Plane = vtkPlane::New();
  this->MergePoints = false;
  this->InterpolateAttributes = true;
  this->ComputeNormals = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;
}

//------------------------------------------------------------------------------
vtk3DLinearGridPlaneCutter::~vtk3DLinearGridPlaneCutter()
{
  this->SetPlane(nullptr);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtk3DLinearGridPlaneCutter::GetMTime()
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
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridPlaneCutter::ProcessPiece(
  vtkUnstructuredGrid* input, vtkPlane* plane, vtkPolyData* output)
{
  // Make sure there is input data to process
  if (!input || !plane || !output)
  {
    vtkLog(INFO, "Null input, plane, or output");
    return 1;
  }

  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = inPts ? inPts->GetNumberOfPoints() : 0;
  vtkCellArray* cells = input->GetCells();
  vtkIdType numCells = cells ? cells->GetNumberOfCells() : 0;
  if (numPts <= 0 || numCells <= 0)
  {
    vtkLog(INFO, "Empty input");
    return 1;
  }

  // Check the input point type. Only real types are supported.
  int inPtsType = inPts->GetDataType();
  if (inPtsType != VTK_FLOAT && inPtsType != VTK_DOUBLE)
  {
    vtkLog(ERROR, "Input point type not supported");
    return 0;
  }

  // Create the output points. Only real types are supported.
  auto outPts = vtkSmartPointer<vtkPoints>::New();
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

  // Output triangles go here.
  auto newPolys = vtkSmartPointer<vtkCellArray>::New();

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Compute plane-cut scalars
  unsigned char* inout = nullptr;
  double* distance = nullptr;
  int ptsType = inPts->GetDataType();
  if (ptsType == VTK_FLOAT)
  {
    ClassifyPoints<float> classify(inPts, plane, this);
    vtkSMPTools::For(0, numPts, classify);
    inout = classify.InOutArray;
    distance = classify.DistanceArray;
  }
  else if (ptsType == VTK_DOUBLE)
  {
    ClassifyPoints<double> classify(inPts, plane, this);
    vtkSMPTools::For(0, numPts, classify);
    inout = classify.InOutArray;
    distance = classify.DistanceArray;
  }

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  // Determine the size/type of point and cell ids needed to index points
  // and cells. Using smaller ids results in a greatly reduced memory footprint
  // and faster processing.
  this->LargeIds = (numPts >= VTK_INT_MAX || numCells >= VTK_INT_MAX);

  // Generate all of the merged points and triangles
  if (!this->LargeIds)
  {
    if (!ProcessEdges<int>(numCells, inPts, cellIter, plane, inout, distance, outPts, newPolys,
          this->MergePoints, this->InterpolateAttributes, this->SequentialProcessing,
          this->NumberOfThreadsUsed, inPD, outPD, inCD, outCD, this))
    {
      return 0;
    }
  }
  else
  {
    if (!ProcessEdges<vtkIdType>(numCells, inPts, cellIter, plane, inout, distance, outPts,
          newPolys, this->MergePoints, this->InterpolateAttributes, this->SequentialProcessing,
          this->NumberOfThreadsUsed, inPD, outPD, inCD, outCD, this))
    {
      return 0;
    }
  }

  // If requested, compute point normals. Just set the point normals to the
  // plane normal.
  if (this->ComputeNormals)
  {
    ComputePointNormals::Execute(this->SequentialProcessing, outPts, plane, outPD, this);
  }

  // Report the results of execution
  vtkLog(TRACE,
    "Created: " << outPts->GetNumberOfPoints() << " points, " << newPolys->GetNumberOfCells()
                << " triangles");

  // Clean up
  delete[] inout;
  delete[] distance;
  delete cellIter;
  output->SetPoints(outPts);
  output->SetPolys(newPolys);

  return 1;
}

//------------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int vtk3DLinearGridPlaneCutter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    if (vtkPolyData::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a vtkMultiBlockDataSet as output;
    if (vtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  vtkLog(ERROR, "Not sure what type of output to create!");
  return 0;
}

//------------------------------------------------------------------------------
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridPlaneCutter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* inputGrid =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* outputPolyData =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCompositeDataSet* inputCDS =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkMultiBlockDataSet* outputMBDS =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputPolyData == nullptr) &&
    (inputCDS == nullptr || outputMBDS == nullptr))
  {
    return 0;
  }

  // Need a plane to do the cutting
  vtkPlane* plane = this->Plane;
  if (!plane)
  {
    vtkLog(ERROR, "Cut plane not defined");
    return 0;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output vtkPolyData.
  if (inputGrid)
  {
    this->ProcessPiece(inputGrid, plane, outputPolyData);
    this->CheckAbort();
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a vtkPolyData that is added to
  // the output multiblock dataset.
  else
  {
    vtkUnstructuredGrid* grid;
    vtkPolyData* polydata;
    outputMBDS->CopyStructure(inputCDS);
    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (this->GetAbortOutput())
      {
        break;
      }
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = vtkUnstructuredGrid::SafeDownCast(ds)))
      {
        polydata = vtkPolyData::New();
        this->ProcessPiece(grid, plane, polydata);
        outputMBDS->SetDataSet(inIter, polydata);
        polydata->Delete();
      }
      else
      {
        vtkLog(INFO, "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtk3DLinearGridPlaneCutter::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtk3DLinearGridPlaneCutter::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//------------------------------------------------------------------------------
bool vtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(vtkDataObject* object)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    // Get list of cell types in the unstructured grid
    if (vtkUnsignedCharArray* cellTypes = ug->GetDistinctCellTypesArray())
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        if (cellType != VTK_VOXEL && cellType != VTK_TETRA && cellType != VTK_HEXAHEDRON &&
          cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
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
      if (!CanFullyProcessDataObject(leafDS))
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
int vtk3DLinearGridPlaneCutter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtk3DLinearGridPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";

  os << indent << "Merge Points: " << (this->MergePoints ? "true\n" : "false\n");
  os << indent
     << "Interpolate Attributes: " << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "true\n" : "false\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: " << (this->LargeIds ? "true\n" : "false\n");
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
VTK_ABI_NAMESPACE_END
