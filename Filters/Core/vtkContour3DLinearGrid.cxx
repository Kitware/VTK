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

#include "vtk3DLinearGridInternal.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourValues.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkSpanSpace.h"
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
#include <map>
#include <numeric>
#include <set>
#include <utility> //make_pair

vtkStandardNewMacro(vtkContour3DLinearGrid);
vtkCxxSetObjectMacro(vtkContour3DLinearGrid, ScalarTree, vtkScalarTree);

//-----------------------------------------------------------------------------
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
  if (!_seq)                                                                                       \
  {                                                                                                \
    vtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op(0, _num);                                                                                  \
  }

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  if (!_seq)                                                                                       \
  {                                                                                                \
    vtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op.Initialize();                                                                              \
    _op(0, _num);                                                                                  \
    _op.Reduce();                                                                                  \
  }                                                                                                \
  _nt = _op.NumThreadsUsed;

namespace
{

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
    LocalDataType() { this->LocalPts.reserve(2048); }
  };

  CellIter* Iter;
  const TIP* InPts;
  const TS* Scalars;
  double Value;
  vtkPoints* NewPts;
  vtkCellArray* NewPolys;

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  // Related to the compositing Reduce() method
  vtkIdType NumPts;
  vtkIdType NumTris;
  int NumThreadsUsed;
  vtkIdType TotalPts;  // the total points thus far (support multiple contours)
  vtkIdType TotalTris; // the total triangles thus far (support multiple contours)
  vtkTypeBool Sequential;

  ContourCellsBase(TIP* inPts, CellIter* iter, TS* s, double value, vtkPoints* outPts,
    vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris, vtkTypeBool seq)
    : Iter(iter)
    , InPts(inPts)
    , Scalars(s)
    , Value(value)
    , NewPts(outPts)
    , NewPolys(tris)
    , NumPts(0)
    , NumTris(0)
    , NumThreadsUsed(0)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
    , Sequential(seq)
  {
  }

  // Set up the iteration process.
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() method implemented by subclasses (with and without scalar tree)

  // Produce points for non-merged points. This is basically a parallel copy
  // into the final VTK points array.
  template <typename TP>
  struct ProducePoints
  {
    const std::vector<LocalPtsType*>* LocalPts;
    const std::vector<vtkIdType>* PtOffsets;
    TP* OutPts;
    ProducePoints(const std::vector<LocalPtsType*>* lp, const std::vector<vtkIdType>* o, TP* outPts)
      : LocalPts(lp)
      , PtOffsets(o)
      , OutPts(outPts)
    {
    }
    void operator()(vtkIdType threadId, vtkIdType endThreadId)
    {

      vtkIdType ptOffset;
      LocalPtsType* lPts;
      typename LocalPtsType::iterator pItr, pEnd;
      TP* pts;

      for (; threadId < endThreadId; ++threadId)
      {
        ptOffset = (*this->PtOffsets)[threadId];
        pts = this->OutPts + 3 * ptOffset;
        lPts = (*this->LocalPts)[threadId];
        pEnd = lPts->end();
        for (pItr = lPts->begin(); pItr != pEnd;)
        {
          *pts++ = *pItr++;
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
  void Reduce()
  {
    // Count the number of points. For fun keep track of the number of
    // threads used. Also keep track of the thread id so they can
    // be processed in parallel later (copy points in ProducePoints).
    vtkIdType numPts = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    std::vector<LocalPtsType*> localPts;
    std::vector<vtkIdType> localPtOffsets;
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      localPts.push_back(&((*ldItr).LocalPts));
      localPtOffsets.push_back((this->TotalPts + numPts));
      numPts += static_cast<vtkIdType>(((*ldItr).LocalPts.size() / 3)); // x-y-z components
      this->NumThreadsUsed++;
    }

    // (Re)Allocate space for output. Multiple contours require writing into
    // the end of the arrays.
    this->NumPts = numPts;
    this->NumTris = numPts / 3;
    this->NewPts->GetData()->WriteVoidPointer(0, 3 * (this->NumPts + this->TotalPts));
    TOP* pts = static_cast<TOP*>(this->NewPts->GetVoidPointer(0));
    this->NewPolys->ResizeExact(
      this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy points output to VTK structures. Only point coordinates are
    // copied for now; later we'll define the triangle topology.
    ProducePoints<TOP> producePts(&localPts, &localPtOffsets, pts);
    EXECUTE_SMPFOR(this->Sequential, this->NumThreadsUsed, producePts)

    // Now produce the output triangles (topology) for this contour n parallel
    ProduceTriangles produceTris(this->TotalTris, this->NewPolys);
    EXECUTE_SMPFOR(this->Sequential, this->NumTris, produceTris)
  } // Reduce
};  // ContourCellsBase

// Fast path operator() without scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCells : public ContourCellsBase<TIP, TOP, TS>
{
  ContourCells(TIP* inPts, CellIter* iter, TS* s, double value, vtkPoints* outPts,
    vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris, vtkTypeBool seq)
    : ContourCellsBase<TIP, TOP, TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris, seq)
  {
  }

  // Set up the iteration process.
  void Initialize() { this->ContourCellsBase<TIP, TOP, TS>::Initialize(); }

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
    const TIP* x[MAX_CELL_VERTS];

    for (; cellId < endCellId; ++cellId)
    {
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
        isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
      }
      edges = cellIter->GetCase(isoCase);

      if (*edges > 0)
      {
        numEdges = *edges++;
        for (i = 0; i < cellIter->NumVerts; ++i)
        {
          x[i] = this->InPts + 3 * c[i];
        }

        for (i = 0; i < numEdges; ++i, edges += 2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          lPts.emplace_back(x[v0][0] + t * (x[v1][0] - x[v0][0]));
          lPts.emplace_back(x[v0][1] + t * (x[v1][1] - x[v0][1]));
          lPts.emplace_back(x[v0][2] + t * (x[v1][2] - x[v0][2]));
        }                   // for all edges in this case
      }                     // if contour passes through this cell
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite results from each thread
  void Reduce() { this->ContourCellsBase<TIP, TOP, TS>::Reduce(); } // Reduce
};                                                                  // ContourCells

// Fast path operator() with a scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCellsST : public ContourCellsBase<TIP, TOP, TS>
{
  vtkScalarTree* ScalarTree;
  vtkIdType NumBatches;

  ContourCellsST(TIP* inPts, CellIter* iter, TS* s, double value, vtkScalarTree* st,
    vtkPoints* outPts, vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris, vtkTypeBool seq)
    : ContourCellsBase<TIP, TOP, TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris, seq)
    , ScalarTree(st)
  {
    //    this->ScalarTree->BuildTree();
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }

  // Set up the iteration process.
  void Initialize() { this->ContourCellsBase<TIP, TOP, TS>::Initialize(); }

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
    const TIP* x[MAX_CELL_VERTS];
    const vtkIdType* cellIds;
    vtkIdType idx, numCells;

    for (; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
          isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
        }
        edges = cellIter->GetCase(isoCase);

        if (*edges > 0)
        {
          numEdges = *edges++;
          for (i = 0; i < cellIter->NumVerts; ++i)
          {
            x[i] = this->InPts + 3 * c[i];
          }

          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            lPts.emplace_back(x[v0][0] + t * (x[v1][0] - x[v0][0]));
            lPts.emplace_back(x[v0][1] + t * (x[v1][1] - x[v0][1]));
            lPts.emplace_back(x[v0][2] + t * (x[v1][2] - x[v0][2]));
          } // for all edges in this case
        }   // if contour passes through this cell
      }     // for all cells in this batch
    }       // for each batch
  }

  // Composite results from each thread
  void Reduce() { this->ContourCellsBase<TIP, TOP, TS>::Reduce(); } // Reduce
};                                                                  // ContourCellsST

// Dispatch method for Fast path processing. Handles template dispatching etc.
template <typename TS>
void ProcessFastPath(vtkIdType numCells, vtkPoints* inPts, CellIter* cellIter, TS* s,
  double isoValue, vtkScalarTree* st, vtkPoints* outPts, vtkCellArray* tris, vtkTypeBool seq,
  int& numThreads, vtkIdType totalPts, vtkIdType totalTris)
{
  double val = static_cast<double>(isoValue);
  int inPtsType = inPts->GetDataType();
  void* inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  if (inPtsType == VTK_FLOAT && outPtsType == VTK_FLOAT)
  {
    if (st != nullptr)
    {
      ContourCellsST<float, float, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<float, float, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else if (inPtsType == VTK_DOUBLE && outPtsType == VTK_DOUBLE)
  {
    if (st != nullptr)
    {
      ContourCellsST<double, double, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<double, double, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else if (inPtsType == VTK_FLOAT && outPtsType == VTK_DOUBLE)
  {
    if (st != nullptr)
    {
      ContourCellsST<float, double, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<float, double, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else // if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_FLOAT )
  {
    if (st != nullptr)
    {
      ContourCellsST<double, float, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<double, float, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
};

//========================= GENERAL PATH (POINT MERGING) =======================
// Use vtkStaticEdgeLocatorTemplate for edge-based point merging. Processing is
// available with and without a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesBase
{
  typedef std::vector<EdgeTuple<IDType, float> > EdgeVectorType;
  typedef std::vector<MergeTuple<IDType, float> > MergeVectorType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    EdgeVectorType LocalEdges;
    CellIter LocalCellIter;

    LocalDataType() { this->LocalEdges.reserve(2048); }
  };

  CellIter* Iter;
  const TS* Scalars;
  double Value;
  MergeTuple<IDType, float>* Edges;
  vtkCellArray* Tris;
  vtkIdType NumTris;
  int NumThreadsUsed;
  vtkIdType TotalTris; // the total triangles thus far (support multiple contours)
  vtkTypeBool Sequential;

  // Keep track of generated points and triangles on a per thread basis
  vtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractEdgesBase(
    CellIter* c, TS* s, double value, vtkCellArray* tris, vtkIdType totalTris, vtkTypeBool seq)
    : Iter(c)
    , Scalars(s)
    , Value(value)
    , Edges(nullptr)
    , Tris(tris)
    , NumTris(0)
    , NumThreadsUsed(0)
    , TotalTris(totalTris)
    , Sequential(seq)
  {
  }

  // Set up the iteration process
  void Initialize()
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
    const std::vector<EdgeVectorType*>* LocalEdges;
    const std::vector<vtkIdType>* TriOffsets;
    MergeTuple<IDT, float>* OutEdges;
    ProduceEdges(const std::vector<EdgeVectorType*>* le, const std::vector<vtkIdType>* o,
      MergeTuple<IDT, float>* outEdges)
      : LocalEdges(le)
      , TriOffsets(o)
      , OutEdges(outEdges)
    {
    }
    void operator()(vtkIdType threadId, vtkIdType endThreadId)
    {
      vtkIdType triOffset, edgeNum;
      const EdgeVectorType* lEdges;
      MergeTuple<IDT, float>* edges;

      for (; threadId < endThreadId; ++threadId)
      {
        triOffset = (*this->TriOffsets)[threadId];
        edgeNum = 3 * triOffset;
        edges = this->OutEdges + edgeNum;
        lEdges = (*this->LocalEdges)[threadId];
        auto eEnd = lEdges->end();
        for (auto eItr = lEdges->begin(); eItr != eEnd; ++eItr)
        {
          edges->V0 = eItr->V0;
          edges->V1 = eItr->V1;
          edges->T = eItr->T;
          edges->EId = edgeNum;
          edges++;
          edgeNum++;
        }
      } // for all threads
    }
  };

  // Composite local thread data
  void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    vtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    std::vector<EdgeVectorType*> localEdges;
    std::vector<vtkIdType> localTriOffsets;
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      localEdges.push_back(&((*ldItr).LocalEdges));
      localTriOffsets.push_back(numTris);
      numTris +=
        static_cast<vtkIdType>(((*ldItr).LocalEdges.size() / 3)); // three edges per triangle
      this->NumThreadsUsed++;
    }

    // Allocate space for VTK triangle output. Take into account previous
    // contours.
    this->NumTris = numTris;
    this->Tris->ResizeExact(this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy local edges to composited edge array.
    this->Edges = new MergeTuple<IDType, float>[3 * this->NumTris]; // three edges per triangle
    ProduceEdges<IDType> produceEdges(&localEdges, &localTriOffsets, this->Edges);
    EXECUTE_SMPFOR(this->Sequential, this->NumThreadsUsed, produceEdges);
    // EdgeVectorType emptyVector;
    //(*ldItr).LocalEdges.swap(emptyVector); //frees memory

  } // Reduce
};  // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without scalar tree).
template <typename IDType, typename TS>
struct ExtractEdges : public ExtractEdgesBase<IDType, TS>
{
  ExtractEdges(
    CellIter* c, TS* s, double value, vtkCellArray* tris, vtkIdType totalTris, vtkTypeBool seq)
    : ExtractEdgesBase<IDType, TS>(c, s, value, tris, totalTris, seq)
  {
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TS>::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;

    for (; cellId < endCellId; ++cellId)
    {
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
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
  void Reduce() { this->ExtractEdgesBase<IDType, TS>::Reduce(); } // Reduce
};                                                                // ExtractEdges

// Generate edges using a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesST : public ExtractEdgesBase<IDType, TS>
{
  vtkScalarTree* ScalarTree;
  vtkIdType NumBatches;

  ExtractEdgesST(CellIter* c, TS* s, double value, vtkScalarTree* st, vtkCellArray* tris,
    vtkIdType totalTris, vtkTypeBool seq)
    : ExtractEdgesBase<IDType, TS>(c, s, value, tris, totalTris, seq)
    , ScalarTree(st)
  {
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TS>::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(vtkIdType batchNum, vtkIdType endBatchNum)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c;
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const vtkIdType* cellIds;
    vtkIdType idx, numCells;

    for (; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
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
  void Reduce() { this->ExtractEdgesBase<IDType, TS>::Reduce(); } // Reduce

}; // ExtractEdgesST

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProduceMergedTriangles
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  vtkIdType NumTris;
  vtkCellArray* Tris;
  vtkIdType TotalPts;
  vtkIdType TotalTris;
  int NumThreadsUsed; // placeholder

  ProduceMergedTriangles(const MergeTupleType* merge, const IDType* offsets, vtkIdType numTris,
    vtkCellArray* tris, vtkIdType totalPts, vtkIdType totalTris)
    : MergeArray(merge)
    , Offsets(offsets)
    , NumTris(numTris)
    , Tris(tris)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
    , NumThreadsUsed(1)
  {
  }

  void Initialize()
  {
    ; // without this method Reduce() is not called
  }

  struct Impl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, vtkIdType ptId, const vtkIdType endPtId,
      const vtkIdType ptOffset, const vtkIdType connOffset, const IDType* offsets,
      const MergeTupleType* mergeArray)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* conn = state.GetConnectivity();

      for (; ptId < endPtId; ++ptId)
      {
        const IDType numPtsInGroup = offsets[ptId + 1] - offsets[ptId];
        for (IDType i = 0; i < numPtsInGroup; ++i)
        {
          const IDType connIdx = mergeArray[offsets[ptId] + i].EId + connOffset;
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
    this->Tris->Visit(
      Impl{}, ptId, endPtId, this->TotalPts, 3 * this->TotalTris, this->Offsets, this->MergeArray);
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
template <typename TIP, typename TOP, typename IDType>
struct ProduceMergedPoints
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  const TIP* InPts;
  TOP* OutPts;

  ProduceMergedPoints(
    const MergeTupleType* merge, const IDType* offsets, TIP* inPts, TOP* outPts, vtkIdType totalPts)
    : MergeArray(merge)
    , Offsets(offsets)
    , InPts(inPts)
  {
    this->OutPts = outPts + 3 * totalPts;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTupleType* mergeTuple;
    IDType v0, v1;
    const TIP *x0, *x1, *inPts = this->InPts;
    TOP *x, *outPts = this->OutPts;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->MergeArray + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      x0 = inPts + 3 * v0;
      x1 = inPts + 3 * v1;
      x = outPts + 3 * ptId;
      x[0] = x0[0] + t * (x1[0] - x0[0]);
      x[1] = x0[1] + t * (x1[1] - x0[1]);
      x[2] = x0[2] + t * (x1[2] - x0[2]);
    }
  }
};

// If requested, interpolate point data attributes. The merge tuple contains an
// interpolation value t for the merged edge.
template <typename TIds>
struct ProduceAttributes
{
  const MergeTuple<TIds, float>* Edges; // all edges, sorted into groups of merged edges
  const TIds* Offsets;                  // refer to single, unique, merged edge
  ArrayList* Arrays;                    // carry list of attributes to interpolate
  vtkIdType TotalPts;                   // total points / multiple contours computed previously

  ProduceAttributes(
    const MergeTuple<TIds, float>* mt, const TIds* offsets, ArrayList* arrays, vtkIdType totalPts)
    : Edges(mt)
    , Offsets(offsets)
    , Arrays(arrays)
    , TotalPts(totalPts)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const MergeTuple<TIds, float>* mergeTuple;
    TIds v0, v1;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->Edges + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      this->Arrays->InterpolateEdge(v0, v1, t, ptId + this->TotalPts);
    }
  }
};

// Make the source code a little more readable
#define EXTRACT_MERGED(VTK_type, _type)                                                            \
  case VTK_type:                                                                                   \
  {                                                                                                \
    if (st == nullptr)                                                                             \
    {                                                                                              \
      ExtractEdges<TIds, _type> extractEdges(                                                      \
        cellIter, (_type*)s, isoValue, newPolys, totalTris, seqProcessing);                        \
      EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);                   \
      numTris = extractEdges.NumTris;                                                              \
      mergeEdges = extractEdges.Edges;                                                             \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ExtractEdgesST<TIds, _type> extractEdges(                                                    \
        cellIter, (_type*)s, isoValue, st, newPolys, totalTris, seqProcessing);                    \
      EXECUTE_REDUCED_SMPFOR(seqProcessing, extractEdges.NumBatches, extractEdges, numThreads);    \
      numTris = extractEdges.NumTris;                                                              \
      mergeEdges = extractEdges.Edges;                                                             \
    }                                                                                              \
  }                                                                                                \
  break;

// Wrapper to handle multiple template types for merged processing
template <typename TIds>
int ProcessMerged(vtkIdType numCells, vtkPoints* inPts, CellIter* cellIter, int sType, void* s,
  double isoValue, vtkPoints* outPts, vtkCellArray* newPolys, vtkTypeBool intAttr,
  vtkDataArray* inScalars, vtkPointData* inPD, vtkPointData* outPD, ArrayList* arrays,
  vtkScalarTree* st, vtkTypeBool seqProcessing, int& numThreads, vtkIdType totalPts,
  vtkIdType totalTris)
{
  // Extract edges that the contour intersects. Templated on type of scalars.
  // List below the explicit choice of scalars that can be processed.
  vtkIdType numTris = 0;
  MergeTuple<TIds, float>* mergeEdges = nullptr; // may need reference counting
  switch (sType) // process these scalar types, others could easily be added
  {
    EXTRACT_MERGED(VTK_UNSIGNED_INT, unsigned int);
    EXTRACT_MERGED(VTK_INT, int);
    EXTRACT_MERGED(VTK_FLOAT, float);
    EXTRACT_MERGED(VTK_DOUBLE, double);
    default:
      vtkGenericWarningMacro(<< "Scalar type not supported");
      return 0;
  };
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
  vtkStaticEdgeLocatorTemplate<TIds, float> loc;
  const TIds* offsets = loc.MergeEdges(3 * numTris, mergeEdges, numPts);

  // Generate triangles.
  ProduceMergedTriangles<TIds> produceTris(
    mergeEdges, offsets, numTris, newPolys, totalPts, totalTris);
  EXECUTE_REDUCED_SMPFOR(seqProcessing, numPts, produceTris, numThreads);
  numThreads = nt;

  // Generate points (one per unique edge)
  outPts->GetData()->WriteVoidPointer(0, 3 * (numPts + totalPts));
  int inPtsType = inPts->GetDataType();
  void* inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  void* outPtsPtr = outPts->GetVoidPointer(0);

  // Only handle combinations of real types
  if (inPtsType == VTK_FLOAT && outPtsType == VTK_FLOAT)
  {
    ProduceMergedPoints<float, float, TIds> producePts(
      mergeEdges, offsets, (float*)inPtsPtr, (float*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else if (inPtsType == VTK_DOUBLE && outPtsType == VTK_DOUBLE)
  {
    ProduceMergedPoints<double, double, TIds> producePts(
      mergeEdges, offsets, (double*)inPtsPtr, (double*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else if (inPtsType == VTK_FLOAT && outPtsType == VTK_DOUBLE)
  {
    ProduceMergedPoints<float, double, TIds> producePts(
      mergeEdges, offsets, (float*)inPtsPtr, (double*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else // if ( inPtsType == VTK_DOUBLE && outPtsType == VTK_FLOAT )
  {
    ProduceMergedPoints<double, float, TIds> producePts(
      mergeEdges, offsets, (double*)inPtsPtr, (float*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }

  // Now process point data attributes if requested
  if (intAttr)
  {
    if (totalPts <= 0) // first contour value generating output
    {
      outPD->InterpolateAllocate(inPD, numPts);
      outPD->RemoveArray(inScalars->GetName());
      arrays->ExcludeArray(inScalars);
      arrays->AddArrays(numPts, inPD, outPD);
    }
    else
    {
      arrays->Realloc(totalPts + numPts);
    }
    ProduceAttributes<TIds> interpolate(mergeEdges, offsets, arrays, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, interpolate);
  }

  // Clean up
  delete[] mergeEdges;
  return 1;
}
#undef EXTRACT_MERGED

// Functor for computing cell normals. Could easily be templated on output
// point type but we are trying to control object size.
struct ComputeCellNormals
{
  vtkPoints* Points;
  vtkCellArray* Tris;
  float* CellNormals;

  ComputeCellNormals(vtkPoints* pts, vtkCellArray* tris, float* cellNormals)
    : Points(pts)
    , Tris(tris)
    , CellNormals(cellNormals)
  {
  }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    auto cellIt = vtk::TakeSmartPointer(this->Tris->NewIterator());

    float* n = this->CellNormals + 3 * triId;
    double nd[3];

    vtkIdType unused = 3;
    const vtkIdType* tri = nullptr;

    for (cellIt->GoToCell(triId); cellIt->GetCurrentCellId() < endTriId; cellIt->GoToNextCell())
    {
      cellIt->GetCurrentCell(unused, tri);
      vtkTriangle::ComputeNormal(this->Points, 3, tri, nd);
      *n++ = nd[0];
      *n++ = nd[1];
      *n++ = nd[2];
    }
  }
};

// Generate normals on output triangles
vtkFloatArray* GenerateTriNormals(vtkTypeBool seqProcessing, vtkPoints* pts, vtkCellArray* tris)
{
  vtkIdType numTris = tris->GetNumberOfCells();

  vtkFloatArray* cellNormals = vtkFloatArray::New();
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numTris);
  float* n = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Execute functor over all triangles
  ComputeCellNormals computeNormals(pts, tris, n);
  EXECUTE_SMPFOR(seqProcessing, numTris, computeNormals);

  return cellNormals;
}

// Functor for averaging normals at each merged point.
template <typename TId>
struct AverageNormals
{
  vtkStaticCellLinksTemplate<TId>* Links;
  const float* CellNormals;
  float* PointNormals;

  AverageNormals(vtkStaticCellLinksTemplate<TId>* links, float* cellNormals, float* ptNormals)
    : Links(links)
    , CellNormals(cellNormals)
    , PointNormals(ptNormals)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    TId i, numTris;
    const TId* tris;
    const float* nc;
    float* n = this->PointNormals + 3 * ptId;

    for (; ptId < endPtId; ++ptId, n += 3)
    {
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
void GeneratePointNormals(vtkTypeBool seqProcessing, vtkPoints* pts, vtkCellArray* tris,
  vtkFloatArray* cellNormals, vtkPointData* pd)
{
  vtkIdType numPts = pts->GetNumberOfPoints();

  vtkFloatArray* ptNormals = vtkFloatArray::New();
  ptNormals->SetName("Normals");
  ptNormals->SetNumberOfComponents(3);
  ptNormals->SetNumberOfTuples(numPts);
  float* ptN = static_cast<float*>(ptNormals->GetVoidPointer(0));

  // Grab the computed triangle normals
  float* triN = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Build cell links
  vtkPolyData* dummy = vtkPolyData::New();
  dummy->SetPoints(pts);
  dummy->SetPolys(tris);
  vtkStaticCellLinksTemplate<TId> links;
  links.BuildLinks(dummy);

  // Process all points, averaging normals
  AverageNormals<TId> average(&links, triN, ptN);
  EXECUTE_SMPFOR(seqProcessing, numPts, average);

  // Clean up and get out
  dummy->Delete();
  pd->SetNormals(ptNormals);
  cellNormals->Delete();
  ptNormals->Delete();
};

} // anonymous namespace

// Map scalar trees to input datasets. Necessary due to potential composite
// data set input types, where each piece may have a different scalar tree.
struct vtkScalarTreeMap : public std::map<vtkUnstructuredGrid*, vtkScalarTree*>
{
};

//-----------------------------------------------------------------------------
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
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;
  this->ScalarTreeMap = new vtkScalarTreeMap;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

// Make code more readable
#define EXTRACT_FAST_PATH(VTK_SCALAR_type, _type)                                                  \
  case VTK_SCALAR_type:                                                                            \
    ProcessFastPath<_type>(numCells, inPts, cellIter, (_type*)sPtr, value, stree, outPts,          \
      newPolys, this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris);       \
    break;

//-----------------------------------------------------------------------------
// Specialized contouring filter to handle unstructured grids with 3D linear
// cells (tetrahedras, hexes, wedges, pyradmids, voxels).
//
void vtkContour3DLinearGrid::ProcessPiece(
  vtkUnstructuredGrid* input, vtkDataArray* inScalars, vtkPolyData* output)
{

  // Make sure there is data to process
  vtkCellArray* cells = input->GetCells();
  vtkIdType numPts, numCells;
  if (cells == nullptr || (numCells = cells->GetNumberOfCells()) < 1)
  {
    vtkDebugMacro(<< "No data in this piece");
    return;
  }

  // Get the contour values.
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double value, *values = this->ContourValues->GetValues();

  // Setup scalar processing
  int sType = inScalars->GetDataType();
  void* sPtr = inScalars->GetVoidPointer(0);

  // Check the input point type. Only real types are supported.
  vtkPoints* inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();
  int inPtsType = inPts->GetDataType();
  if ((inPtsType != VTK_FLOAT && inPtsType != VTK_DOUBLE))
  {
    vtkLog(ERROR, "Input point type not supported");
    return;
  }
  // Create the output points. Only real types are supported.
  vtkPoints* outPts = vtkPoints::New();
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
  inScalars->GetRange(scalarRange);
  double rangeDiff = scalarRange[1] - scalarRange[0];

  // If a scalar tree is requested, retrieve previous or if not found,
  // create a default or clone the factory.
  vtkScalarTree* stree = nullptr;
  if (this->UseScalarTree && rangeDiff > 0.0)
  {
    vtkScalarTreeMap::iterator mapIter = this->ScalarTreeMap->find(input);
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
  vtkCellArray* newPolys = vtkCellArray::New();

  // Process all contour values
  vtkIdType totalPts = 0;
  vtkIdType totalTris = 0;

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Now produce the output: fast path or general path
  int mergePoints = this->MergePoints | this->ComputeNormals | this->InterpolateAttributes;
  if (!mergePoints)
  { // fast path
    // Generate all of the points at once (for multiple contours) and then produce the triangles.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      switch (sType) // process these scalar types, others could easily be added
      {
        EXTRACT_FAST_PATH(VTK_UNSIGNED_INT, unsigned int);
        EXTRACT_FAST_PATH(VTK_INT, int);
        EXTRACT_FAST_PATH(VTK_FLOAT, float);
        EXTRACT_FAST_PATH(VTK_DOUBLE, double);
        default:
          vtkGenericWarningMacro(<< "Scalar type not supported");
          return;
      };

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    } // for all contours
  }

  else // Need to merge points, and possibly perform attribute interpolation
       // and generate normals. Hence use the slower path.
  {
    vtkPointData* inPD = input->GetPointData();
    vtkPointData* outPD = output->GetPointData();
    ArrayList arrays;

    // Determine the size/type of point and cell ids needed to index points
    // and cells. Using smaller ids results in a greatly reduced memory footprint
    // and faster processing.
    this->LargeIds = (numPts >= VTK_INT_MAX || numCells >= VTK_INT_MAX ? true : false);

    // Generate all of the merged points and triangles at once (for multiple
    // contours) and then produce the normals if requested.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      if (this->LargeIds == false)
      {
        if (!ProcessMerged<int>(numCells, inPts, cellIter, sType, sPtr, value, outPts, newPolys,
              this->InterpolateAttributes, inScalars, inPD, outPD, &arrays, stree,
              this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris))
        {
          return;
        }
      }
      else
      {
        if (!ProcessMerged<vtkIdType>(numCells, inPts, cellIter, sType, sPtr, value, outPts,
              newPolys, this->InterpolateAttributes, inScalars, inPD, outPD, &arrays, stree,
              this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris))
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
      vtkFloatArray* triNormals = GenerateTriNormals(this->SequentialProcessing, outPts, newPolys);
      if (this->LargeIds)
      {
        GeneratePointNormals<vtkIdType>(
          this->SequentialProcessing, outPts, newPolys, triNormals, outPD);
      }
      else
      {
        GeneratePointNormals<int>(this->SequentialProcessing, outPts, newPolys, triNormals, outPD);
      }
    }
  } // slower path requires point merging

  // Report the results of execution
  vtkDebugMacro(<< "Created: " << outPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // Clean up
  delete cellIter;
  output->SetPoints(outPts);
  outPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
}

//----------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int vtkContour3DLinearGrid::RequestDataObject(
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

  vtkErrorMacro("Not sure what type of output to create!");
  return 0;
}

//-----------------------------------------------------------------------------
// RequestData checks the input, manages composite data, and handles the
// (optional) scalar tree. For each input vtkUnstructuredGrid, it produces an
// output vtkPolyData piece by performing contouring on the input dataset.
//
int vtkContour3DLinearGrid::RequestData(
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

    double scalarRange[2];
    inScalars->GetRange(scalarRange);
    double rangeDiff = scalarRange[1] - scalarRange[0];

    // Use provided scalar tree if not a composite data set input and scalar array range
    // difference between min and max is non-zero.
    if (this->UseScalarTree && this->ScalarTree && rangeDiff > 0.0)
    {
      this->ScalarTreeMap->insert(std::make_pair(inputGrid, this->ScalarTree));
    }
    this->ProcessPiece(inputGrid, inScalars, outputPolyData);
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
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = vtkUnstructuredGrid::SafeDownCast(ds)))
      {
        int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
        inScalars = this->GetInputArrayToProcess(0, grid, association);
        if (!inScalars)
        {
          vtkLog(TRACE, "No scalars available");
          continue;
        }
        polydata = vtkPolyData::New();
        this->ProcessPiece(grid, inScalars, polydata);
        outputMBDS->SetDataSet(inIter, polydata);
        polydata->Delete();
      }
      else
      {
        vtkDebugMacro(<< "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkContour3DLinearGrid::SetOutputPointsPrecision(int precision)
{
  if (this->OutputPointsPrecision != precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkContour3DLinearGrid::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
bool vtkContour3DLinearGrid::CanFullyProcessDataObject(
  vtkDataObject* object, const char* scalarArrayName)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    vtkDataArray* array = ug->GetPointData()->GetArray(scalarArrayName);
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
    vtkNew<vtkCellTypes> cellTypes;
    ug->GetCellTypes(cellTypes);
    for (vtkIdType i = 0; i < cellTypes->GetNumberOfTypes(); ++i)
    {
      unsigned char cellType = cellTypes->GetCellType(i);
      if (cellType != VTK_VOXEL && cellType != VTK_TETRA && cellType != VTK_HEXAHEDRON &&
        cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
      {
        // Unsupported cell type, can't process data
        return false;
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

//-----------------------------------------------------------------------------
int vtkContour3DLinearGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
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
#undef EXTRACT_MERGED
#undef EXTRACT_FAST_PATH
