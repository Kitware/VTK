// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStaticCellLocator.h"

#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <array>
#include <queue>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStaticCellLocator);

//------------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
//
// Note that are two key classes: the vtkCellBinner and the vtkCellProcessor.
// the Binner is used to perform binning operations are cells are placed into
// the uniformly subdivided bin space. The Processor is a templated class
// (templated over ID type to reduce memory and speed sorting when the cell
// ids are small).
//
// The algorithm is multipass. First, the overall bounds of the data is
// determined, and then the space is subdivided into uniform bins. Next each
// cell is visited, it's bounds are obtained. and the ijk footprint into the
// binning is obtained. The footprint implicitly indicates the number of bins
// the cell touches (i.e., the number of cell fragment tuples
// (cellId,binId)), and this number is stored in an array. Once all cells
// have been visited (in parallel), a prefix sum is executed on the counts to
// determine the total number of fragments. Next another (parallel) pass is
// made over each cell and the fragments are placed into a tuple array (using
// the count offsets), which is then (parallel) sorted on binIds. This
// produces contiguous runs of cellIds for each bin. Finally, integral
// offsets are created that for each cell point at the beginning of each
// run.
//
// This algorithm is implemented into two parts as mentioned previously. The
// Binner is non templated and simply deals with cell bounds and eventually
// computes the number of cell fragments. Depending on the size of the
// fragment count, a templated class of either int or vtkIdType is
// created. Different types are used because of 1) significant reduction in
// memory and 2) significant speed up in the parallel sort.

//========================= CELL LOCATOR MACHINERY ====================================

// PIMPLd class which wraps binning functionality.
struct vtkCellBinner
{
  vtkStaticCellLocator* Locator; // locator
  vtkIdType NumCells;            // the number of cells to bin
  vtkIdType NumBins;
  vtkIdType NumFragments; // total number of (cellId,binId) tuples

  // These are internal data members used for performance reasons
  vtkDataSet* DataSet;
  int Divisions[3];
  double Bounds[6];
  std::shared_ptr<std::vector<double>> CellBoundsSharedPtr;
  double* CellBounds;
  std::shared_ptr<std::vector<vtkIdType>> CountsSharedPtr;
  vtkIdType* Counts;
  double H[3];
  double hX, hY, hZ;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  vtkCellBinner() = default;

  // Construction
  vtkCellBinner(vtkStaticCellLocator* loc, vtkIdType numCells, vtkIdType numBins)
  {
    this->Locator = loc;
    this->NumCells = numCells;
    this->NumBins = numBins;
    this->NumFragments = 0;
    this->DataSet = loc->GetDataSet();
    loc->GetDivisions(this->Divisions);

    // Allocate data. Note that these arrays are deleted elsewhere
    this->CellBoundsSharedPtr = std::make_shared<std::vector<double>>(numCells * 6);
    this->CellBounds = this->CellBoundsSharedPtr->data();
    this->CountsSharedPtr =
      std::make_shared<std::vector<vtkIdType>>(numCells + 1); // one extra holds total count
    this->Counts = this->CountsSharedPtr->data();

    // This is done to cause non-thread safe initialization to occur due to
    // side effects from GetCellBounds().
    this->DataSet->GetCellBounds(0, this->CellBounds);

    // Setup internal data members for more efficient processing.
    this->hX = this->H[0] = loc->H[0];
    this->hY = this->H[1] = loc->H[1];
    this->hZ = this->H[2] = loc->H[2];
    this->fX = 1.0 / loc->H[0];
    this->fY = 1.0 / loc->H[1];
    this->fZ = 1.0 / loc->H[2];
    this->bX = this->Bounds[0] = loc->Bounds[0];
    this->Bounds[1] = loc->Bounds[1];
    this->bY = this->Bounds[2] = loc->Bounds[2];
    this->Bounds[3] = loc->Bounds[3];
    this->bZ = this->Bounds[4] = loc->Bounds[4];
    this->Bounds[5] = loc->Bounds[5];
    this->xD = this->Divisions[0];
    this->yD = this->Divisions[1];
    this->zD = this->Divisions[2];
    this->xyD = this->Divisions[0] * this->Divisions[1];
  }

  ~vtkCellBinner()
  {
    this->CellBoundsSharedPtr.reset();
    this->CellBounds = nullptr;
    this->CountsSharedPtr.reset();
    this->Counts = nullptr;
  }

  void GetBinIndices(const double* x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    ijk[0] = static_cast<int>(((x[0] - bX) * fX));
    ijk[1] = static_cast<int>(((x[1] - bY) * fY));
    ijk[2] = static_cast<int>(((x[2] - bZ) * fZ));

    ijk[0] = (ijk[0] < 0 ? 0 : (ijk[0] >= xD ? xD - 1 : ijk[0]));
    ijk[1] = (ijk[1] < 0 ? 0 : (ijk[1] >= yD ? yD - 1 : ijk[1]));
    ijk[2] = (ijk[2] < 0 ? 0 : (ijk[2] >= zD ? zD - 1 : ijk[2]));
  }

  void GetBinIndices(vtkIdType binId, int ijk[3]) const
  {
    ijk[0] = binId % xD;
    vtkIdType tmp = binId / xD;
    ijk[1] = tmp % yD;
    ijk[2] = tmp / yD;
  }

  // Given a point x, determine which bin it is in. Note that points
  // are clamped to lie inside of the locator.
  vtkIdType GetBinIndex(const double* x) const
  {
    int ijk[3];
    this->GetBinIndices(x, ijk);
    return ijk[0] + ijk[1] * xD + ijk[2] * xyD;
  }

  vtkIdType GetBinIndex(int ijk[3]) const { return ijk[0] + ijk[1] * xD + ijk[2] * xyD; }

  // These are helper functions
  vtkIdType CountBins(const int ijkMin[3], const int ijkMax[3])
  {
    // Ensure all temporary values are vtkIdType:
    vtkIdType result = ijkMax[0] - ijkMin[0] + 1;
    result *= ijkMax[1] - ijkMin[1] + 1;
    result *= ijkMax[2] - ijkMin[2] + 1;

    return result;
  }

  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    double* bds = this->CellBounds + cellId * 6;
    vtkIdType* counts = this->Counts + cellId;
    double xmin[3], xmax[3];
    int ijkMin[3], ijkMax[3];

    for (; cellId < endCellId; ++cellId, bds += 6)
    {
      this->DataSet->GetCellBounds(cellId, bds);
      xmin[0] = bds[0];
      xmin[1] = bds[2];
      xmin[2] = bds[4];
      xmax[0] = bds[1];
      xmax[1] = bds[3];
      xmax[2] = bds[5];

      this->GetBinIndices(xmin, ijkMin);
      this->GetBinIndices(xmax, ijkMax);

      *counts++ = this->CountBins(ijkMin, ijkMax);
    }
  }

  void Reduce()
  {
    // Perform prefix sum
    vtkIdType* counts = this->Counts;
    vtkIdType numBins, total = 0, numCells = this->NumCells;
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      numBins = *counts;
      *counts++ = total;
      total += numBins;
    }
    this->NumFragments = total;
  }

}; // vtkCellBinner

//------------------------------------------------------------------------------
// The following tuple is what is sorted in the map. Note that it is templated
// because depending on the number of points / buckets to process we may want
// to use vtkIdType. Otherwise for performance reasons it's best to use an int
// (or other integral type). Typically sort() is 25-30% faster on smaller
// integral types, plus it takes a heck less memory (when vtkIdType is 64-bit
// and int is 32-bit).
template <typename TId>
struct CellFragments
{
  TId CellId; // originating cell id
  TId BinId;  // i-j-k index into bin space

  // Operator< used to support the subsequent sort operation.
  bool operator<(const CellFragments& tuple) const { return BinId < tuple.BinId; }
};

// Perform locator operations like FindCell. Uses templated subclasses
// to reduce memory and enhance speed.
struct vtkCellProcessor
{
  vtkCellBinner* Binner;
  vtkDataSet* DataSet;
  double* Bounds;
  double* CellBounds;
  vtkIdType* Counts;
  vtkIdType NumFragments;
  vtkIdType NumCells;
  vtkIdType NumBins;
  int BatchSize;
  int NumBatches;
  vtkIdType xD, xyD;
  size_t MaxCellSize;

  vtkCellProcessor() = default;

  vtkCellProcessor(vtkCellBinner* cb)
    : Binner(cb)
  {
    this->DataSet = cb->DataSet;
    this->Bounds = cb->Bounds;
    this->CellBounds = cb->CellBounds;
    this->Counts = cb->Counts;
    this->NumCells = cb->NumCells;
    this->NumFragments = cb->NumFragments;
    this->NumBins = cb->NumBins;
    this->BatchSize = 10000; // building the offset array
    this->NumBatches =
      static_cast<int>(std::ceil(static_cast<double>(this->NumFragments) / this->BatchSize));

    this->xD = cb->xD; // for speeding up computation
    this->xyD = cb->xyD;
    this->MaxCellSize = static_cast<size_t>(cb->DataSet->GetMaxCellSize());
  }

  virtual ~vtkCellProcessor() = default;

  // Satisfy cell locator API
  virtual vtkIdType FindCell(
    const double pos[3], vtkGenericCell* cell, int& subId, double pcoords[3], double* weights) = 0;
  virtual void FindCellsWithinBounds(double* bbox, vtkIdList* cells) = 0;
  virtual void FindCellsAlongPlane(
    const double o[3], const double n[3], double tolerance, vtkIdList* cells) = 0;
  virtual int IntersectWithLine(const double a0[3], const double a1[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) = 0;
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol,
    vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell) = 0;
  virtual bool InsideCellBounds(const double x[3], vtkIdType cellId) = 0;
  virtual vtkIdType FindClosestPointWithinRadius(const double x[3], double radius,
    double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2,
    int& inside) = 0;

  // Convenience for computing
  virtual int IsEmpty(vtkIdType binId) = 0;
};

namespace
{ // anonymous to wrap non-public stuff

// Typed subclass
template <typename T>
struct CellProcessor : public vtkCellProcessor
{
  // Type dependent members
  std::shared_ptr<std::vector<CellFragments<T>>> MapSharedPtr; // SharedPtr of map
  CellFragments<T>* Map;                                       // the map to be sorted
  std::shared_ptr<std::vector<T>> OffsetsShardPtr;             // SharedPtr of offsets
  T* Offsets;                                                  // offsets for each bin into the map

  CellProcessor() = default;

  CellProcessor(vtkCellBinner* cb)
    : vtkCellProcessor(cb)
  {
    // Prepare to sort
    // one extra to simplify traversal
    this->MapSharedPtr = std::make_shared<std::vector<CellFragments<T>>>(this->NumFragments + 1);
    this->Map = this->MapSharedPtr->data();
    this->Map[this->NumFragments].BinId = this->NumBins;
    this->OffsetsShardPtr = std::make_shared<std::vector<T>>(this->NumBins + 1);
    this->Offsets = this->OffsetsShardPtr->data();
    this->Offsets[this->NumBins] = this->NumFragments;
  }

  ~CellProcessor() override
  {
    this->MapSharedPtr.reset();
    this->Map = nullptr;
    this->OffsetsShardPtr.reset();
    this->Offsets = nullptr;
  }

  // The number of cell ids in a bin is determined by computing the
  // difference between the offsets into the sorted cell fragments array.
  T GetNumberOfIds(vtkIdType binNum) { return (this->Offsets[binNum + 1] - this->Offsets[binNum]); }

  // Given a bin number, return the cells ids in that bin.
  const CellFragments<T>* GetIds(vtkIdType binNum) { return this->Map + this->Offsets[binNum]; }

  void ComputeBinBounds(int i, int j, int k, double binBounds[6])
  {
    double* bds = this->Binner->Bounds;
    double* h = this->Binner->H;
    binBounds[0] = bds[0] + i * h[0];
    binBounds[1] = binBounds[0] + h[0];
    binBounds[2] = bds[2] + j * h[1];
    binBounds[3] = binBounds[2] + h[1];
    binBounds[4] = bds[4] + k * h[2];
    binBounds[5] = binBounds[4] + h[2];
  }

  static bool IsInBounds(const double bounds[6], const double x[3], double tol = 0.0)
  {
    return (bounds[0] - tol) <= x[0] && x[0] <= (bounds[1] + tol) && (bounds[2] - tol) <= x[1] &&
      x[1] <= (bounds[3] + tol) && (bounds[4] - tol) <= x[2] && x[2] <= (bounds[5] + tol);
  }

  // Methods to satisfy vtkCellProcessor virtual API
  vtkIdType FindCell(const double pos[3], vtkGenericCell* cell, int& subId, double pcoords[3],
    double* weights) override;
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;
  void FindCellsAlongPlane(
    const double o[3], const double n[3], double tolerance, vtkIdList* cells) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;
  bool InsideCellBounds(const double x[3], vtkIdType cellId) override;
  vtkIdType FindClosestPointWithinRadius(const double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;
  int IsEmpty(vtkIdType binId) override
  {
    return (this->GetNumberOfIds(static_cast<T>(binId)) > 0 ? 0 : 1);
  }

  // This functor is used to perform the final cell binning
  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    const double* bds = this->CellBounds + cellId * 6;
    CellFragments<T>* t = this->Map + *(this->Counts + cellId);
    double xmin[3], xmax[3];
    int ijkMin[3], ijkMax[3];
    int i, j, k;
    vtkIdType binId;

    for (; cellId < endCellId; ++cellId, bds += 6)
    {
      xmin[0] = bds[0];
      xmin[1] = bds[2];
      xmin[2] = bds[4];
      xmax[0] = bds[1];
      xmax[1] = bds[3];
      xmax[2] = bds[5];

      this->Binner->GetBinIndices(xmin, ijkMin);
      this->Binner->GetBinIndices(xmax, ijkMax);

      for (k = ijkMin[2]; k <= ijkMax[2]; ++k)
      {
        for (j = ijkMin[1]; j <= ijkMax[1]; ++j)
        {
          for (i = ijkMin[0]; i <= ijkMax[0]; ++i)
          {
            binId = i + j * xD + k * xyD;
            t->CellId = cellId;
            t->BinId = binId;
            t++;
          }
        }
      }
    }
  }

  void Reduce() {}
}; // CellProcessor

// This functor class creates offsets for each cell into the sorted tuple
// array. The offsets enable random access to cells.
template <typename TId>
struct MapOffsets
{
  CellProcessor<TId>* Processor;
  CellFragments<TId>* Map;
  TId* Offsets;
  vtkIdType NumCells;
  int NumBins;
  vtkIdType NumFragments;
  int BatchSize;

  MapOffsets(CellProcessor<TId>* p)
    : Processor(p)
  {
    this->Map = p->Map;
    this->Offsets = p->Offsets;
    this->NumCells = p->NumCells;
    this->NumBins = p->NumBins;
    this->NumFragments = p->NumFragments;
    this->BatchSize = p->BatchSize;
  }

  // Traverse sorted points (i.e., tuples) and update bin offsets.
  void operator()(vtkIdType batch, vtkIdType batchEnd)
  {
    TId* offsets = this->Offsets;
    const CellFragments<TId>* curPt = this->Map + batch * this->BatchSize;
    const CellFragments<TId>* endBatchPt = this->Map + batchEnd * this->BatchSize;
    const CellFragments<TId>* endPt = this->Map + this->NumFragments;
    const CellFragments<TId>* prevPt;
    endBatchPt = (endBatchPt > endPt ? endPt : endBatchPt);

    // Special case at the very beginning of the mapped points array.  If
    // the first point is in bin# N, then all bins up and including
    // N must refer to the first point.
    if (curPt == this->Map)
    {
      prevPt = this->Map;
      std::fill_n(offsets, curPt->BinId + 1, 0); // point to the first points
    } // at the very beginning of the map (sorted points array)

    // We are entering this functor somewhere in the interior of the
    // mapped points array. All we need to do is point to the entry
    // position because we are interested only in prevPt->BinId.
    else
    {
      prevPt = curPt;
    } // else in the middle of a batch

    // Okay we have a starting point for a bin run. Now we can begin
    // filling in the offsets in this batch. A previous thread should
    // have/will have completed the previous and subsequent runs outside
    // of the [batch,batchEnd) range
    for (curPt = prevPt; curPt < endBatchPt;)
    {
      for (; curPt->BinId == prevPt->BinId && curPt <= endBatchPt; ++curPt)
      {
        // advance
      }
      // Fill in any gaps in the offset array
      std::fill_n(offsets + prevPt->BinId + 1, curPt->BinId - prevPt->BinId, curPt - this->Map);
      prevPt = curPt;
    } // for all batches in this range
  }   // operator()

}; // MapOffsets

//------------------------------------------------------------------------------
template <typename T>
vtkIdType CellProcessor<T>::FindCell(
  const double pos[3], vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  // check if pos outside of bounds
  if (!CellProcessor::IsInBounds(this->Bounds, pos))
  {
    return -1;
  }
  vtkIdType binId = this->Binner->GetBinIndex(pos);
  T numIds = this->GetNumberOfIds(binId);

  // Only thread the evaluation if enough cells need to be processed
  if (numIds < 1)
  {
    return -1;
  }
  // Run through serially. A parallel implementation is possible but does
  // not seem to be much faster.
  else
  {
    const CellFragments<T>* cellIds = this->GetIds(binId);
    double dist2;
    T cellId;

    for (T j = 0; j < numIds; j++)
    {
      cellId = cellIds[j].CellId;

      if (this->InsideCellBounds(pos, cellId))
      {
        this->DataSet->GetCell(cellId, cell);
        if (cell->EvaluatePosition(pos, nullptr, subId, pcoords, dist2, weights) == 1)
        {
          return cellId;
        }
      } // in bounding box
    }   // for cells in this bin

    return -1; // nothing found
  }            // serial
}

//------------------------------------------------------------------------------
template <typename T>
void CellProcessor<T>::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  vtkIdType binNum, numIds, jOffset, kOffset;
  int i, j, k, ii, ijkMin[3], ijkMax[3];
  double pMin[3], pMax[3];
  const CellFragments<T>* ids;

  // Initialize the list of cells
  if (!cells)
  {
    return;
  }
  cells->Reset();

  // Get the locator locations for the two extreme corners of the bounding box
  pMin[0] = bbox[0];
  pMin[1] = bbox[2];
  pMin[2] = bbox[4];
  pMax[0] = bbox[1];
  pMax[1] = bbox[3];
  pMax[2] = bbox[5];

  this->Binner->GetBinIndices(pMin, ijkMin);
  this->Binner->GetBinIndices(pMax, ijkMax);

  // Loop over the block of bins and add cells that have not yet been visited.
  for (k = ijkMin[2]; k <= ijkMax[2]; ++k)
  {
    kOffset = k * this->xyD;
    for (j = ijkMin[1]; j <= ijkMax[1]; ++j)
    {
      jOffset = j * this->xD;
      for (i = ijkMin[0]; i <= ijkMax[0]; ++i)
      {
        binNum = i + jOffset + kOffset;

        if ((numIds = this->GetNumberOfIds(binNum)) > 0)
        {
          ids = this->GetIds(binNum);
          for (ii = 0; ii < numIds; ii++)
          {
            // Could use query mechanism to speed up at some point
            cells->InsertUniqueId(ids[ii].CellId);
          } // for all points in bucket
        }   // if points in bucket
      }     // i-footprint
    }       // j-footprint
  }         // k-footprint
}

//------------------------------------------------------------------------------
struct IntersectionInfo
{
  vtkIdType CellId;
  std::array<double, 3> IntersectionPoint;
  double T;

  IntersectionInfo(vtkIdType cellId, double x[3], double t)
    : CellId(cellId)
    , IntersectionPoint({ x[0], x[1], x[2] })
    , T(t)
  {
  }
};

//------------------------------------------------------------------------------
// This code traverses the cell locator by following the intersection ray. All
// cells in intersected bins are placed into the output cellId vtkIdList. See
// the IntersectWithLine method for more information on voxel traversal.
template <typename T>
int CellProcessor<T>::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  // Initialize the list of points/cells
  if (points)
  {
    points->Reset();
  }
  if (cellIds)
  {
    cellIds->Reset();
  }

  // Make sure the bounding box of the locator is hit. Also, determine the
  // entry and exit points into and out of the locator. This is used to
  // determine the bins where the ray starts and ends.
  double* bounds = this->Binner->Bounds;
  double t0, t1, x0[3], x1[3], rayDir[3], t, x[3], pcoords[3];
  int plane0, plane1, subId;
  vtkMath::Subtract(p2, p1, rayDir);

  if (vtkBox::IntersectWithLine(bounds, p1, p2, t0, t1, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Okay process line
  int* ndivs = this->Binner->Divisions;
  vtkIdType prod = ndivs[0] * ndivs[1];
  double* h = this->Binner->H;
  T i, numCellsInBin;
  int ijk[3], ijkEnd[3];
  vtkIdType idx, cId;
  double hitCellBoundsPosition[3], tHitCell;
  double step[3], next[3], tMax[3], tDelta[3];
  double binBounds[6];

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->NumCells, false);

  // Get the i-j-k point of intersection and bin index. This is
  // clamped to the boundary of the locator. Also get the exit bin
  // of the line.
  this->Binner->GetBinIndices(x0, ijk);
  this->Binner->GetBinIndices(x1, ijkEnd);
  idx = ijk[0] + ijk[1] * ndivs[0] + ijk[2] * prod;

  // Set up some traversal parameters for traversing through bins
  step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
  step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;
  step[2] = (rayDir[2] >= 0.0) ? 1.0 : -1.0;

  // If the ray is going in the negative direction, then the next voxel boundary
  // is on the "-" direction so we stay in the current voxel.
  next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ijk[0] + step[0]) : ijk[0]);
  next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ijk[1] + step[1]) : ijk[1]);
  next[2] = bounds[4] + h[2] * (rayDir[2] >= 0.0 ? (ijk[2] + step[2]) : ijk[2]);

  tMax[0] = (rayDir[0] != 0.0) ? (next[0] - x0[0]) / rayDir[0] : VTK_FLOAT_MAX;
  tMax[1] = (rayDir[1] != 0.0) ? (next[1] - x0[1]) / rayDir[1] : VTK_FLOAT_MAX;
  tMax[2] = (rayDir[2] != 0.0) ? (next[2] - x0[2]) / rayDir[2] : VTK_FLOAT_MAX;

  tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
  tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;
  tDelta[2] = (rayDir[2] != 0.0) ? (h[2] / rayDir[2]) * step[2] : VTK_FLOAT_MAX;

  // we will sort intersections by t, so keep track using these lists
  std::vector<IntersectionInfo> cellIntersections;
  // Start walking through the bins, continue until traversed the entire
  // locator. Note that termination of the while(1) loop occurs when the ray
  // passes out of the locator via the break command.
  while (true)
  {
    if ((numCellsInBin = this->GetNumberOfIds(idx)) > 0) // there are some cell here
    {
      const CellFragments<T>* cellFragments = this->GetIds(idx);
      this->ComputeBinBounds(ijk[0], ijk[1], ijk[2], binBounds);
      for (i = 0; i < numCellsInBin; i++)
      {
        cId = cellFragments[i].CellId;
        if (!cellHasBeenVisited[cId])
        {
          cellHasBeenVisited[cId] = true;

          // check whether we intersect the cell bounds
          int hitCellBounds = vtkBox::IntersectBox(
            this->CellBounds + (6 * cId), p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

          if (hitCellBounds)
          {
            // Note because of cellHasBeenVisited[], we know this cId is unique
            if (cell)
            {
              this->DataSet->GetCell(cId, cell);
              if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
              {
                // Make sure that intersection occurs within this bin or else spurious cell
                // intersections can occur behind this bin which are not the correct answer.
                if (!CellProcessor::IsInBounds(binBounds, x, tol))
                {
                  cellHasBeenVisited[cId] = false; // mark the cell non-visited
                }
                else
                {
                  cellIntersections.emplace_back(cId, x, t);
                }
              }
            }
            else
            {
              cellIntersections.emplace_back(cId, hitCellBoundsPosition, tHitCell);
            }
          } // if (hitCellBounds)
        }   // if (!cellHasBeenVisited[cId])
      }     // over all cells in bin
    }       // if cells in bin

    // See if the traversal is complete (reached the end of the line).
    if (ijk[0] == ijkEnd[0] && ijk[1] == ijkEnd[1] && ijk[2] == ijkEnd[2])
    {
      break;
    }

    // Advance to the next voxel
    if (tMax[0] < tMax[1])
    {
      if (tMax[0] < tMax[2])
      {
        ijk[0] += static_cast<int>(step[0]);
        tMax[0] += tDelta[0];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }
    else
    {
      if (tMax[1] < tMax[2])
      {
        ijk[1] += static_cast<int>(step[1]);
        tMax[1] += tDelta[1];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }

    // If exiting the locator, we are done
    if (ijk[0] < 0 || ijk[0] >= ndivs[0] || ijk[1] < 0 || ijk[1] >= ndivs[1] || ijk[2] < 0 ||
      ijk[2] >= ndivs[2])
    {
      break;
    }
    else
    {
      idx = ijk[0] + ijk[1] * ndivs[0] + ijk[2] * prod;
    }
  } // while still in locator

  // if we had intersections, sort them by increasing t
  if (!cellIntersections.empty())
  {
    T numIntersections = static_cast<T>(cellIntersections.size());
    std::sort(cellIntersections.begin(), cellIntersections.end(),
      [&](const IntersectionInfo& a, const IntersectionInfo& b) { return a.T < b.T; });
    if (points)
    {
      points->SetNumberOfPoints(numIntersections);
      for (i = 0; i < numIntersections; ++i)
      {
        points->SetPoint(i, cellIntersections[i].IntersectionPoint.data());
      }
    }
    if (cellIds)
    {
      cellIds->SetNumberOfIds(numIntersections);
      for (i = 0; i < numIntersections; ++i)
      {
        cellIds->SetId(i, cellIntersections[i].CellId);
      }
    }
    return 1;
  }
  return 0;
}

// This functor identifies candidate cells as to whether they may intersect
// a specified plane. Locator bins are culled first, and if they intersect
// the plane, then the cell bounding boxes are used.
template <typename TId>
struct CellPlaneCandidates
{
  CellProcessor<TId>* Processor;
  const vtkCellBinner* Binner;
  double Origin[3];
  double Normal[3];
  unsigned char* CellVisited;
  double BinOffsetX, BinOffsetY, BinOffsetZ;
  double BinRadius;

  CellPlaneCandidates(CellProcessor<TId>* p, const vtkCellBinner* b, const double* o,
    const double* n, unsigned char* visited)
    : Processor(p)
    , Binner(b)
    , CellVisited(visited)
  {
    this->Origin[0] = o[0];
    this->Origin[1] = o[1];
    this->Origin[2] = o[2];

    this->Normal[0] = n[0];
    this->Normal[1] = n[1];
    this->Normal[2] = n[2];
    vtkMath::Normalize(this->Normal);

    // Offset from the bin origin to the bin center
    this->BinOffsetX = this->Binner->hX / 2.0;
    this->BinOffsetY = this->Binner->hY / 2.0;
    this->BinOffsetZ = this->Binner->hZ / 2.0;

    // The BinRadius is used to cull bins quickly. It's a variant of a sphere
    // tree test (with the center of a sphere corresponding to the center of a
    // bin). Note that the plane orientation affects the radius: the end
    // result is that a smaller sphere radius can typically be used (as
    // compared to using the 0.5*(diagonal length) of a bin). This radius
    // needs only to be computed once since the relative orientation of each
    // bin to the plane is unchanged during processing. The bin radius is
    // simply the maximum distance that one of the eight bin corner points is
    // away from a plane passing through the center of the bin.
    double x[3], d, dMax = 0.0;
    x[0] = -this->BinOffsetX;
    x[1] = -this->BinOffsetY;
    x[2] = -this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal); // simplified because plane passes through origin
    dMax = (d > dMax ? d : dMax);

    x[0] = this->BinOffsetX;
    x[1] = -this->BinOffsetY;
    x[2] = -this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = -this->BinOffsetX;
    x[1] = this->BinOffsetY;
    x[2] = -this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = this->BinOffsetX;
    x[1] = this->BinOffsetY;
    x[2] = -this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = -this->BinOffsetX;
    x[1] = -this->BinOffsetY;
    x[2] = this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = this->BinOffsetX;
    x[1] = -this->BinOffsetY;
    x[2] = this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = -this->BinOffsetX;
    x[1] = this->BinOffsetY;
    x[2] = this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    x[0] = this->BinOffsetX;
    x[1] = this->BinOffsetY;
    x[2] = this->BinOffsetZ;
    d = vtkMath::Dot(x, this->Normal);
    dMax = (d > dMax ? d : dMax);

    this->BinRadius = dMax;
  }

  // Operate on slabs (e.g., z-slab) of bins. The algorithm works by checking
  // whether the current bin is intersected by the plane; if it is, then the
  // cell bounding box is evaluated as well. Note a potential data race
  // situation exists since a cell may be marked simultaneously (using the
  // same value). Since the same value will always be written this has never
  // been found to be an issue, although it is a potential problem.
  void operator()(vtkIdType k, vtkIdType kEnd)
  {
    double *o = this->Origin, *n = this->Normal;
    double center[3], d, *bounds;
    vtkIdType i, j, iEnd = this->Binner->Divisions[0], jEnd = this->Binner->Divisions[1];

    // Over all z slabs
    for (; k < kEnd; ++k)
    {
      center[2] = this->Binner->Bounds[4] + k * this->Binner->hZ + this->BinOffsetZ;
      for (j = 0; j < jEnd; ++j)
      {
        center[1] = this->Binner->Bounds[2] + j * this->Binner->hY + this->BinOffsetY;
        for (i = 0; i < iEnd; ++i)
        {
          center[0] = this->Binner->Bounds[0] + i * this->Binner->hX + this->BinOffsetX;

          // Now see if the bin could be intersected by the plane. It's a pseudo
          // sphere tree operation.
          d = vtkPlane::DistanceToPlane(center, n, o);
          if (d <= this->BinRadius)
          {
            vtkIdType cId, ii, numCellsInBin;
            vtkIdType bin = i + j * this->Binner->xD + k * this->Binner->xyD;
            if ((numCellsInBin = this->Processor->GetNumberOfIds(bin)) >
              0) // there are some cell here
            {
              const CellFragments<TId>* cellIds = this->Processor->GetIds(bin);
              for (ii = 0; ii < numCellsInBin; ii++)
              {
                cId = cellIds[ii].CellId;
                if (!this->CellVisited[cId])
                {
                  bounds = this->Processor->CellBounds + (6 * cId);
                  this->CellVisited[cId] = (vtkBox::IntersectWithPlane(bounds, o, n) ? 2 : 1);
                }
              }
            }
          } // if bin intersects
        }   // i
      }     // j
    }       // k
  }         // operator()
};

//------------------------------------------------------------------------------
// This code evaluates cells in intersecting bins and places them in the
// output list.
template <typename T>
void CellProcessor<T>::FindCellsAlongPlane(
  const double o[3], const double n[3], double vtkNotUsed(tol), vtkIdList* cells)
{
  // Initialize the list of cells
  if (!cells)
  {
    return;
  }
  cells->Reset();

  // Make sure that the bounding box of the locator is intersected.
  double* bounds = this->Binner->Bounds;
  double origin[3] = { o[0], o[1], o[2] };
  double normal[3] = { n[0], n[1], n[2] };
  if (vtkBox::IntersectWithPlane(bounds, origin, normal) == 0)
  {
    return;
  }

  // Okay now evaluate which bins intersect the plane, and then the cells in
  // the bins. We do this in parallel and mark the cells. Later the marked
  // cells will be added (serially) to the output list. cellHasBeenVisited
  // has three states: 0(not visited), 1(visited but not intersecting),
  // 2(visited and potential intersection candidate).
  std::vector<unsigned char> cellHasBeenVisited(this->NumCells, 0);

  // For now we will parallelize over z-slabs of bins.
  CellPlaneCandidates<T> cellCandidates(this, this->Binner, o, n, cellHasBeenVisited.data());
  vtkSMPTools::For(0, this->Binner->Divisions[2], cellCandidates);

  // Populate the output list.
  for (vtkIdType cellId = 0; cellId < this->NumCells; ++cellId)
  {
    if (cellHasBeenVisited[cellId] >= 2) // candidate
    {
      cells->InsertNextId(cellId);
    }
  }
}

//
//------------------------------------------------------------------------------
// Calculate the distance between the point x and the specified bounds
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make it 25% slower!!!!
// Return closest point (if any) AND the cell on which this closest point lies
double Distance2ToBounds(const double x[3], const double bounds[6])
{
  // Are we within the bounds?
  if (x[0] >= bounds[0] && x[0] <= bounds[1] && x[1] >= bounds[2] && x[1] <= bounds[3] &&
    x[2] >= bounds[4] && x[2] <= bounds[5])
  {
    return 0.0;
  }
  double deltas[3];
  deltas[0] = x[0] < bounds[0] ? bounds[0] - x[0] : (x[0] > bounds[1] ? x[0] - bounds[1] : 0.0);
  deltas[1] = x[1] < bounds[2] ? bounds[2] - x[1] : (x[1] > bounds[3] ? x[1] - bounds[3] : 0.0);
  deltas[2] = x[2] < bounds[4] ? bounds[4] - x[2] : (x[2] > bounds[5] ? x[2] - bounds[5] : 0.0);
  return vtkMath::SquaredNorm(deltas);
}

//------------------------------------------------------------------------------
// Return closest point (if any) AND the cell on which this closest point lies
template <typename T>
vtkIdType CellProcessor<T>::FindClosestPointWithinRadius(const double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& closestCellId, int& closestSubId,
  double& minDist2, int& inside)
{
  std::vector<bool> binHasBeenQueued(this->NumBins, false);
  std::vector<double> weights(this->MaxCellSize);
  double pcoords[3], point[3], bds[6], *bounds;
  double distance2ToCellBounds, dist2, binDist2;
  int subId, stat, ijk[3];
  vtkIdType retVal = 0;
  T numIds, j, cellId;

  using node = std::pair<double, vtkIdType>;
  std::priority_queue<node, std::vector<node>, std::greater<>> queue;

  // first get ijk containing point
  vtkIdType binId = this->Binner->GetBinIndex(x);
  queue.emplace(0.0, binId);
  binHasBeenQueued[binId] = true;

  // minimum squared distance to the closest point
  minDist2 = radius * radius;

  // Process the queue of candidate bins until the candidate bins are
  // further away than the current closest point.
  while (!queue.empty())
  {
    auto& top = queue.top();
    binId = top.second;
    binDist2 = top.first;

    // stop if bounding box is further away than current closest point
    if (binDist2 > minDist2)
    {
      break;
    }
    // perform pop after ensuring that the bin is within the bounds
    queue.pop();

    // compute distance to cells in bin, if any
    numIds = this->GetNumberOfIds(binId);
    if (numIds >= 1)
    {
      const CellFragments<T>* cellIds = this->GetIds(binId);

      for (j = 0; j < numIds; j++)
      {
        cellId = cellIds[j].CellId;

        // compute distance to cell bounding box
        bounds = this->CellBounds + 6 * cellId;
        distance2ToCellBounds = Distance2ToBounds(x, bounds);

        // compute distance to cell only if distance to bounding box smaller than minDist2
        if (distance2ToCellBounds < minDist2)
        {
          this->DataSet->GetCell(cellId, cell);

          // evaluate the position to find the closest point
          // stat==(-1) is numerical error; stat==0 means outside;
          // stat=1 means inside. However, for real world performance,
          // we sometime select stat==0 cells if the distance is close
          // enough
          stat = cell->EvaluatePosition(x, point, subId, pcoords, dist2, weights.data());

          if (stat != -1 && dist2 < minDist2)
          {
            retVal = 1;
            inside = stat;
            minDist2 = dist2;
            closestCellId = cellId;
            closestSubId = subId;
            closestPoint[0] = point[0];
            closestPoint[1] = point[1];
            closestPoint[2] = point[2];
          }
        }
      }
    }

    // Expand the search: queue neighbors if they are not already processed. (TODO: this
    // is pretty inefficient with sparse locators, since bins are revisited repeatedly.)
    this->Binner->GetBinIndices(binId, ijk);
    int ijkLo[3] = { std::max(ijk[0] - 1, 0), std::max(ijk[1] - 1, 0), std::max(ijk[2] - 1, 0) };
    int ijkHi[3] = { std::min(ijk[0] + 1, this->Binner->Divisions[0] - 1),
      std::min(ijk[1] + 1, this->Binner->Divisions[1] - 1),
      std::min(ijk[2] + 1, this->Binner->Divisions[2] - 1) };

    for (ijk[0] = ijkLo[0]; ijk[0] <= ijkHi[0]; ++ijk[0])
    {
      for (ijk[1] = ijkLo[1]; ijk[1] <= ijkHi[1]; ++ijk[1])
      {
        for (ijk[2] = ijkLo[2]; ijk[2] <= ijkHi[2]; ++ijk[2])
        {
          binId = this->Binner->GetBinIndex(ijk);
          if (!binHasBeenQueued[binId])
          {
            binHasBeenQueued[binId] = true;

            // get bin bounding box
            bds[0] = this->Binner->Bounds[0] + ijk[0] * this->Binner->hX;
            bds[2] = this->Binner->Bounds[2] + ijk[1] * this->Binner->hY;
            bds[4] = this->Binner->Bounds[4] + ijk[2] * this->Binner->hZ;
            bds[1] = bds[0] + this->Binner->hX;
            bds[3] = bds[2] + this->Binner->hY;
            bds[5] = bds[4] + this->Binner->hZ;

            // compute distance to box
            distance2ToCellBounds = Distance2ToBounds(x, bds);

            // add to queue
            queue.emplace(distance2ToCellBounds, binId);
          }
        }
      }
    }
  }
  return retVal;
}

//------------------------------------------------------------------------------
// This code traverses the cell locator by following the intersection ray. As
// each bin is intersected, the cells contained in the bin are
// intersected. The cell with the smallest parametric coordinate t is
// returned (assuming 0<=t<=1). Otherwise no intersection is returned. See
// for reference: A Fast Voxel Traversal Algorithm for Ray Tracing by John
// Amanatides & Andrew Woo. Also see the code repository which inspired some
// of this code:
// https://github.com/francisengelmann/fast_voxel_traversal/blob/master/main.cpp.
template <typename T>
int CellProcessor<T>::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  double* bounds = this->Binner->Bounds;
  int* ndivs = this->Binner->Divisions;
  vtkIdType prod = ndivs[0] * ndivs[1];
  double* h = this->Binner->H;
  T i, numCellsInBin;
  double rayDir[3];
  vtkMath::Subtract(p2, p1, rayDir);
  double t0, t1, x0[3], x1[3], tBest = VTK_FLOAT_MAX, xBest[3], pCoordsBest[3];
  int ijk[3], ijkEnd[3], plane0, plane1, subIdBest = -1;
  vtkIdType idx, cId, cellIdBest = -1;
  double hitCellBoundsPosition[3], tHitCell;
  double step[3], next[3], tMax[3], tDelta[3];
  double binBounds[6];

  // Make sure the bounding box of the locator is hit. Also, determine the
  // entry and exit points into and out of the locator. This is used to
  // determine the bins where the ray starts and ends.
  cellId = -1;
  subId = 0;
  if (vtkBox::IntersectWithLine(bounds, p1, p2, t0, t1, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->NumCells, false);

  // Get the i-j-k point of intersection and bin index. This is
  // clamped to the boundary of the locator.
  this->Binner->GetBinIndices(x0, ijk);
  this->Binner->GetBinIndices(x1, ijkEnd);
  idx = ijk[0] + ijk[1] * ndivs[0] + ijk[2] * prod;

  // Set up some traversal parameters for traversing through bins
  step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
  step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;
  step[2] = (rayDir[2] >= 0.0) ? 1.0 : -1.0;

  // If the ray is going in the negative direction, then the next voxel boundary
  // is on the "-" direction so we stay in the current voxel.
  next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ijk[0] + step[0]) : ijk[0]);
  next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ijk[1] + step[1]) : ijk[1]);
  next[2] = bounds[4] + h[2] * (rayDir[2] >= 0.0 ? (ijk[2] + step[2]) : ijk[2]);

  tMax[0] = (rayDir[0] != 0.0) ? (next[0] - x0[0]) / rayDir[0] : VTK_FLOAT_MAX;
  tMax[1] = (rayDir[1] != 0.0) ? (next[1] - x0[1]) / rayDir[1] : VTK_FLOAT_MAX;
  tMax[2] = (rayDir[2] != 0.0) ? (next[2] - x0[2]) / rayDir[2] : VTK_FLOAT_MAX;

  tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
  tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;
  tDelta[2] = (rayDir[2] != 0.0) ? (h[2] / rayDir[2]) * step[2] : VTK_FLOAT_MAX;

  // Start walking through the bins, find the best cell of
  // intersection. Note that the ray may not penetrate all of the way
  // through the locator so may terminate when (t > 1.0).
  for (cellIdBest = (-1); cellIdBest < 0;)
  {
    if ((numCellsInBin = this->GetNumberOfIds(idx)) > 0) // there are some cell here
    {
      const CellFragments<T>* cellIds = this->GetIds(idx);
      this->ComputeBinBounds(ijk[0], ijk[1], ijk[2], binBounds);
      for (i = 0; i < numCellsInBin; i++)
      {
        cId = cellIds[i].CellId;
        if (!cellHasBeenVisited[cId])
        {
          cellHasBeenVisited[cId] = true;

          // check whether we intersect the cell bounds
          int hitCellBounds = vtkBox::IntersectBox(
            this->CellBounds + (6 * cId), p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

          if (hitCellBounds)
          {
            // now, do the expensive GetCell call and the expensive
            // intersect with line call
            this->DataSet->GetCell(cId, cell);
            if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) && t < tBest)
            {
              // Make sure that intersection occurs within this bin or else spurious cell
              // intersections can occur behind this bin which are not the correct answer.
              if (!CellProcessor::IsInBounds(binBounds, x, tol))
              {
                cellHasBeenVisited[cId] = false; // mark the cell non-visited
              }
              else
              {
                tBest = t;
                xBest[0] = x[0];
                xBest[1] = x[1];
                xBest[2] = x[2];
                pCoordsBest[0] = pcoords[0];
                pCoordsBest[1] = pcoords[1];
                pCoordsBest[2] = pcoords[2];
                subIdBest = subId;
                cellIdBest = cId;
              }
            } // if intersection
          }   // if (hitCellBounds)
        }     // if (!cellHasBeenVisited[cId])
      }       // over all cells in bin
    }         // if cells in bin

    // Exit before end of ray, saves a few cycles
    if (cellIdBest >= 0)
    {
      break;
    }

    // See if the traversal is complete (reached the end of the line).
    if (ijk[0] == ijkEnd[0] && ijk[1] == ijkEnd[1] && ijk[2] == ijkEnd[2])
    {
      break;
    }

    // Advance to next voxel
    if (tMax[0] < tMax[1])
    {
      if (tMax[0] < tMax[2])
      {
        ijk[0] += static_cast<int>(step[0]);
        tMax[0] += tDelta[0];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }
    else
    {
      if (tMax[1] < tMax[2])
      {
        ijk[1] += static_cast<int>(step[1]);
        tMax[1] += tDelta[1];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }

    if (ijk[0] < 0 || ijk[0] >= ndivs[0] || ijk[1] < 0 || ijk[1] >= ndivs[1] || ijk[2] < 0 ||
      ijk[2] >= ndivs[2])
    {
      break;
    }
    else
    {
      idx = ijk[0] + ijk[1] * ndivs[0] + ijk[2] * prod;
    }
  } // for looking for valid intersected cell

  // If a cell has been intersected, recover the information and return.
  if (cellIdBest >= 0)
  {
    this->DataSet->GetCell(cellIdBest, cell);
    t = tBest;
    x[0] = xBest[0];
    x[1] = xBest[1];
    x[2] = xBest[2];
    pcoords[0] = pCoordsBest[0];
    pcoords[1] = pCoordsBest[1];
    pcoords[2] = pCoordsBest[2];
    subId = subIdBest;
    cellId = cellIdBest;
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
template <typename T>
bool CellProcessor<T>::InsideCellBounds(const double x[3], vtkIdType cellId)
{
  return CellProcessor::IsInBounds(this->CellBounds + 6 * cellId, x);
}
} // anonymous namespace

//------------------------------------------------------------------------------
// Here is the VTK class proper.

//------------------------------------------------------------------------------
vtkStaticCellLocator::vtkStaticCellLocator()
{
  this->CacheCellBounds = 1; // always cached

  this->Binner = nullptr;
  this->Processor = nullptr;

  this->NumberOfCellsPerNode = 10;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 100;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  for (int i = 0; i < 6; i++)
  {
    this->Bounds[i] = 0;
  }

  this->MaxNumberOfBuckets = VTK_INT_MAX;
  this->LargeIds = false;
}

//------------------------------------------------------------------------------
vtkStaticCellLocator::~vtkStaticCellLocator()
{
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::FreeSearchStructure()
{
  if (this->Binner)
  {
    delete this->Binner;
    this->Binner = nullptr;
  }
  if (this->Processor)
  {
    delete this->Processor;
    this->Processor = nullptr;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticCellLocator::FindCell(
  double pos[3], double, vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return -1;
  }
  return this->Processor->FindCell(pos, cell, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2,
  int& inside)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return 0;
  }
  return this->Processor->FindClosestPointWithinRadius(
    x, radius, closestPoint, cell, cellId, subId, dist2, inside);
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return;
  }
  this->Processor->FindCellsWithinBounds(bbox, cells);
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::FindCellsAlongPlane(
  const double o[3], const double n[3], double tol, vtkIdList* cells)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return;
  }
  this->Processor->FindCellsAlongPlane(o, n, tol, cells);
}

//------------------------------------------------------------------------------
int vtkStaticCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return 0;
  }
  return this->Processor->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId, cell);
}

//------------------------------------------------------------------------------
int vtkStaticCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (!this->Processor)
  {
    return 0;
  }
  return this->Processor->IntersectWithLine(p1, p2, tol, points, cellIds, cell);
}

//------------------------------------------------------------------------------
bool vtkStaticCellLocator::InsideCellBounds(double x[3], vtkIdType cellId)
{
  return this->Processor->InsideCellBounds(x, cellId);
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Binner && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->Binner && this->UseExistingSearchStructure)
  {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::BuildLocatorInternal()
{
  vtkDebugMacro(<< "Building static cell locator");
  vtkIdType numCells;
  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    vtkErrorMacro(<< "No cells to build");
    return;
  }

  // Prepare
  this->FreeSearchStructure();

  // The bounding box can be slow
  int i, ndivs[3];
  const double* bounds = this->DataSet->GetBounds();
  vtkIdType numBins = static_cast<vtkIdType>(
    static_cast<double>(numCells) / static_cast<double>(this->NumberOfCellsPerNode));
  numBins = (numBins > this->MaxNumberOfBuckets ? this->MaxNumberOfBuckets : numBins);

  vtkBoundingBox bbox(bounds);
  if (this->Automatic)
  {
    bbox.ComputeDivisions(numBins, this->Bounds, ndivs);
  }
  else
  {
    bbox.Inflate(); // make sure non-zero volume
    bbox.GetBounds(this->Bounds);
    for (i = 0; i < 3; i++)
    {
      ndivs[i] = (this->Divisions[i] < 1 ? 1 : this->Divisions[i]);
    }
  }

  this->Divisions[0] = ndivs[0];
  this->Divisions[1] = ndivs[1];
  this->Divisions[2] = ndivs[2];
  numBins = static_cast<vtkIdType>(ndivs[0]) * static_cast<vtkIdType>(ndivs[1]) *
    static_cast<vtkIdType>(ndivs[2]);

  // Compute bin/bucket widths
  for (i = 0; i < 3; i++)
  {
    this->H[i] = (this->Bounds[2 * i + 1] - this->Bounds[2 * i]) / this->Divisions[i];
  }

  // Actually do the hard work of creating the locator. Clear out old stuff.
  delete this->Binner;
  delete this->Processor;
  this->Binner = new vtkCellBinner(this, numCells, numBins);
  vtkSMPTools::For(0, numCells, *(this->Binner));

  // Create sorted cell fragments tuples of (cellId,binId). Depending
  // on problem size, different types are used.
  vtkIdType numFragments = this->Binner->NumFragments;
  if (numFragments >= VTK_INT_MAX)
  {
    this->LargeIds = true;
    CellProcessor<vtkIdType>* processor = new CellProcessor<vtkIdType>(this->Binner);
    vtkSMPTools::For(0, numCells, *processor);
    vtkSMPTools::Sort(processor->Map, processor->Map + numFragments);
    MapOffsets<vtkIdType> mapOffsets(processor);
    vtkSMPTools::For(0, processor->NumBatches, mapOffsets);
    this->Processor = processor;
  }
  else
  {
    this->LargeIds = false;
    CellProcessor<int>* processor = new CellProcessor<int>(this->Binner);
    vtkSMPTools::For(0, numCells, *processor);
    vtkSMPTools::Sort(processor->Map, processor->Map + numFragments);
    MapOffsets<int> mapOffsets(processor);
    vtkSMPTools::For(0, processor->NumBatches, mapOffsets);
    this->Processor = processor;
  }

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
// Produce a polygonal representation of the locator. Each bin which contains
// a potential cell candidate contributes to the representation. Note that
// since the locator has only a single level, the level method parameter is
// ignored.
void vtkStaticCellLocator::GenerateRepresentation(int vtkNotUsed(level), vtkPolyData* pd)
{
  // Make sure locator has been built successfully
  this->BuildLocator();
  if (!this->Processor)
  {
    return;
  }

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToFloat();
  vtkNew<vtkCellArray> polys;
  pd->SetPoints(pts);
  pd->SetPolys(polys);

  int* dims = this->Divisions;
  int i, j, k;
  vtkIdType idx, kOffset, jOffset, kSlice = dims[0] * dims[1], pIds[8];
  double* s = this->H;
  double x[3], xT[3], origin[3];
  origin[0] = this->Bounds[0];
  origin[1] = this->Bounds[2];
  origin[2] = this->Bounds[4];

  // A locator is used to avoid duplicate points
  vtkNew<vtkMergePoints> locator;
  locator->InitPointInsertion(pts, this->Bounds, dims[0] * dims[1] * dims[2]);

  for (k = 0; k < dims[2]; k++)
  {
    x[2] = origin[2] + k * s[2];
    kOffset = k * kSlice;
    for (j = 0; j < dims[1]; j++)
    {
      x[1] = origin[1] + j * s[1];
      jOffset = j * dims[0];
      for (i = 0; i < dims[0]; i++)
      {
        x[0] = origin[0] + i * s[0];
        idx = i + jOffset + kOffset;

        // Check to see if bin contains anything
        // If so, insert up to eight points and six quad faces (depending on
        // local topology).
        if (!this->Processor->IsEmpty(idx))
        {
          // Points in (i-j-k) order. A locator is used to avoid duplicate points.
          locator->InsertUniquePoint(x, pIds[0]);
          xT[0] = x[0] + s[0];
          xT[1] = x[1];
          xT[2] = x[2];
          locator->InsertUniquePoint(xT, pIds[1]);
          xT[0] = x[0];
          xT[1] = x[1] + s[1];
          xT[2] = x[2];
          locator->InsertUniquePoint(xT, pIds[2]);
          xT[0] = x[0] + s[0];
          xT[1] = x[1] + s[1];
          xT[2] = x[2];
          locator->InsertUniquePoint(xT, pIds[3]);
          xT[0] = x[0];
          xT[1] = x[1];
          xT[2] = x[2] + s[2];
          locator->InsertUniquePoint(xT, pIds[4]);
          xT[0] = x[0] + s[0];
          xT[1] = x[1];
          xT[2] = x[2] + s[2];
          locator->InsertUniquePoint(xT, pIds[5]);
          xT[0] = x[0];
          xT[1] = x[1] + s[1];
          xT[2] = x[2] + s[2];
          locator->InsertUniquePoint(xT, pIds[6]);
          xT[0] = x[0] + s[0];
          xT[1] = x[1] + s[1];
          xT[2] = x[2] + s[2];
          locator->InsertUniquePoint(xT, pIds[7]);

          // Loop over all bins. Any bin containing cell candidates may
          // generate output. Faces are output if they are on the boundary of
          // the locator or if the bin neighbor contains no cells (i.e., there
          // are no face neighbors). This prevents duplicate faces.

          //  -x bin boundary face
          if (i == 0 || this->Processor->IsEmpty(idx - 1))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[4]);
            polys->InsertCellPoint(pIds[6]);
            polys->InsertCellPoint(pIds[2]);
          }

          // +x boundary face
          if (i == (dims[0] - 1) || this->Processor->IsEmpty(idx + 1))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[1]);
            polys->InsertCellPoint(pIds[3]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[5]);
          }

          // -y boundary face
          if (j == 0 || this->Processor->IsEmpty(idx - dims[0]))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[1]);
            polys->InsertCellPoint(pIds[5]);
            polys->InsertCellPoint(pIds[4]);
          }

          // +y boundary face
          if (j == (dims[1] - 1) || this->Processor->IsEmpty(idx + dims[0]))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[2]);
            polys->InsertCellPoint(pIds[6]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[3]);
          }

          // -z boundary face
          if (k == 0 || this->Processor->IsEmpty(idx - kSlice))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[2]);
            polys->InsertCellPoint(pIds[3]);
            polys->InsertCellPoint(pIds[1]);
          }

          // +z boundary face
          if (k == (dims[2] - 1) || this->Processor->IsEmpty(idx + kSlice))
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[4]);
            polys->InsertCellPoint(pIds[5]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[6]);
          }
        } // if not empty
      }   // x
    }     // y
  }       // z
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  vtkStaticCellLocator* cellLocator = vtkStaticCellLocator::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to vtkStaticCellLocator.");
    return;
  }
  // we only copy what's actually used by vtkStaticCellLocator

  // vtkLocator parameters
  this->SetUseExistingSearchStructure(cellLocator->GetUseExistingSearchStructure());
  this->SetAutomatic(cellLocator->GetAutomatic());

  // vtkAbstractCellLocator parameters
  this->SetNumberOfCellsPerNode(cellLocator->GetNumberOfCellsPerNode());

  // vtkStaticCellLocator parameters
  std::copy_n(cellLocator->Bounds, 6, this->Bounds);
  std::copy_n(cellLocator->Divisions, 3, this->Divisions);
  std::copy_n(cellLocator->H, 3, this->H);
  this->SetMaxNumberOfBuckets(cellLocator->GetMaxNumberOfBuckets());
  this->LargeIds = cellLocator->LargeIds;
  // copy binner
  this->Binner = new vtkCellBinner();
  this->Binner->Locator = this; // this should be different
  this->Binner->NumCells = cellLocator->Binner->NumCells;
  this->Binner->NumBins = cellLocator->Binner->NumBins;
  this->Binner->NumFragments = cellLocator->Binner->NumFragments;
  this->Binner->DataSet = this->DataSet; // this should be different
  std::copy_n(cellLocator->Binner->Divisions, 3, this->Binner->Divisions);
  std::copy_n(cellLocator->Binner->Bounds, 6, this->Binner->Bounds);
  this->Binner->CellBoundsSharedPtr = cellLocator->Binner->CellBoundsSharedPtr; // this is important
  this->Binner->CellBounds =
    this->Binner->CellBoundsSharedPtr.get() ? this->Binner->CellBoundsSharedPtr->data() : nullptr;
  this->Binner->CountsSharedPtr = cellLocator->Binner->CountsSharedPtr; // this is important
  this->Binner->Counts =
    this->Binner->CountsSharedPtr.get() ? this->Binner->CountsSharedPtr->data() : nullptr;
  std::copy_n(cellLocator->Binner->H, 3, this->Binner->H);
  this->Binner->hX = cellLocator->Binner->hX;
  this->Binner->hY = cellLocator->Binner->hY;
  this->Binner->hZ = cellLocator->Binner->hZ;
  this->Binner->fX = cellLocator->Binner->fX;
  this->Binner->fY = cellLocator->Binner->fY;
  this->Binner->fZ = cellLocator->Binner->fZ;
  this->Binner->bX = cellLocator->Binner->bX;
  this->Binner->bY = cellLocator->Binner->bY;
  this->Binner->bZ = cellLocator->Binner->bZ;
  this->Binner->xD = cellLocator->Binner->xD;
  this->Binner->yD = cellLocator->Binner->yD;
  this->Binner->zD = cellLocator->Binner->zD;
  this->Binner->xyD = cellLocator->Binner->xyD;
  // copy processor
  if (this->LargeIds)
  {
    auto cellLocatorProcessor = static_cast<CellProcessor<vtkIdType>*>(cellLocator->Processor);
    CellProcessor<vtkIdType>* processor = new CellProcessor<vtkIdType>();
    processor->Binner = this->Binner;
    processor->DataSet = this->DataSet;
    processor->Bounds = this->Binner->Bounds;
    processor->CellBounds = this->Binner->CellBounds;
    processor->Counts = this->Binner->Counts;
    processor->NumCells = this->Binner->NumCells;
    processor->NumBins = this->Binner->NumBins;
    processor->BatchSize = cellLocatorProcessor->BatchSize;
    processor->NumBatches = cellLocatorProcessor->NumBatches;
    processor->xD = this->Binner->xD;
    processor->xyD = this->Binner->xyD;
    processor->MaxCellSize = cellLocatorProcessor->MaxCellSize;
    processor->MapSharedPtr = cellLocatorProcessor->MapSharedPtr; // this is important
    processor->Map = processor->MapSharedPtr.get() ? processor->MapSharedPtr->data() : nullptr;
    processor->OffsetsShardPtr = cellLocatorProcessor->OffsetsShardPtr; // this is important
    processor->Offsets =
      processor->OffsetsShardPtr.get() ? processor->OffsetsShardPtr->data() : nullptr;
    this->Processor = processor;
  }
  else
  {
    auto cellLocatorProcessor = static_cast<CellProcessor<int>*>(cellLocator->Processor);
    CellProcessor<int>* processor = new CellProcessor<int>();
    processor->Binner = this->Binner;
    processor->DataSet = this->DataSet;
    processor->Bounds = this->Binner->Bounds;
    processor->CellBounds = this->Binner->CellBounds;
    processor->Counts = this->Binner->Counts;
    processor->NumCells = this->Binner->NumCells;
    processor->NumBins = this->Binner->NumBins;
    processor->BatchSize = cellLocatorProcessor->BatchSize;
    processor->NumBatches = cellLocatorProcessor->NumBatches;
    processor->xD = this->Binner->xD;
    processor->xyD = this->Binner->xyD;
    processor->MaxCellSize = cellLocatorProcessor->MaxCellSize;
    processor->MapSharedPtr = cellLocatorProcessor->MapSharedPtr; // this is important
    processor->Map = processor->MapSharedPtr.get() ? processor->MapSharedPtr->data() : nullptr;
    processor->OffsetsShardPtr = cellLocatorProcessor->OffsetsShardPtr; // this is important
    processor->Offsets =
      processor->OffsetsShardPtr.get() ? processor->OffsetsShardPtr->data() : nullptr;
    this->Processor = processor;
  }
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkStaticCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  // Cell bounds are always cached
  this->CacheCellBounds = 1;
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Max Number Of Buckets: " << this->MaxNumberOfBuckets << "\n";

  os << indent << "Large IDs: " << this->LargeIds << "\n";
}
VTK_ABI_NAMESPACE_END
