/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCellLocator.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkGenericCell.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkBox.h"
#include "vtkBoundingBox.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"

vtkStandardNewMacro(vtkStaticCellLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
//
// Note that are two key classes: the vtkCellBinner and the vtkCellProcessor.
// the Binner is used to perform binning operations are cells are placed into
// the uniformally subdivided bin space. The Processor is a templated class
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

// PIMPLd class which wraps binning functionality.
struct vtkCellBinner
{
  vtkStaticCellLocator *Locator; //locater
  vtkIdType NumCells; //the number of cells to bin
  vtkIdType NumBins;
  vtkIdType NumFragments; //total number of (cellId,binId) tuples

  // These are internal data members used for performance reasons
  vtkDataSet *DataSet;
  int Divisions[3];
  double Bounds[6];
  double *CellBounds;
  vtkIdType *Counts;
  double H[3];
  double hX, hY, hZ;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  // Construction
  vtkCellBinner(vtkStaticCellLocator *loc, vtkIdType numCells, int numBins)
  {
    this->Locator = loc;
    this->NumCells = numCells;
    this->NumBins = numBins;
    this->NumFragments = 0;
    this->DataSet = loc->GetDataSet();
    loc->GetDivisions(this->Divisions);

    // Allocate data. Note that these arrays are deleted elsewhere
    this->CellBounds = new double [numCells*6];
    this->Counts = new vtkIdType [numCells+1]; //one extra holds total count

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
    delete [] this->CellBounds;
    delete [] this->Counts;
  }

  void GetBinIndices(const double *x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    ijk[0] = static_cast<int>(((x[0] - bX) * fX));
    ijk[1] = static_cast<int>(((x[1] - bY) * fY));
    ijk[2] = static_cast<int>(((x[2] - bZ) * fZ));

    ijk[0] = (ijk[0] < 0 ? 0 : (ijk[0] >= xD ? xD-1 : ijk[0]));
    ijk[1] = (ijk[1] < 0 ? 0 : (ijk[1] >= yD ? yD-1 : ijk[1]));
    ijk[2] = (ijk[2] < 0 ? 0 : (ijk[2] >= zD ? zD-1 : ijk[2]));
  }

  // Given a point x, determine which bin it is in. Note that points
  // are clamped to lie inside of the locator.
  vtkIdType GetBinIndex(const double *x) const
  {
    int ijk[3];
    this->GetBinIndices(x, ijk);
    return ijk[0] + ijk[1]*xD + ijk[2]*xyD;
  }

  // These are helper functions
  vtkIdType CountBins(const int ijkMin[3], const int ijkMax[3])
  {
    return ( (ijkMax[0]-ijkMin[0]+1) * (ijkMax[1]-ijkMin[1]+1) *
             (ijkMax[2]-ijkMin[2]+1) );
  }

  void Initialize()
  {
  }

  void operator() (vtkIdType cellId, vtkIdType endCellId)
  {
    double *bds = this->CellBounds + cellId*6;
    vtkIdType *counts = this->Counts + cellId;
    double xmin[3], xmax[3];
    int ijkMin[3], ijkMax[3];

    for ( ; cellId < endCellId; ++cellId, bds+=6 )
    {
      this->DataSet->GetCellBounds(cellId,bds);
      xmin[0] = bds[0];
      xmin[1] = bds[2];
      xmin[2] = bds[4];
      xmax[0] = bds[1];
      xmax[1] = bds[3];
      xmax[2] = bds[5];

      this->GetBinIndices(xmin,ijkMin);
      this->GetBinIndices(xmax,ijkMax);

      *counts++ = this->CountBins(ijkMin,ijkMax);
    }
  }

  void Reduce()
  {
    //Perform prefix sum
    vtkIdType *counts = this->Counts;
    vtkIdType numBins, total=0, numCells=this->NumCells;
    for ( vtkIdType i=0; i < numCells; ++i )
      {
      numBins = *counts;
      *counts++ = total;
      total += numBins;
      }
    this->NumFragments = total;
  }

}; //vtkCellBinner

//-----------------------------------------------------------------------------
// The following tuple is what is sorted in the map. Note that it is templated
// because depending on the number of points / buckets to process we may want
// to use vtkIdType. Otherwise for performance reasons it's best to use an int
// (or other integral type). Typically sort() is 25-30% faster on smaller
// integral types, plus it takes a heck less memory (when vtkIdType is 64-bit
// and int is 32-bit).
template <typename TId>
struct CellFragments
{
  TId CellId; //originating cell id
  TId BinId; //i-j-k index into bin space

  //Operator< used to support the subsequent sort operation.
  bool operator< (const CellFragments& tuple) const
    {return BinId < tuple.BinId;}
};

// Perform locator operations like FindCell. Uses templated subclasses
// to reduce memory and enhance speed.
struct vtkCellProcessor
{
  vtkCellBinner *Binner;
  vtkDataSet *DataSet;
  double *CellBounds;
  vtkIdType *Counts;
  vtkIdType NumFragments;
  vtkIdType NumCells;
  int NumBins;
  int BatchSize;
  int NumBatches;
  vtkIdType xD, xyD;
  unsigned char *CellHasBeenVisited; //intersection operations
  unsigned char QueryNumber;

  vtkCellProcessor(vtkCellBinner *cb) : Binner(cb)
  {
    this->DataSet = cb->DataSet;
    this->CellBounds = cb->CellBounds;
    this->Counts = cb->Counts;
    this->NumCells = cb->NumCells;
    this->NumFragments = cb->NumFragments;
    this->NumBins = cb->NumBins;
    this->BatchSize = 10000; //building the offset array
    this->NumBatches = static_cast<int>(
      ceil(static_cast<double>(this->NumFragments) / this->BatchSize));

    xD = cb->xD; //for speeding up computation
    xyD = cb->xyD;

    this->CellHasBeenVisited = nullptr;
    this->QueryNumber = 0;
  }

  virtual ~vtkCellProcessor()
  {
    delete [] this->CellHasBeenVisited;
    this->CellHasBeenVisited = nullptr;
  }

  void ClearCellHasBeenVisited()
  {
    if (this->CellHasBeenVisited && this->DataSet)
    {
      memset(this->CellHasBeenVisited, 0, this->DataSet->GetNumberOfCells());
    }
  }

  // Satisfy cell locator API
  virtual vtkIdType FindCell(double pos[3], vtkGenericCell *cell,
                             double pcoords[3], double* weights ) = 0;
  virtual void FindCellsWithinBounds(double *bbox, vtkIdList *cells) = 0;
  virtual int IntersectWithLine(double a0[3], double a1[3], double tol,
                                double& t, double x[3], double pcoords[3],
                                int &subId, vtkIdType &cellId,
                                vtkGenericCell *cell) = 0;
  // Convenience for computing
  virtual int IsEmpty(vtkIdType binId) = 0;
};

// Typed subclass
template <typename T>
struct CellProcessor : public vtkCellProcessor
{
  // Type dependent members
  CellFragments<T> *Map; //the map to be sorted
  T                *Offsets; //offsets for each bin into the map

  CellProcessor(vtkCellBinner *cb) : vtkCellProcessor(cb)
  {
    // Prepare to sort
    // one extra to simplify traversal
    this->Map = new CellFragments<T>[this->NumFragments+1];
    this->Map[this->NumFragments].BinId = this->NumBins;
    this->Offsets = new T[this->NumBins+1];
    this->Offsets[this->NumBins] = this->NumFragments;
  }

  virtual ~CellProcessor()
  {
    delete [] this->Map;
    delete [] this->Offsets;
  }

  // The number of cell ids in a bin is determined by computing the
  // difference between the offsets into the sorted cell fragments array.
  T GetNumberOfIds(vtkIdType binNum)
  {
    return (this->Offsets[binNum+1] - this->Offsets[binNum]);
  }

  // Given a bin number, return the cells ids in that bin.
  const CellFragments<T> *GetIds(vtkIdType binNum)
  {
    return this->Map + this->Offsets[binNum];
  }

  void ComputeBinBounds(int i, int j, int k, double binBounds[6])
  {
    double *bds = this->Binner->Bounds;
    double *h = this->Binner->H;
    binBounds[0] = bds[0] + i*h[0];
    binBounds[1] = binBounds[0] + h[0];
    binBounds[2] = bds[2] + j*h[1];
    binBounds[3] = binBounds[2] + h[1];
    binBounds[4] = bds[4] + k*h[2];
    binBounds[5] = binBounds[4] + h[2];
  }

  int IsInBinBounds(double binBounds[6], double x[3], double tol = 0.0)
  {
    if ( binBounds[0]-tol <= x[0] && x[0] <= binBounds[1]+tol &&
         binBounds[2]-tol <= x[1] && x[1] <= binBounds[3]+tol &&
         binBounds[4]-tol <= x[2] && x[2] <= binBounds[5]+tol )
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  // Methods to satisfy vtkCellProcessor virtual API
  virtual vtkIdType FindCell(double pos[3], vtkGenericCell *cell,
                             double pcoords[3], double* weights );
  virtual void FindCellsWithinBounds(double *bbox, vtkIdList *cells);
  virtual int IntersectWithLine(double a0[3], double a1[3], double tol,
                                double& t, double x[3], double pcoords[3],
                                int &subId, vtkIdType &cellId,
                                vtkGenericCell *cell);
  virtual int IsEmpty(vtkIdType binId)
  {
    return ( this->GetNumberOfIds(static_cast<T>(binId)) > 0 ? 0 : 1 );
  }

  // This functor is used to perform the final cell binning
  void Initialize()
  {
  }

  void operator() (vtkIdType cellId, vtkIdType endCellId)
  {
    const double *bds = this->CellBounds + cellId*6;
    CellFragments<T> *t = this->Map + *(this->Counts + cellId);
    double xmin[3], xmax[3];
    int ijkMin[3], ijkMax[3];
    int i, j, k;
    vtkIdType binId;

    for ( ; cellId < endCellId; ++cellId, bds+=6 )
    {
      xmin[0] = bds[0];
      xmin[1] = bds[2];
      xmin[2] = bds[4];
      xmax[0] = bds[1];
      xmax[1] = bds[3];
      xmax[2] = bds[5];

      this->Binner->GetBinIndices(xmin,ijkMin);
      this->Binner->GetBinIndices(xmax,ijkMax);

      for (k=ijkMin[2]; k <= ijkMax[2]; ++k)
       {
        for (j=ijkMin[1]; j <= ijkMax[1]; ++j)
        {
          for (i=ijkMin[0]; i <= ijkMax[0]; ++i)
          {
            binId = i + j*xD + k*xyD;
            t->CellId = cellId;
            t->BinId = binId;
            t++;
          }
        }
      }
    }
  }

  void Reduce()
  {
  }
}; //CellProcessor

// This functor class creates offsets for each cell into the sorted tuple
// array. The offsets enable random access to cells.
template <typename TId>
struct MapOffsets
{
  CellProcessor<TId> *Processor;
  CellFragments<TId> *Map;
  TId *Offsets;
  vtkIdType NumCells;
  int NumBins;
  vtkIdType NumFragments;
  int BatchSize;

  MapOffsets(CellProcessor<TId> *p) : Processor(p)
  {
    this->Map = p->Map;
    this->Offsets = p->Offsets;
    this->NumCells = p->NumCells;
    this->NumBins = p->NumBins;
    this->NumFragments = p->NumFragments;
    this->BatchSize = p->BatchSize;
  }

  // Traverse sorted points (i.e., tuples) and update bin offsets.
  void  operator()(vtkIdType batch, vtkIdType batchEnd)
  {
    TId *offsets = this->Offsets;
    const CellFragments<TId> *curPt =
      this->Map + batch*this->BatchSize;
    const CellFragments<TId> *endBatchPt =
      this->Map + batchEnd*this->BatchSize;
    const CellFragments<TId> *endPt =
      this->Map + this->NumFragments;
    const CellFragments<TId> *prevPt;
    endBatchPt = ( endBatchPt > endPt ? endPt : endBatchPt );

    // Special case at the very beginning of the mapped points array.  If
    // the first point is in bin# N, then all bins up and including
    // N must refer to the first point.
    if ( curPt == this->Map )
    {
      prevPt = this->Map;
      std::fill_n(offsets, curPt->BinId+1, 0); //point to the first points
    }//at the very beginning of the map (sorted points array)

    // We are entering this functor somewhere in the interior of the
    // mapped points array. All we need to do is point to the entry
    // position because we are interested only in prevPt->BinId.
    else
    {
      prevPt = curPt;
    }//else in the middle of a batch

    // Okay we have a starting point for a bin run. Now we can begin
    // filling in the offsets in this batch. A previous thread should
    // have/will have completed the previous and subsequent runs outside
    // of the [batch,batchEnd) range
    for ( curPt=prevPt; curPt < endBatchPt; )
    {
      for ( ; curPt->BinId == prevPt->BinId && curPt <= endBatchPt;
            ++curPt )
      {
        ; //advance
      }
      // Fill in any gaps in the offset array
      std::fill_n(offsets + prevPt->BinId + 1,
                  curPt->BinId - prevPt->BinId,
                  curPt - this->Map);
      prevPt = curPt;
    }//for all batches in this range
  }//operator()

}; //MapOffsets


//-----------------------------------------------------------------------------
template <typename T> vtkIdType CellProcessor<T>::
FindCell(double pos[3], vtkGenericCell *cell, double pcoords[3], double* weights)
{
  vtkIdType binId = this->Binner->GetBinIndex(pos);
  T numIds = this->GetNumberOfIds(binId);

  // Only thread the evaluation if enough cells need to be processed
  if ( numIds < 1 )
  {
    return -1;
  }
  // Run through serially. A parallel implementation is possible but does
  // not seem to be much faster.
  else
  {
    const CellFragments<T> *cellIds = this->GetIds(binId);
    double dist2, *bounds, bds[6], delta[3] = {0.0, 0.0, 0.0};
    int subId;
    vtkIdType cellId;

    for (int j=0; j < numIds; j++)
    {
      cellId = cellIds[j].CellId;
      if (this->CellBounds)
      {
        bounds = this->CellBounds + 6*cellId;
      }
      else
      {
        this->DataSet->GetCellBounds(cellId,bds);
        bounds = bds;
      }

      if ( vtkMath::PointIsWithinBounds(pos, bounds, delta) )
      {
        this->DataSet->GetCell(cellId, cell);
        if (cell->EvaluatePosition(pos, nullptr, subId, pcoords, dist2, weights) == 1)
        {
          return cellId;
        }
      }//in bounding box
    }//for cells in this bin

    return -1; //nothing found
  }//serial
}

//-----------------------------------------------------------------------------
template <typename T> void CellProcessor<T>::
FindCellsWithinBounds(double *bbox, vtkIdList *cells)
{
  vtkIdType binNum, numIds, jOffset, kOffset;
  int i, j, k, ii, ijkMin[3], ijkMax[3];
  double pMin[3], pMax[3];
  const CellFragments<T> *ids;

  cells->Reset();

  // Get the locator locations for the two extreme corners of the bounding box
  pMin[0] = bbox[0];
  pMin[1] = bbox[2];
  pMin[2] = bbox[4];
  pMax[0] = bbox[1];
  pMax[1] = bbox[3];
  pMax[2] = bbox[5];

  this->Binner->GetBinIndices(pMin,ijkMin);
  this->Binner->GetBinIndices(pMax,ijkMax);

  // Loop over the block of bins and add cells that have not yet been visited.
  for ( k=ijkMin[2]; k <= ijkMax[2]; ++k)
  {
    kOffset = k*this->xyD;
    for ( j=ijkMin[1]; j <= ijkMax[1]; ++j)
    {
      jOffset = j*this->xD;
      for ( i=ijkMin[0]; i <= ijkMax[0]; ++i)
      {
        binNum = i + jOffset + kOffset;

        if ( (numIds = this->GetNumberOfIds(binNum)) > 0 )
        {
          ids = this->GetIds(binNum);
          for (ii=0; ii < numIds; ii++)
          {
            // Could use query mechanism to speed up at some point
            cells->InsertUniqueId( ids[ii].CellId );
          }//for all points in bucket
        }//if points in bucket
      }//i-footprint
    }//j-footprint
  }//k-footprint
}

//-----------------------------------------------------------------------------
// This code is adapted from vtkCellLocator which has been tested over the
// years.  Why mess with success? If I was to rewrite the algorithm I'd use a
// deterministic approach to identify the bins, in order, that the line
// intersects.
template <typename T> int CellProcessor<T>::
IntersectWithLine(double a0[3], double a1[3], double tol, double& t, double x[3],
                  double pcoords[3], int &subId, vtkIdType &cellId,
                  vtkGenericCell *cell)
{
  double origin[3];
  double direction1[3];
  double direction2[3];
  double direction3[3];
  double hitPosition[3];
  double hitCellBoundsPosition[3];
  double result;
  int *ndivs=this->Binner->Divisions;
  double binBounds[6];
  double *bounds=this->Binner->Bounds, bounds2[6];
  int i, loop;
  vtkIdType bestCellId=(-1), cId;
  int idx;
  double tMax, dist[3];
  int npos[3];
  int pos[3];
  int bestDir;
  double stopDist, currDist;
  double deltaT, pDistance, minPDistance=1.0e38;
  double length, maxLength=0.0;
  T numCellsInBin;
  vtkIdType numDivisions=0, prod=ndivs[0]*ndivs[1];

  // convert the line into i,j,k coordinates
  tMax = 0.0;
  for (i=0; i < 3; i++)
  {
    direction1[i] = a1[i] - a0[i];
    length = bounds[2*i+1] - bounds[2*i];
    if ( length > maxLength )
    {
      maxLength = length;
    }
    if ( ndivs[i] > numDivisions )
    {
      numDivisions = ndivs[i];
    }
    origin[i] = (a0[i] - bounds[2*i]) / length;
    direction2[i] = direction1[i]/length;

    bounds2[2*i]   = 0.0;
    bounds2[2*i+1] = 1.0;
    tMax += direction2[i]*direction2[i];
  }

  tMax = sqrt(tMax);

  // create a parametric range around the tolerance
  deltaT = tol/maxLength;

  stopDist = tMax*numDivisions;
  for (i = 0; i < 3; i++)
  {
    direction3[i] = direction2[i]/tMax;
  }

  if (vtkBox::IntersectBox(bounds2, origin, direction2, hitPosition, result))
  {
    // start walking through the bins
    bestCellId = -1;

    // Initialize intersection query array if necessary
    if ( this->CellHasBeenVisited == nullptr )
    {
      this->CellHasBeenVisited = new unsigned char [ this->NumCells ];
    }

    // Clear the array that indicates whether we have visited this cell.
    // The array is only cleared when the query number rolls over.  This
    // saves a number of calls to memset.
    this->QueryNumber++;
    if (this->QueryNumber == 0)
    {
      this->ClearCellHasBeenVisited();
      this->QueryNumber++;    // can't use 0 as a marker
    }

    // set up curr and stop dist
    currDist = 0;
    for (i = 0; i < 3; i++)
    {
      currDist += (hitPosition[i] - origin[i])*(hitPosition[i] - origin[i]);
    }
    currDist = sqrt(currDist)*numDivisions;

    // add one offset due to the problems around zero
    for (loop = 0; loop <3; loop++)
    {
      hitPosition[loop] = hitPosition[loop]*numDivisions + 1.0;
      pos[loop] = static_cast<int>(hitPosition[loop]);
      // Adjust right boundary condition: if we intersect from the top, right,
      // or back; then pos must be adjusted to a valid bin index
      if (pos[loop] > numDivisions)
      {
        pos[loop] = numDivisions;
      }
    }

    idx = pos[0] - 1 + (pos[1] - 1)*ndivs[0] + (pos[2] - 1)*prod;

    while ((bestCellId < 0) && (pos[0] > 0) && (pos[1] > 0) && (pos[2] > 0) &&
      (pos[0] <= ndivs[0]) && (pos[1] <= ndivs[1]) && (pos[2] <= ndivs[2]) &&
      (currDist < stopDist))
    {
      if ( (numCellsInBin=this->GetNumberOfIds(idx)) > 0 ) //there are some cell here
      {
        const CellFragments<T> *cellIds = this->GetIds(idx);
        this->ComputeBinBounds(pos[0]-1,pos[1]-1,pos[2]-1, binBounds);
        for (tMax = VTK_DOUBLE_MAX, cellId=0; cellId < numCellsInBin; cellId++)
        {
          cId = cellIds[cellId].CellId;
          if (this->CellHasBeenVisited[cId] != this->QueryNumber)
          {
            this->CellHasBeenVisited[cId] = this->QueryNumber;
            int hitCellBounds = 0;

            // check whether we intersect the cell bounds
            hitCellBounds = vtkBox::IntersectBox(this->CellBounds+(6*cId),
                                                 a0, direction1,
                                                 hitCellBoundsPosition, result);

            if (hitCellBounds)
            {
              // now, do the expensive GetCell call and the expensive
              // intersect with line call
              this->DataSet->GetCell(cId, cell);
              if (cell->IntersectWithLine(a0, a1, tol, t, x, pcoords, subId) )
              {
                if ( ! this->IsInBinBounds(binBounds, x, tol) )
                {
                  this->CellHasBeenVisited[cId] = 0; //mark the cell non-visited
                }
                else
                {
                  if ( t < (tMax+deltaT) ) //it might be close
                  {
                    pDistance = cell->GetParametricDistance(pcoords);
                    if ( pDistance < minPDistance ||
                         (pDistance == minPDistance && t < tMax) )
                    {
                      tMax = t;
                      minPDistance = pDistance;
                      bestCellId = cId;
                    }
                  } //intersection point is in current bin
                } //if within current parametric range
              } // if intersection
            } // if (hitCellBounds)
          } // if (!this->CellHasBeenVisited[cId])
        }
      }

      // move to the next bin
      tMax = VTK_DOUBLE_MAX;
      bestDir = 0;
      for (loop = 0; loop < 3; loop++)
      {
        if (direction3[loop] > 0)
        {
          npos[loop] = pos[loop] + 1;
          dist[loop] = (1.0 - hitPosition[loop] + pos[loop])/direction3[loop];
          if (dist[loop] == 0)
          {
            dist[loop] = 1.0/direction3[loop];
          }
          if (dist[loop] < 0)
          {
            dist[loop] = 0;
          }
          if (dist[loop] < tMax)
          {
            bestDir = loop;
            tMax = dist[loop];
          }
        }
        if (direction3[loop] < 0)
        {
          npos[loop] = pos[loop] - 1;
          dist[loop] = (pos[loop] - hitPosition[loop])/direction3[loop];
          if (dist[loop] == 0)
          {
            dist[loop] = -0.01/direction3[loop];
          }
          if (dist[loop] < 0)
          {
            dist[loop] = 0;
          }
          if (dist[loop] < tMax)
          {
            bestDir = loop;
            tMax = dist[loop];
          }
        }
      }
      // update our position
      for (loop = 0; loop < 3; loop++)
      {
        hitPosition[loop] += dist[bestDir]*direction3[loop];
      }
      currDist += dist[bestDir];
      // now make the move, find the smallest distance
      // only cross one boundary at a time
      pos[bestDir] = npos[bestDir];

      idx = pos[0] - 1 + (pos[1]-1)*numDivisions +
        (pos[2]-1)*prod;
    }
  } // if (vtkBox::IntersectBox(...))

  if (bestCellId >= 0)
  {
    this->DataSet->GetCell(bestCellId, cell);
    cell->IntersectWithLine(a0, a1, tol, t, x, pcoords, subId);

    // store the best cell id in the return "parameter"
    cellId = bestCellId;
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------
// Here is the VTK class proper.

//-----------------------------------------------------------------------------
vtkStaticCellLocator::vtkStaticCellLocator()
{
  this->CacheCellBounds = 1; //always cached

  this->Binner = nullptr;
  this->Processor = nullptr;

  this->CellHasBeenVisited = nullptr;

  this->NumberOfCellsPerNode = 10;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 100;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  for(int i=0;i<6;i++)
  {
    this->Bounds[i] = 0;
  }
  this->MaxNumberOfBuckets = VTK_INT_MAX;
  this->LargeIds = false;

  this->CellHasBeenVisited = nullptr;
  this->QueryNumber = 0;
}

//-----------------------------------------------------------------------------
vtkStaticCellLocator::~vtkStaticCellLocator()
{
  this->FreeSearchStructure();
}

//-----------------------------------------------------------------------------
void vtkStaticCellLocator::FreeSearchStructure()
{
  if ( this->Binner )
  {
    delete this->Binner;
    this->Binner = nullptr;
  }
  if ( this->Processor )
  {
    delete this->Processor;
    this->Processor = nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkStaticCellLocator::
FindCell(double pos[3], double, vtkGenericCell *cell,
         double pcoords[3], double* weights )
{
  this->BuildLocator();
  if ( ! this->Processor )
  {
    return -1;
  }
  return this->Processor->FindCell(pos,cell,pcoords,weights);
}


//-----------------------------------------------------------------------------
void vtkStaticCellLocator::
FindCellsWithinBounds(double *bbox, vtkIdList *cells)
{
  this->BuildLocator();
  if ( ! this->Processor )
  {
    return;
  }
  return this->Processor->FindCellsWithinBounds(bbox, cells);
}


//-----------------------------------------------------------------------------
int vtkStaticCellLocator::
IntersectWithLine(double p1[3], double p2[3], double tol,
                  double &t, double x[3], double pcoords[3],
                  int &subId, vtkIdType &cellId, vtkGenericCell *cell)
{
  this->BuildLocator();
  if ( ! this->Processor )
  {
    return 0;
  }
  return this->Processor->
    IntersectWithLine(p1,p2,tol,t,x,pcoords,subId,cellId,cell);
}


//-----------------------------------------------------------------------------
void vtkStaticCellLocator::
BuildLocator()
{
  vtkDebugMacro( << "Building static cell locator" );

  // Do we need to build?
  if ( (this->Binner != nullptr) && (this->BuildTime > this->MTime)
       && (this->BuildTime > this->DataSet->GetMTime()) )
  {
    return;
  }

  vtkIdType numCells;
  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
  {
    vtkErrorMacro( << "No cells to build");
    return;
  }

  // Prepare
  if ( this->Binner )
  {
    this->FreeSearchStructure();
  }

  // The bounding box can be slow
  int i, ndivs[3];
  const double *bounds = this->DataSet->GetBounds();
  vtkIdType numBins = static_cast<vtkIdType>( static_cast<double>(numCells) /
                                              static_cast<double>(this->NumberOfCellsPerNode) );
  numBins = ( numBins > this->MaxNumberOfBuckets ? this->MaxNumberOfBuckets : numBins );

  vtkBoundingBox bbox(bounds);
  if ( this->Automatic )
  {
    bbox.ComputeDivisions(numBins, this->Bounds, ndivs);
  }
  else
  {
    bbox.Inflate(); //make sure non-zero volume
    bbox.GetBounds(this->Bounds);
    for (i=0; i<3; i++)
    {
      ndivs[i] = ( this->Divisions[i] < 1 ? 1 : this->Divisions[i] );
    }
  }

  this->Divisions[0] = ndivs[0];
  this->Divisions[1] = ndivs[1];
  this->Divisions[2] = ndivs[2];
  numBins = static_cast<vtkIdType>(ndivs[0]) *
    static_cast<vtkIdType>(ndivs[1]) * static_cast<vtkIdType>(ndivs[2]);

  // Compute bin/bucket widths
  for (i=0; i<3; i++)
  {
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / this->Divisions[i] ;
  }

  // Actually do the hard work of creating the locator. Clear out old stuff.
  delete this->Binner;
  delete this->Processor;
  this->Binner = new vtkCellBinner(this, numCells, numBins);
  vtkSMPTools::For(0, numCells, *(this->Binner));

  // Create sorted cell fragments tuples of (cellId,binId). Depending
  // on problem size, different types are used.
  vtkIdType numFragments = this->Binner->NumFragments;
  if ( numFragments >= VTK_INT_MAX )
  {
    this->LargeIds = true;
    CellProcessor<vtkIdType> *processor =
      new CellProcessor<vtkIdType>(this->Binner);
    vtkSMPTools::For(0, numCells, *processor);
    vtkSMPTools::Sort(processor->Map, processor->Map+numFragments);
    MapOffsets<vtkIdType> mapOffsets(processor);
    vtkSMPTools::For(0, processor->NumBatches, mapOffsets);
    this->Processor = processor;
  }
  else
  {
    this->LargeIds = false;
    CellProcessor<int> *processor =
      new CellProcessor<int>(this->Binner);
    vtkSMPTools::For(0, numCells, *processor);
    vtkSMPTools::Sort(processor->Map, processor->Map+numFragments);
    MapOffsets<int> mapOffsets(processor);
    vtkSMPTools::For(0, processor->NumBatches, mapOffsets);
    this->Processor = processor;
  }

  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Produce a polygonal representation of the locator. Each bin which contains
// a potential cell candidate contributes to the representation. Note that
// since the locator has only a single level, the level method parameter is
// ignored.
void vtkStaticCellLocator::
GenerateRepresentation(int vtkNotUsed(level), vtkPolyData *pd)
{
  // Make sure locator has been built successfully
  this->BuildLocator();
  if ( ! this->Processor )
  {
    return;
  }

  vtkPoints *pts = vtkPoints::New();
  pts->SetDataTypeToFloat();
  vtkCellArray *polys = vtkCellArray::New();
  pd->SetPoints(pts);
  pd->SetPolys(polys);

  int *dims = this->Divisions;
  int i, j, k, idx;
  vtkIdType kOffset, jOffset, kSlice=dims[0]*dims[1], pIds[8];
  double *s = this->H;
  double x[3], xT[3], origin[3];
  origin[0] = this->Bounds[0];
  origin[1] = this->Bounds[2];
  origin[2] = this->Bounds[4];

  // A locator is used to avoid duplicate points
  vtkMergePoints *locator = vtkMergePoints::New();
  locator->InitPointInsertion(pts, this->Bounds, dims[0]*dims[1]*dims[2]);

  for ( k=0; k < dims[2]; k++)
  {
    x[2] = origin[2] + k*s[2];
    kOffset = k * kSlice;
    for ( j=0; j < dims[1]; j++)
    {
      x[1] = origin[1] + j*s[1];
      jOffset = j * dims[0];
      for ( i=0; i < dims[0]; i++)
      {
        x[0] = origin[0] + i*s[0];
        idx = i + jOffset + kOffset;

        // Check to see if bin contains anything
        // If so, insert up to eight points and six quad faces (depending on
        // local topology).
        if ( ! this->Processor->IsEmpty(idx) )
        {
          // Points in (i-j-k) order. A locator is used to avoid duplicate points.
          locator->InsertUniquePoint(x, pIds[0]);
          xT[0] = x[0]+s[0]; xT[1]=x[1]; xT[2]=x[2];
          locator->InsertUniquePoint(xT, pIds[1]);
          xT[0]=x[0]; xT[1]=x[1]+s[1]; xT[2]=x[2];
          locator->InsertUniquePoint(xT, pIds[2]);
          xT[0]=x[0]+s[0]; xT[1]=x[1]+s[1]; xT[2]=x[2];
          locator->InsertUniquePoint(xT, pIds[3]);
          xT[0]=x[0]; xT[1]=x[1]; xT[2]=x[2]+s[2];
          locator->InsertUniquePoint(xT, pIds[4]);
          xT[0]=x[0]+s[0]; xT[1]=x[1]; xT[2]=x[2]+s[2];
          locator->InsertUniquePoint(xT, pIds[5]);
          xT[0]=x[0]; xT[1]=x[1]+s[1]; xT[2]=x[2]+s[2];
          locator->InsertUniquePoint(xT, pIds[6]);
          xT[0]=x[0]+s[0]; xT[1]=x[1]+s[1]; xT[2]=x[2]+s[2];
          locator->InsertUniquePoint(xT, pIds[7]);

          // Loop over all bins. Any bin containing cell candidates may
          // generate output. Faces are output if they are on the boundary of
          // the locator or if the bin neighbor contains no cells (i.e., there
          // are no face neighbors). This prevents duplicate faces.

          //  -x bin boundary face
          if ( i == 0 || this->Processor->IsEmpty(idx-1) )
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[4]);
            polys->InsertCellPoint(pIds[6]);
            polys->InsertCellPoint(pIds[2]);
          }

          // +x boundary face
          if ( i == (dims[0]-1) || this->Processor->IsEmpty(idx+1) )
            {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[1]);
            polys->InsertCellPoint(pIds[3]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[5]);
            }

          // -y boundary face
          if ( j == 0 || this->Processor->IsEmpty(idx-dims[0]) )
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[1]);
            polys->InsertCellPoint(pIds[5]);
            polys->InsertCellPoint(pIds[4]);
          }

          // +y boundary face
          if ( j == (dims[1]-1) || this->Processor->IsEmpty(idx+dims[0]) )
            {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[2]);
            polys->InsertCellPoint(pIds[6]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[3]);
            }

          // -z boundary face
          if ( k == 0 || this->Processor->IsEmpty(idx-kSlice) )
          {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[0]);
            polys->InsertCellPoint(pIds[2]);
            polys->InsertCellPoint(pIds[3]);
            polys->InsertCellPoint(pIds[1]);
          }

          // +z boundary face
          if ( k == (dims[2]-1) || this->Processor->IsEmpty(idx+kSlice) )
            {
            polys->InsertNextCell(4);
            polys->InsertCellPoint(pIds[4]);
            polys->InsertCellPoint(pIds[5]);
            polys->InsertCellPoint(pIds[7]);
            polys->InsertCellPoint(pIds[6]);
            }
        }//if not empty
      }//x
    }//y
  }//z

  // Clean up
  locator->Delete();
  polys->Delete();
  pts->Delete();

}

//-----------------------------------------------------------------------------
void vtkStaticCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  // Cell bounds are always cached
  this->CacheCellBounds = 1;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Max Number Of Buckets: "
     << this->MaxNumberOfBuckets << "\n";

  os << indent << "Large IDs: " << this->LargeIds << "\n";
}
