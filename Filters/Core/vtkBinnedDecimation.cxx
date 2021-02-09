/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinnedDecimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBinnedDecimation.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkTriangle.h"

#include <atomic>
#include <vector>

vtkStandardNewMacro(vtkBinnedDecimation);

//----------------------------------------------------------------------------
// Core algorithms and functors to implement threading and type dispatch. Note
// that four different algorithms are implemented, partially for demonstrative
// purposes but also for the fun of it. If I had to pick one, I'd pick
// algorithm 4) BIN_CENTERS which scales better for large numbers of bins, and
// generally produces the best results.

namespace
{

// There are essentially four different algorithms implemented here depending
// on how the output points are generated in each bin: 1) reuse the input
// points; 2) generate new points by selecting one of the points falling
// into each bin; 3) generate new points at bin centers; and 4) average the
// points (and attribute data) contained within the bins.

// Functor to bin points to generate bin ids. The binning functor is common
// to all the algorithms / output options.
template <typename PointsT, typename TIds>
struct BinPoints
{
  PointsT* Points;
  TIds* BinIds;
  int Divisions[3];
  double Bounds[6];
  double H[3];
  // For performance
  double hX, hY, hZ;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  BinPoints(
    PointsT* pts, TIds* binIds, const int* dims, const double* bounds, const double* spacing)
    : Points(pts)
    , BinIds(binIds)
  {
    for (auto i = 0; i < 3; ++i)
    {
      this->Divisions[i] = dims[i];
      this->Bounds[2 * i] = bounds[2 * i];
      this->Bounds[2 * i + 1] = bounds[2 * i + 1];
      this->H[i] = spacing[i];
    }

    // Used for performance
    this->hX = this->H[0] = spacing[0];
    this->hY = this->H[1] = spacing[1];
    this->hZ = this->H[2] = spacing[2];
    this->fX = 1.0 / spacing[0];
    this->fY = 1.0 / spacing[1];
    this->fZ = 1.0 / spacing[2];
    this->bX = this->Bounds[0] = bounds[0];
    this->Bounds[1] = bounds[1];
    this->bY = this->Bounds[2] = bounds[2];
    this->Bounds[3] = bounds[3];
    this->bZ = this->Bounds[4] = bounds[4];
    this->Bounds[5] = bounds[5];
    this->xD = this->Divisions[0];
    this->yD = this->Divisions[1];
    this->zD = this->Divisions[2];
    this->xyD = this->Divisions[0] * this->Divisions[1];
  }

  // Functions to obtain bin index inlined for performance.
  void GetBinIndices(const double* x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    TIds tmp0 = static_cast<TIds>(((x[0] - bX) * fX));
    TIds tmp1 = static_cast<TIds>(((x[1] - bY) * fY));
    TIds tmp2 = static_cast<TIds>(((x[2] - bZ) * fZ));

    ijk[0] = tmp0 < 0 ? 0 : (tmp0 >= xD ? xD - 1 : tmp0);
    ijk[1] = tmp1 < 0 ? 0 : (tmp1 >= yD ? yD - 1 : tmp1);
    ijk[2] = tmp2 < 0 ? 0 : (tmp2 >= zD ? zD - 1 : tmp2);
  }

  TIds GetBinIndex(const double* x) const
  {
    int ijk[3];
    this->GetBinIndices(x, ijk);
    return (ijk[0] + ijk[1] * xD + ijk[2] * xyD);
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto points = vtk::DataArrayTupleRange<3>(this->Points, ptId, endPtId);
    double x[3];
    TIds* bins = this->BinIds + ptId;
    for (const auto tuple : points)
    {
      x[0] = static_cast<double>(tuple[0]);
      x[1] = static_cast<double>(tuple[1]);
      x[2] = static_cast<double>(tuple[2]);
      *bins++ = this->GetBinIndex(x);
    }
  }
};

// Generate the output triangles. This functor is common to
// three of the algorithms #1-3.
template <typename TIds, typename TPtMap>
struct GenerateTriangles
{
  const TIds* BinIds;
  const TPtMap* PointMap;
  vtkCellArray* Tris;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  const TIds* TriMap;
  vtkIdType* OutTris;
  vtkIdType* OutTriOffsets;
  ArrayList* Arrays;

  GenerateTriangles(TIds* bins, TPtMap* ptMap, vtkCellArray* tris, TIds* triMap, vtkIdType* outTris,
    vtkIdType* outTriOffsets, ArrayList* arrays)
    : BinIds(bins)
    , PointMap(ptMap)
    , Tris(tris)
    , TriMap(triMap)
    , OutTris(outTris)
    , OutTriOffsets(outTriOffsets)
    , Arrays(arrays)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Tris->NewIterator()); }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    const TIds* binIds = this->BinIds;
    const TPtMap* ptMap = this->PointMap;
    vtkIdType npts;
    const vtkIdType* tri;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    const TIds* triMap = this->TriMap;
    vtkIdType *outTris = this->OutTris, *outTri;
    vtkIdType *outTriOffsets = this->OutTriOffsets, *outOffsets;

    for (; triId < endTriId; ++triId)
    {
      if ((triMap[triId + 1] - triMap[triId]) > 0) // spit out triangle
      {
        cellIter->GetCellAtId(triId, npts, tri);
        outOffsets = outTriOffsets + triMap[triId];
        *outOffsets = triMap[triId] * 3;
        outTri = outTris + *outOffsets;
        outTri[0] = ptMap[binIds[tri[0]]];
        outTri[1] = ptMap[binIds[tri[1]]];
        outTri[2] = ptMap[binIds[tri[2]]];
        if (this->Arrays) // copy cell data if requested
        {
          this->Arrays->Copy(triId, triMap[triId]);
        }
      }
    }
  }

  void Reduce() {}
};

//============================ Implement algorithms ==================

//============================ 1) Reuse INPUT_POINTS =================
// Traverse cells and mark points and cells that are included in the output
template <typename TIds>
struct SelectOutput
{
  const TIds* BinIds;
  unsigned char* PointUses;
  vtkCellArray* Tris;
  TIds* TriMap;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  SelectOutput(TIds* bins, unsigned char* ptUses, vtkCellArray* tris, TIds* triMap)
    : BinIds(bins)
    , PointUses(ptUses)
    , Tris(tris)
    , TriMap(triMap)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Tris->NewIterator()); }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    vtkIdType npts;
    const vtkIdType* tri;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    TIds* triMap = this->TriMap + triId;
    unsigned char* ptUses = this->PointUses;

    for (; triId < endTriId; ++triId, ++triMap)
    {
      cellIter->GetCellAtId(triId, npts, tri);
      // All three points have to be in different bins
      if (this->BinIds[tri[0]] != this->BinIds[tri[1]] &&
        this->BinIds[tri[0]] != this->BinIds[tri[2]] &&
        this->BinIds[tri[1]] != this->BinIds[tri[2]])
      {
        *triMap = 1;
        ptUses[tri[0]] = 1;
        ptUses[tri[1]] = 1;
        ptUses[tri[2]] = 1;
      }
      else // mark excluded from output
      {
        *triMap = 0;
      }
    }
  }

  void Reduce() {}
};

// Only initialize bins that actually contain a point. We are doing
// this to avoid using std::atomic which the other algorithms use.
template <typename TIds>
struct InitializePointMap
{
  const TIds* BinIds;
  const unsigned char* PointUses;
  TIds* PointMap;

  InitializePointMap(TIds* binIds, unsigned char* ptUses, TIds* ptMap)
    : BinIds(binIds)
    , PointUses(ptUses)
    , PointMap(ptMap)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const TIds* binIds = this->BinIds;
    const unsigned char* ptUses = this->PointUses + ptId;
    TIds* ptMap = this->PointMap;

    for (; ptId < endPtId; ++ptId)
    {
      if (*ptUses++ > 0)
      {
        ptMap[binIds[ptId]] = (-1); // mark unvisited
      }
    }
  }
};

//----------------------------------------------------------------------------
// Decimation algorithm #1 which reuses the input points.
template <typename PointsT, typename TIds>
void ReuseDecimate(vtkIdType numPts, PointsT* pts, vtkIdType numTris, vtkCellArray* tris,
  vtkCellData* inCD, vtkCellData* outCD, vtkIdType numBins, const int* dims, const double* bounds,
  const double* spacing, vtkPolyData* output)
{
  // Setup execution. Several arrays are used to transform the data.
  // The bin id of each point
  TIds* binIds = new TIds[numPts];
  // Is the point used in the output ?
  unsigned char* ptUses = new unsigned char[numPts];
  std::fill_n(ptUses, numPts, 0);
  // The output point id assigned to each bin (if bin contains an output point).
  TIds* ptMap = new TIds[numBins];
  // Which triangle cells are output? And later, the offsets into the
  // output cell array.
  TIds* triMap = new TIds[numTris + 1];

  // Bin points to generate a bin index for each point
  BinPoints<PointsT, TIds> binPoints(pts, binIds, dims, bounds, spacing);
  vtkSMPTools::For(0, numPts, binPoints);

  // Select which triangles and points are sent to the output.
  SelectOutput<TIds> selectOutput(binIds, ptUses, tris, triMap);
  vtkSMPTools::For(0, numTris, selectOutput);

  // Initialize the point map, only the bins that contain something
  InitializePointMap<TIds> initPtMap(binIds, ptUses, ptMap);
  vtkSMPTools::For(0, numPts, initPtMap);

  // Prefix sums to roll up the points and cells, and setup offsets for
  // subsequent threading. This could be threaded, although the gains
  // are likely modest.
  vtkIdType numOutPts = 0;
  for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    if (ptUses[ptId] > 0)
    {
      if (ptMap[binIds[ptId]] < 0)
      {
        ptMap[binIds[ptId]] = ptId;
        ++numOutPts;
      }
    }
  }
  vtkIdType mark, numOutTris = 0;
  for (vtkIdType triId = 0; triId < numTris; ++triId)
  {
    mark = triMap[triId];
    triMap[triId] = numOutTris;
    numOutTris += mark;
  }
  triMap[numTris] = numOutTris;

  // Produce the decimated output
  vtkCellArray* outTrisArray = output->GetPolys();
  vtkNew<vtkIdTypeArray> outConn;
  vtkIdType* outTris = outConn->WritePointer(0, numOutTris * 3);
  vtkNew<vtkIdTypeArray> outOffsets;
  vtkIdType* outTriOffsets = outOffsets->WritePointer(0, numOutTris + 1);
  outTriOffsets[numOutTris] = 3 * numOutTris;

  ArrayList arrays;
  if (outCD) // copy cell data if requested
  {
    outCD->CopyAllocate(inCD, numOutTris);
    arrays.AddArrays(numOutTris, inCD, outCD);
  }

  // Produce output triangles.
  GenerateTriangles<TIds, TIds> genTris(
    binIds, ptMap, tris, triMap, outTris, outTriOffsets, (outCD != nullptr ? (&arrays) : nullptr));
  vtkSMPTools::For(0, numTris, genTris);
  outTrisArray->SetData(outOffsets, outConn);

  // Clean up
  delete[] triMap;
  delete[] ptMap;
  delete[] ptUses;
  delete[] binIds;
}

//----------------------------------------------------------------------------
// Dispatch to the decimate algorithm #1 which reuses input points.
struct PointReuseWorker
{
  template <typename DataT>
  void operator()(DataT* pts, bool largeIds, vtkCellArray* tris, vtkCellData* inCD,
    vtkCellData* outCD, int* divs, double bounds[6], double spacing[3], vtkPolyData* output)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();
    vtkIdType numTris = tris->GetNumberOfCells();
    vtkIdType numBins = divs[0] * divs[1] * divs[2];

    // Use the appropriate id type for memory and performance reasons.
    if (!largeIds)
    {
      ReuseDecimate<DataT, int>(
        numPts, pts, numTris, tris, inCD, outCD, numBins, divs, bounds, spacing, output);
    }
    else
    {
      ReuseDecimate<DataT, vtkIdType>(
        numPts, pts, numTris, tris, inCD, outCD, numBins, divs, bounds, spacing, output);
    }
  }
};

//==========2) Generate new points from selected points BIN_POINTS =============
//==========3) Generate new points at BIN_CENTERS ==============================
// These algorithms #2 and #3 are essentially the same with the difference
// being how the output points are created.

// Traverse cells and map input points and cells to output points and cells
template <typename TIds>
struct MapOutput
{
  const TIds* BinIds;
  std::atomic<TIds>* PointMap;
  vtkCellArray* Tris;
  TIds* TriMap;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  MapOutput(TIds* bins, std::atomic<TIds>* ptMap, vtkCellArray* tris, TIds* triMap)
    : BinIds(bins)
    , PointMap(ptMap)
    , Tris(tris)
    , TriMap(triMap)
  {
  }

  // This method is used to select a point id within a bin from a potential
  // set of contributing point ids, which becomes the single point id
  // associated with the bin. Since there are possibly multiple, simultaneous
  // writes to a bin, an atomic is used to prevent data races etc.
  inline void WritePtId(std::atomic<TIds>& binId, vtkIdType ptId)
  {
    // Because of zero initialization, a negative ptId is written. The end result is
    // that we select the smallest point id from the set of points within a bin.
    TIds currentId, targetId = (-(ptId + 1));
    do
    {
      currentId = binId;
      if (currentId < targetId)
      {
        return;
      }
    } while (!atomic_compare_exchange_weak(&binId, &currentId, targetId));
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Tris->NewIterator()); }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    vtkIdType npts;
    const vtkIdType* tri;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    TIds* triMap = this->TriMap + triId;
    std::atomic<TIds>* ptMap = this->PointMap;
    TIds binIds[3];

    for (; triId < endTriId; ++triId, ++triMap)
    {
      cellIter->GetCellAtId(triId, npts, tri);
      binIds[0] = this->BinIds[tri[0]];
      binIds[1] = this->BinIds[tri[1]];
      binIds[2] = this->BinIds[tri[2]];

      // All three points have to be in different bins for triangle insertion
      if (binIds[0] != binIds[1] && binIds[0] != binIds[2] && binIds[1] != binIds[2])
      {
        *triMap = 1;
        this->WritePtId(ptMap[binIds[0]], tri[0]);
        this->WritePtId(ptMap[binIds[1]], tri[1]);
        this->WritePtId(ptMap[binIds[2]], tri[2]);
      }
      else // mark excluded from output
      {
        *triMap = 0;
      }
    }
  }

  // Reduce is required if Initialize() defined.
  void Reduce() {}
};

// Count the number of points in each z-slice of the binning volume.  The
// resulting slice offsets are used to thread the generation of the output
// points.
template <typename TIds>
struct CountPoints
{
  const int* Dims;
  std::atomic<TIds>* PointMap;
  int* SliceOffsets;

  CountPoints(const int* dims, std::atomic<TIds>* ptMap, int* sliceOffsets)
    : Dims(dims)
    , PointMap(ptMap)
    , SliceOffsets(sliceOffsets)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType slice, vtkIdType endSlice)
  {
    int binOffset = slice * this->Dims[0] * this->Dims[1];

    for (; slice < endSlice; ++slice)
    {
      vtkIdType numSlicePts = 0;
      for (auto j = 0; j < this->Dims[1]; ++j)
      {
        for (auto i = 0; i < this->Dims[0]; ++i)
        {
          if (this->PointMap[binOffset] != 0)
          {
            ++numSlicePts;
          }
          ++binOffset;
        }
      }
      this->SliceOffsets[slice] = numSlicePts;
    } // for all slices in this batch
  }

  void Reduce()
  {
    // Prefix sum to roll up total point count across all of the slices.
    TIds numSlicePts, numNewPts = 0;
    for (auto i = 0; i < this->Dims[2]; ++i)
    {
      numSlicePts = this->SliceOffsets[i];
      this->SliceOffsets[i] = numNewPts;
      numNewPts += numSlicePts;
    }
    this->SliceOffsets[this->Dims[2]] = numNewPts;
  }
};

// Generate output points; either from bin centers, or from
// selecting one of the points in the bin.
template <typename PointsT, typename TIds>
struct GenerateBinPoints
{
  int PointGenerationMode;
  const double* Bounds;
  const double* Spacing;
  const int* Dims;
  int* SliceOffsets;
  std::atomic<TIds>* PointMap;
  PointsT* InPoints;
  ArrayList* Arrays;
  float* OutPoints;

  GenerateBinPoints(int genMode, const double* bounds, const double* spacing, const int* dims,
    int* sliceOffsets, std::atomic<TIds>* ptMap, PointsT* inPts, ArrayList* arrays, float* outPts)
    : PointGenerationMode(genMode)
    , Bounds(bounds)
    , Spacing(spacing)
    , Dims(dims)
    , SliceOffsets(sliceOffsets)
    , PointMap(ptMap)
    , InPoints(inPts)
    , Arrays(arrays)
    , OutPoints(outPts)
  {
  }

  void operator()(vtkIdType slice, vtkIdType endSlice)
  {
    int binOffset = slice * this->Dims[0] * this->Dims[1];
    vtkIdType oldPtId;
    vtkIdType newPtId = this->SliceOffsets[slice];
    float* xOut;
    double xIn[3];
    const auto pts = vtk::DataArrayTupleRange<3>(this->InPoints);

    for (; slice < endSlice; ++slice)
    {
      for (auto j = 0; j < this->Dims[1]; ++j)
      {
        for (auto i = 0; i < this->Dims[0]; ++i)
        {
          oldPtId = this->PointMap[binOffset];
          if (oldPtId != 0)
          {
            oldPtId = -(oldPtId + 1); // transform back to non-negative point id
            xOut = this->OutPoints + 3 * newPtId;
            if (this->PointGenerationMode == vtkBinnedDecimation::BIN_CENTERS)
            {
              xIn[0] = this->Bounds[0] + ((0.5 + static_cast<double>(i)) * this->Spacing[0]);
              xIn[1] = this->Bounds[2] + ((0.5 + static_cast<double>(j)) * this->Spacing[1]);
              xIn[2] = this->Bounds[4] + ((0.5 + static_cast<double>(slice)) * this->Spacing[2]);
            }
            else // genMode == vtkBinnedDecimation::BIN_POINTS
            {
              const auto xp = pts[oldPtId];
              xIn[0] = xp[0];
              xIn[1] = xp[1];
              xIn[2] = xp[2];
            }
            xOut[0] = xIn[0];
            xOut[1] = xIn[1];
            xOut[2] = xIn[2];
            this->PointMap[binOffset] = newPtId; // update to new point id
            if (this->Arrays)                    // copy point data if requested
            {
              this->Arrays->Copy(oldPtId, newPtId);
            }
            newPtId++;
          }
          ++binOffset;
        }
      }
    } // for all slices in this batch
  }
};

//----------------------------------------------------------------------------
// Decimation algorithms #2-3 which generates new points for each bin. Either
// a bin center point is generated, or one of the points contained in the bin
// is selected and copied to the output.
template <typename PointsT, typename TIds>
void BinPointsDecimate(int genMode, vtkIdType numPts, PointsT* pts, vtkPointData* inPD,
  vtkPointData* outPD, vtkIdType numTris, vtkCellArray* tris, vtkCellData* inCD, vtkCellData* outCD,
  vtkIdType numBins, const int* dims, const double* bounds, const double* spacing,
  vtkPolyData* output)
{
  // Setup execution. Several arrays are used to transform the data.
  // The bin id of each point.
  TIds* binIds = new TIds[numPts];

  // Now bin points to generate a bin index for each point.
  BinPoints<PointsT, TIds> binPoints(pts, binIds, dims, bounds, spacing);
  vtkSMPTools::For(0, numPts, binPoints);

  // The ptMap is the output point id assigned to each bin (if the bin
  // contains an output point). Note that multiple, simultaneous writes can
  // occur to a bin hence the use of atomics. Initialize to zero via {}. Zero
  // is a problem because a ptId can == zero; as a workaround, we'll
  // initially use negative ids, and convert to positive ids in the
  // final composition.
  std::atomic<TIds>* ptMap = new std::atomic<TIds>[numBins]();

  // Is the triangle output? And eventually the offset into the output cell array.
  TIds* triMap = new TIds[numTris + 1];

  // Begin to construct mappings of input points and cells, to output points
  // and cells.
  MapOutput<TIds> mapOutput(binIds, ptMap, tris, triMap);
  vtkSMPTools::For(0, numTris, mapOutput);

  // Now generate the new points. First generate new point ids, and then
  // produce the actual points.
  int* sliceOffsets = new int[dims[2] + 1];
  CountPoints<TIds> countPts(dims, ptMap, sliceOffsets);
  vtkSMPTools::For(0, dims[2], countPts);
  int numNewPts = sliceOffsets[dims[2]];

  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(VTK_FLOAT); // could be the same type as the input point type
  newPts->SetNumberOfPoints(numNewPts);
  ArrayList ptArrays;
  if (outPD) // copy point data if requested
  {
    outPD->CopyAllocate(inPD, numNewPts);
    ptArrays.AddArrays(numNewPts, inPD, outPD);
  }

  GenerateBinPoints<PointsT, TIds> genPts(genMode, bounds, spacing, dims, sliceOffsets, ptMap, pts,
    (outPD != nullptr ? (&ptArrays) : nullptr),
    vtkFloatArray::FastDownCast(newPts->GetData())->GetPointer(0));
  vtkSMPTools::For(0, dims[2], genPts);
  output->SetPoints(newPts);

  // Create a mapping of the input triangles to the output triangles.
  vtkIdType mark, numOutTris = 0;
  for (vtkIdType triId = 0; triId < numTris; ++triId)
  {
    mark = triMap[triId];
    triMap[triId] = numOutTris;
    numOutTris += mark;
  }
  triMap[numTris] = numOutTris;

  // Produce the decimated output. We'll directly create the offset
  // and connectivity arrays for the output polydata.
  vtkCellArray* outTrisArray = output->GetPolys();
  vtkNew<vtkIdTypeArray> outConn;
  vtkIdType* outTris = outConn->WritePointer(0, numOutTris * 3);
  vtkNew<vtkIdTypeArray> outOffsets;
  vtkIdType* outTriOffsets = outOffsets->WritePointer(0, numOutTris + 1);
  outTriOffsets[numOutTris] = 3 * numOutTris;

  ArrayList arrays;
  if (outCD) // copy cell data if requested
  {
    outCD->CopyAllocate(inCD, numOutTris);
    arrays.AddArrays(numOutTris, inCD, outCD);
  }

  GenerateTriangles<TIds, std::atomic<TIds>> genTris(
    binIds, ptMap, tris, triMap, outTris, outTriOffsets, (outCD != nullptr ? (&arrays) : nullptr));
  vtkSMPTools::For(0, numTris, genTris);
  outTrisArray->SetData(outOffsets, outConn);

  // Clean up
  delete[] sliceOffsets;
  delete[] triMap;
  delete[] ptMap;
  delete[] binIds;
}

//----------------------------------------------------------------------------
// Invoke the decimate algorithm which generates either a selected bin point,
// or bin centered point. Depending on the id size, use large 64-bit ids or
// 32-bit ids (enhances performance and reduces memory usage).
struct BinPointsWorker
{
  template <typename DataT>
  void operator()(DataT* pts, vtkPointData* inPD, vtkPointData* outPD, bool largeIds, int genMode,
    vtkCellArray* tris, vtkCellData* inCD, vtkCellData* outCD, int* divs, double bounds[6],
    double spacing[3], vtkPolyData* output)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();
    vtkIdType numTris = tris->GetNumberOfCells();
    vtkIdType numBins = divs[0] * divs[1] * divs[2];

    if (!largeIds)
    {
      BinPointsDecimate<DataT, int>(genMode, numPts, pts, inPD, outPD, numTris, tris, inCD, outCD,
        numBins, divs, bounds, spacing, output);
    }
    else
    {
      BinPointsDecimate<DataT, vtkIdType>(genMode, numPts, pts, inPD, outPD, numTris, tris, inCD,
        outCD, numBins, divs, bounds, spacing, output);
    }
  }
};

//========================== 4) Generate points from BIN_AVERAGES ============
// Traverse cells and mark output triangles. Sort points based on the bins
// they fall into. We need to keep track of the inserted points in each bin,
// which are combined later to produce an average position.

//------------------------------------------------------------------------------
// Sort bins with associated bin ids. This creates runs of points for each
// bin, which are later averaged to create a new point in each bin.
template <typename TIds>
struct BinTuple
{
  TIds PtId; // originating point id
  TIds Bin;  // i-j-k index into bin space

  // Operator< used to support the sort operation. Just sort on bin
  // id; points within a bin can be in any order.
  bool operator<(const BinTuple& tuple) const { return (Bin < tuple.Bin ? true : false); }
};

template <typename PointsT, typename TIds>
struct BinPointTuples : public BinPoints<PointsT, TIds>
{
  BinTuple<TIds>* BinTuples;

  BinPointTuples(PointsT* pts, BinTuple<TIds>* binTuples, const int* dims, const double* bounds,
    const double* spacing)
    : BinPoints<PointsT, TIds>(pts, nullptr, dims, bounds, spacing)
    , BinTuples(binTuples)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto points = vtk::DataArrayTupleRange<3>(this->Points, ptId, endPtId);
    double x[3];
    BinTuple<TIds>* bins = this->BinTuples + ptId;
    for (const auto tuple : points)
    {
      (*bins).PtId = ptId++;
      x[0] = static_cast<double>(tuple[0]);
      x[1] = static_cast<double>(tuple[1]);
      x[2] = static_cast<double>(tuple[2]);
      (*bins).Bin = this->GetBinIndex(x);
      ++bins;
    }
  }
};

template <typename TIds>
struct MarkBinnedTris
{
  const BinTuple<TIds>* BinTuples;
  vtkCellArray* Tris;
  TIds* TriMap;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  MarkBinnedTris(BinTuple<TIds>* bt, vtkCellArray* tris, TIds* triMap)
    : BinTuples(bt)
    , Tris(tris)
    , TriMap(triMap)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Tris->NewIterator()); }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    vtkIdType npts;
    const vtkIdType* tri;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    TIds* triMap = this->TriMap + triId;
    TIds binIds[3];

    for (; triId < endTriId; ++triId, ++triMap)
    {
      cellIter->GetCellAtId(triId, npts, tri);
      binIds[0] = this->BinTuples[tri[0]].Bin;
      binIds[1] = this->BinTuples[tri[1]].Bin;
      binIds[2] = this->BinTuples[tri[2]].Bin;

      // All three points have to be in different bins for triangle insertion
      if (binIds[0] != binIds[1] && binIds[0] != binIds[2] && binIds[1] != binIds[2])
      {
        *triMap = 1;
      }
      else // mark excluded from output
      {
        *triMap = 0;
      }
    }
  }

  void Reduce() {}
};

// Produce the output triangles from the bin tuples.
template <typename TIds>
struct BinAveTriangles
{
  const BinTuple<TIds>* BinTuples;
  vtkCellArray* Tris;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  const TIds* TriMap;
  vtkIdType* OutTris;
  vtkIdType* OutTriOffsets;
  ArrayList* Arrays;

  BinAveTriangles(const BinTuple<TIds>* bt, vtkCellArray* tris, TIds* triMap, vtkIdType* outTris,
    vtkIdType* outTriOffsets, ArrayList* arrays)
    : BinTuples(bt)
    , Tris(tris)
    , TriMap(triMap)
    , OutTris(outTris)
    , OutTriOffsets(outTriOffsets)
    , Arrays(arrays)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->Tris->NewIterator()); }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    const BinTuple<TIds>* binTuples = this->BinTuples;
    vtkIdType npts;
    const vtkIdType* tri;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    const TIds* triMap = this->TriMap;
    vtkIdType *outTris = this->OutTris, *outTri;
    vtkIdType *outTriOffsets = this->OutTriOffsets, *outOffsets;

    for (; triId < endTriId; ++triId)
    {
      if ((triMap[triId + 1] - triMap[triId]) > 0) // spit out triangle
      {
        cellIter->GetCellAtId(triId, npts, tri);

        outOffsets = outTriOffsets + triMap[triId];
        *outOffsets = triMap[triId] * 3;
        outTri = outTris + *outOffsets;

        outTri[0] = binTuples[tri[0]].Bin; // set the bin id
        outTri[1] = binTuples[tri[1]].Bin;
        outTri[2] = binTuples[tri[2]].Bin;
        if (this->Arrays) // copy cell data if requested
        {
          this->Arrays->Copy(triId, triMap[triId]);
        }
      }
    }
  }

  void Reduce() {}
};

// Generate the output triangles by rewriting bin ids into new point ids
template <typename TIds>
struct GenerateAveTriangles
{
  const BinTuple<TIds>* BinTuples;
  const TIds* Offsets;
  vtkIdType* OutTris;

  GenerateAveTriangles(const BinTuple<TIds>* bt, TIds* offsets, vtkIdType* outTris)
    : BinTuples(bt)
    , Offsets(offsets)
    , OutTris(outTris)
  {
  }

  void operator()(vtkIdType triId, vtkIdType endTriId)
  {
    const BinTuple<TIds>* binTuples = this->BinTuples;
    const TIds* offsets = this->Offsets;
    vtkIdType* outTri = this->OutTris + 3 * triId;

    for (; triId < endTriId; ++triId, outTri += 3)
    {
      outTri[0] = (*(binTuples + offsets[outTri[0]])).PtId;
      outTri[1] = (*(binTuples + offsets[outTri[1]])).PtId;
      outTri[2] = (*(binTuples + offsets[outTri[2]])).PtId;
    }
  }
};

// A clever way to build offsets in parallel. Basically each thread builds
// offsets across a range of the sorted map.
template <typename TIds>
struct MapOffsets
{
  const BinTuple<TIds>* BinTuples;
  TIds* Offsets;
  TIds NumPts;
  TIds NumBins;
  TIds BatchSize;

  MapOffsets(BinTuple<TIds>* bt, TIds* offsets, TIds numPts, TIds numBins, TIds numBatches)
    : BinTuples(bt)
    , Offsets(offsets)
    , NumPts(numPts)
    , NumBins(numBins)
  {
    this->BatchSize = static_cast<int>(ceil(static_cast<double>(numPts) / numBatches));
  }

  // Traverse sorted points (i.e., tuples) and update bin offsets.
  void operator()(vtkIdType batch, vtkIdType batchEnd)
  {
    TIds* offsets = this->Offsets;
    const BinTuple<TIds>* curPt = this->BinTuples + batch * this->BatchSize;
    const BinTuple<TIds>* endBatchPt = this->BinTuples + batchEnd * this->BatchSize;
    const BinTuple<TIds>* endPt = this->BinTuples + this->NumPts;
    const BinTuple<TIds>* prevPt;
    endBatchPt = (endBatchPt > endPt ? endPt : endBatchPt);

    // Special case at the very beginning of the bin tuples array.  If
    // the first point is in bin# N, then all bins up and including
    // N must refer to the first point.
    if (curPt == this->BinTuples)
    {
      prevPt = this->BinTuples;
      std::fill_n(offsets, curPt->Bin + 1, 0); // offset to the first points
    } // at the very beginning of the map (sorted points array)

    // We are entering this functor somewhere in the interior of the
    // mapped points array. All we need to do is point to the entry
    // position because we are interested only in prevPt->Bin.
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
      for (; curPt->Bin == prevPt->Bin && curPt <= endBatchPt; ++curPt)
      {
        ; // advance
      }
      // Fill in any gaps in the offset array
      if (curPt < endPt) // still within range of points
      {
        std::fill_n(offsets + prevPt->Bin + 1, curPt->Bin - prevPt->Bin, curPt - this->BinTuples);
        prevPt = curPt;
      }
      else // at the end of the points
      {
        std::fill_n(
          offsets + prevPt->Bin + 1, this->NumBins - prevPt->Bin - 1, curPt - this->BinTuples);
        return;
      }
    } // for all batches in this range
  }   // operator()
};

// Count the number of averaged points in each z-slice of the binning volume.
// The resulting offsets are used to thread the generation of the output
// points.
template <typename TIds>
struct CountAvePts
{
  const int* Dims;
  const TIds* Offsets;
  int* SliceOffsets;

  CountAvePts(const int* dims, const TIds* offsets, int* sliceOffsets)
    : Dims(dims)
    , Offsets(offsets)
    , SliceOffsets(sliceOffsets)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType slice, vtkIdType endSlice)
  {
    int binNum = slice * this->Dims[0] * this->Dims[1];

    for (; slice < endSlice; ++slice)
    {
      vtkIdType numSlicePts = 0;
      for (auto j = 0; j < this->Dims[1]; ++j)
      {
        for (auto i = 0; i < this->Dims[0]; ++i)
        {
          if ((this->Offsets[binNum + 1] - this->Offsets[binNum]) > 0)
          {
            ++numSlicePts;
          }
          ++binNum;
        }
      }
      this->SliceOffsets[slice] = numSlicePts;
    } // for all slices in this batch
  }

  void Reduce()
  {
    // Prefix sum to roll up total point count in each slice
    TIds numSlicePts, numNewPts = 0;
    for (auto i = 0; i < this->Dims[2]; ++i)
    {
      numSlicePts = this->SliceOffsets[i];
      this->SliceOffsets[i] = numNewPts;
      numNewPts += numSlicePts;
    }
    this->SliceOffsets[this->Dims[2]] = numNewPts;
  }
};

// Generate points from the binning -- in this case from the average
// position of all points in each bin.
template <typename PointsT, typename TIds>
struct GenerateAveBinPoints
{
  const int* Dims;
  PointsT* InPoints;
  const int* SliceOffsets;
  BinTuple<TIds>* BinTuples;
  const TIds* Offsets;
  ArrayList* Arrays;
  float* OutPoints;
  vtkSMPThreadLocal<std::vector<vtkIdType>> PtIds;

  GenerateAveBinPoints(const int* dims, PointsT* inPts, const int* sliceOffsets,
    BinTuple<TIds>* binTuples, const TIds* offsets, ArrayList* arrays, float* outPts)
    : Dims(dims)
    , InPoints(inPts)
    , SliceOffsets(sliceOffsets)
    , BinTuples(binTuples)
    , Offsets(offsets)
    , Arrays(arrays)
    , OutPoints(outPts)
  {
  }

  void operator()(vtkIdType slice, vtkIdType endSlice)
  {
    int binNum = slice * this->Dims[0] * this->Dims[1];
    vtkIdType newPtId = this->SliceOffsets[slice];
    float* xOut;
    double xAve[3];
    const auto pts = vtk::DataArrayTupleRange<3>(this->InPoints);
    BinTuple<TIds>* binTuples = this->BinTuples;
    const TIds* offsets = this->Offsets;
    BinTuple<TIds>* pIds;
    TIds pId;
    std::vector<vtkIdType> v = this->PtIds.Local();

    for (; slice < endSlice; ++slice)
    {
      for (auto j = 0; j < this->Dims[1]; ++j)
      {
        for (auto i = 0; i < this->Dims[0]; ++i)
        {
          TIds npts = offsets[binNum + 1] - offsets[binNum];
          if (npts > 0)
          {
            // Average the points in the bin
            xAve[0] = xAve[1] = xAve[2] = 0.0;
            pIds = binTuples + offsets[binNum];
            v.resize(npts);
            for (auto idx = 0; idx < npts; ++idx)
            {
              pId = (*(pIds + idx)).PtId;
              v[idx] = pId;
              const auto p = pts[pId];
              xAve[0] += p[0];
              xAve[1] += p[1];
              xAve[2] += p[2];
            }
            xAve[0] /= static_cast<double>(npts);
            xAve[1] /= static_cast<double>(npts);
            xAve[2] /= static_cast<double>(npts);

            xOut = this->OutPoints + 3 * newPtId;
            xOut[0] = xAve[0];
            xOut[1] = xAve[1];
            xOut[2] = xAve[2];
            if (this->Arrays) // average point data if requested
            {
              this->Arrays->Average(npts, v.data(), newPtId);
            }
            (*pIds).PtId = newPtId; // update to new point id
            newPtId++;
          }
          ++binNum;
        } // for i
      }   // for j
    }     // for all slices in this batch
  }
};

//----------------------------------------------------------------------------
// Decimation algorithm which generates new points for each bin by averaging
// the point coordinates and point attributes in each bin. This algorithm
// typically produces the best results. For small numbers of bins it can be
// a tad slower, but generally scales better as the number of bins is
// increased.
template <typename PointsT, typename TIds>
void AvePointsDecimate(vtkIdType numPts, PointsT* pts, vtkPointData* inPD, vtkPointData* outPD,
  vtkIdType numTris, vtkCellArray* tris, vtkCellData* inCD, vtkCellData* outCD, vtkIdType numBins,
  const int* dims, const double* bounds, const double* spacing, vtkPolyData* output)
{
  // Setup execution. Several arrays are used to transform the data.
  // Define the bin id and associated point id of each point.
  BinTuple<TIds>* binTuples = new BinTuple<TIds>[numPts];

  // Now bin points to generate a bin index for each point.
  BinPointTuples<PointsT, TIds> binPoints(pts, binTuples, dims, bounds, spacing);
  vtkSMPTools::For(0, numPts, binPoints);

  // Initially, triMap indicates which triangles are output. And then
  // contains the offsets into the output triangles array.
  TIds* triMap = new TIds[numTris + 1];

  // Begin to construct mappings of input points and cells, to output points
  // and cells. First identify the triangles to be sent to the output.
  MarkBinnedTris<TIds> markBinnedTris(binTuples, tris, triMap);
  vtkSMPTools::For(0, numTris, markBinnedTris);

  // Create a mapping of the input triangles to the output triangles.
  vtkIdType mark, numOutTris = 0;
  for (vtkIdType triId = 0; triId < numTris; ++triId)
  {
    mark = triMap[triId];
    triMap[triId] = numOutTris;
    numOutTris += mark;
  }
  triMap[numTris] = numOutTris;

  // Generate the cell output (decimated list of triangles), with the
  // triangle connectivity based on bin ids (not point ids). We'll directly
  // create the offet and connectivity arrays for the output polydata.
  vtkCellArray* outTrisArray = output->GetPolys();
  vtkNew<vtkIdTypeArray> outConn;
  vtkIdType* outTris = outConn->WritePointer(0, numOutTris * 3);
  vtkNew<vtkIdTypeArray> outOffsets;
  vtkIdType* outTriOffsets = outOffsets->WritePointer(0, numOutTris + 1);
  outTriOffsets[numOutTris] = 3 * numOutTris;

  ArrayList arrays;
  if (outCD) // copy cell data if requested
  {
    outCD->CopyAllocate(inCD, numOutTris);
    arrays.AddArrays(numOutTris, inCD, outCD);
  }

  BinAveTriangles<TIds> binTris(
    binTuples, tris, triMap, outTris, outTriOffsets, (outCD != nullptr ? (&arrays) : nullptr));
  vtkSMPTools::For(0, numTris, binTris);
  outTrisArray->SetData(outOffsets, outConn);

  // Now sort the bin tuples by bin id. This is the first step in
  // transforming the input points to new points. The sort operation
  // will organize points into lists per bin.
  vtkSMPTools::Sort(binTuples, binTuples + numPts);

  // To rapidly (random) access the bins, we have to build an offset array
  // into the sorted bin tuples (i.e., runs of points in each bin).
  TIds* offsets = new TIds[numBins + 1];
  TIds numBatches = (numPts < 10000 ? 1 : 100); // totally arbitrary
  MapOffsets<TIds> offMapper(binTuples, offsets, numPts, numBins, numBatches);
  vtkSMPTools::For(0, numBatches, offMapper);
  offsets[numBins] = numPts;

  // Now to generate the new points, build an offset array that basically
  // represents the number of new points generated in each z-slice. First we
  // have to count the new points, and then accumulate them with a prefix
  // sum. For convenience, this is done on a bin slice-by-slice manner.
  int* sliceOffsets = new int[dims[2] + 1];
  CountAvePts<TIds> countPts(dims, offsets, sliceOffsets);
  vtkSMPTools::For(0, dims[2], countPts);
  int numNewPts = sliceOffsets[dims[2]];

  // The points (and optional point attributes) are generated by averaging
  // the contributions of all points in each bin. A new point is generated
  // and its id is placed into the bin tuples array (needed for final
  // generation of the triangles).
  // Should output point type be the same as the input point type?
  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(VTK_FLOAT);
  newPts->SetNumberOfPoints(numNewPts);
  ArrayList ptArrays;
  if (outPD) // copy point data if requested
  {
    outPD->CopyAllocate(inPD, numNewPts);
    ptArrays.AddArrays(numNewPts, inPD, outPD);
  }

  // Do the core work of averaging point coordinates and attributes.
  GenerateAveBinPoints<PointsT, TIds> genPts(dims, pts, sliceOffsets, binTuples, offsets,
    (outPD != nullptr ? (&ptArrays) : nullptr),
    vtkFloatArray::FastDownCast(newPts->GetData())->GetPointer(0));
  vtkSMPTools::For(0, dims[2], genPts);
  output->SetPoints(newPts);

  // Finally map the triangle connectivity list to the new point ids.
  GenerateAveTriangles<TIds> genAveTris(binTuples, offsets, outTris);
  vtkSMPTools::For(0, numOutTris, genAveTris);

  // Clean up
  delete[] binTuples;
  delete[] triMap;
  delete[] offsets;
  delete[] sliceOffsets;
}

//----------------------------------------------------------------------------
// Invoke the decimate algorithm which generates an averaged point from the
// points within a bin.
struct AvePointsWorker
{
  template <typename DataT>
  void operator()(DataT* pts, vtkPointData* inPD, vtkPointData* outPD, bool largeIds,
    vtkCellArray* tris, vtkCellData* inCD, vtkCellData* outCD, int* divs, double bounds[6],
    double spacing[3], vtkPolyData* output)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();
    vtkIdType numTris = tris->GetNumberOfCells();
    vtkIdType numBins = divs[0] * divs[1] * divs[2];

    if (!largeIds)
    {
      AvePointsDecimate<DataT, int>(numPts, pts, inPD, outPD, numTris, tris, inCD, outCD, numBins,
        divs, bounds, spacing, output);
    }
    else
    {
      AvePointsDecimate<DataT, vtkIdType>(numPts, pts, inPD, outPD, numTris, tris, inCD, outCD,
        numBins, divs, bounds, spacing, output);
    }
  }
};

} // anonymous namespace

//----------------------------------------------------------------------------
// Construct with default NumberOfDivisions to 256, DivisionSpacing to 1
// in all (x,y,z) directions. AutoAdjustNumberOfDivisions is set to ON.
// ComputeNumberOfDivisions to OFF.
vtkBinnedDecimation::vtkBinnedDecimation()
{
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] = 0.0;
  this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0.0;
  this->NumberOfXDivisions = this->NumberOfDivisions[0] = 256;
  this->NumberOfYDivisions = this->NumberOfDivisions[1] = 256;
  this->NumberOfZDivisions = this->NumberOfDivisions[2] = 256;

  this->AutoAdjustNumberOfDivisions = true;
  this->ComputeNumberOfDivisions = 0;
  this->DivisionOrigin[0] = 0.0;
  this->DivisionOrigin[1] = 0.0;
  this->DivisionOrigin[2] = 0.0;
  this->DivisionSpacing[0] = 1.0;
  this->DivisionSpacing[1] = 1.0;
  this->DivisionSpacing[2] = 1.0;

  this->PointGenerationMode = BIN_POINTS;

  this->ProducePointData = true;
  this->ProduceCellData = false;
  this->LargeIds = false;
}

//----------------------------------------------------------------------------
vtkBinnedDecimation::~vtkBinnedDecimation() {}

//----------------------------------------------------------------------------
// Use the same approach as vtkQuadricClustering (which is a bit of a mess but
// we want consistent behavior between the two classes).
void vtkBinnedDecimation::ConfigureBinning(vtkPolyData* input, vtkIdType numPts)
{
  // Prepare the bounds. Use the faster vtkPoints::GetBounds() because the
  // general vtkPolyData::GetBounds() is much slower.
  double bounds[6];
  input->GetPoints()->GetBounds(bounds);
  for (vtkIdType i = 0; i < 6; ++i)
  {
    this->Bounds[i] = bounds[i];
  }

  // Estimate the number of divisions based on the number of points in the
  // input.  (To minimize chance of overflow, force math in vtkIdType type,
  // which is sometimes bigger than int, and never smaller.)
  vtkIdType target = numPts;
  vtkIdType numDiv = static_cast<vtkIdType>(this->NumberOfXDivisions) * this->NumberOfYDivisions *
    this->NumberOfZDivisions / 2;

  // This particular implementation is limited to 2^31 (VTK_INT_MAX) bins.
  // This is to manage memory allocation and simplify the algorithm.
  if (this->AutoAdjustNumberOfDivisions && numDiv > target)
  {
    double factor = pow(((double)numDiv / (double)target), 0.33333);
    this->NumberOfDivisions[0] = this->NumberOfXDivisions =
      (int)(0.5 + (double)(this->NumberOfXDivisions) / factor);
    this->NumberOfDivisions[1] = this->NumberOfYDivisions =
      (int)(0.5 + (double)(this->NumberOfYDivisions) / factor);
    this->NumberOfDivisions[2] = this->NumberOfZDivisions =
      (int)(0.5 + (double)(this->NumberOfZDivisions) / factor);
  }
  else
  {
    this->NumberOfDivisions[0] = this->NumberOfXDivisions;
    this->NumberOfDivisions[1] = this->NumberOfYDivisions;
    this->NumberOfDivisions[2] = this->NumberOfZDivisions;
  }

  if (this->ComputeNumberOfDivisions)
  {
    // Extend the bounds so that it will not produce fractions of bins.
    double x, y, z;
    x = floor((bounds[0] - this->DivisionOrigin[0]) / this->DivisionSpacing[0]);
    y = floor((bounds[2] - this->DivisionOrigin[1]) / this->DivisionSpacing[1]);
    z = floor((bounds[4] - this->DivisionOrigin[2]) / this->DivisionSpacing[2]);
    this->Bounds[0] = this->DivisionOrigin[0] + (x * this->DivisionSpacing[0]);
    this->Bounds[2] = this->DivisionOrigin[1] + (y * this->DivisionSpacing[1]);
    this->Bounds[4] = this->DivisionOrigin[2] + (z * this->DivisionSpacing[2]);
    x = ceil((bounds[1] - this->Bounds[0]) / this->DivisionSpacing[0]);
    y = ceil((bounds[3] - this->Bounds[2]) / this->DivisionSpacing[1]);
    z = ceil((bounds[5] - this->Bounds[4]) / this->DivisionSpacing[2]);
    this->Bounds[1] = this->Bounds[0] + (x * this->DivisionSpacing[0]);
    this->Bounds[3] = this->Bounds[2] + (y * this->DivisionSpacing[1]);
    this->Bounds[5] = this->Bounds[4] + (z * this->DivisionSpacing[2]);
    this->NumberOfDivisions[0] = static_cast<int>(x);
    this->NumberOfDivisions[1] = static_cast<int>(y);
    this->NumberOfDivisions[2] = static_cast<int>(z);
    vtkLog(INFO,
      "Auto adjusted to Divisions(" << this->NumberOfDivisions[0] << ","
                                    << this->NumberOfDivisions[1] << ","
                                    << this->NumberOfDivisions[2] << ")");
  }
  else
  {
    this->DivisionOrigin[0] = bounds[0];
    this->DivisionOrigin[1] = bounds[2];
    this->DivisionOrigin[2] = bounds[4];
    this->DivisionSpacing[0] = (bounds[1] - bounds[0]) / this->NumberOfDivisions[0];
    this->DivisionSpacing[1] = (bounds[3] - bounds[2]) / this->NumberOfDivisions[1];
    this->DivisionSpacing[2] = (bounds[5] - bounds[4]) / this->NumberOfDivisions[2];
  }

  vtkBoundingBox::ClampDivisions(VTK_INT_MAX, this->NumberOfDivisions);
}

//----------------------------------------------------------------------------
int vtkBinnedDecimation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = nullptr;
  if (inInfo)
  {
    input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Ensure there is some input
  vtkIdType numPts;
  if (!input || (numPts = input->GetNumberOfPoints()) < 1)
  {
    vtkLog(INFO, "Empty input (no points).");
    return 1;
  }

  // Do a quick check as to whether triangles are available.
  vtkCellArray* inTris = input->GetPolys();
  vtkIdType numTris, triSize;
  if (inTris == nullptr || (numTris = inTris->GetNumberOfCells()) < 1 ||
    (triSize = inTris->GetNumberOfConnectivityEntries()) != numTris * 4)
  {
    vtkLog(INFO, "Empty input, or non-triangles in input.");
    return 1;
  }

  // Setup the binning (divisions,origin,spacing).
  this->ConfigureBinning(input, numPts);
  vtkIdType numBins = static_cast<vtkIdType>(this->NumberOfDivisions[0]) *
    this->NumberOfDivisions[1] * this->NumberOfDivisions[2];

  // Grab relevant information
  vtkPoints* inPts = input->GetPoints();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = (this->ProducePointData ? output->GetPointData() : nullptr);
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = (this->ProduceCellData ? output->GetCellData() : nullptr);

  // Prepare the triangle output.
  vtkNew<vtkCellArray> outTris;
  output->SetPolys(outTris);

  // Determine what type of ids are needed: smaller id types save a lot of
  // memory.
  this->LargeIds =
    ((numBins > VTK_INT_MAX || numPts > VTK_INT_MAX || numTris > VTK_INT_MAX) ? true : false);

  // Fast path: dispatch to real point types
  using vtkArrayDispatch::Reals;

  // There are four possible algorithms to take depending on the desired
  // output. Algorithms 2-3 are very similar and combined here.
  if (this->PointGenerationMode == vtkBinnedDecimation::INPUT_POINTS)
  { // Algorithm 1: reuse input points
    output->SetPoints(inPts);
    if (outPD)
    {
      outPD->PassData(inPD);
    }

    using PointReuseDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    PointReuseWorker deciWorker;
    if (!PointReuseDispatch::Execute(inPts->GetData(), deciWorker, this->LargeIds, inTris, inCD,
          outCD, this->NumberOfDivisions, this->Bounds, this->DivisionSpacing, output))
    { // Fallback to slowpath for other point types
      deciWorker(inPts->GetData(), this->LargeIds, inTris, inCD, outCD, this->NumberOfDivisions,
        this->Bounds, this->DivisionSpacing, output);
    }
  }

  // Algorithms 2 & 3: generate new points: from selecting a point, or bin centers
  else if (this->PointGenerationMode == vtkBinnedDecimation::BIN_CENTERS ||
    this->PointGenerationMode == vtkBinnedDecimation::BIN_POINTS)
  {
    using BinPointsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    BinPointsWorker deciWorker;
    if (!BinPointsDispatch::Execute(inPts->GetData(), deciWorker, inPD, outPD, this->LargeIds,
          this->PointGenerationMode, inTris, inCD, outCD, this->NumberOfDivisions, this->Bounds,
          this->DivisionSpacing, output))
    { // Fallback to slowpath for other point types
      deciWorker(inPts->GetData(), inPD, outPD, this->LargeIds, this->PointGenerationMode, inTris,
        inCD, outCD, this->NumberOfDivisions, this->Bounds, this->DivisionSpacing, output);
    }
  }

  // Algorithm 4: average points in bins
  else // if ( this->PointGenerationMode == vtkBinnedDecimation::BIN_AVERAGE )
  {
    using AvePointsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    AvePointsWorker deciWorker;
    if (!AvePointsDispatch::Execute(inPts->GetData(), deciWorker, inPD, outPD, this->LargeIds,
          inTris, inCD, outCD, this->NumberOfDivisions, this->Bounds, this->DivisionSpacing,
          output))
    { // Fallback to slowpath for other point types
      deciWorker(inPts->GetData(), inPD, outPD, this->LargeIds, inTris, inCD, outCD,
        this->NumberOfDivisions, this->Bounds, this->DivisionSpacing, output);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetNumberOfDivisions(int div0, int div1, int div2)
{
  this->NumberOfDivisions[0] = div0;
  this->NumberOfDivisions[1] = div1;
  this->NumberOfDivisions[2] = div2;
  this->SetNumberOfXDivisions(div0);
  this->SetNumberOfYDivisions(div1);
  this->SetNumberOfZDivisions(div2);
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetNumberOfXDivisions(int num)
{
  if (this->NumberOfXDivisions == num && this->ComputeNumberOfDivisions == 0)
  {
    return;
  }
  if (num < 1)
  {
    vtkLog(ERROR, "You cannot use less than one division.");
    return;
  }
  this->Modified();
  this->NumberOfXDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetNumberOfYDivisions(int num)
{
  if (this->NumberOfYDivisions == num && this->ComputeNumberOfDivisions == 0)
  {
    return;
  }
  if (num < 1)
  {
    vtkLog(ERROR, "You cannot use less than one division.");
    return;
  }
  this->Modified();
  this->NumberOfYDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetNumberOfZDivisions(int num)
{
  if (this->NumberOfZDivisions == num && this->ComputeNumberOfDivisions == 0)
  {
    return;
  }
  if (num < 1)
  {
    vtkLog(ERROR, "You cannot use less than one division.");
    return;
  }
  this->Modified();
  this->NumberOfZDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}

//----------------------------------------------------------------------------
int* vtkBinnedDecimation::GetNumberOfDivisions()
{
  static int divs[3];
  this->GetNumberOfDivisions(divs);
  return divs;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::GetNumberOfDivisions(int divs[3])
{
  divs[0] = this->NumberOfXDivisions;
  divs[1] = this->NumberOfYDivisions;
  divs[2] = this->NumberOfZDivisions;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetDivisionOrigin(double x, double y, double z)
{
  if (this->ComputeNumberOfDivisions && this->DivisionOrigin[0] == x &&
    this->DivisionOrigin[1] == y && this->DivisionOrigin[2] == z)
  {
    return;
  }
  this->Modified();
  this->DivisionOrigin[0] = x;
  this->DivisionOrigin[1] = y;
  this->DivisionOrigin[2] = z;
  this->ComputeNumberOfDivisions = 1;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::SetDivisionSpacing(double x, double y, double z)
{
  if (this->ComputeNumberOfDivisions && this->DivisionSpacing[0] == x &&
    this->DivisionSpacing[1] == y && this->DivisionSpacing[2] == z)
  {
    return;
  }
  if (x <= 0)
  {
    vtkLog(ERROR, "Spacing (x) should be larger than 0.0, setting to 1.0");
    x = 1.0;
  }
  if (y <= 0)
  {
    vtkLog(ERROR, "Spacing (x) should be larger than 0.0, setting to 1.0");
    y = 1.0;
  }
  if (z <= 0)
  {
    vtkLog(ERROR, "Spacing (x) should be larger than 0.0, setting to 1.0");
    z = 1.0;
  }
  this->Modified();
  this->DivisionSpacing[0] = x;
  this->DivisionSpacing[1] = y;
  this->DivisionSpacing[2] = z;
  this->ComputeNumberOfDivisions = 1;
}

//----------------------------------------------------------------------------
int vtkBinnedDecimation::FillInputPortInformation(int port, vtkInformation* info)
{
  int retval = this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return retval;
}

//----------------------------------------------------------------------------
void vtkBinnedDecimation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1] << " " << this->Bounds[2]
     << " " << this->Bounds[3] << " " << this->Bounds[4] << " " << this->Bounds[5] << "\n";

  if (this->ComputeNumberOfDivisions)
  {
    os << indent << "Using Spacing and Origin to construct bins\n";
  }
  else
  {
    os << indent << "Using input bounds and NumberOfDivisions to construct bins\n";
  }

  os << indent << "Division Spacing: " << this->DivisionSpacing[0] << ", "
     << this->DivisionSpacing[1] << ", " << this->DivisionSpacing[2] << endl;
  os << indent << "Division Origin: " << this->DivisionOrigin[0] << ", " << this->DivisionOrigin[1]
     << ", " << this->DivisionOrigin[2] << endl;

  os << indent << "Number of X Divisions: " << this->NumberOfXDivisions << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfYDivisions << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfZDivisions << "\n";

  os << indent << "Auto Adjust Number Of Divisions: "
     << (this->AutoAdjustNumberOfDivisions ? "On\n" : "Off\n");

  os << indent << "Point Generation Mode :" << this->PointGenerationMode << endl;
  os << indent << "Pass Point Data : " << this->ProducePointData << endl;
  os << indent << "Produce Cell Data : " << this->ProduceCellData << endl;
}
