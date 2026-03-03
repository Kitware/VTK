// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLabeledImagePointSampler.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkJogglePoints.h"
#include "vtkLabelMapLookup.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"
#include "vtkVoronoiCore.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLabeledImagePointSampler);

//------------------------------------------------------------------------------
namespace // anonymous
{

// Type of input image
enum InputImageType
{
  XYSLICE = 0,
  VOLUME = 1
};

// Determine if an integer number x is a power of y. Specialized for this
// filter.
unsigned int IsPower(unsigned int x, unsigned y)
{
  if (x == 1)
  {
    return 1; // points next to a label/region boundary are always selected.
  }
  else if (x == VTK_INT_MAX)
  {
    return 0; // distant points are never selected.
  }
  else
  {
    double r = std::log(static_cast<double>(x)) / std::log(static_cast<double>(y));
    return static_cast<unsigned int>(r == std::floor(r));
  }
}

// Determine if an integer number x is a multiple of y. Specialized for
// this filter.
unsigned int IsMultiple(unsigned int x, unsigned y)
{
  if (x == 1)
  {
    return 1; // points next to a label/region boundary are always selected.
  }
  else if (x == VTK_INT_MAX)
  {
    return 0; // distant points are never selected.
  }
  else
  {
    double r = static_cast<double>(x) / static_cast<double>(y);
    return static_cast<unsigned int>(r == std::floor(r));
  }
}

// A volume index used to iterate across edges in the x-y-z directions.
// It's used in ProcessEdge as a general purpose edge iterator for edges in
// the x, y, and z directions.
struct VIndex
{
  int EdgeType;          // the type of edge: x, y, or z
  vtkIdType Dims[3];     // the dimensions of the volume or xy image
  vtkIdType Incr;        // depending on the edge type, the index increment
  vtkIdType SliceOffset; // size of a z-slice
  vtkIdType Idx;         // the i-j-k index of the volume
  int NumVoxels;         // the number of voxels along the edge being processed

  VIndex()
    : EdgeType(0)
    , Dims{ 0, 0, 0 }
    , Incr(0)
    , Idx(0)
  {
  }

  // Configure the index for traversal across a specified edge direction. The edgeType
  // must be (0,1,2) indicating x, y, or z edges.
  void Initialize(int edgeType, vtkIdType dims[3])
  {
    this->EdgeType = edgeType;
    this->Dims[0] = dims[0];
    this->Dims[1] = dims[1];
    this->Dims[2] = dims[2];
    this->SliceOffset = (dims[2] <= 1 ? 0 : dims[0] * dims[1]);

    switch (this->EdgeType)
    {
      case 0: // edge along x direction
        this->Incr = 1;
        this->NumVoxels = this->Dims[0];
        break;
      case 1: // edge along y direction
        this->Incr = this->Dims[0];
        this->NumVoxels = this->Dims[1];
        break;
      case 2: // edge along z direction
        this->Incr = this->SliceOffset;
        this->NumVoxels = this->Dims[2];
        break;
    } // switch over x-y-z edge types
  }

  // Begin edge iteration at the specified voxel@i,j,k. Make sure
  // Initialize() is called first.
  vtkIdType Begin(int i, int j, int k)
  {
    this->Idx = i + (j * this->Dims[0]) + (k * SliceOffset);
    return this->Idx;
  }

  // Begin forward edge iteration at the specified edge number. Note that
  // this requires initialization prior to use. Returns the starting edge
  // index.
  vtkIdType Begin(vtkIdType eId)
  {
    int i = 0, j = 0, k = 0;
    switch (this->EdgeType)
    {
      case 0: // edges along x direction
        i = 0;
        j = eId % this->Dims[1];
        k = eId / this->Dims[1];
        break;
      case 1: // edges along y direction
        i = eId % this->Dims[0];
        j = 0;
        k = eId / this->Dims[0];
        break;
      case 2: // edges along z direction
        i = eId % this->Dims[0];
        j = eId / this->Dims[0];
        k = 0;
        break;
    } // switch over x-y-z edge types

    return this->Begin(i, j, k);
  }

  // Begin backward iteration at the specified edge number. Note that this
  // requires initialization prior to use. Returns the ending edge index.
  vtkIdType End(vtkIdType eId)
  {
    this->Begin(eId); // side effect sets this->Idx at start of edge
    this->Idx += ((this->NumVoxels - 1) * this->Incr); // advance to end of edge
    return this->Idx;
  }

  // Operator= updates the iteration index. All other data membera were set
  // by the Initialize() method.
  VIndex(const VIndex& v) = default;
  void operator=(VIndex& v) { this->Idx = v.Idx; }

  // Return the current index value.
  vtkIdType operator()() { return this->Idx; }

  // Advance the iterator in the forward direction (prefix ++idx).
  VIndex& operator++()
  {
    this->Idx += this->Incr;
    return *this;
  }

  // Advance the iterator in the backward direction (prefix --idx).
  VIndex& operator--()
  {
    this->Idx -= this->Incr;
    return *this;
  }

  // Return the number of voxels along the current edge.
  int GetNumVoxelsAlongEdge() { return this->NumVoxels; }
};

// Process a volume edge. The basic idea is to label every voxel with a
// "distance" measure from region boundaries (as specified by the input
// labels). This is done by sweeping across the edge, first in the positive
// direction, then in the negative direction (i.e., a two-pass process).
// Voxels adjacent to a region boundary are set to a distance=1. The
// distance increases +1 moving away from the boundary. Because the
// algorithm processes the volume in x, then y, then z directions, the
// distance will be affected from region boundaries in all directions. The
// minimum distance value is iteratively updated as each x-y-z pass
// proceeds. Note that in the interest of simplicity and reduced code, the
// ProcessEdge functor will handle x, y, or z edges at the cost of some
// inefficiency.  Performance improvements are possible by specializing
// the processing for separate x-y-z edges.
//
// Given a volume of size dims[3], starting at voxel ijk, and incrementing
// as specified by the edge type.
struct ProcessEdge
{
  int EdgeType;                        // the type of edge (x,y, or z) being processed
  vtkIdType Dims[3];                   // input volume dimensions
  const int* RegionIds;                // input region ids (segmentation labels)
  unsigned int* Distances;             // voxel distances from label boundaries
  vtkLabeledImagePointSampler* Filter; // owning filter instance

  // Each thread requires its own pair of volume indices for iteration.
  // Volume indices are initialized by the threads by the SMPTools Initialize().
  vtkSMPThreadLocal<VIndex> VLeft;
  vtkSMPThreadLocal<VIndex> VRight;

  // Each thread requires its own label lookup instance.
  vtkSMPThreadLocal<vtkLabelMapLookup<int>*> LMap;

  ProcessEdge(int edgeType, int dims[3], const int* regionIds, unsigned int* distances,
    vtkLabeledImagePointSampler* filter)
    : EdgeType(edgeType)
    , Dims{ dims[0], dims[1], dims[2] }
    , RegionIds(regionIds)
    , Distances(distances)
    , Filter(filter)
  {
  }

  // Return the region id from the volume index provided.
  vtkIdType GetRegionId(VIndex& v) { return this->RegionIds[v()]; }

  // Return a pointer to the distance field from the volume index provided.
  unsigned int* GetDistancePtr(VIndex& v) { return this->Distances + v(); }

  // Assign a distance value to the current voxel id.
  void AssignDistance(unsigned int dist, unsigned int* dPtr)
  {
    // Take the lesser of the values.
    *dPtr = std::min(dist, *dPtr);
  }

  // Increase the distance, but take the minimum of it and the current
  // distance.
  unsigned int IncrementDistance(unsigned int dist, unsigned int* dPtr)
  {
    if (dist < VTK_INT_MAX)
    {
      ++dist;
    }
    dist = (dist < *dPtr ? dist : *dPtr);
    return dist;
  }

  // Initialize the processing of the thread.
  void Initialize()
  {
    this->VLeft.Local().Initialize(this->EdgeType, this->Dims);
    this->VRight.Local().Initialize(this->EdgeType, this->Dims);

    // Specify the segmented label set. These are used to determine if a
    // segmented region is part of the background, or should be used to
    // produce output points.
    this->LMap.Local() = vtkLabelMapLookup<int>::CreateLabelLookup(
      this->Filter->GetValues(), this->Filter->GetNumberOfLabels());
  }

  // Process each edge in the current direction (based on EdgeType). Recall
  // that initally distance values have been set to VTK_INT_MAX.
  void operator()(vtkIdType edgeId, vtkIdType endEdgeId)
  {
    // Create the two iteration indices: the left and right indices. Retrieve
    // the label map lookup.
    VIndex& vL = this->VLeft.Local();
    VIndex& vR = this->VRight.Local();
    vtkLabelMapLookup<int>* lMap = this->LMap.Local();

    // Keep track of left/right region ids and distances.
    int ridL, ridR;
    unsigned int dist, *dLPtr, *dRPtr;
    bool isLabelL, isLabelR;

    // Forward and backward iteration over each edge
    int numVoxels = vL.GetNumVoxelsAlongEdge();
    for (; edgeId < endEdgeId; ++edgeId)
    {
      // Setup for forward and backward iterations. Traverse the edge,
      // setting distance values depending on region boundaries and the
      // results of previous edge passes.
      vR.Begin(edgeId);
      ridR = this->GetRegionId(vR);
      isLabelR = lMap->IsLabelValue(ridR);
      dRPtr = this->GetDistancePtr(vR);
      dist = VTK_INT_MAX; // initial distance on forward iteration
      this->AssignDistance(dist, dRPtr);

      for (int idx = 0; idx < (numVoxels - 1); ++idx)
      {
        // Forward advance along the edge (vL->vR direction). Forward iteration
        // sets the distance=1 on either side of region boundaries. It also forward
        // propagates the distance.
        vL = vR;
        ++vR;
        ridL = ridR;
        ridR = this->GetRegionId(vR);
        isLabelL = isLabelR;
        isLabelR = lMap->IsLabelValue(ridR);
        dLPtr = dRPtr;
        dRPtr = this->GetDistancePtr(vR);

        // See if we are straddling a region boundary, and mark accordingly.
        if (ridL != ridR && (isLabelL || isLabelR))
        {
          dist = 1;
          *dLPtr = dist;
          *dRPtr = dist;
        }
        else
        {
          dist = this->IncrementDistance(dist, dRPtr);
          this->AssignDistance(dist, dRPtr);
        }
      }

      // Setup for backward iteration, in the vL<-vR direction.
      vL.End(edgeId);
      dLPtr = this->GetDistancePtr(vL);
      dist = *dLPtr;

      // Now traverse edge in the backward direction, setting distance
      // values depending on region boundaries and the results of
      // previous edge passes.
      for (int idx = 0; idx < (numVoxels - 1); ++idx)
      {
        // Backward advance along the edge. This will reverse propagate
        // the distance from the left side of region boundaries. (Region
        // boundaries have already been found in the forward pass.)
        vR = vL;
        --vL;
        dRPtr = dLPtr;
        dLPtr = this->GetDistancePtr(vL);

        dist = this->IncrementDistance(dist, dLPtr);
        this->AssignDistance(dist, dLPtr);
      }
    } // for all edges in this batch
  }   // operator()

  // Clean up
  void Reduce()
  {
    // Delete all of the label map lookups. Need auto_ptr eventually.
    for (auto lmItr = this->LMap.begin(); lmItr != this->LMap.end(); ++lmItr)
    {
      delete *lmItr;
    } // over all threads
  }

}; // Process Edge

// Process the volume x-edges.
void ProcessXEdges(
  int dims[3], int* regionIds, unsigned int* dists, vtkLabeledImagePointSampler* filter)
{
  vtkIdType numXEdges = dims[1] * dims[2];
  ProcessEdge processEdge(0, dims, regionIds, dists, filter);
  vtkSMPTools::For(0, numXEdges, processEdge);
}

// Process the volume y-edges.
void ProcessYEdges(
  int dims[3], int* regionIds, unsigned int* dists, vtkLabeledImagePointSampler* filter)
{
  vtkIdType numYEdges = dims[0] * dims[2];
  ProcessEdge processEdge(1, dims, regionIds, dists, filter);
  vtkSMPTools::For(0, numYEdges, processEdge);
}

// Process the volume z-edges.
void ProcessZEdges(
  int dims[3], int* regionIds, unsigned int* dists, vtkLabeledImagePointSampler* filter)
{
  vtkIdType numZEdges = dims[0] * dims[1];
  ProcessEdge processEdge(2, dims, regionIds, dists, filter);
  vtkSMPTools::For(0, numZEdges, processEdge);
}

// Functor to mark voxels as to whether they produce an output
// point or not.
struct MarkVoxels
{
  int ImageType; // volume or xyslice
  int Dims[3];   // input volume dimensions
  vtkIdType Slice;
  unsigned int* Dists;
  const int* RegionIds;
  int DistType;
  unsigned int N;
  int OutputType;
  vtkTypeBool Randomize;
  double RandomRange[2];
  vtkLabeledImagePointSampler* Filter;

  // Used to manage region labels
  vtkIdType NumLabels;
  const double* LabelValues;
  vtkSMPThreadLocal<vtkLabelMapLookup<int>*> LMap;

  // Used to produce random sequences
  vtkSMPThreadLocal<vtkVoronoiRandom01Range> LocalGenerator;

  MarkVoxels(
    int dims[3], unsigned int* dists, const int* regionIds, vtkLabeledImagePointSampler* filter)
    : Dims{ dims[0], dims[1], dims[2] }
    , Dists(dists)
    , RegionIds(regionIds)
    , Filter(filter)
  {
    this->DistType = filter->GetDensityDistribution();
    this->N = filter->GetN();
    this->OutputType = filter->GetOutputType();

    this->NumLabels = filter->GetNumberOfLabels();
    this->LabelValues = filter->GetValues();

    this->Randomize = this->Filter->GetRandomize();
    this->Filter->GetRandomProbabilityRange(this->RandomRange);

    this->ImageType = (dims[2] <= 1 ? XYSLICE : VOLUME);
    this->Slice = this->Dims[0] * this->Dims[1];
  }

  void Initialize()
  {
    // Specify the region label set. These are used to determine
    // if a label is part of the background, or a used to produce
    // output points.
    this->LMap.Local() =
      vtkLabelMapLookup<int>::CreateLabelLookup(this->LabelValues, this->NumLabels);
  }

  // Voxels marked "0" by this functor will not produce a point; =1 will
  // produce a point.
  void operator()(vtkIdType edgeId, vtkIdType endEdgeId)
  {
    vtkLabelMapLookup<int>* lMap = this->LMap.Local();
    auto& localGen = LocalGenerator.Local();
    int numXVoxels = this->Dims[0];
    vtkIdType voxId;
    double* pRange = this->RandomRange;

    // Traverse all edges in this batch.
    for (; edgeId < endEdgeId; ++edgeId)
    {
      // Ensure that output is invariant across executions.
      localGen.Seed(edgeId);

      int j = edgeId % this->Dims[1];
      int k = (this->ImageType == XYSLICE ? 0 : edgeId / this->Dims[1]);
      int startVoxId = j * this->Dims[0] + k * this->Slice;

      // Run along x-edge
      for (int i = 0; i < numXVoxels; ++i)
      {
        voxId = startVoxId + i;
        bool isLabel = lMap->IsLabelValue(this->RegionIds[voxId]);
        if (this->OutputType == vtkLabeledImagePointSampler::LABELED_POINTS && !isLabel)
        {
          this->Dists[voxId] = 0;
          continue;
        }

        if (this->OutputType == vtkLabeledImagePointSampler::BACKGROUND_POINTS && isLabel)
        {
          this->Dists[voxId] = 0;
          continue;
        }

        // Prefilter with probability distribution
        if (this->Randomize)
        {
          double pThresh = 1.0 / static_cast<double>(this->Dists[voxId]);
          // Clamp to probability range
          if (pThresh < pRange[0])
          {
            pThresh = pRange[0];
          }
          else if (pThresh > pRange[1])
          {
            pThresh = pRange[1];
          }

          if (localGen.Next() > pThresh)
          {
            this->Dists[voxId] = 0;
            continue;
          }
        }

        // Evaluate distribution function
        if (this->DistType == vtkLabeledImagePointSampler::EXPONENTIAL)
        {
          this->Dists[voxId] = IsPower(this->Dists[voxId], this->N);
        }
        else // if ( distType == vtkLabeledImagePointSampler::LINEAR )
        {
          this->Dists[voxId] = IsMultiple(this->Dists[voxId], this->N);
        }
      } // for all voxels alonig x-edge
    }   // for all voxel values
  }

  // Clean up
  void Reduce()
  {
    // Delete all of the label map lookups. Need auto_ptr eventually.
    for (auto lmItr = this->LMap.begin(); lmItr != this->LMap.end(); ++lmItr)
    {
      delete *lmItr;
    } // over all threads
  }

}; // MarkVoxels

// Functor to count the number of points generated along each x-edge.  This
// is followed by a prefix sum process to count the total number of points
// generated, as well as build offsets for generating the filter output
// points and region ids.
struct CountPoints
{
  vtkIdType NumXEdges;     // number of y-z x-edges
  int Dims[3];             // input volume dimensions
  double Origin;           // input volume origin
  double Spacing;          // input volume spacing
  vtkIdType Slice;         // size of a x-y slice
  unsigned int* Distances; // distances from region boundaries
  vtkIdType* EMD;          // x-edge meta data (the number of points per edge)
  vtkIdType NumPoints;     // the total number of output points

  CountPoints(vtkIdType numXEdges, int dims[3], unsigned int* distances, vtkIdType* eMD)
    : NumXEdges(numXEdges)
    , Dims{ dims[0], dims[1], dims[2] }
    , Distances(distances)
    , EMD(eMD)
    , NumPoints(0)
  {
    this->Slice = this->Dims[0] * this->Dims[1];
  }

  void Initialize() {}

  // Thread over x-edges.
  void operator()(vtkIdType edgeId, vtkIdType endEdgeId)
  {
    for (; edgeId < endEdgeId; ++edgeId)
    {
      // Get a pointer to the current x-edge data
      int j = edgeId % this->Dims[1];
      int k = edgeId / this->Dims[1];
      unsigned int* dists = this->Distances + j * this->Dims[0] + k * this->Slice;

      // Count the number of points along this edge to generate
      vtkIdType numEdgePts = 0;
      for (int i = 0; i < this->Dims[0]; ++i)
      {
        if (*dists++ > 0)
        {
          numEdgePts++;
        }
      } // for all x-edge voxels

      // Update the x-edge meta data
      this->EMD[j + k * this->Dims[1]] = numEdgePts;
    } // for all x-edges
  }

  // Prefix sum to determine total number of output points,
  // and build output point numbering.
  void Reduce()
  {
    // Build x-edge offsets
    vtkIdType numPts, totalPts = 0;
    for (vtkIdType eid = 0; eid < this->NumXEdges; ++eid)
    {
      numPts = this->EMD[eid];
      this->EMD[eid] = totalPts;
      totalPts += numPts;
    }
    // Finish off the offsets metadata and update the total points.
    this->EMD[this->NumXEdges] = this->NumPoints = totalPts;
  }
}; // CountPoints

// Now generate the output points and region ids. Produce them
// over each x-edge.
struct GeneratePoints
{
  int ImageType;                       // 3D volume, 2D x-y slice
  int Dims[3];                         // input volume dimensions
  double Origin[3];                    // input volume origin
  double Spacing[3];                   // input volume spacing
  vtkIdType Slice;                     // size of a x-y slice
  unsigned int* Distances;             // distances from region boundaries
  const int* RegionIds;                // input region ids (segmentation labels)
  vtkIdType* EMD;                      // x-edge meta data (the number of points per edge)
  bool Joggle;                         // indicate whether to joggle output points
  double JoggleRadius;                 // radius of joggle perturbation
  double* OutPts;                      // output points
  int* OutputRegionIds;                // output region ids
  bool MapBackgroundPoints;            // indicate whether to map background points
  int BackgroundPointLabel;            // the label to use for background points
  vtkLabeledImagePointSampler* Filter; // owning filter instance

  // For joggleing points
  vtkSMPThreadLocal<vtkVoronoiRandom01Range> LocalGenerator;
  // Each thread requires its own label lookup instance.
  vtkSMPThreadLocal<vtkLabelMapLookup<int>*> LMap;

  GeneratePoints(int dims[3], double origin[3], double spacing[3], unsigned int* distances,
    const int* regionIds, vtkIdType* eMD, bool joggle, double joggleRadius, double* outPts,
    int* outRegionIds, vtkLabeledImagePointSampler* filter)
    : Dims{ dims[0], dims[1], dims[2] }
    , Origin{ origin[0], origin[1], origin[2] }
    , Spacing{ spacing[0], spacing[1], spacing[2] }
    , Distances(distances)
    , RegionIds(regionIds)
    , EMD(eMD)
    , Joggle(joggle)
    , JoggleRadius(joggleRadius)
    , OutPts(outPts)
    , OutputRegionIds(outRegionIds)
    , Filter(filter)
  {
    this->ImageType = (dims[2] <= 1 ? XYSLICE : VOLUME);
    this->Slice = this->Dims[0] * this->Dims[1];

    this->MapBackgroundPoints = filter->GetBackgroundPointMapping();
    this->BackgroundPointLabel = filter->GetBackgroundPointLabel();
  }

  // Initialize the processing of the thread.
  void Initialize()
  {
    // Specify the segmented label set. These are used to determine if a
    // segmented region is part of the background, or should be used to
    // produce output points.
    this->LMap.Local() = vtkLabelMapLookup<int>::CreateLabelLookup(
      this->Filter->GetValues(), this->Filter->GetNumberOfLabels());
  }

  // Thread over x-edges.
  void operator()(vtkIdType edgeId, vtkIdType endEdgeId)
  {
    unsigned int* dists;
    const int* rids;
    double* outPts = this->OutPts;
    int* outRIds = this->OutputRegionIds;
    bool mapBackgroundPoints = this->MapBackgroundPoints;
    int backgroundPointLabel = this->BackgroundPointLabel;
    vtkLabelMapLookup<int>* lMap = this->LMap.Local();
    int* dims = this->Dims;
    double* origin = this->Origin;
    double* spacing = this->Spacing;
    auto& localGen = LocalGenerator.Local();
    bool joggle = this->Joggle;
    double radius = this->JoggleRadius;

    for (; edgeId < endEdgeId; ++edgeId)
    {
      // Get pointers to the beginning of the current x-edge data.
      int j = edgeId % this->Dims[1];
      int k = (this->ImageType == XYSLICE ? 0 : edgeId / this->Dims[1]);
      dists = this->Distances + j * this->Dims[0] + k * this->Slice;
      rids = this->RegionIds + j * this->Dims[0] + k * this->Slice;

      // Now run across the x-edge
      vtkIdType numEdgePts = this->EMD[edgeId + 1] - this->EMD[edgeId];
      vtkIdType startPtId = this->EMD[edgeId]; // number previously counted
      double* xOut = outPts + 3 * startPtId;
      int* ridsOut = outRIds + startPtId;
      int pointCount = 0; // the number of points produced

      if (joggle)
      {
        localGen.Seed(edgeId); // produce invariant output
      }

      // The loop is completed when the number of points produced
      // is equal to the number of points previously counted.
      for (int i = 0; i < dims[0] && pointCount < numEdgePts; ++i)
      {
        if (dists[i] > 0) // produce the point
        {
          xOut[0] = origin[0] + i * spacing[0];
          xOut[1] = origin[1] + j * spacing[1];
          xOut[2] = origin[2] + k * spacing[2];
          if (joggle)
          {
            if (this->ImageType == XYSLICE)
            {
              vtkVoronoiJoggle::JoggleXY(xOut, xOut, radius, localGen);
            }
            else
            {
              vtkVoronoiJoggle::JoggleXYZ(xOut, xOut, radius, localGen);
            }
          }
          int regionId = *(rids + i);
          if (mapBackgroundPoints && !lMap->IsLabelValue(regionId))
          {
            regionId = backgroundPointLabel;
          }
          *ridsOut = regionId;
          xOut += 3;
          ridsOut++;
          pointCount++;
        }
      } // for all data along this x-edge
    }   // for all x-edges in this batch of edges
  }

  // Clean up
  void Reduce()
  {
    // Delete all of the label map lookups. Need auto_ptr eventually.
    for (auto lmItr = this->LMap.begin(); lmItr != this->LMap.end(); ++lmItr)
    {
      delete *lmItr;
    } // over all threads
  }

}; // GeneratePoints

// Generate the output points and regions ids point data. After the
// previous edge processing, we have distance values for each voxel. We
// first select the points to output by marking voxels 0/1 based on
// distance value and selection criterion. Then we count the number of
// output points (marked=1) using a prefix sum operation, and then write
// the points out. An option exists to only generate points that reside
// within labeled and/or background regions.
void ProducePoints(int dims[3], double origin[3], double spacing[3], bool joggle,
  double joggleRadius, const int* regionIds, unsigned int* dists,
  vtkLabeledImagePointSampler* filter, vtkPoints* newPts, vtkCellArray* verts,
  vtkIntArray* ptRegions)
{
  // Mark the voxels as to whether to generate a points. Repurpose the
  // distances array for this, marking voxels 0/1 according to the point
  // distribution function selected, and whether to only produce labeled
  // points. Note that the threading occurs over x-edges--this is partly
  // for performance, but also to produce invariant output if randomization
  // is on. (X-edge ids are used as a seed to the randomization sequence.)
  vtkIdType numXEdges = dims[1] * dims[2]; // y-z plane
  MarkVoxels markVoxels(dims, dists, regionIds, filter);
  vtkSMPTools::For(0, numXEdges, markVoxels);

  // Now count the points generated by each x-edge, and perform a prefix sum
  // to produce offsets into the output points array. We'll need to create
  // a metadata vector corresponding to the y-z plane of x-edges.
  std::vector<vtkIdType> eMD(numXEdges + 1, 0); // extra for offsets
  CountPoints countPoints(numXEdges, dims, dists, eMD.data());
  vtkSMPTools::For(0, numXEdges, countPoints);
  vtkIdType numPts = countPoints.NumPoints;

  // Now allocate the output points and region ids. Threading occurs over
  // all x-edges.
  newPts->SetNumberOfPoints(numPts);
  double* newPtsPtr = vtkDoubleArray::FastDownCast(newPts->GetData())->GetPointer(0);
  ptRegions->SetNumberOfTuples(numPts);
  GeneratePoints genPoints(dims, origin, spacing, dists, regionIds, eMD.data(), joggle,
    joggleRadius, newPtsPtr, ptRegions->GetPointer(0), filter);
  vtkSMPTools::For(0, numXEdges, genPoints);

  // For rendering, a vertex cell must be generated.
  if (verts)
  {
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(2);
    offsets->SetValue(0, 0);
    offsets->SetValue(1, numPts);

    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(numPts);
    vtkIdType* c = conn->GetPointer(0);
    vtkIdType ptId = 0;
    std::generate(c, c + numPts, [&] { return ptId++; });

    verts->SetData(offsets, conn);
  }

} // ProducePoints

} // anonymous namespace

//------------------------------------------------------------------------------
// Here is the VTK class proper.
// Construct object with a single contour value of 0.0.
vtkLabeledImagePointSampler::vtkLabeledImagePointSampler()
{
  this->BackgroundPointMapping = false;
  this->BackgroundPointLabel = (-100);

  this->DensityDistribution = vtkLabeledImagePointSampler::EXPONENTIAL;
  this->N = 2;
  this->OutputType = vtkLabeledImagePointSampler::ALL_POINTS;

  this->Randomize = true;
  this->RandomProbabilityRange[0] = 0.0;
  this->RandomProbabilityRange[1] = 1.0;

  this->GenerateVerts = true;

  this->Joggle = true;
  this->JoggleRadius = 0.001;
  this->JoggleRadiusIsAbsolute = false;
  this->ConstrainJoggle = false;
  this->JoggleConstraint = 0.666667;

  this->Labels = vtkSmartPointer<vtkContourValues>::New();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkLabeledImagePointSampler::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->Labels->GetMTime();
  return (mTime2 > mTime ? mTime2 : mTime);
}

//------------------------------------------------------------------------------
int vtkLabeledImagePointSampler::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing point sampling filter");

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // to be safe recompute the update extent
  this->RequestUpdateExtent(request, inputVector, outputVector);

  // Make sure that the image has been segmented, is of type vtkIntArray, and has one
  // component.
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars)
  {
    vtkErrorMacro("No scalar labels.");
    return 0;
  }

  vtkSmartPointer<vtkIntArray> rIds;
  rIds = vtkIntArray::FastDownCast(inScalars);
  if (inScalars && !rIds)
  {
    vtkWarningMacro("Region labels must be of type vtkIntArray, converting to int");
    rIds = ConvertRegionLabels(inScalars);
  }
  if (rIds)
  {
    if (rIds->GetNumberOfComponents() > 1)
    {
      vtkErrorMacro("Region Ids must have 1 component");
      return 0;
    }
  }
  int* regionIds = rIds->GetPointer(0);

  // Determine the extent, make sure it's a volume or an x-y plane
  int* inExt = input->GetExtent();
  int exExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), exExt);
  for (int i = 0; i < 3; i++)
  {
    exExt[2 * i] = std::max(inExt[2 * i], exExt[2 * i]);
    exExt[2 * i + 1] = std::min(inExt[2 * i + 1], exExt[2 * i + 1]);
  }
  if (exExt[0] >= exExt[1] || exExt[2] >= exExt[3])
  {
    vtkDebugMacro(<< "Filter requires 3D volume data or XY image");
    return 0;
  }
  // We'll need the volume origin and spacing later.
  double origin[3], spacing[3];
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  // Okay, now process the data
  int dims[3];
  dims[0] = exExt[1] - exExt[0] + 1;
  dims[1] = exExt[3] - exExt[2] + 1;
  dims[2] = exExt[5] - exExt[4] + 1;
  vtkIdType numVoxels = dims[0] * dims[1] * dims[2];

  // Distances from region boundaries are large to begin with, and adjusted
  // down as edges are processed.
  vtkNew<vtkUnsignedIntArray> distances;
  distances->SetNumberOfTuples(numVoxels);
  unsigned int* dists = distances->GetPointer(0);
  vtkSMPTools::Fill(dists, dists + numVoxels, VTK_INT_MAX);

  // Parallel process the edges in the x, y and z directions to compute
  // distances from region boundaries.
  ProcessXEdges(dims, regionIds, dists, this);
  ProcessYEdges(dims, regionIds, dists, this);
  if (dims[2] > 1) // check if a xyslice or a volume
  {
    ProcessZEdges(dims, regionIds, dists, this);
  }

  // Now we are ready to generate output points. We first mark the voxels
  // which will produce output points, count the number of resulting points,
  // perform a prefix sum to produce offsets, and generate the points in
  // parallel. A voxel marked to produce an output point will generated the
  // point at the voxel position, along with the region id to which the
  // voxel belongs.
  vtkNew<vtkPoints> newPts;
  newPts->SetDataTypeToDouble();
  vtkSmartPointer<vtkCellArray> verts;
  if (this->GenerateVerts)
  {
    verts = vtkSmartPointer<vtkCellArray>::New();
  }
  vtkNew<vtkIntArray> ptRegions;
  ptRegions->SetName("Labeled Points");
  double joggleRadius = this->JoggleRadius;
  if (!this->JoggleRadiusIsAbsolute)
  {
    joggleRadius *= input->GetLength();
  }
  if (this->ConstrainJoggle)
  {
    double minSpacing = std::min(spacing[0], spacing[1]);
    minSpacing = (spacing[2] > 0 ? std::min(minSpacing, spacing[2]) : minSpacing);
    joggleRadius = std::min(joggleRadius, minSpacing * this->JoggleConstraint);
  }

  ProducePoints(dims, origin, spacing, this->Joggle, joggleRadius, regionIds, dists, this, newPts,
    verts, ptRegions);

  // Update the output
  output->SetPoints(newPts);
  if (verts)
  {
    output->SetVerts(verts); // needed for rendering
  }
  output->GetPointData()->SetScalars(ptRegions);

  // Report output
  vtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points");

  return 1;
}

//------------------------------------------------------------------------------
int vtkLabeledImagePointSampler::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkLabeledImagePointSampler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Labels->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Background Point Mapping: " << (this->BackgroundPointMapping ? "On\n" : "Off\n");
  os << indent << "Background Point Label: " << this->BackgroundPointLabel << endl;
  os << indent << "Density Distribution: " << this->DensityDistribution << endl;
  os << indent << "N: " << this->N << endl;
  os << indent << "Output Type: " << this->OutputType << endl;

  os << indent << "Randomize: " << (this->Randomize ? "On\n" : "Off\n");
  os << indent << "Random Probability Range: (" << this->RandomProbabilityRange[0] << ", "
     << this->RandomProbabilityRange[1] << ")\n";

  os << indent << "Joggle: " << (this->Joggle ? "On\n" : "Off\n");
  os << indent << "Joggle Radius: " << this->JoggleRadius << endl;
  os << indent
     << "Joggle Radius Is Absolute: " << (this->JoggleRadiusIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Constrain Joggle: " << (this->ConstrainJoggle ? "On\n" : "Off\n");
  os << indent << "Joggle Constraint: " << this->JoggleConstraint << endl;

  os << indent << "Generate Verts: " << (this->GenerateVerts ? "On\n" : "Off\n");
}

VTK_ABI_NAMESPACE_END
