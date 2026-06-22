// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_7_0()
#define VTK_DEPRECATION_LEVEL 0
#include "vtkSurfaceNets3D.h"
#include "vtkSurfaceNets3DNonManifoldCases.h"

#include "vtkAppendPolyData.h"
#include "vtkArrayComponents.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageTransform.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLabelMapLookup.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSurfaceNetsAtlas.h"
#include "vtkTriangle.h"
#include "vtkTypeInt8Array.h"

#include <cstdint>
#include <limits>
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSurfaceNets3D);

//============================================================================
// The generation of surface nets consists of two major steps: 1) Extract a
// boundary surface from the labeled data, and 2) smooth the surface to
// improve its quality. (In the case of 3D, the "surface" is either a quad or
// triangle mesh.) Note that the smoothing of the surface requires smoothing
// stencils, which connect points in the center of voxel cells to potential
// points from face neighbors, and is used in an iterative smoothing
// process. In this implementation of surface nets, a
// vtkConstrainedSmoothingFilter performs the smoothing.
//
// A templated surface nets extraction algorithm implementation follows. It
// uses an edge-by-edge parallel algorithm (aka flying edges approach) for
// performance.
//
// The surface extraction portion of this implementation is organized as five
// passes: 1) classify x-edges; 2) classify y-z-edges and voxels; 3) prefix sum
// and allocate output data; 4) generate an auxiliary EdgeRowIndices array
// (recording the x-position of each generated point); and 5) generate points,
// polygons, and optional scalar data. The EdgeRowIndices pass accelerates
// output generation because the majority of trimmed interval [xMin_i,xMax_i) of a
// row contains many triads that do not actually emit points.
//
// This extra EdgeRowIndices pass is an optimization applicable to any discrete
// (label-map-based) Flying Edges-style algorithm (e.g., Discrete Flying Edges);
// it is simply implemented here. Because label maps often contain large homogeneous
// regions, the skip rate within a trim interval is high. A typical continuous
// Flying Edges-style pipeline would proceed directly from the prefix sum (Pass 3)
// to output generation, where this skipping is less beneficial.
//
// Following surface extraction, an optional smoothing step may be applied to
// improve mesh quality.
//
// Some terminology: Eight voxel points (which in VTK is point-associated data) are
// combined to create regular hexahedron (which in VTK are voxel
// cells). (Note that since surface nets operates on the "dual" of a standard
// VTK image, there is potential confusion for the meaning of a voxel. In the
// surface nets algorithm, a voxel is a region of constant value surrounding
// a data point, and a cube/voxel cell is the hexahedral region connecting eight
// voxels). We associate a triad with each voxel which is composed of
// the three voxel edges emanating from the voxel origin (lower-left corner)
// in the +x, +y, +z directions.  The triad carries information about the
// classification of the three voxel edges and associated voxel. Triads are
// combined (from neighboring voxels) to create voxel cell edge and face "cases"
// which in turn defines the polygonals and smoothing stencils to
// generate. This triad information is also aggregated to configure the
// filter output, and controls the generation of the output boundary polygons
// (and smoothing stencils).
//
// Implementation detail: a triad is associated with each voxel, except on
// the boundaries. On the boundaries, a layer of extra triads "pads" the
// volume.  This is done to simplify the generation of the surface net (i.e.,
// due to typical boundary effects when processing images), and to enable the
// resulting boundary edges to extend 1/2 voxel beyond the edges of the
// volume, since we are stretching VTK's definition of a voxel (value at a
// point) to be a region of constant value.
//
// The reason for triads is that they can be independently computed in
// parallel (without race conditions), and then later combined to provide
// information about the voxel cell that they define. The triads are combined
// to produce a 12-bit "edge case" number, and a 6-bit "face case"
// number. The edge case number indicates, for each of the 12 voxel edges,
// which edges are "intersected" meaning the end values of an edge are in two
// separate labeled regions. The face case number is used to define a
// smoothing stencil: for each of the six voxel faces, which faces are
// connected via a smoothing edge to their face neighbor.
//
// The edge case number ranges from [0,2**12), considering the XIntersection,
// YIntersection, and ZIntersection bits from all the contributing triads
// on each of the 12 voxel edges; and the face case number ranges from
// [0,2**6) (indicating stencil connections on the 6 voxel faces). There is a
// dependent relationship between the edge and face case numbers. For every
// intersected edge, then the two voxel faces using that edge will also be
// "intersected" by a smoothing stencil connection. Therefore, an edge case
// number can be converted into a face case number. Note because the edge
// case number exceeds what is representable by an 8-bit unsigned char, the
// edge case number is represented by a 16-bit unsigned short (but is
// converted to an 8-bit unsigned char, face case number as needed). (One
// interesting note about edge cases: although as implemented here the 12
// voxel edges can be set independently, this is geometrically impossible
// since if one edge is intersected, than at least two others must be as
// well. It would be a fun little research project to flesh out the
// relationships between edge cases and face cases etc.)

namespace
{ // anonymous

// The SurfaceNets struct below implements the core of the surface nets'
// algorithm. It uses a flying edges approach to parallel process data
// edge-by-edge, which provides edge-based parallel tasking, reduces the
// number of voxel lookups and eliminates costly coincident point merging.
//
// Logically, the surface extraction portion of the Surface Nets algorithm is
// implemented over five passes. In Pass#1 and Pass#2, the triads are
// classified and used to gather information about the voxels. In particular,
// the information gathered is whether the x-edge, y-edge, and/or z-edge
// requires "intersection", whether a point needs to be inserted into the
// center of the voxel, and whether the voxel origin point/triad origin is
// inside any labeled region, or outside. In Pass#3, a prefix sum is used
// to characterize the output, and allocate the appropriate output
// arrays. In Pass#4, the algorithm generates EdgeRowIndices, which record the
// x-position of the triad that produced each output point (multiple
// consecutive entries share the same x when a non-manifold triad produces more
// than one point). This additional pass is used to accelerate output
// generation (Pass#5) when the trimmed interval [xMin_i,xMax_i) still contains
// many non-emitting triads, and is an optimization beyond a typical Flying
// Edges-style implementation.
//
// Following the surface extraction, an optional smoothing operation is used
// to improve the quality of the output. Prior to smoothing, a quad polygon
// surface mesh is produced; but if smoothing occurs the quad polygon mesh is
// (typically) triangulated since smoothing generally causes the quads to
// become non-planar.
//
// A key concept of this implementation is EdgeMetaData. The edge metadata
// maintains information about each volume x-edge (i.e., row) which is necessary
// for threading the implementation. The information maintained is: the number
// points produced along the x-row; the number of quad primitives produced
// from this row; the number of stencil edges. Note that eMD is transformed in Pass3:
// a prefix sum operation accumulates the counts in preparation for output
// generation in Pass5. A parallel EdgeTrimType array (EdgeTrims) separately
// tracks xMin_i and xMax_i (the so-called trim edges used for computational
// trimming) and is not part of the prefix sum. Note that eMD defines a y-z
// "plane" of volume x-edges (including padding) which keeps track of
// information needed to thread the algorithm across the volume x-edges.
//
// Another way to look at this: the edge metadata characterizes each row of
// voxel triads. eMD keeps track of the number of points, quads, and stencil
// edges generated by a row of voxel triads. EdgeTrims maintains the clipped
// region [xMin_i,xMax_i) or [xL,xR) along the edge of voxel triads in which
// primitives may be generated (i.e., tracks computational trimming). Together
// they provide the bookkeeping necessary to support threaded computing.

// Some structs
struct EdgeMetaDataType
{
  vtkIdType NumPoints = 0;       // number of points produced along this row
  vtkIdType NumQuads = 0;        // number of quad primitives produced along this row
  vtkIdType NumStencilEdges = 0; // number of stencil edges
  EdgeMetaDataType& operator+=(const EdgeMetaDataType& other)
  {
    this->NumPoints += other.NumPoints;
    this->NumQuads += other.NumQuads;
    this->NumStencilEdges += other.NumStencilEdges;
    return *this;
  }
  EdgeMetaDataType operator+(const EdgeMetaDataType& other) const
  {
    EdgeMetaDataType result = *this;
    result += other;
    return result;
  }
};

struct EdgeTrimType
{
  vtkIdType XMin = 0;  // minimum index of first intersection along this row
  vtkIdType XMax = -1; // maximum index of intersection along this row
};

// const values to access the correct dimension of the data
enum Dim : std::uint8_t
{
  X = 0,
  Y = 1,
  Z = 2
};

template <typename TArray, typename TEdgeRowIndex>
struct SurfaceNets
{
  // Some typedefs to clarify code.
#ifndef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
  using TriadType = std::uint8_t;
#else
  using TriadType = std::uint16_t;
#endif
  using EdgeCaseType = unsigned short;
  using FaceCaseType = unsigned char;
  using TrimmedEdgesCaseType = unsigned char;
  using NonManifoldCases = vtkSurfaceNets3DNonManifoldCases;
  using NonManifoldCaseType = NonManifoldCases::NonManifoldCaseType;
  using VoxelCaseType = NonManifoldCases::VoxelCaseType;
  using VoxelNeighborhood = NonManifoldCases::VoxelNeighborhood<vtk::GetAPIType<TArray>>;
  using EdgeRowIndexType = TEdgeRowIndex;

  // The triad classification carries information on 8/16 different bits.
  // 1. Bit 1 indicates whether the origin of the triad is inside or outside
  //    *any* labeled region.
  // 2. Bit 2 indicates whether the x-edge needs intersection (i.e., a surface net
  //    passes through it);
  // 3. Bit 3 indicates whether the y-edge needs intersection;
  // 4. Bit 4 indicates whether the z-edge needs intersection.
#ifndef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
  // 5. Bits 5-8 (4 bits total) encode a state for generated points:
#else
  // 5. Bits 5-9 (5 bits total) encode a state for generated points:
#endif
  // Triad edges require intersection when the two end point values are not equal to one
  // another, and at least one of the end point values is "Inside" a labeled region.
  enum TriadClassification : TriadType
  {
    Outside = 0,       // triad origin point is outside any labeled region
    Inside = 1,        // triad origin inside some labeled region
    XIntersection = 2, // triad x-axis requires intersection
    YIntersection = 4, // triad y-axis requires intersection
    ZIntersection = 8, // triad z-axis requires intersection
#ifndef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
    // 8-bit mode: Use 4 bits (Values 0-15) to support Index 0-8
    StateMask = 0b1111'0000,
#else
    // 16-bit mode: Use 5 bits (Values 0-16) to support Index 0-9
    StateMask = 0b0000'0001'1111'0000,
#endif
  };

  // Given an array of 7 triads defining a voxel cell (from the points: (x,y,z); ([x+1],y,z);
  // (x,[y+1],z); ([x+1],[y+1],z); (x,y,[z+1]); ([x+1],y,[z+1]); (x,[y+1],[z+1])), compute
  // the edge case number for this voxel cell. Note that a resulting value of zero means that
  // the voxel cell is not intersected (i.e., no edge is intersected).
  static EdgeCaseType ComputeEdgeCase(const TriadType triads[7])
  {
    // x-edges
    EdgeCaseType edgeCase = (triads[0] & TriadClassification::XIntersection) >> 1;
    edgeCase |= (triads[2] & TriadClassification::XIntersection);
    edgeCase |= (triads[4] & TriadClassification::XIntersection) << 1;
    edgeCase |= (triads[6] & TriadClassification::XIntersection) << 2;
    // y-edges
    edgeCase |= (triads[0] & TriadClassification::YIntersection) << 2;
    edgeCase |= (triads[1] & TriadClassification::YIntersection) << 3;
    edgeCase |= (triads[4] & TriadClassification::YIntersection) << 4;
    edgeCase |= (triads[5] & TriadClassification::YIntersection) << 5;
    // z-edges
    edgeCase |= (triads[0] & TriadClassification::ZIntersection) << 5;
    edgeCase |= (triads[1] & TriadClassification::ZIntersection) << 6;
    edgeCase |= (triads[2] & TriadClassification::ZIntersection) << 7;
    edgeCase |= (triads[3] & TriadClassification::ZIntersection) << 8;

    return edgeCase;
  }

  // Given a pointer to a voxel's triad, first determine the seven triad cases
  // (from the points defining a voxel cell: (x,y,z); ([x+1],y,z); (x,[y+1],z);
  // ([x+1],[y+1],z); (x,y,[z+1]); ([x+1],y,[z+1]); (x,[y+1],[z+1]), and then
  // compute the edge case number for this voxel cell. Note that a resulting
  // value of zero means that the voxel cell is not intersected (i.e., no edge is
  // intersected). This method assumes that the triadPtr is not on the boundary
  // of the padded volume.
  EdgeCaseType GetEdgeCase(const TriadType* triadPtr)
  {
    TriadType triads[7];
    triads[0] = triadPtr[0];
    triads[1] = triadPtr[1];
    triads[2] = triadPtr[this->TriadDims[X]];
    triads[3] = triadPtr[this->TriadDims[X] + 1];
    triads[4] = triadPtr[this->TriadSliceOffset];
    triads[5] = triadPtr[this->TriadSliceOffset + 1];
    triads[6] = triadPtr[this->TriadSliceOffset + this->TriadDims[X]];

    return ComputeEdgeCase(triads);
  } // GetEdgeCase

  // Given an array of 8 triads defining the 8 corners of a voxel cell (from the points:
  // (x,y,z); ([x+1],y,z); (x,[y+1],z); ([x+1],[y+1],z); (x,y,[z+1]); ([x+1],y,[z+1]);
  // (x,[y+1],[z+1]); ([x+1],[y+1],[z+1])), compute the voxel case number for this voxel
  // cell. Note that a resulting value of zero means that no corner is inside a labeled region.
  VoxelCaseType ComputeVoxelCase(const TriadType triads[8])
  {
    static_assert(TriadClassification::Inside == 1,
      "ComputeVoxelCase relies on Inside == 1 to pack corner bits correctly");
    VoxelCaseType voxelCase = (triads[0] & TriadClassification::Inside);
    voxelCase |= (triads[1] & TriadClassification::Inside) << 1;
    voxelCase |= (triads[2] & TriadClassification::Inside) << 2;
    voxelCase |= (triads[3] & TriadClassification::Inside) << 3;
    voxelCase |= (triads[4] & TriadClassification::Inside) << 4;
    voxelCase |= (triads[5] & TriadClassification::Inside) << 5;
    voxelCase |= (triads[6] & TriadClassification::Inside) << 6;
    voxelCase |= (triads[7] & TriadClassification::Inside) << 7;
    return voxelCase;
  }

  // Given a pointer to a voxel's triad, first determine the eight triad cases
  // (from the points defining a voxel cell: (x,y,z); ([x+1],y,z); (x,[y+1],z);
  // ([x+1],[y+1],z); (x,y,[z+1]); ([x+1],y,[z+1]); (x,[y+1],[z+1]);
  // ([x+1],[y+1],[z+1])), and then compute the edge case, voxel case, and material
  // labels for this voxel cell. The edge case is a 12-bit bitmask indicating which
  // of the twelve edges are intersected by the surface net, and the voxel case is an
  // 8-bit bitmask indicating which of the eight corners are inside a labeled region.
  // Material labels are only computed for active corners (i.e., those inside a labeled
  // region); inactive corners are assigned the background label. This method assumes
  // that the triadPtr is not on the boundary of the padded volume.
  VoxelNeighborhood GetVoxelNeighborhood(
    TriadType* triadPtr, vtkIdType i, vtkIdType row, vtkIdType slice)
  {
    VoxelNeighborhood neigh;
    // Cache the 8 triads that define this voxel cell
    TriadType triads[8];
    triads[0] = triadPtr[0];
    triads[1] = triadPtr[1];
    triads[2] = triadPtr[this->TriadDims[X]];
    triads[3] = triadPtr[this->TriadDims[X] + 1];
    triads[4] = triadPtr[this->TriadSliceOffset];
    triads[5] = triadPtr[this->TriadSliceOffset + 1];
    triads[6] = triadPtr[this->TriadSliceOffset + this->TriadDims[X]];
    triads[7] = triadPtr[this->TriadSliceOffset + this->TriadDims[X] + 1];

    // 1. Compute the Edge Case (12 edges)
    neigh.EdgeCase = SurfaceNets::ComputeEdgeCase(triads);

    // If this edgeCase does not have non-manifold cases, it's for sure manifold, and we are done
    if (neigh.EdgeCase == 0 || NonManifoldCases::GetNumberOfNonManifoldCases(neigh.EdgeCase) == 0)
    {
      return neigh;
    }
    // else we need to compute the voxel case and query the labels

    // 2. Compute the Voxel Case (8 corners)
    neigh.VoxelCase = SurfaceNets::ComputeVoxelCase(triads);

    // 3. Get material labels only for active voxels
    const auto& vCase = neigh.VoxelCase;
    const auto& bLabel = this->BackgroundLabel;
    neigh.Labels[0] = (vCase & 1) ? this->GetVoxelForTriad(i, row, slice) : bLabel;
    neigh.Labels[1] = (vCase & 2) ? this->GetVoxelForTriad(i + 1, row, slice) : bLabel;
    neigh.Labels[2] = (vCase & 4) ? this->GetVoxelForTriad(i, row + 1, slice) : bLabel;
    neigh.Labels[3] = (vCase & 8) ? this->GetVoxelForTriad(i + 1, row + 1, slice) : bLabel;
    neigh.Labels[4] = (vCase & 16) ? this->GetVoxelForTriad(i, row, slice + 1) : bLabel;
    neigh.Labels[5] = (vCase & 32) ? this->GetVoxelForTriad(i + 1, row, slice + 1) : bLabel;
    neigh.Labels[6] = (vCase & 64) ? this->GetVoxelForTriad(i, row + 1, slice + 1) : bLabel;
    neigh.Labels[7] = (vCase & 128) ? this->GetVoxelForTriad(i + 1, row + 1, slice + 1) : bLabel;

    return neigh;
  }

  // Given a voxel cell edge case, convert it to a voxel face case. While
  // this could be done through a table, the size of the table is large
  // enough that a procedural approach simplifies the code. Basically, each
  // intersected voxel cell edge will activate two voxel faces.
  static FaceCaseType GetFaceCase(EdgeCaseType edgeCase)
  {
    FaceCaseType faceCase = 0;
    // Process each of the voxel's twelve edges. If edge is set, then set
    // the two faces using the edge.
    if (edgeCase & 1) // edge 0, faces 2 & 4
    {
      faceCase |= 20;
    }
    if (edgeCase & 2) // edge 1, faces 3 & 4
    {
      faceCase |= 24;
    }
    if (edgeCase & 4) // edge 2, faces 2 & 5
    {
      faceCase |= 36;
    }
    if (edgeCase & 8) // edge 3, faces 3 & 5
    {
      faceCase |= 40;
    }
    if (edgeCase & 16) // edge 4, faces 0 & 4
    {
      faceCase |= 17;
    }
    if (edgeCase & 32) // edge 5, faces 1 & 4
    {
      faceCase |= 18;
    }
    if (edgeCase & 64) // edge 6, faces 0 & 5
    {
      faceCase |= 33;
    }
    if (edgeCase & 128) // edge 7, faces 1 & 5
    {
      faceCase |= 34;
    }
    if (edgeCase & 256) // edge 8, faces 0 & 2
    {
      faceCase |= 5;
    }
    if (edgeCase & 512) // edge 9, faces 1 & 2
    {
      faceCase |= 6;
    }
    if (edgeCase & 1024) // edge 10, faces 0 & 3
    {
      faceCase |= 9;
    }
    if (edgeCase & 2048) // edge 11, faces 1 & 3
    {
      faceCase |= 10;
    }

    return faceCase;
  } // GetFaceCase

  // Perform analysis of voxel edge case: count the number of edge intersections on each
  // of the six voxel cell faces. Return the maximum number of edge intersections on any face.
  unsigned char CountFaceIntersections(EdgeCaseType edgeCase, unsigned char faceCounts[6])
  {
    std::fill_n(faceCounts, 6, 0);

    // Process each of the voxel's twelve edges. If edge is set, then increment
    // the two face counts using the edge.
    if (edgeCase & 1) // edge 0, faces 2 & 4
    {
      faceCounts[2]++;
      faceCounts[4]++;
    }
    if (edgeCase & 2) // edge 1, faces 3 & 4
    {
      faceCounts[3]++;
      faceCounts[4]++;
    }
    if (edgeCase & 4) // edge 2, faces 2 & 5
    {
      faceCounts[2]++;
      faceCounts[5]++;
    }
    if (edgeCase & 8) // edge 3, faces 3 & 5
    {
      faceCounts[3]++;
      faceCounts[5]++;
    }
    if (edgeCase & 16) // edge 4, faces 0 & 4
    {
      faceCounts[0]++;
      faceCounts[4]++;
    }
    if (edgeCase & 32) // edge 5, faces 1 & 4
    {
      faceCounts[1]++;
      faceCounts[4]++;
    }
    if (edgeCase & 64) // edge 6, faces 0 & 5
    {
      faceCounts[0]++;
      faceCounts[5]++;
    }
    if (edgeCase & 128) // edge 7, faces 1 & 5
    {
      faceCounts[1]++;
      faceCounts[5]++;
    }
    if (edgeCase & 256) // edge 8, faces 0 & 2
    {
      faceCounts[0]++;
      faceCounts[2]++;
    }
    if (edgeCase & 512) // edge 9, faces 1 & 2
    {
      faceCounts[1]++;
      faceCounts[2]++;
    }
    if (edgeCase & 1024) // edge 10, faces 0 & 3
    {
      faceCounts[0]++;
      faceCounts[3]++;
    }
    if (edgeCase & 2048) // edge 11, faces 1 & 3
    {
      faceCounts[1]++;
      faceCounts[3]++;
    }

    return *std::max_element(faceCounts, faceCounts + 6);
  } // CountFaceIntersections

  // Obtain information indicating whether quad polygons are to be generated
  // from the triad specified.  A triad may produce up to three quad polygons
  // corresponding to the lower left corner of a voxel. One is an x-y quad; an
  // x-z quad, and a y-z quad.
  static bool GenerateXYQuad(TriadType triad)
  {
    return (triad & TriadClassification::ZIntersection) > 0;
  }
  static bool GenerateXZQuad(TriadType triad)
  {
    return (triad & TriadClassification::YIntersection) > 0;
  }
  static bool GenerateYZQuad(TriadType triad)
  {
    return (triad & TriadClassification::XIntersection) > 0;
  }
  static bool ProducesQuad(TriadType triad)
  {
    static constexpr TriadType triadMask = TriadClassification::XIntersection |
      TriadClassification::YIntersection | TriadClassification::ZIntersection;
    return (triad & triadMask) > 0;
  }
  static uint8_t GetNumberOfQuads(TriadType triad)
  {
    uint8_t numQuads = SurfaceNets::GenerateXYQuad(triad);
    numQuads += SurfaceNets::GenerateXZQuad(triad);
    numQuads += SurfaceNets::GenerateYZQuad(triad);
    return numQuads;
  }
  // Set the number of points and non-manifold table index into the triad's high bits.
  static constexpr void SetState(TriadType& triad, uint8_t numPoints, int8_t tableIndex)
  {
    const uint8_t State = NonManifoldCases::ComputeState(numPoints, tableIndex);
    triad |= static_cast<TriadType>(State) << 4;
  }
  // Return both the point count and table index for this triad in a single table lookup.
  static constexpr VTK_ALWAYS_INLINE auto GetStateInfo(TriadType triad)
  {
    const uint8_t state = (triad & TriadClassification::StateMask) >> 4;
    return NonManifoldCases::GetStateInfo(state);
  }
  // Return whether a triad generates at least one point (cheaper than GetNumberOfPoints > 0).
  static constexpr VTK_ALWAYS_INLINE bool ProducesPoints(TriadType triad)
  {
    return (triad & TriadClassification::StateMask) != 0;
  }
  // Return the number of points to generate for this triad (1 for manifold, >=2 for non-manifold).
  static constexpr VTK_ALWAYS_INLINE uint8_t GetNumberOfPoints(TriadType triad)
  {
    const uint8_t state = (triad & TriadClassification::StateMask) >> 4;
    return NonManifoldCases::GetStateInfo(state).NumPoints;
  }
  // Return the non-manifold table index for this triad (ManifoldIndex when numPoints == 1 and
  // manifold).
  static constexpr VTK_ALWAYS_INLINE int8_t GetNonManifoldTableIndex(TriadType triad)
  {
    const uint8_t state = (triad & TriadClassification::StateMask) >> 4;
    return NonManifoldCases::GetStateInfo(state).TableIndex;
  }

  // This smoothing stencil table is indexed by the voxel face case.  For each
  // voxel cell, up to six stencil edges may be generated corresponding to
  // connections to each of the cell's six face neighbors. The table consists
  // of: 1) the number of edge connections, and 2) 0/1 values indicating
  // which of the six edge are to be generated. The table of stencil cases was
  // generated programmatically (see GenerateStencils).
  static const unsigned char StencilFaceCases[64][7];

  static unsigned char GetNumberOfStencilFaceEdges(FaceCaseType faceCase)
  {
    return SurfaceNets::StencilFaceCases[faceCase][0];
  }
  static const unsigned char* GetStencilFaceEdges(FaceCaseType faceCase)
  {
    return SurfaceNets::StencilFaceCases[faceCase];
  }
  void GenerateFaceStencils(unsigned char stencils[][7]);

  // This smoothing stencil table is indexed by the voxel *edge* case. It
  // indexes into the face-case-based smoothing stencils. This table is
  // constructed programmatically.
  constexpr uint8_t GetNumberOfStencilEdges(EdgeCaseType edgeCase)
  {
    return SurfaceNets::StencilFaceCases[this->StencilTable[edgeCase]][0];
  }
  // Given an edge case and the non-manifold case, return the total number of stencil edges.
  // This is used to allocate the correct amount of memory for the stencils, and to loop through
  // the stencils when generating them.
  constexpr uint8_t GetNumberOfStencilEdges(
    EdgeCaseType edgeCase, const int8_t tableIndex, const uint8_t numPoints)
  {
    // if manifold, return quickly
    if (VTK_LIKELY(numPoints <= 1))
    {
      return this->GetNumberOfStencilEdges(edgeCase);
    }
    // non-manifold case
    const auto manifoldSubEdgeCases =
      NonManifoldCases::GetManifoldSubEdgeCases(edgeCase, tableIndex);
    const uint8_t numManifoldSubCases = numPoints - 1;
    uint8_t totalEdges = 0;
    VTK_ASSUME(numManifoldSubCases <= 4);
    for (uint8_t j = 0; j < numManifoldSubCases; ++j)
    {
      // We create stencils for the manifold sub-edge cases as well
      totalEdges += this->GetNumberOfStencilEdges(manifoldSubEdgeCases[j]);
    }
    // We also create stencils for the edge case itself
    totalEdges += this->GetNumberOfStencilEdges(edgeCase);
    return totalEdges;
  }
  const unsigned char* GetStencilEdges(EdgeCaseType edgeCase)
  {
    return SurfaceNets::StencilFaceCases[this->StencilTable[edgeCase]];
  }
  void GenerateEdgeStencils(int optLevel = 0);

  // Input and output data.
  using TInPtr = typename vtk::detail::ValueRange<TArray, 1>::iterator;
  using TOutPtr = typename vtk::detail::ValueRange<TArray, 2>::iterator;
  using T = vtk::GetAPIType<TArray>;
  TInPtr Scalars;                  // input image scalars
  float* NewPts;                   // output points
  vtkCellArray* NewQuads;          // output quad polygons
  TOutPtr NewScalars;              // output 2-component cell scalars if requested
  int8_t* NonManifoldTableIndices; // output nonManifoldTableIndices
  vtkCellArray* NewStencils;       // output smoothing stencils

  // Internal variable to handle label processing.
  vtkIdType NumLabels;
  const double* LabelValues;
  T BackgroundLabel; // the label of any outside region

  // Internal variables used by the various algorithm methods. Interfaces VTK
  // image data in an efficient form more convenient to the algorithm.
  vtkIdType Dims[3];
  int Min[3];
  int Max[3];
  int Inc[3];

  // Algorithm-derived data for bookkeeping data locations when parallel computing.
  std::vector<TriadType> Triads;
  vtkIdType TriadDims[3];
  vtkIdType TriadSliceOffset;
  std::vector<EdgeMetaDataType> EdgeMetaData;
  std::vector<EdgeTrimType> EdgeTrims;          // per-row trim interval [XMin, XMax)
  std::vector<EdgeRowIndexType> EdgeRowIndices; // x-position per output point, indexed by point id

  // The stencil table used to obtain smoothing stencils from the voxel *edge
  // case*. This table indexes into the StencilFaceCases[64][7] using the voxel
  // edge case - this saves a few cycles (i.e., GetFaceCase(edgeCase) is not
  // called in inner loops). Also, it adds some flexibility to use different
  // smoothing stencils (e.g., optimized to better smooth edges).
  unsigned int StencilTable[4096];

  // Instantiate key data members.
  SurfaceNets()
    : NewPts(nullptr)
    , NewQuads(nullptr)
    , NewStencils(nullptr)
    , NumLabels(0)
    , LabelValues(nullptr)
    , BackgroundLabel(0)
    , Dims{ 0, 0, 0 }
    , Min{ 0, 0, 0 }
    , Max{ 0, 0, 0 }
    , Inc{ 0, 0, 0 }
    , TriadDims{ 0, 0, 0 }
    , TriadSliceOffset(0)
  {
    this->GenerateEdgeStencils(1);
  }

  // Classify the triad y-edges. Use the triad cases at both ends of the y-edge
  // first; if necessary, access the voxel values. The indices i and row are
  // expressed in the triad coordinates.
  TriadClassification ClassifyYEdge(
    TInPtr inPtr, vtkIdType i, TriadType triad0, vtkIdType row, TriadType triad1)
  {
    // If on padded boundary, edge is never intersected.
    if (row >= this->Dims[Y])
    {
      return TriadClassification::Outside;
    }

    const TriadType inout0 = triad0 & TriadClassification::Inside;
    const TriadType inout1 = triad1 & TriadClassification::Inside;
    if (inout0 == inout1)
    {
      if (inout0 == TriadClassification::Outside) // both triad origins are outside
      {
        return TriadClassification::Outside;
      }
      else // both triad origins are inside, need to check regions
      {
        const vtkIdType idx = i - 1; // shift into volume (i.e, no padding)
        const T& s0 = inPtr[idx];
        const T& s1 = inPtr[idx + this->Inc[Y]];
        return s0 == s1 ? TriadClassification::Outside : TriadClassification::YIntersection;
      }
    }
    else // one triad origin point is inside, one outside, so y-edge-intersection
    {
      return TriadClassification::YIntersection;
    }
  } // ClassifyYEdge

  // Classify the triad z-edges. Use the triad cases at both ends of the z-edge
  // first; if necessary, access the voxel values. The indices i and slice are
  // expressed in the triad coordinates.
  TriadClassification ClassifyZEdge(
    TInPtr inPtr, vtkIdType i, TriadType triad0, vtkIdType slice, TriadType triad1)
  {
    // If on padded boundary, edge is never intersected.
    if (slice >= this->Dims[Z])
    {
      return TriadClassification::Outside;
    }

    const TriadType inout0 = triad0 & TriadClassification::Inside;
    const TriadType inout1 = triad1 & TriadClassification::Inside;
    if (inout0 == inout1)
    {
      if (inout0 == TriadClassification::Outside) // both triad origins are outside
      {
        return TriadClassification::Outside;
      }
      else // both triad origins are inside, need to check regions
      {
        const vtkIdType idx = i - 1; // shift into volume (i.e., no padding)
        const T& s0 = inPtr[idx];
        const T& s1 = inPtr[idx + this->Inc[Z]];
        return s0 == s1 ? TriadClassification::Outside : TriadClassification::ZIntersection;
      }
    }
    else // one triad origin point is inside, one outside, so z-edge-intersection
    {
      return TriadClassification::ZIntersection;
    }
  } // ClassifyZEdge

  // Composite trimming information to determine which portion of the
  // volume x-edge (row,slice) to process. In particular, gather the 2x2
  // trim edge metadata that forms a row of voxel cells.
  EdgeMetaDataType* Get2x2EdgeTrim(vtkIdType row, vtkIdType slice, vtkIdType& xMin, vtkIdType& xMax)
  {
    // Gather the metadata for the four (2x2) edge rows that form a column of voxel cells.
    const vtkIdType edgeRow = slice * this->TriadDims[Y] + row;
    EdgeMetaDataType* eMDPtr = this->EdgeMetaData.data() + edgeRow;
    EdgeTrimType* eTrimPtr = this->EdgeTrims.data() + edgeRow;

    const std::array<EdgeTrimType*, 4> eTrimPtrs = {
      eTrimPtr,                         // current edge row
      eTrimPtr + 1,                     // to the right
      eTrimPtr + this->TriadDims[Y],    // above
      eTrimPtr + this->TriadDims[Y] + 1 // above and to the right
    };

    // Determine the trim over the 2x2 bundle.
    xMin = this->TriadDims[X];
    xMax = 0;
    for (auto& eTrim : eTrimPtrs)
    {
      xMin = std::min(xMin, eTrim->XMin);
      xMax = std::max(xMax, eTrim->XMax);
    }
    return eMDPtr;
  } // Get2x2EdgeTrim

  // Gather pointers for a 3x3 bundle of edge rows and their corresponding triad
  // rows centered at (row,slice) in the y-z plane.
  //
  // This helper is used by output generation, which traverses the center row
  // (k=4) and must also access up to eight neighboring rows (k!=4) to produce
  // quads and smoothing stencils. Rows on the -y/-z boundaries are represented
  // by nullptr pointers.
  void Get3x3EdgeAndTriadPointers(vtkIdType row, vtkIdType slice,
    std::array<EdgeMetaDataType*, 9>& eMDPtrs, std::array<TriadType*, 9>& triadPtrs)
  {
    // Grab the metadata for the 3x3 bundle of rows. Watch out for
    // bundles near the (-x,-y,-z) boundaries. (The (+x,+y,+z) boundaries
    // are always okay due to the nature of the padding, and iteration
    // over rows and slices).
    const vtkIdType& sliceOffset = this->TriadSliceOffset;

    // Initialize the triads and edge metadata. This simplifies the code.
    std::fill_n(eMDPtrs.begin(), 9, nullptr);
    std::fill_n(triadPtrs.begin(), 9, nullptr);

    // These portions of the bundle are always valid, with no boundary issues.
    eMDPtrs[4] = this->EdgeMetaData.data() + (slice * this->TriadDims[Y] + row); // current edge row
    triadPtrs[4] = this->Triads.data() + row * this->TriadDims[X] + slice * sliceOffset;

    eMDPtrs[5] = eMDPtrs[4] + 1; // to the right of the current edge
    triadPtrs[5] = triadPtrs[4] + this->TriadDims[X];

    eMDPtrs[7] = eMDPtrs[4] + this->TriadDims[Y]; // above the current edge
    triadPtrs[7] = triadPtrs[4] + sliceOffset;

    eMDPtrs[8] = eMDPtrs[7] + 1; // above and to the right of the current edge
    triadPtrs[8] = triadPtrs[7] + this->TriadDims[X];

    // May be near the -x,-y,-z boundaries.
    // If at origin of y-z plane.
    if (row != 0 && slice != 0)
    {
      eMDPtrs[0] = eMDPtrs[4] - 1 - this->TriadDims[Y];
      triadPtrs[0] = triadPtrs[4] - this->TriadDims[X] - sliceOffset;
    }

    if (slice != 0) // if not on -z boundary
    {
      eMDPtrs[1] = eMDPtrs[4] - this->TriadDims[Y];
      triadPtrs[1] = triadPtrs[4] - sliceOffset;

      eMDPtrs[2] = eMDPtrs[4] + 1 - this->TriadDims[Y];
      triadPtrs[2] = triadPtrs[4] + this->TriadDims[X] - sliceOffset;
    }

    if (row != 0) // if not on -y boundary
    {
      eMDPtrs[3] = eMDPtrs[4] - 1;
      triadPtrs[3] = triadPtrs[4] - this->TriadDims[X];

      eMDPtrs[6] = eMDPtrs[4] - 1 + this->TriadDims[Y];
      triadPtrs[6] = triadPtrs[4] - this->TriadDims[X] + sliceOffset;
    }
  } // Get3x3EdgeAndTriadPointers

  // Initialize and advance iterators used during output generation.
  //
  // Output generation traverses a 3x3 bundle of edge rows centered at k=4.
  // Each edge row has an associated contiguous range of output point ids
  // [eMD[k]->NumPoints, next(eMD[k])->NumPoints). The EdgeRowIndices array (built
  // in Pass 4) maps each output point id in that range to the x-position of the
  // generating triad within the corresponding edge row.
  //
  // InitRowIterator initializes pointIds/endPtIds for the 3x3 bundle and performs
  // a one-time pre-alignment: neighboring rows are advanced to the first
  // x-position present in the current row. Subsequent iterations maintain this
  // invariant via AdvanceRowIterator().
  void InitRowIterator(const std::array<EdgeMetaDataType*, 9>& eMDPtrs,
    std::array<vtkIdType, 9>& pointIds, std::array<vtkIdType, 9>& endPtIds)
  {
    assert(eMDPtrs[4] != nullptr); // current row is never a boundary row, so always has metadata
    // Initialize point numbering and build end-of-section point ids for each row
    // in the 3x3 bundle (used to bound EdgeRowIndices traversal). For null rows
    // (on boundaries), use -1 so that pointIds[k] = -1 never satisfies
    // pointIds[k] < endPtIds[k] and we never index EdgeRowIndices.
    const EdgeMetaDataType* eMDEnd = this->EdgeMetaData.data() + this->EdgeMetaData.size();
    for (int k = 0; k < 9; ++k)
    {
      const auto eMDPtr = eMDPtrs[k];
      if (eMDPtr == nullptr)
      {
        pointIds[k] = -1;
        endPtIds[k] = -1;
        continue;
      }

      pointIds[k] = eMDPtr->NumPoints;
      const EdgeMetaDataType* nextEMD = eMDPtr + 1;
      endPtIds[k] = (nextEMD < eMDEnd) ? nextEMD->NumPoints : this->EdgeRowIndices.size();
    }

    // One-time pre-alignment: catch neighboring rows up to the first active
    // position in the current row. Subsequent iterations will maintain this
    // invariant via AdvanceIteratorsForNextPosition().
    const vtkIdType endPtId4 = endPtIds[4];
    if (pointIds[4] < endPtId4)
    {
      const EdgeRowIndexType i = this->EdgeRowIndices[pointIds[4]];
      // Neighbor rows live in the 3x3 bundle excluding the current row k=4.
      // Use two tight loops (0..3 and 5..8) to keep the hot path simple.
      for (int k = 0; k < 4; ++k)
      {
        while (pointIds[k] < endPtIds[k] && this->EdgeRowIndices[pointIds[k]] < i)
        {
          ++pointIds[k];
        }
      }
      for (int k = 5; k < 9; ++k)
      {
        while (pointIds[k] < endPtIds[k] && this->EdgeRowIndices[pointIds[k]] < i)
        {
          ++pointIds[k];
        }
      }
    }
  }

  // Single-call iterator update used by the main traversal loop. After generating
  // output at position i, this method:
  // 1) Advances all rows (including k=4) past i (consumes all points at i).
  // 2) If another active position remains in the current row (k=4), advances
  //    neighboring rows (k != 4) forward to the first point at position >= nextI.
  //
  // This maintains the invariant that at the top of the next iteration, neighbor
  // pointIds are already aligned with the current position (the next active i).
  VTK_ALWAYS_INLINE void AdvanceRowIterator(uint8_t numPoints4, std::array<vtkIdType, 9>& pointIds,
    const std::array<vtkIdType, 9>& endPtIds)
  {
    // Advance past the current position. For manifold voxels numPoints==1 (advance by 1);
    // for non-manifold voxels numPoints>=2 (skip all duplicate entries at this x).
    pointIds[4] += numPoints4;
    if (pointIds[4] >= endPtIds[4])
    {
      // No further positions will be processed, so no need to advance neighbors.
      return;
    }
    // Pre-align neighbor rows to the next active position nextI in the current row.
    const EdgeRowIndexType nextI = this->EdgeRowIndices[pointIds[4]];
    for (int k = 0; k < 4; ++k)
    {
      while (pointIds[k] < endPtIds[k] && this->EdgeRowIndices[pointIds[k]] < nextI)
      {
        ++pointIds[k];
      }
    }
    for (int k = 5; k < 9; ++k)
    {
      while (pointIds[k] < endPtIds[k] && this->EdgeRowIndices[pointIds[k]] < nextI)
      {
        ++pointIds[k];
      }
    }
  }

  // Given an i,j,k triad index, create a new point in the center of the
  // triad. It is possible for some points to be generated outside the
  // actual image (i.e., in the padded boundary triads).  The point is
  // generated in image space, later it will be transformed into world space
  // via vtkImageTransform. (Recall that the volume is padded out in the
  // x-y-z directions.)
  void GeneratePoint(vtkIdType ptId, vtkIdType i, vtkIdType j, vtkIdType k, int8_t tableIndex)
  {
    float* p = this->NewPts + 3 * ptId;
    p[X] = this->Min[X] + static_cast<float>(i) - 0.5;
    p[Y] = this->Min[Y] + static_cast<float>(j) - 0.5;
    p[Z] = this->Min[Z] + static_cast<float>(k) - 0.5;
    this->NonManifoldTableIndices[ptId] = tableIndex;
  }

  // Produce the output polygons (quads) for this triad. Note that at most
  // three quads (the "lower left" quads) corresponding to edge numbers (0,
  // 4, 8) can be produced (i.e., for the edges of the voxel triad). Scalar
  // data indicating the regions/labels on either side of the quad are also
  // written.
  struct GenerateQuadsImpl : public vtkCellArray::DispatchUtilities
  {
    template <class OffsetsT, class ConnectivityT>
    void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* conn, vtkIdType i, vtkIdType row,
      vtkIdType slice, SurfaceNets* snet, EdgeCaseType edgeCase, uint8_t numPoints4,
      int8_t tableIndex4, const std::array<TriadType*, 9>& triadPtrs,
      std::array<vtkIdType, 9>& pointIds, vtkIdType& quadId)
    {
      auto connRange = GetRange(conn);
      auto connIter = connRange.begin() + (quadId * 4);

      // Prepare to write scalar data. s0 is the triad origin.
      const T backgroundLabel = snet->BackgroundLabel;
      const T s0Origin = snet->GetVoxelForTriad(i, row, slice);
      const bool s0IsBackground = s0Origin == backgroundLabel;
      const TriadType& triad = triadPtrs[4][i];

      if (SurfaceNets::GenerateXYQuad(triad))
      {
        // (i, row, slice)
        vtkIdType c0 = pointIds[4];
        if (VTK_UNLIKELY(numPoints4 != 1))
        {
          c0 += NonManifoldCases::GetNonManiFoldPointIndex<8>(edgeCase, tableIndex4);
        }
        // (i - 1, row, slice)
        vtkIdType c1 = pointIds[4] - 1;
        const auto [numPoints4_1, tableIndex4_1] = SurfaceNets::GetStateInfo(triadPtrs[4][i - 1]);
        if (VTK_UNLIKELY(numPoints4_1 != 1))
        {
          c1 = pointIds[4] - numPoints4_1 +
            NonManifoldCases::GetNonManiFoldPointIndex<9>(
              snet->GetEdgeCase(triadPtrs[4] + i - 1), tableIndex4_1);
        }
        // (i - 1, row - 1, slice)
        vtkIdType c2 = pointIds[3] - 1;
        const auto [numPoints3_1, tableIndex3_1] = SurfaceNets::GetStateInfo(triadPtrs[3][i - 1]);
        if (VTK_UNLIKELY(numPoints3_1 != 1))
        {
          c2 = pointIds[3] - numPoints3_1 +
            NonManifoldCases::GetNonManiFoldPointIndex<11>(
              snet->GetEdgeCase(triadPtrs[3] + i - 1), tableIndex3_1);
        }
        // (i, row - 1, slice)
        vtkIdType c3 = pointIds[3];
        const auto [numPoints3, tableIndex3] = SurfaceNets::GetStateInfo(triadPtrs[3][i]);
        if (VTK_UNLIKELY(numPoints3 != 1))
        {
          c3 += NonManifoldCases::GetNonManiFoldPointIndex<10>(
            snet->GetEdgeCase(triadPtrs[3] + i), tableIndex3);
        }
        T s0 = s0Origin;
        T s1 = snet->GetVoxelForTriad(i, row, slice + 1);
        if (s0IsBackground || (s1 != backgroundLabel && s0 > s1))
        {
          std::swap(s0, s1);
          std::swap(c1, c3);
        }

        *connIter++ = c0;
        *connIter++ = c1;
        *connIter++ = c2;
        *connIter++ = c3;

        snet->WriteScalarTuple(s0, s1, quadId++);
      }

      if (SurfaceNets::GenerateXZQuad(triad))
      {
        // (i, row, slice)
        vtkIdType c0 = pointIds[4];
        if (VTK_UNLIKELY(numPoints4 != 1))
        {
          c0 += NonManifoldCases::GetNonManiFoldPointIndex<4>(edgeCase, tableIndex4);
        }
        // (i, row, slice - 1)
        vtkIdType c1 = pointIds[1];
        const auto [numPoints1, tableIndex1] = SurfaceNets::GetStateInfo(triadPtrs[1][i]);
        if (VTK_UNLIKELY(numPoints1 != 1))
        {
          c1 += NonManifoldCases::GetNonManiFoldPointIndex<6>(
            snet->GetEdgeCase(triadPtrs[1] + i), tableIndex1);
        }
        // (i - 1, row, slice - 1)
        vtkIdType c2 = pointIds[1] - 1;
        const auto [numPoints1_1, tableIndex1_1] = SurfaceNets::GetStateInfo(triadPtrs[1][i - 1]);
        if (VTK_UNLIKELY(numPoints1_1 != 1))
        {
          c2 = pointIds[1] - numPoints1_1 +
            NonManifoldCases::GetNonManiFoldPointIndex<7>(
              snet->GetEdgeCase(triadPtrs[1] + i - 1), tableIndex1_1);
        }
        // (i - 1, row, slice)
        vtkIdType c3 = pointIds[4] - 1;
        const auto [numPoints4_1, tableIndex4_1] = SurfaceNets::GetStateInfo(triadPtrs[4][i - 1]);
        if (VTK_UNLIKELY(numPoints4_1 != 1))
        {
          c3 = pointIds[4] - numPoints4_1 +
            NonManifoldCases::GetNonManiFoldPointIndex<5>(
              snet->GetEdgeCase(triadPtrs[4] + i - 1), tableIndex4_1);
        }

        T s0 = s0Origin;
        T s1 = snet->GetVoxelForTriad(i, row + 1, slice);
        if (s0IsBackground || (s1 != backgroundLabel && s0 > s1))
        {
          std::swap(s0, s1);
          std::swap(c1, c3);
        }

        *connIter++ = c0;
        *connIter++ = c1;
        *connIter++ = c2;
        *connIter++ = c3;

        snet->WriteScalarTuple(s0, s1, quadId++);
      }

      if (SurfaceNets::GenerateYZQuad(triad))
      {
        // (i, row, slice)
        vtkIdType c0 = pointIds[4];
        if (VTK_UNLIKELY(numPoints4 != 1))
        {
          c0 += NonManifoldCases::GetNonManiFoldPointIndex<0>(edgeCase, tableIndex4);
        }
        // (i, row - 1, slice)
        vtkIdType c1 = pointIds[3];
        const auto [numPoints3, tableIndex3] = SurfaceNets::GetStateInfo(triadPtrs[3][i]);
        if (VTK_UNLIKELY(numPoints3 != 1))
        {
          c1 += NonManifoldCases::GetNonManiFoldPointIndex<1>(
            snet->GetEdgeCase(triadPtrs[3] + i), tableIndex3);
        }
        // (i, row - 1, slice - 1)
        vtkIdType c2 = pointIds[0];
        const auto [numPoints0, tableIndex0] = SurfaceNets::GetStateInfo(triadPtrs[0][i]);
        if (VTK_UNLIKELY(numPoints0 != 1))
        {
          c2 += NonManifoldCases::GetNonManiFoldPointIndex<3>(
            snet->GetEdgeCase(triadPtrs[0] + i), tableIndex0);
        }
        // (i, row, slice - 1)
        vtkIdType c3 = pointIds[1];
        const auto [numPoints1, tableIndex1] = SurfaceNets::GetStateInfo(triadPtrs[1][i]);
        if (VTK_UNLIKELY(numPoints1 != 1))
        {
          c3 += NonManifoldCases::GetNonManiFoldPointIndex<2>(
            snet->GetEdgeCase(triadPtrs[1] + i), tableIndex1);
        }

        T s0 = s0Origin;
        T s1 = snet->GetVoxelForTriad(i + 1, row, slice);
        if (s0IsBackground || (s1 != backgroundLabel && s0 > s1))
        {
          std::swap(s0, s1);
          std::swap(c1, c3);
        }

        *connIter++ = c0;
        *connIter++ = c1;
        *connIter++ = c2;
        *connIter++ = c3;

        snet->WriteScalarTuple(s0, s1, quadId++);
      }

    } // operator()
  }; // GenerateQuadsImpl

  // Produce the smoothing stencils for this voxel cell.
  struct GenerateStencilImpl : public vtkCellArray::DispatchUtilities
  {
    template <class OffsetsT, class ConnectivityT>
    void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType i, SurfaceNets* snet,
      EdgeCaseType edgeCase, uint8_t numPoints4, int8_t tableIndex4,
      const std::array<TriadType*, 9>& triadPtrs, std::array<vtkIdType, 9>& pointIds,
      vtkIdType& sOffset)
    {
      // The point on which the stencil operates
      const vtkIdType& pointId4 = pointIds[4];

      auto offsetRange = GetRange(offsets);
      auto offsetIter = offsetRange.begin() + pointId4;
      auto connRange = GetRange(conn);
      auto connIter = connRange.begin() + sOffset;

      // Create the stencil. Note that for stencils with just one connection
      // (e.g., on the boundary of the image), the stencil point is "locked"
      // in place to prevent any motion to avoid shrinkage etc.
      // Locked stencil: boundary points are anchored to themselves.
      if (VTK_UNLIKELY(snet->GetNumberOfStencilEdges(edgeCase, tableIndex4, numPoints4) == 1))
      {
        *offsetIter = sOffset++;
        *connIter = pointId4;
        return;
      }

      const auto manifoldSubEdgeCases =
        NonManifoldCases::GetManifoldSubEdgeCases(edgeCase, tableIndex4);
      VTK_ASSUME(numPoints4 >= 1 && numPoints4 <= 5);
      const uint8_t numManifoldSubCases = std::max(1, numPoints4 - 1);
      for (uint8_t j = 0; j < numPoints4; ++j, ++offsetIter)
      {
        // If point is manifold, we create stencils for the edge case.
        // Else, we create stencils for the manifold sub-edge cases and also the edge case itself
        const bool isEdgeCase = (numManifoldSubCases == 1 || j == numManifoldSubCases);
        const EdgeCaseType edgeCaseToProcess = isEdgeCase ? edgeCase : manifoldSubEdgeCases[j];
        const unsigned char numEdges = snet->GetNumberOfStencilEdges(edgeCaseToProcess);
        *offsetIter = sOffset;
        sOffset += numEdges;
        // Create up to six stencil edges connecting the voxel edge face neighbors.
        const unsigned char* stencilEdges = snet->GetStencilEdges(edgeCaseToProcess);

        // -x face
        if (stencilEdges[1])
        {
          // Anchor stencil is always the last point id regardless of the numPoints in the triad.
          *connIter++ = pointId4 - 1;
        }
        // +x face
        if (stencilEdges[2])
        {
          *connIter++ =
            pointId4 + numPoints4 + SurfaceNets::GetNumberOfPoints(triadPtrs[4][i + 1]) - 1;
        }
        // -y face
        if (stencilEdges[3])
        {
          *connIter++ = pointIds[3] + SurfaceNets::GetNumberOfPoints(triadPtrs[3][i]) - 1;
        }
        // +y face
        if (stencilEdges[4])
        {
          *connIter++ = pointIds[5] + SurfaceNets::GetNumberOfPoints(triadPtrs[5][i]) - 1;
        }
        // -z face
        if (stencilEdges[5])
        {
          *connIter++ = pointIds[1] + SurfaceNets::GetNumberOfPoints(triadPtrs[1][i]) - 1;
        }
        // +z face
        if (stencilEdges[6])
        {
          *connIter++ = pointIds[7] + SurfaceNets::GetNumberOfPoints(triadPtrs[7][i]) - 1;
        }
      }
    } // operator()
  }; // GenerateStencilImpl

  // Given a triad i,j,k return the voxel value. Note that the
  // triad i,j,k is shifted by 1 due to the padding of the image
  // with boundary triads.
  T GetVoxelForTriad(vtkIdType i, vtkIdType row, vtkIdType slice)
  {
    return this
      ->Scalars[(slice - 1) * this->Inc[Z] + (row - 1) * this->Inc[Y] + (i - 1) * this->Inc[X]];
  }

  // Helper function writes the scalar 2-tuple.
  void WriteScalarTuple(T s0, T s1, vtkIdType quadId)
  {
    TOutPtr scalars = this->NewScalars + 2 * quadId;
    scalars[0] = s0;
    scalars[1] = s1;
  } // WriteScalarTuple

  // The following are methods supporting the four passes of the
  // surface nets extraction.

  // The first pass is used to classify the x-edges of the triads.
  // Threading integration via SMPTools; this method processes a
  // single x-edge.
  void ClassifyXEdges(
    TInPtr inPtr, vtkIdType row, vtkIdType slice, vtkLabelMapLookup<T>* lMap); // PASS 1

  // The second pass is used to classify the y- and z-edges of the triads.
  // This method processes an x-row of voxels.
  void ClassifyYZEdges(TInPtr inPtr, vtkIdType row, vtkIdType slice); // PASS 2

  // The third pass is a prefix sum over the edge metadata to determine where
  // the algorithm should write its output, and then allocate output. This is
  // a serial method.
  void ProduceVoxelCases(vtkIdType group, int edgeNum, vtkIdType numRowPairs);
  void ConfigureOutput(vtkPoints* newPts, vtkTypeInt8Array* nonManifoldTableIndices,
    vtkCellArray* newQuads, TArray* newScalars, vtkCellArray* stencils); // PASS 3

  // PASS 4: Build an auxiliary array that records, for each generated output point,
  // the x-position (within the current edge row) of the triad that generated it.
  void BuildPointGeneratingEdgeRowXIndices(vtkIdType row, vtkIdType slice);

  // The fifth pass produces the output geometry (i.e., points) and topology
  // (quads and smoothing stencils). It processes an x-row of voxels.
  void GenerateOutput(vtkIdType row, vtkIdType slice); // PASS 5

}; // SurfaceNets

// Initialize the smoothing stencil cases. There are 64 possible stencil face
// cases, for each case, the number of active stencil edges, and then 0/1
// flags indicating whether each of the six possible face connected stencil
// edges is enabled. Note that the voxel's faces are numbered as defined by a
// vtkVoxel cell (i.e., so that the ordering of stencil edges is
// -x,+x,-y,+y,-z,+z).
template <class TArray, class TEdgeRowIndex>
const unsigned char SurfaceNets<TArray, TEdgeRowIndex>::StencilFaceCases[64][7] = {
  { 0, 0, 0, 0, 0, 0, 0 }, // case 0
  { 1, 1, 0, 0, 0, 0, 0 }, // case 1
  { 1, 0, 1, 0, 0, 0, 0 }, // case 2
  { 2, 1, 1, 0, 0, 0, 0 }, // case 3
  { 1, 0, 0, 1, 0, 0, 0 }, // case 4
  { 2, 1, 0, 1, 0, 0, 0 }, // case 5
  { 2, 0, 1, 1, 0, 0, 0 }, // case 6
  { 3, 1, 1, 1, 0, 0, 0 }, // case 7
  { 1, 0, 0, 0, 1, 0, 0 }, // case 8
  { 2, 1, 0, 0, 1, 0, 0 }, // case 9
  { 2, 0, 1, 0, 1, 0, 0 }, // case 10
  { 3, 1, 1, 0, 1, 0, 0 }, // case 11
  { 2, 0, 0, 1, 1, 0, 0 }, // case 12
  { 3, 1, 0, 1, 1, 0, 0 }, // case 13
  { 3, 0, 1, 1, 1, 0, 0 }, // case 14
  { 4, 1, 1, 1, 1, 0, 0 }, // case 15
  { 1, 0, 0, 0, 0, 1, 0 }, // case 16
  { 2, 1, 0, 0, 0, 1, 0 }, // case 17
  { 2, 0, 1, 0, 0, 1, 0 }, // case 18
  { 3, 1, 1, 0, 0, 1, 0 }, // case 19
  { 2, 0, 0, 1, 0, 1, 0 }, // case 20
  { 3, 1, 0, 1, 0, 1, 0 }, // case 21
  { 3, 0, 1, 1, 0, 1, 0 }, // case 22
  { 4, 1, 1, 1, 0, 1, 0 }, // case 23
  { 2, 0, 0, 0, 1, 1, 0 }, // case 24
  { 3, 1, 0, 0, 1, 1, 0 }, // case 25
  { 3, 0, 1, 0, 1, 1, 0 }, // case 26
  { 4, 1, 1, 0, 1, 1, 0 }, // case 27
  { 3, 0, 0, 1, 1, 1, 0 }, // case 28
  { 4, 1, 0, 1, 1, 1, 0 }, // case 29
  { 4, 0, 1, 1, 1, 1, 0 }, // case 30
  { 5, 1, 1, 1, 1, 1, 0 }, // case 31
  { 1, 0, 0, 0, 0, 0, 1 }, // case 32
  { 2, 1, 0, 0, 0, 0, 1 }, // case 33
  { 2, 0, 1, 0, 0, 0, 1 }, // case 34
  { 3, 1, 1, 0, 0, 0, 1 }, // case 35
  { 2, 0, 0, 1, 0, 0, 1 }, // case 36
  { 3, 1, 0, 1, 0, 0, 1 }, // case 37
  { 3, 0, 1, 1, 0, 0, 1 }, // case 38
  { 4, 1, 1, 1, 0, 0, 1 }, // case 39
  { 2, 0, 0, 0, 1, 0, 1 }, // case 40
  { 3, 1, 0, 0, 1, 0, 1 }, // case 41
  { 3, 0, 1, 0, 1, 0, 1 }, // case 42
  { 4, 1, 1, 0, 1, 0, 1 }, // case 43
  { 3, 0, 0, 1, 1, 0, 1 }, // case 44
  { 4, 1, 0, 1, 1, 0, 1 }, // case 45
  { 4, 0, 1, 1, 1, 0, 1 }, // case 46
  { 5, 1, 1, 1, 1, 0, 1 }, // case 47
  { 2, 0, 0, 0, 0, 1, 1 }, // case 48
  { 3, 1, 0, 0, 0, 1, 1 }, // case 49
  { 3, 0, 1, 0, 0, 1, 1 }, // case 50
  { 4, 1, 1, 0, 0, 1, 1 }, // case 51
  { 3, 0, 0, 1, 0, 1, 1 }, // case 52
  { 4, 1, 0, 1, 0, 1, 1 }, // case 53
  { 4, 0, 1, 1, 0, 1, 1 }, // case 54
  { 5, 1, 1, 1, 0, 1, 1 }, // case 55
  { 3, 0, 0, 0, 1, 1, 1 }, // case 56
  { 4, 1, 0, 0, 1, 1, 1 }, // case 57
  { 4, 0, 1, 0, 1, 1, 1 }, // case 58
  { 5, 1, 1, 0, 1, 1, 1 }, // case 59
  { 4, 0, 0, 1, 1, 1, 1 }, // case 60
  { 5, 1, 0, 1, 1, 1, 1 }, // case 61
  { 5, 0, 1, 1, 1, 1, 1 }, // case 62
  { 6, 1, 1, 1, 1, 1, 1 }, // case 63
};

// This internal function procedurally generates the face-case-based stencil
// cases StencilFaceCases[64][7]. For each of the 64 possible face cases, seven
// values define the associated smoothing stencil. For each case, the first
// number is the number of active edges in the stencil; the next six numbers
// are 0/1 values indicating whether the ith edge is active. The code is left
// here for instructional purposes (since the stencil cases are statically
// included in the code above).
template <class TArray, class TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::GenerateFaceStencils(unsigned char stencils[][7])
{
  for (FaceCaseType faceCase = 0; faceCase < 64; ++faceCase)
  {
    stencils[faceCase][0] = 0;
    for (int faceNum = 1; faceNum <= 6; ++faceNum)
    {
      stencils[faceCase][faceNum] = (faceCase & (1 << (faceNum - 1))) > 0;
      stencils[faceCase][0] += stencils[faceCase][faceNum]; // count the faces/stencil edges
    }
  } // for all face cases
} // GenerateFaceStencils

// This method provides a lookup table that indexes from the voxel edge case number
// into the face-case-based stencil array. This avoids having to perform conversion
// of the edge case into the face case. It also enables optimization of the stencils.
template <class TArray, class TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::GenerateEdgeStencils(int optLevel)
{
  // Create the basic stencils without optimization. Basically, convert from
  // the 2^12 edge cases to the 2^6 stencil face cases.
  static constexpr int numEdgeCases = 4096;
  for (auto edgeCase = 0; edgeCase < numEdgeCases; ++edgeCase)
  {
    this->StencilTable[edgeCase] = this->GetFaceCase(edgeCase);
  }

  if (optLevel <= 0)
  {
    return;
  }

  // If edge optimization is enabled, the mapping of edge to face stencils is
  // modified; i.e., certain edges are disabled to encourage surface edge
  // smoothing.

  // Loop over all edge cases, obtain the associated stencil face case, and
  // evaluate the topology of the voxel cell faces. If a voxel has any face with 3 or
  // more intersections, then a different smoothing stencil should be associated
  // with the edge case (i.e., the stencil enhanced surface edge smoothing).
  unsigned char faceCount[6]; // the number of edge intersections on each of the voxels' six faces.
  for (int edgeCase = 0; edgeCase < numEdgeCases; ++edgeCase)
  {
    unsigned char maxInts = this->CountFaceIntersections(edgeCase, faceCount);
    if (maxInts <= 2)
    {
      continue; // if no complex voxel faces, the stencil remains unchanged
    }

    // Recompute the face stencil lookup
    unsigned char faceStencilCase = 0;
    for (auto i = 0; i < 6; ++i)
    {
      if (faceCount[i] > 2) // JunctionFace, contributes to stencil
      {
        faceStencilCase |= (1 << i);
      }
    }
    this->StencilTable[edgeCase] = faceStencilCase; // reference new stencil
  }

} // GenerateEdgeStencils

// Implementations of the four passes of the surface nets boundary extraction
// process.

//------------------------------------------------------------------------------
// Support PASS 1: Process a single x-volume-row and associated triad's
// x-axis for each voxel on that row.  Determine trim interval [xL,xR) along
// the row. Note that only triads associated with a voxel are processed: the
// padded / partial triads are treated as special cases. A note about these
// trimmed edges: the idea is to limit the amount of work to be performed. In
// Pass1/ClassifyXEdges all voxels have to be visited, but we can begin
// culling out "outside" regions. Then progressively the trim edges ([xL,xR)
// / EdgeMetaData[3],EdgeMetaData[4]) are squeezed down so that when
// generating the output many fewer triads / voxel cells need to be
// processed.
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::ClassifyXEdges(
  TInPtr inPtr, vtkIdType row, vtkIdType slice, vtkLabelMapLookup<T>* lMap)
{
  T s0, s1 = (*inPtr); // s1 is the first voxel value in the current row
  bool isLV0, isLV1 = lMap->IsLabelValue(s1);
  const vtkIdType& numTriads = this->TriadDims[X];
  TriadType* rowTriadPtr =
    this->Triads.data() + row * this->TriadDims[X] + slice * this->TriadSliceOffset;
  EdgeTrimType& eTrim = this->EdgeTrims[slice * this->TriadDims[Y] + row];
  vtkIdType xMin = numTriads, xMax = 0;
  const vtkIdType numTriadsMinus1 = numTriads - 1, numTriadsMinus2 = numTriads - 2;

  // Run along the entire x-edge classifying the triad x axes. Be careful
  // with the padded triads: the 0th and (n-1) triads will not produce
  // intersections because they are in a padded voxel.  Note that the ith
  // triad corresponds to the (i-1) image voxel.
  for (vtkIdType i = 0; i < numTriadsMinus1; ++i)
  {
    // This handles the left-hand edge of the slice as well as setting up for the next triad.
    s0 = s1;
    isLV0 = isLV1;

    if (i == numTriadsMinus2)
    {
      // Edge of slice, voxel value s1 does not exist due to padding
      s1 = s0;
      isLV1 = isLV0;
    }
    else
    {
      // Processing triads which are associated with voxels.
      s1 = inPtr[i * this->Inc[X]];
      isLV1 = s0 == s1 ? isLV0 : lMap->IsLabelValue(s1);
    }

    // Is the current triad origin vertex a label value?
    TriadType triad = isLV0 ? TriadClassification::Inside : TriadClassification::Outside;

    // Is the current x-edge split (i.e., different labels on each end).
    // Also update edge trim.
    if ((isLV0 || isLV1) && s0 != s1)
    {
      triad |= TriadClassification::XIntersection;
      xMin = std::min(xMin, i);
      xMax = i + 1;
    }

    // If non-initialized (zero) state, update classification.
    if (triad > TriadClassification::Outside)
    {
      rowTriadPtr[i] = triad;
    } // if contour interacts with this triad
  } // for all triad-x-edges along this image x-edge

  // The beginning and ending of intersections [xMin, xMax) along the edge is used
  // for computational trimming.
  eTrim.XMin = xMin;
  eTrim.XMax = std::min(xMax, numTriads);
} // ClassifyXEdges

//------------------------------------------------------------------------------
// Support PASS 2: Classify the yz-axis portion of the triads along a single
// x-row of triads. Note that only actual rows and slices containing data
// (i.e., not padded voxel cells) are processed by this method.
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::ClassifyYZEdges(
  TInPtr inPtr, vtkIdType row, vtkIdType slice)
{
  // Classify y- and z- triad edges.
  // Triad cases(triadPtr,tCase): this row, the next row (y-classification), and the
  // next slice (z-classification).
  const vtkIdType& numTriads = this->TriadDims[X];
  TriadType* triadPtr = this->Triads.data() + row * numTriads + slice * this->TriadSliceOffset;
  TriadType* triadPtrY = triadPtr + this->TriadDims[X];
  TriadType* triadPtrZ = triadPtr + this->TriadSliceOffset;

  // Edge trim: this edgeRow, in the y-direction, and the z-direction.
  const vtkIdType edgeRow = row + slice * this->TriadDims[Y];
  EdgeTrimType& eTrim = this->EdgeTrims[edgeRow];
  EdgeTrimType& eTrimY = (&eTrim)[1];
  EdgeTrimType& eTrimZ = (&eTrim)[this->TriadDims[Y]];

  const vtkIdType numTriadsMinus1 = numTriads - 1, numTriadsMinus2 = numTriads - 2;
  // By default, all non-padded voxels on this volume-x-row will be
  // processed. However, based on the edge trim from the first pass, or the
  // particulars of the data surrounding this edge, the edge trim (xMin,xMax) may be modified.
  vtkIdType xMin = 1, xMax = numTriadsMinus1;

  // A quick check to determine whether this row of voxels needs processing
  // (this is a relatively common situation). If no x-edge intersections
  // exist (eTrim.XMin==numTriads) in this row or the rows to the right and
  // above, and the x-, y-, and z-rows are the same value, then the row can
  // be skipped. Note that triadPtr[1] is the first triad with an associated
  // voxel value (due to padding).
  const bool xInts =
    !(eTrim.XMin >= numTriads && eTrimY.XMin >= numTriads && eTrimZ.XMin >= numTriads);
  if (!xInts)
  {
    if (triadPtr[1] == TriadClassification::Outside)
    {
      if (triadPtrY[1] == TriadClassification::Outside &&
        triadPtrZ[1] == TriadClassification::Outside)
      {
        return; // This is a fairly common situation
      }
    }
    else // no volume-x-edge intersections, voxel values inside the same labeled region
    {
      TriadClassification yEdgeClass =
        this->ClassifyYEdge(inPtr, 1, triadPtr[1], row, triadPtrY[1]);
      TriadClassification zEdgeClass =
        this->ClassifyZEdge(inPtr, 1, triadPtr[1], slice, triadPtrZ[1]);
      if (yEdgeClass == TriadClassification::Outside && zEdgeClass == TriadClassification::Outside)
      {
        return; // no volume-x-edge ints, and voxel values are in the same material
      }
    }
  } // If no intersections along volume-x-edges

  else // There are intersections along one of the volume-x-edges
  {
    // First check the triad edges x-y,x-z to make sure they are in the
    // same material. If not, leave edge trim to default values. Otherwise,
    // reset the edge trim to the trim values determined in Pass1 / ClassifyXEdges
    // because this is the only place where voxel value changes can occur.
    TriadClassification yEdgeClass = this->ClassifyYEdge(inPtr, 1, triadPtr[1], row, triadPtrY[1]);
    TriadClassification zEdgeClass =
      this->ClassifyZEdge(inPtr, 1, triadPtr[1], slice, triadPtrZ[1]);
    if (yEdgeClass == TriadClassification::Outside && zEdgeClass == TriadClassification::Outside)
    {
      xMin = eTrim.XMin;
      xMin = std::min(xMin, eTrimY.XMin);
      xMin = std::min(xMin, eTrimZ.XMin);
      xMin = std::max(xMin, static_cast<vtkIdType>(1));
    }
    const vtkIdType& lastValue = numTriadsMinus2;
    yEdgeClass =
      this->ClassifyYEdge(inPtr, lastValue, triadPtr[lastValue], row, triadPtrY[lastValue]);
    zEdgeClass =
      this->ClassifyZEdge(inPtr, lastValue, triadPtr[lastValue], slice, triadPtrZ[lastValue]);
    if (yEdgeClass == TriadClassification::Outside && zEdgeClass == TriadClassification::Outside)
    {
      xMax = eTrim.XMax;
      xMax = std::max(xMax, eTrimY.XMax);
      xMax = std::max(xMax, eTrimZ.XMax);
      xMax = std::min(xMax, numTriadsMinus1);
    }
  } // Intersections along one of the volume-x-edges

  // Classify all the triad y- and z-edges, excluding the padded triads.
  for (vtkIdType i = xMin; i < xMax; ++i)
  {
    TriadType tCase = triadPtr[i];
    tCase |= this->ClassifyYEdge(inPtr, i, triadPtr[i], row, triadPtrY[i]);
    tCase |= this->ClassifyZEdge(inPtr, i, triadPtr[i], slice, triadPtrZ[i]);
    if (triadPtr[i] != tCase)
    {
      triadPtr[i] = tCase;
    }
  } // for all voxels in this volume x-row

  // Update the edge trim
  eTrim.XMin = xMin;
  eTrim.XMax = xMax;
} // ClassifyYZEdges

// Process the voxels in a row, combining triads to determine the voxel
// cases.  If a voxel case is non-zero, then a point will be generated in the
// voxel, as well as a stencil and possibly some quad polygons. To simplify
// the code, bits are set in the triad corresponding to the voxel indicating
// the number of generated points and the non-manifold table index.
// Because the triads from four rows are combined to produce
// a voxel case, setting this bit could produce a race condition. Thus, the
// processing of voxels is 4-way interleaved in a checkerboard way to avoid race
// conditions i.e., 0<=whichEdge<4 with a group defined as the bundle of four
// neighboring edges with origin (x,y,z) in the +y,+z directions.
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::ProduceVoxelCases(
  vtkIdType group, int whichEdge, vtkIdType numRowPairs)
{
  const vtkIdType& numTriads = this->TriadDims[X];
  const vtkIdType row = 2 * (group % numRowPairs) + (whichEdge % 2);
  const vtkIdType slice = 2 * (group / numRowPairs) + (whichEdge / 2);

  // Make sure we don't process bogus padded triads.
  if (row >= (this->TriadDims[Y] - 1) || slice >= (this->TriadDims[Z] - 1))
  {
    return; // don't process +y,+z padded boundaries
  }

  // Grab the triad data for this row; and the metadata for this row, and the rows
  // that are needed to form a column of voxel cells.
  vtkIdType xMin, xMax; // computational edge trim
  EdgeMetaDataType& eMD = *this->Get2x2EdgeTrim(row, slice, xMin, xMax);
  EdgeTrimType& eTrim = this->EdgeTrims[slice * this->TriadDims[Y] + row];
  TriadType* triadPtr = this->Triads.data() + row * numTriads + slice * this->TriadSliceOffset;

  // Loop across voxels in this row. We need to determine the case number of
  // each voxel from the seven triads that contribute to the complete edge
  // case. (The eighth triad contributes nothing to the edge case.) Note,
  // because the smoothing stencils may include +/-x points before and after
  // the current voxel, the left edge trim is started one before the current
  // location along the voxel-x-edge.
  xMin = std::max(xMin - 1, static_cast<vtkIdType>(0));
  for (vtkIdType i = xMin; i < xMax; ++i)
  {
    VoxelNeighborhood voxelNeighborhood = this->GetVoxelNeighborhood(triadPtr + i, i, row, slice);
    const EdgeCaseType& edgeCase = voxelNeighborhood.EdgeCase;
    if (edgeCase > 0) // then a point must be generated in this voxel
    {
      const int8_t tableIndex = NonManifoldCases::GetNonManifoldTableIndex(voxelNeighborhood);
      const NonManifoldCaseType nonManifoldCase =
        NonManifoldCases::GetNonManifoldCase(edgeCase, tableIndex);
      const uint8_t numPoints = NonManifoldCases::GetNumberOfPoints(nonManifoldCase);
      // Encode the number of points and the non-manifold table index into the triad.
      SurfaceNets::SetState(triadPtr[i], numPoints, tableIndex);

      // Update metadata for this volume edge
      eMD.NumPoints += numPoints;
      eMD.NumQuads += SurfaceNets::GetNumberOfQuads(triadPtr[i]);
      eMD.NumStencilEdges += this->GetNumberOfStencilEdges(edgeCase, tableIndex, numPoints);
    } // if produces a point
  } // for all triads on this row

  // Update the edge trim
  eTrim.XMin = xMin;
  eTrim.XMax = xMax;
} // ProduceVoxelCases

//------------------------------------------------------------------------------
// PASS 3: Triad classification is complete. Now combine the triads to produce
// voxel cases, which indicate whether points, quads, and stencils are to
// be generated. A prefix sum is used to sum up and determine beginning point,
// quad, and stencil numbers for each row. The trim edges per row may also be
// updated (to avoid processing voxels during output generation).
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::ConfigureOutput(vtkPoints* newPts,
  vtkTypeInt8Array* nonManifoldTableIndices, vtkCellArray* newQuads, TArray* newScalars,
  vtkCellArray* stencils)
{
  // Traverse all rows, combining triads to determine voxel cases. Using the
  // case, sum up the number of points, quads, and stencils generated for
  // each row. Note that to avoid race conditions, row processing is interleaved
  // (i.e., groups of four rows: +/-y +/-z volume edges).
  const vtkIdType numRows = this->TriadDims[Y];
  const vtkIdType numRowPairs = numRows / 2;
  const vtkIdType numSlices = this->TriadDims[Z];
  const vtkIdType numSlicePairs = numSlices / 2;
  const vtkIdType numGroups = numRowPairs * numSlicePairs;

  // Process the four edges that compose a group in order. The four edges form
  // a 2x2 bundle, in the order (j,k),(j+1,k),(j,k+1),(j+1,k+1).
  for (int edgeNum = 0; edgeNum < 4; ++edgeNum)
  {
    // Edge groups consist of four neighboring volume x-edges (+/-y,+/-z). Process
    // in interleaving fashion (i.e., checkerboard) to avoid races (ProduceVoxelCases()
    // may write to the voxel classifications while they are being processed).
    vtkSMPTools::For(0, numGroups,
      [this, edgeNum, numRowPairs](vtkIdType group, vtkIdType endGroup)
      {
        for (; group < endGroup; ++group)
        {
          this->ProduceVoxelCases(group, edgeNum, numRowPairs);
        }
      });
  }

  // Perform prefix sum to build offsets into the output points, quads, and
  // Accumulate the total number of points, quads, and stencil edges across all the image x-rows.
  const EdgeMetaDataType outputEMD = vtkSMPTools::ExclusiveScan(
    this->EdgeMetaData.begin(), this->EdgeMetaData.end(), EdgeMetaDataType{});

  // Output can now be allocated.
  if (outputEMD.NumPoints > 0)
  {
    // Points, which are floats
    newPts->SetNumberOfPoints(outputEMD.NumPoints);
    vtkFloatArray* fPts = static_cast<vtkFloatArray*>(newPts->GetData());
    this->NewPts = fPts->GetPointer(0);

    nonManifoldTableIndices->SetNumberOfValues(outputEMD.NumPoints);
    this->NonManifoldTableIndices = nonManifoldTableIndices->GetPointer(0);

    // Boundaries, a set of quads contained in vtkCellArray
    newQuads->UseFixedSizeDefaultStorage(4);
    newQuads->ResizeExact(outputEMD.NumQuads, 4 * outputEMD.NumQuads);
    this->NewQuads = newQuads;

    // Scalars, which are of type T and 2-components
    newScalars->SetNumberOfTuples(outputEMD.NumQuads);
    this->NewScalars = vtk::DataArrayValueRange<2>(newScalars).begin();

    // Smoothing stencils, which are represented by a vtkCellArray
    stencils->ResizeExact(outputEMD.NumPoints, outputEMD.NumStencilEdges);
    stencils->SetOffset(outputEMD.NumPoints, outputEMD.NumStencilEdges);
    this->NewStencils = stencils;

    // Edge row Indices
    this->EdgeRowIndices.resize(static_cast<size_t>(outputEMD.NumPoints));
  }
} // ConfigureOutput

//------------------------------------------------------------------------------
// PASS 4: Build an auxiliary array that records, for each generated output point,
// the x-position (within the current edge row) of the triad that generated it.
// This allows output generation to traverse only point-generating triads in a
// row even when the trimmed interval [xMin_i,xMax_i) is sparse in such triads.
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::BuildPointGeneratingEdgeRowXIndices(
  vtkIdType row, vtkIdType slice)
{
  const vtkIdType edgeRow = slice * this->TriadDims[Y] + row;
  const EdgeTrimType& eTrim = this->EdgeTrims[edgeRow];
  if (eTrim.XMin >= eTrim.XMax)
  {
    return;
  }

  const TriadType* triadPtr =
    this->Triads.data() + row * this->TriadDims[X] + slice * this->TriadSliceOffset;
  const EdgeMetaDataType& eMD = this->EdgeMetaData[edgeRow];
  vtkIdType offset = eMD.NumPoints;
  for (vtkIdType i = eTrim.XMin; i < eTrim.XMax; ++i)
  {
    const TriadType& triad = triadPtr[i];
    if (VTK_UNLIKELY(SurfaceNets::ProducesPoints(triad)))
    {
      const EdgeRowIndexType n = SurfaceNets::GetNumberOfPoints(triad);
      VTK_ASSUME(n >= 1 && n <= 5);
      for (EdgeRowIndexType j = 0; j < n; ++j)
      {
        this->EdgeRowIndices[offset++] = i;
      }
    }
  }
}

//------------------------------------------------------------------------------
// PASS 5: Process the x-row triads to generate output primitives, including
// point coordinates, quad primitives, and smoothing stencils. This is the
// fifth pass of the extraction algorithm. Implementation notes: the image origin,
// spacing, and orientation is taken into account later when
// vtkImageTransform::TransformPointSet() is invoked.  When generating the
// points below, computations are performed in canonical image space. Also,
// to generate points, quads, and stencils, the point ids are determined by
// advancing the starting point ids from the current triad row, as well as the
// rows immediately surrounding the current row (i.e., all those rows to
// which stencil edges connect to, as well as generated quads). This forms
// a 3x3 bundle of volume edges / voxel rows centered on the current x-row.
// The edge trim is used to reduce the amount of computation to perform.
template <typename TArray, typename TEdgeRowIndex>
void SurfaceNets<TArray, TEdgeRowIndex>::GenerateOutput(vtkIdType row, vtkIdType slice)
{
  // This volume edge's metadata, and the neighboring edge.
  const EdgeMetaDataType& eMD = this->EdgeMetaData[slice * this->TriadDims[Y] + row];
  const EdgeMetaDataType& eMDNei = (&eMD)[1];

  // Return if there is nothing to do (i.e., no points, quads or stencils to
  // generate). After the prefix sum in Pass3, eMD[0] is the starting number
  // of the points generated along this volume x-edge. So if (eMDNei[0]-eMD[0])<=0
  // nothing is produced along the edge (i.e., no points generated) so it can be
  // skipped.
  if (eMDNei.NumPoints <= eMD.NumPoints)
  {
    return;
  }

  // Given a volume x-edge to process (defined by [row,slice]), determine the
  // trim edges and the 3x3 row triad cases centered around the current
  // x-edge.
  std::array<TriadType*, 9> triadPtrs;      // pointers to the 3x3 bundle of row triad cases
  std::array<EdgeMetaDataType*, 9> eMDPtrs; // pointers to the 3x3 bundle of edge metadata
  this->Get3x3EdgeAndTriadPointers(row, slice, eMDPtrs, triadPtrs);
  TriadType* triadPtr = triadPtrs[4]; // triad pointers for current row

  // Initialize the point numbering process using a row iterator. This uses
  // the information gathered from the prefix sum (Pass3) and contained in
  // the edge metadata to obtain point numbers/ids, and the number/size of
  // quads and stencils. The nine pointIds are the current starting point ids for
  // rows surrounding the current edge (in total, a 3x3 stencil, which
  // includes in the center of the stencil, the current edge).  The pointIds
  // are initialized with the edge metadata, and advanced as a function of
  // the EdgeRowIndices along the nine edges.
  std::array<vtkIdType, 9> pointIds, endPtIds;
  this->InitRowIterator(eMDPtrs, pointIds, endPtIds);
  vtkIdType quadId = eMD.NumQuads;         // starting quad id for this row
  vtkIdType sOffset = eMD.NumStencilEdges; // starting stencil offset for this row

  // Traverse only the active positions in this row by following EdgeRowIndices
  // for the current row (k=4). For each active position i, first catch up
  // neighboring rows to i, then generate output, then advance all rows past i.
  const vtkIdType endPtId4 = endPtIds[4];
  while (pointIds[4] < endPtId4)
  {
    const vtkIdType i = static_cast<vtkIdType>(this->EdgeRowIndices[pointIds[4]]);
    const TriadType& triad = triadPtr[i];
    const auto [numPoints, tableIndex] = SurfaceNets::GetStateInfo(triad);
    assert(numPoints > 0); // should be guaranteed
    // Output a point, or more for non-manifold cases, at the center of the voxel.
    VTK_ASSUME(numPoints >= 1 && numPoints <= 5);
    for (uint8_t j = 0; j < numPoints; ++j)
    {
      this->GeneratePoint(pointIds[4] + j, i, row, slice, tableIndex);
    }

    const EdgeCaseType edgeCase = this->GetEdgeCase(triadPtr + i);
    // Produce quads if necessary.
    if (SurfaceNets::ProducesQuad(triad))
    {
      this->NewQuads->Dispatch(GenerateQuadsImpl{}, i, row, slice, this, edgeCase, numPoints,
        tableIndex, triadPtrs, pointIds, quadId);
    }

    // If a point is generated, then smoothing stencils are required (i.e.,
    // stencils indicate how the generated point is connected to other
    // points). Up to six connections corresponding to six face neighbors
    // may be generated.
    this->NewStencils->Dispatch(GenerateStencilImpl{}, i, this, edgeCase, numPoints, tableIndex,
      triadPtrs, pointIds, sOffset);

    // Advance all rows past position i, and pre-align neighbor rows to the next
    // active position in the current row (single call per iteration).
    this->AdvanceRowIterator(numPoints, pointIds, endPtIds);
  } // while active positions remain in this row

} // GenerateOutput

// This worker controls the overall algorithm flow, and handles templated
// dispatch based on the input scalar type. It also interfaces the algorithm
// to the vtkSMPTools / threading infrastructure.
struct NetsWorker
{
  // PASS 1: Process all triads on the given x-rows to classify triad
  // x-axis. Interface to vtkSMPTools::For(). Note that triad row "i"
  // corresponds to image row (i-1) (due to padding). Also note that looking
  // up labels can be expensive, so a vtkLabelMapLookup is used to accelerate
  // the lookup process. Note that the parallel for (vtkSMPTools::For()) is
  // over the volume slices. Empirically this tends to provide better load
  // balancing / reduce threading overhead and therefore slightly better
  // performance.
  template <typename TArray, typename TEdgeRowIndex>
  struct Pass1
  {
    using TInPtr = typename vtk::detail::ValueRange<TArray, 1>::iterator;
    using T = vtk::GetAPIType<TArray>;
    SurfaceNets<TArray, TEdgeRowIndex>* Algo;
    // The label map lookup caches information, so to avoid race conditions,
    // an instance per thread must be created.
    vtkSMPThreadLocal<vtkLabelMapLookup<T>*> LMap;
    Pass1(SurfaceNets<TArray, TEdgeRowIndex>* algo) { this->Algo = algo; }
    void Initialize()
    {
      this->LMap.Local() =
        vtkLabelMapLookup<T>::CreateLabelLookup(Algo->LabelValues, Algo->NumLabels);
    }
    void operator()(vtkIdType slice, vtkIdType endSlice)
    {
      vtkLabelMapLookup<T>* lMap = this->LMap.Local();
      TInPtr rowPtr, slicePtr = this->Algo->Scalars + (slice - 1) * this->Algo->Inc[Z];

      // Process slice-by-slice. Note that the bottom and top slices are not
      // processed (they have been 0-initialized).
      for (; slice < endSlice; ++slice)
      {
        // Process only triads associated with voxels (i.e., no padded voxels).
        rowPtr = slicePtr;
        for (vtkIdType row = 1, rowEnd = this->Algo->TriadDims[Y] - 1; row < rowEnd; ++row)
        {
          this->Algo->ClassifyXEdges(rowPtr, row, slice, lMap);
          rowPtr += this->Algo->Inc[Y];
        } // for all rows in this slice
        slicePtr += this->Algo->Inc[Z];
      } // for all slices in this batch
    }
    void Reduce()
    {
      // Delete all the label map lookups
      for (auto lmItr = this->LMap.begin(); lmItr != this->LMap.end(); ++lmItr)
      {
        delete *lmItr;
      } // over all threads

      // Note that unlike vtkSurfaceNets2D, the edge metadata has been
      // initialized to a "do not process" state so nothing else needs be
      // done.
    }
  }; // Pass1 dispatch

  // PASS 2: Process all voxels on the given x-rows to classify triad y-z-axes,
  // and classify voxels. To avoid race conditions when reading edge metadata from
  // neighboring slices (eMDZ), Pass 2 is executed in two phases: first processing
  // odd slices (1, 3, 5, ...), then even slices (2, 4, 6, ...). Since each slice
  // reads from its next neighbor (slice k reads from slice k+1), and odd/even
  // neighbors are in different sets, processing one complete set before the other
  // eliminates simultaneous read/write conflicts. Note that even slices (k) read
  // eMDZ values from their odd neighbor (k+1), which has already been written by
  // the odd phase; this is intentional and safe because the odd phase completes
  // entirely before the even phase begins. Interface to vtkSMPTools::For().
  // Note that triad row "i" corresponds to image row (i-1) (i.e., the triads pad
  // out the volume).
  template <typename TArray, typename TEdgeRowIndex>
  struct Pass2
  {
    using TInPtr = typename vtk::detail::ValueRange<TArray, 1>::iterator;
    SurfaceNets<TArray, TEdgeRowIndex>* Algo;
    vtkNew<vtkAffineArray<vtkIdType>> Slices;

    // Constructor creates an affine array containing either odd or even slice indices.
    // The affine array generates the sequence with stride=2, starting at 1 (odd) or 2 (even).
    Pass2(SurfaceNets<TArray, TEdgeRowIndex>* algo, bool odd, vtkIdType numTotalSlices)
    {
      this->Algo = algo;
      vtkIdType numSlices = odd ? (numTotalSlices / 2) : ((numTotalSlices + 1) / 2);
      this->Slices->SetNumberOfValues(numSlices);
      this->Slices->ConstructBackend(2, odd ? 1 : 2); // stride=2, start=1 or 2
    }

    void operator()(vtkIdType beginSliceId, vtkIdType endSliceId)
    {
      // Process slice-by-slice using the affine array of odd or even slices.
      // Note that the bottom and top slices are not processed (they have been 0-initialized).
      for (vtkIdType sliceId = beginSliceId; sliceId < endSliceId; ++sliceId)
      {
        const vtkIdType slice = this->Slices->GetValue(sliceId);
        // Process only triads associated with voxels (i.e., no padded voxels).
        TInPtr rowPtr = this->Algo->Scalars + (slice - 1) * this->Algo->Inc[Z];
        for (vtkIdType row = 1, rowEnd = this->Algo->TriadDims[Y] - 1; row < rowEnd; ++row)
        {
          this->Algo->ClassifyYZEdges(rowPtr, row, slice);
          rowPtr += this->Algo->Inc[Y];
        } // for all rows in this slice
      } // for all slices in this batch
    }
  }; // Pass2 dispatch

  // PASS 3: Configure and allocate output based on classification of
  // the first two passes.
  template <typename TArray, typename TEdgeRowIndex>
  void Pass3(SurfaceNets<TArray, TEdgeRowIndex>* algo, vtkPoints* newPts,
    vtkTypeInt8Array* nonManifoldTableIndices, vtkCellArray* newQuads, TArray* newScalars,
    vtkCellArray* stencils)
  {
    algo->ConfigureOutput(newPts, nonManifoldTableIndices, newQuads, newScalars, stencils);
  } // Pass3

  // PASS 4: BuildPointGeneratingEdgeRowXIndices: for each output point, record the
  // x-position (within the current edge row) of its generating triad.
  // Indexed by point id; multiple consecutive entries share the same x when a
  // non-manifold triad generates more than one point.
  template <typename TArray, typename TEdgeRowIndex>
  struct Pass4
  {
    SurfaceNets<TArray, TEdgeRowIndex>* Algo;
    Pass4(SurfaceNets<TArray, TEdgeRowIndex>* algo) { this->Algo = algo; }

    void operator()(vtkIdType sliceRow, vtkIdType endSliceRow)
    {
      SurfaceNets<TArray, TEdgeRowIndex>* algo = this->Algo;
      // Note that there is no need to process the last (padded) slice or row.
      const vtkIdType numRows = algo->TriadDims[Y] - 1;

      while (sliceRow < endSliceRow)
      {
        const vtkIdType slice = sliceRow / numRows;
        const vtkIdType sliceRowEnd = std::min((slice + 1) * numRows, endSliceRow);
        EdgeMetaDataType* eMD0Ptr = algo->EdgeMetaData.data() + slice * algo->TriadDims[Y];
        EdgeMetaDataType* eMD1Ptr = eMD0Ptr + algo->TriadDims[Y];

        // Make sure that some data is actually generated on this slice.
        // Skip entire slice if possible.
        if (eMD1Ptr->NumPoints > eMD0Ptr->NumPoints) // Are points generated?
        {
          const vtkIdType rowStart = sliceRow - slice * numRows;
          const vtkIdType rowEnd = sliceRowEnd - slice * numRows;
          for (vtkIdType row = rowStart; row < rowEnd; ++row)
          {
            this->Algo->BuildPointGeneratingEdgeRowXIndices(row, slice);
          } // for all rows
        } // if points are generated
        sliceRow = sliceRowEnd; // advance sliceRow
      }
    }
  };

  // PASS 5: Process all voxels on given volume slices to produce
  // output. Interface to vtkSMPTools::For().
  template <typename TArray, typename TEdgeRowIndex>
  struct Pass5
  {
    SurfaceNets<TArray, TEdgeRowIndex>* Algo;
    Pass5(SurfaceNets<TArray, TEdgeRowIndex>* algo) { this->Algo = algo; }
    void operator()(vtkIdType sliceRow, vtkIdType endSliceRow)
    {
      SurfaceNets<TArray, TEdgeRowIndex>* algo = this->Algo;
      // Note that there is no need to process the last (padded) slice or row.
      const vtkIdType numRows = algo->TriadDims[Y] - 1;

      while (sliceRow < endSliceRow)
      {
        const vtkIdType slice = sliceRow / numRows;
        const vtkIdType sliceRowEnd = std::min((slice + 1) * numRows, endSliceRow);
        EdgeMetaDataType* eMD0Ptr = algo->EdgeMetaData.data() + slice * algo->TriadDims[Y];
        EdgeMetaDataType* eMD1Ptr = eMD0Ptr + algo->TriadDims[Y];

        // Make sure that some data is actually generated on this slice.
        // Skip entire slice if possible.
        if (eMD1Ptr->NumPoints > eMD0Ptr->NumPoints) // Are points generated?
        {
          const vtkIdType rowStart = sliceRow - slice * numRows;
          const vtkIdType rowEnd = sliceRowEnd - slice * numRows;
          for (vtkIdType row = rowStart; row < rowEnd; ++row)
          {
            this->Algo->GenerateOutput(row, slice);
          } // for all rows
        } // if points are generated
        sliceRow = sliceRowEnd; // advance sliceRow
      }
    }
  }; // Pass5 dispatch

  // Dispatch to SurfaceNets.
  template <typename TArray, typename TEdgeRowIndex>
  void Execute(TArray* scalarsArray, vtkSurfaceNets3D* self, vtkImageData* input,
    VTK_FUTURE_CONST int updateExt[6], vtkPoints* newPts, vtkTypeInt8Array* nonManifoldTableIndices,
    vtkCellArray* newQuads, vtkDataArray* newScalarsDA, vtkCellArray* stencils)
  {
    // The type of data carried by the scalarsArray
    using T = vtk::GetAPIType<TArray>;
    auto newScalars = TArray::FastDownCast(newScalarsDA);

    // The update extent may be different from the extent of the image.
    // The only problem with using the update extent is that one or two
    // sources enlarge the update extent.  This behavior is slated to be
    // eliminated.
    vtkIdType increments[3];
    input->GetIncrements(increments);

    // Capture information for subsequent processing.
    SurfaceNets<TArray, TEdgeRowIndex> algo;

    algo.Min[X] = updateExt[0];
    algo.Max[X] = updateExt[1];
    algo.Inc[X] = increments[X];
    algo.Min[Y] = updateExt[2];
    algo.Max[Y] = updateExt[3];
    algo.Inc[Y] = increments[Y];
    algo.Min[Z] = updateExt[4];
    algo.Max[Z] = updateExt[5];
    algo.Inc[Z] = increments[Z];

    // Now allocate the working arrays. The Triads array tracks case# for
    // each voxel triad (and the corresponding voxel).  Note that each input
    // image voxel has an associated triad, and the "grid" of triads is
    // padded out in the +/-x, +/-y, and +/-z directions (i.e., on all sides
    // of the volume).  This simplifies the generation of the surface net,
    // but be aware that the triads on the boundaries of the volume are
    // treated specially.  Note that the allocation of the triads initializes
    // them to zero; we depend on this as the initial triad classification.
    algo.Dims[X] = algo.Max[X] - algo.Min[X] + 1;
    algo.Dims[Y] = algo.Max[Y] - algo.Min[Y] + 1;
    algo.Dims[Z] = algo.Max[Z] - algo.Min[Z] + 1;
    algo.TriadDims[X] = algo.Dims[X] + 2; // padded in the +/-x direction
    algo.TriadDims[Y] = algo.Dims[Y] + 2; // padded in the +/-y direction
    algo.TriadDims[Z] = algo.Dims[Z] + 2; // padded in the +/-z direction
    algo.TriadSliceOffset = algo.TriadDims[X] * algo.TriadDims[Y];
    algo.Triads.resize(static_cast<size_t>(algo.TriadSliceOffset * algo.TriadDims[Z]), 0);

    // Also allocate the characterization (metadata) array for all the x
    // volume edges, including the padded out triads. So the x-edge metadata is
    // defined on the y-z plane. This edge metadata array tracks 0) the number points
    // added along each x-row; as well as 1) the number of quad primitives; 2) the number
    // of stencil edges. eMD is zero initialized. A parallel EdgeTrims array tracks the
    // trim interval [xMin_i, xMax_i) and is initialized to a "do not process" state
    // which will likely be updated in pass1, pass2, or pass3.

    // y-z plane of edges
    const size_t numEdges = static_cast<size_t>(algo.TriadDims[Y] * algo.TriadDims[Z]);
    algo.EdgeMetaData.resize(numEdges);
    algo.EdgeTrims.resize(numEdges);
    vtkSMPTools::For(0, static_cast<vtkIdType>(numEdges),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType edgeId = begin; edgeId < end; ++edgeId)
        {
          EdgeTrimType& eTrim = algo.EdgeTrims[edgeId];
          eTrim.XMin = algo.TriadDims[0];
          eTrim.XMax = 0;
        }
      });

    // Compute the starting offset location for scalar data.  We may be operating
    // on a part of the volume.
    algo.Scalars = vtk::DataArrayValueRange<1>(scalarsArray).begin() +
      input->GetValueIndexForExtent(scalarsArray, updateExt);

    // This algorithm executes just once no matter how many contour/label
    // values, requiring a fast lookup whether a data/voxel value is a
    // contour value, or should be considered part of the background. In
    // Pass1, instances of vtkLabelMapLookup<T> are created (per thread)
    // which performs the fast label lookup.
    algo.NumLabels = self->GetNumberOfLabels();
    algo.LabelValues = self->GetValues();
    algo.BackgroundLabel = static_cast<T>(self->GetBackgroundLabel());

    // Now execute the five passes of the surface nets boundary extraction
    // algorithm.

    // Classify the triad x-edges: note that the +/-z boundary-padded triads
    // are not processed. The threads are processing one z-slice of x-edges at
    // a time. Empirically this performs a little better than processing each
    // edge separately.
    Pass1<TArray, TEdgeRowIndex> pass1(&algo);
    vtkSMPTools::For(1, algo.TriadDims[Z] - 1, pass1);

    // Classify the triad y-z-edges; finalize the triad classification.
    // Process in two passes (odd slices, then even slices) to avoid race conditions.
    // Since each slice reads from the next slice (eMDZ), and odd/even neighbors are
    // in different sets, processing one complete set before the other eliminates races.
    Pass2<TArray, TEdgeRowIndex> pass2Odd(&algo, true, algo.TriadDims[Z] - 2);
    vtkSMPTools::For(0, pass2Odd.Slices->GetNumberOfValues(), pass2Odd);

    Pass2<TArray, TEdgeRowIndex> pass2Even(&algo, false, algo.TriadDims[Z] - 2);
    vtkSMPTools::For(0, pass2Even.Slices->GetNumberOfValues(), pass2Even);

    // Prefix sum to determine the size and character of the output, and
    // then allocate it.
    this->Pass3<TArray, TEdgeRowIndex>(
      &algo, newPts, nonManifoldTableIndices, newQuads, newScalars, stencils);

    // Decompose work into (slice, row) pairs for finer-grained parallelism and better
    // load balancing. The last (padded) slice and row are excluded.
    const vtkIdType sliceRows = (algo.TriadDims[Z] - 1) * (algo.TriadDims[Y] - 1);

    // Generate the edge row indices, which are used to accelerate the output generation step by
    // providing direct access to the x-positions of the triads that generate output points.
    Pass4<TArray, TEdgeRowIndex> pass4(&algo);
    vtkSMPTools::For(0, sliceRows, pass4);

    // Generate the output points, quads, and scalar data.
    Pass5<TArray, TEdgeRowIndex> pass5(&algo);
    vtkSMPTools::For(0, sliceRows, pass5);

    algo.Triads.clear();
    algo.EdgeMetaData.clear();
    algo.EdgeRowIndices.clear();
  }

  // Dispatch to SurfaceNets.
  template <typename TArray>
  void operator()(TArray* scalarsArray, vtkSurfaceNets3D* self, vtkImageData* input,
    VTK_FUTURE_CONST int updateExt[6], vtkPoints* newPts, vtkTypeInt8Array* nonManifoldTableIndices,
    vtkCellArray* newQuads, vtkDataArray* newScalarsDA, vtkCellArray* stencils)
  {
    // Make sure that we are processing a 3D image / volume.
    if (updateExt[0] >= updateExt[1] || updateExt[2] >= updateExt[3] ||
      updateExt[4] >= updateExt[5])
    {
      vtkErrorWithObjectMacro(self, "Expecting 3D data (volume).");
      return;
    }

    const vtkIdType triadDimsX = static_cast<vtkIdType>(updateExt[1] - updateExt[0] + 1) + 2;
    const vtkIdType maxRowIndex = triadDimsX - 1;
    if (maxRowIndex <= static_cast<vtkIdType>(std::numeric_limits<std::uint8_t>::max()))
    {
      this->Execute<TArray, std::uint8_t>(scalarsArray, self, input, updateExt, newPts,
        nonManifoldTableIndices, newQuads, newScalarsDA, stencils);
    }
    else if (maxRowIndex <= static_cast<vtkIdType>(std::numeric_limits<std::uint16_t>::max()))
    {
      this->Execute<TArray, std::uint16_t>(scalarsArray, self, input, updateExt, newPts,
        nonManifoldTableIndices, newQuads, newScalarsDA, stencils);
    }
    else
    {
      // Fallback: extremely large x-dimensions.
      this->Execute<TArray, std::uint32_t>(scalarsArray, self, input, updateExt, newPts,
        nonManifoldTableIndices, newQuads, newScalarsDA, stencils);
    }
  }
}; // NetsWorker

// This function is used to compute smoothing constraints from the voxel spacing.
void ComputeSmoothingConstraints(
  vtkConstrainedSmoothingFilter* smoother, double spacing[3], double constraintScale)
{
  smoother->SetConstraintDistance((vtkMath::Norm(spacing) / 2.0) * constraintScale);
  smoother->SetConstraintBox(
    constraintScale * spacing[0], constraintScale * spacing[1], constraintScale * spacing[2]);
}

// This function is used to smooth the output points and quads to produce a
// more pleasing result. Because of smoothing, the quads typically become
// non-planar and are usually decomposed into triangles (although this can
// be controlled by the user.)
void SmoothOutput(vtkPolyData* geomCache, vtkCellArray* stencils, vtkPolyData* output,
  vtkConstrainedSmoothingFilter* smoother)
{
  vtkLog(TRACE, "Smoothing output");

  // Smooth the data and replace the output points.
  smoother->SetInputData(geomCache);
  smoother->SetSmoothingStencils(stencils);
  smoother->Update();

  // Shallow copy / replace points.
  output->CopyStructure(smoother->GetOutput());
  output->GetCellData()->PassData(smoother->GetOutput()->GetCellData());
} // SmoothOutput

// Functor to drive the threaded conversion of a quad output mesh to
// a different type (i.e., triangles).
template <typename TConnectivityArray, typename TScalarsArray>
struct TransformQuadsToTriangles
{
  TConnectivityArray* QuadConnectivity;
  TScalarsArray* InScalars;
  TConnectivityArray* TriConnectivity;
  TScalarsArray* OutScalars;
  vtkFloatArray* Points;
  const int TriStrategy;

  TransformQuadsToTriangles(TConnectivityArray* quadConnectivity, TScalarsArray* inScalars,
    TConnectivityArray* triConnectivity, TScalarsArray* outScalars, vtkFloatArray* pts,
    int triStrategy)
    : QuadConnectivity(quadConnectivity)
    , InScalars(inScalars)
    , TriConnectivity(triConnectivity)
    , OutScalars(outScalars)
    , Points(pts)
    , TriStrategy(triStrategy)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto points = vtk::DataArrayTupleRange<3>(this->Points);
    auto inScalars = vtk::DataArrayTupleRange<2>(this->InScalars, cellId, endCellId).begin();
    auto triConn =
      vtk::DataArrayValueRange<1>(this->TriConnectivity, 6 * cellId, 6 * endCellId).begin();
    auto outScalars =
      vtk::DataArrayTupleRange<2>(this->OutScalars, 2 * cellId, 2 * endCellId).begin();
    auto quadConn =
      vtk::DataArrayValueRange<1>(this->QuadConnectivity, 4 * cellId, 4 * endCellId).begin();

    bool d02;
    double x0[3], x1[3], x2[3], x3[3], a02, a13;
    for (; cellId < endCellId; ++cellId, ++inScalars, quadConn += 4, outScalars += 2, triConn += 6)
    {
      if (this->TriStrategy != vtkSurfaceNets3D::TRIANGULATION_GREEDY)
      {
        points.GetTuple(quadConn[0], x0);
        points.GetTuple(quadConn[1], x1);
        points.GetTuple(quadConn[2], x2);
        points.GetTuple(quadConn[3], x3);
        if (this->TriStrategy == vtkSurfaceNets3D::TRIANGULATION_MIN_EDGE)
        {
          d02 = vtkMath::Distance2BetweenPoints(x0, x2) < vtkMath::Distance2BetweenPoints(x1, x3);
        }
        else // if (this->TriStrategy == vtkSurfaceNets3D::TRIANGULATION_MIN_AREA)
        {
          a02 = vtkTriangle::TriangleArea(x0, x2, x1) + vtkTriangle::TriangleArea(x0, x2, x3);
          a13 = vtkTriangle::TriangleArea(x1, x3, x0) + vtkTriangle::TriangleArea(x1, x3, x2);
          d02 = a02 < a13;
        }
      }
      else // if (this->TriStrategy == vtkSurfaceNets3D::TRIANGULATION_GREEDY)
      {
        d02 = true;
      }

      // The "connectivity" is defined by bisecting edge, and then
      // converted to triangles.
      if (d02)
      {
        // 1st triangle
        triConn[0] = quadConn[0];
        triConn[1] = quadConn[2];
        triConn[2] = quadConn[3];
        // 2nd triangle
        triConn[3] = quadConn[2];
        triConn[4] = quadConn[0];
        triConn[5] = quadConn[1];
      }
      else
      {
        // 1st triangle
        triConn[0] = quadConn[1];
        triConn[1] = quadConn[3];
        triConn[2] = quadConn[0];
        // 2nd triangle
        triConn[3] = quadConn[3];
        triConn[4] = quadConn[1];
        triConn[5] = quadConn[2];
      }
      outScalars[0] = outScalars[1] = inScalars[0];
    } // over this batch of cells
  }
}; // TransformQuadsToTriangles

struct TransformQuadsToTrianglesWorker
{
  template <class TConnectivityArray, class TScalarsArray>
  void operator()(TConnectivityArray* quadConnectivity, TScalarsArray* inScalars,
    vtkDataArray* triConnectivity, vtkDataArray* outScalars, vtkFloatArray* points, int triStrategy)
  {
    TransformQuadsToTriangles<TConnectivityArray, TScalarsArray> worker(quadConnectivity, inScalars,
      TConnectivityArray::FastDownCast(triConnectivity), TScalarsArray::FastDownCast(outScalars),
      points, triStrategy);
    vtkSMPTools::For(0, inScalars->GetNumberOfTuples(), worker);
  }
};

// This function is used to triangulate the output quads produced by the
// Surface Nets boundary extraction, and after subsequent smoothing (if
// any). It basically replaces the output cells with new cells of the
// appropriate type. The input to this method is a quad mesh. The conversion
// process is threaded.
void TransformMeshType(
  int outputMeshType, vtkPolyData* output, vtkDataArray* newScalars, int triStrategy)
{
  // Ensure that we have a specific type, and that we are not requesting
  // quads which are already available.
  outputMeshType =
    (outputMeshType == vtkSurfaceNets3D::MESH_TYPE_DEFAULT ? vtkSurfaceNets3D::MESH_TYPE_TRIANGLES
                                                           : outputMeshType);
  vtkLog(TRACE, "Transforming output mesh type to: " << outputMeshType);

  if (outputMeshType == vtkSurfaceNets3D::MESH_TYPE_QUADS)
  {
    return; // nothing needs to be done
  }

  // Grab the current quad mesh, and convert it to triangles. The points are
  // hardwired to float. Note that the conversion also requires modifying
  // the cell data "BoundaryLabels".
  vtkFloatArray* pts = vtkFloatArray::FastDownCast(output->GetPoints()->GetData());
  vtkCellArray* quadCells = output->GetPolys();
  vtkIdType numQuads = quadCells->GetNumberOfCells();
  vtkNew<vtkCellArray> triCells;
  triCells->UseFixedSizeDefaultStorage(3);
  triCells->ResizeExact(2 * numQuads, 3 * 2 * numQuads);

  vtkSmartPointer<vtkDataArray> updatedScalars;
  updatedScalars.TakeReference(newScalars->NewInstance());
  updatedScalars->SetNumberOfComponents(2);
  updatedScalars->SetName("BoundaryLabels");
  updatedScalars->SetNumberOfTuples(2 * numQuads);

  using Dispatcher = vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::ConnectivityArrays,
    vtkArrayDispatch::AOSArrays>;
  TransformQuadsToTrianglesWorker worker;
  if (!Dispatcher::Execute(quadCells->GetConnectivityArray(), newScalars, worker,
        triCells->GetConnectivityArray(), updatedScalars, pts, triStrategy))
  {
    worker(quadCells->GetConnectivityArray(), newScalars, triCells->GetConnectivityArray(),
      updatedScalars.Get(), pts, triStrategy);
  }
  // Update the cells and scalars
  output->SetPolys(triCells);
  output->GetCellData()->AddArray(updatedScalars);
}

} // anonymous namespace

//============================================================================
//------------------------------------------------------------------------------
// Here is the VTK class proper.
vtkSurfaceNets3D::vtkSurfaceNets3D()
{
  this->Labels = vtkSmartPointer<vtkContourValues>::New();
  this->BackgroundLabel = 0;
  this->ArrayComponent = 0;
  this->OutputMeshType = MESH_TYPE_DEFAULT;

  this->Smoothing = true;
  this->Smoother = vtkSmartPointer<vtkConstrainedSmoothingFilter>::New();
  this->Smoother->SetNumberOfIterations(16);
  this->Smoother->SetRelaxationFactor(0.5);
  this->AutomaticSmoothingConstraints = true;
  this->ConstraintScale = 2.0;

  this->OutputStyle = OUTPUT_STYLE_DEFAULT;

  this->TriangulationStrategy = TRIANGULATION_MIN_EDGE;

  this->DataCaching = true;
  this->GeometryCache = vtkSmartPointer<vtkPolyData>::New();
  this->StencilsCache = vtkSmartPointer<vtkCellArray>::New();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If label values or the smoothing
// filter are modified, then this object is modified as well.
vtkMTimeType vtkSurfaceNets3D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->Labels->GetMTime();
  mTime = (mTime > mTime2 ? mTime : mTime2);
  mTime2 = this->SelectedLabelsTime.GetMTime();
  mTime = (mTime > mTime2 ? mTime : mTime2);

  mTime2 = this->Smoother->GetMTime();

  return (mTime2 > mTime ? mTime2 : mTime);
}

//------------------------------------------------------------------------------
void vtkSurfaceNets3D::SetOutputStyleToDefault()
{
  this->SetOutputStyle(OUTPUT_STYLE_DEFAULT);
}

//------------------------------------------------------------------------------
void vtkSurfaceNets3D::SetOutputStyleToBoundary()
{
  this->SetOutputStyle(OUTPUT_STYLE_BOUNDARY);
}

//------------------------------------------------------------------------------
void vtkSurfaceNets3D::SetOutputStyleToSelected()
{
  this->SetOutputStyle(OUTPUT_STYLE_SELECTED);
}

//------------------------------------------------------------------------------
// Selected labels are used to output regions/labels if the OutputStyle is
// set to OUTPUT_STYLE_SELECTED.
void vtkSurfaceNets3D::InitializeSelectedLabelsList()
{
  this->SelectedLabels.clear();
  this->SelectedLabelsTime.Modified();
}

//------------------------------------------------------------------------------
// Add a selected label.
void vtkSurfaceNets3D::AddSelectedLabel(double label)
{
  this->SelectedLabels.push_back(label);
  this->SelectedLabelsTime.Modified();
}

//------------------------------------------------------------------------------
// Remove a selected label.
void vtkSurfaceNets3D::DeleteSelectedLabel(double label)
{
  std::vector<double>& v = this->SelectedLabels;
  v.erase(std::remove(v.begin(), v.end(), label), v.end());
  this->SelectedLabelsTime.Modified();
}

//------------------------------------------------------------------------------
// Return the number of selected labels.
vtkIdType vtkSurfaceNets3D::GetNumberOfSelectedLabels()
{
  return this->SelectedLabels.size();
}

//------------------------------------------------------------------------------
// Return the ith selected label.
double vtkSurfaceNets3D::GetSelectedLabel(vtkIdType ithLabel)
{
  return this->SelectedLabels[ithLabel];
}

//------------------------------------------------------------------------------
// Surface nets filter specialized to 3D images (i.e., volumes).
//
int vtkSurfaceNets3D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkLog(TRACE, "Executing Surface Nets 3D");

  // Get the information objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // We'll be creating boundary labels cell data
  vtkSmartPointer<vtkDataArray> newScalars;

  // Determine whether boundary extraction is necessary, or whether we can jump
  // directly to smoothing (and reuse the geometry cache).
  if (!this->DataCaching || this->IsCacheEmpty() ||
    this->Superclass::GetMTime() > this->SmoothingTime)
  {
    // Make sure there is data to output.
    vtkIdType numLabels = this->GetNumberOfLabels();
    if (numLabels < 1)
    {
      return 1;
    }

    VTK_FUTURE_CONST int* ext = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    vtkSmartPointer<vtkDataArray> inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (inScalars == nullptr)
    {
      vtkErrorMacro("Scalars must be defined for surface nets");
      return 1;
    }

    int numComps = inScalars->GetNumberOfComponents();
    if (this->ArrayComponent >= numComps)
    {
      vtkErrorMacro("Scalars have " << numComps << " components. "
                                    << "ArrayComponent must be smaller than " << numComps);
      return 1;
    }
    if (numComps > 1)
    {
      inScalars = vtk::ComponentOrNormAsDataArray(inScalars, this->ArrayComponent);
    }

    // Create necessary objects to hold the output. We will defer the
    // actual allocation once the output size is determined.
    vtkNew<vtkCellArray> newQuads;
    vtkNew<vtkPoints> newPts;
    newPts->SetDataTypeToFloat(); // hardwired to float

    // storing the non-manifold table indices so the user can know whether something was
    // non-manifold or not, and whether the algorithm was able to fix it.
    vtkNew<vtkTypeInt8Array> nonManifoldTableIndices;
    nonManifoldTableIndices->SetName("NonManifoldTableIndices");

    // Note that the output scalars are the same type T as the input
    // scalars due to the use of NewInstance().
    newScalars.TakeReference(inScalars->NewInstance());
    newScalars->SetNumberOfComponents(2);
    newScalars->SetName("BoundaryLabels");

    // SurfaceNets requires a smoothing stencil to smooth the output
    // edges. Later the stencil will be allocated and populated as the output
    // is generated.
    vtkNew<vtkCellArray> stencils;

    // The templated algorithm goes here. Dispatch on input scalar type. Note
    // that since all VTK types are processed, we don't need dispatch
    // fallback to vtkDataArray. Note that there is a fastpath when
    // generating output scalars when only a single segmented region is being
    // extracted.
    NetsWorker netsWorker;
    if (!vtkArrayDispatch::Dispatch::Execute(inScalars.Get(), netsWorker, this, input, ext, newPts,
          nonManifoldTableIndices, newQuads, newScalars, stencils))
    {
      netsWorker(inScalars.Get(), this, input, ext, newPts, nonManifoldTableIndices, newQuads,
        newScalars, stencils);
    }

    vtkLog(TRACE,
      "Extracted: " << newPts->GetNumberOfPoints() << " points, " << newQuads->GetNumberOfCells()
                    << " quads");

    // Update ourselves.
    output->SetPoints(newPts);
    output->SetPolys(newQuads);

    // Add the label cell data, this 2-tuple indicates what regions/labels are
    // on either side of the surface polygons.
    output->GetPointData()->AddArray(nonManifoldTableIndices);
    output->GetCellData()->SetScalars(newScalars);

    // Transform results into physical space. It's necessary to do this
    // before smoothing.
    vtkImageTransform::TransformPointSet(input, output);

    // For now let's stash the data. If caching is disabled, we'll flush it at the end.
    this->CacheData(output, stencils);
  } // Extract boundary geometry

  // If smoothing is to occur, then do it now. It has to be done after image
  // transformation. The smoothing process will replace the current output
  // points. Make sure there is something to smooth.
  vtkCellArray* stencils = this->StencilsCache;
  bool smoothing = false;
  if (stencils && stencils->GetNumberOfCells() > 0 && this->Smoothing &&
    this->Smoother->GetNumberOfIterations() > 0)
  {
    smoothing = true;
    if (this->AutomaticSmoothingConstraints)
    {
      ComputeSmoothingConstraints(this->Smoother, input->GetSpacing(), this->ConstraintScale);
    }
    SmoothOutput(this->GeometryCache, this->StencilsCache, output, this->Smoother);
  }
  else
  {
    output->ShallowCopy(this->GeometryCache);
  }
  this->SmoothingTime.Modified();

  // Modify the type of output mesh if necessary. This changes the type
  // of polygons composing the output mesh. By default, the type is
  // quadrilaterals.
  if ((smoothing && this->OutputMeshType != MESH_TYPE_QUADS) ||
    (!smoothing && this->OutputMeshType == MESH_TYPE_TRIANGLES))
  {
    TransformMeshType(this->OutputMeshType, output, newScalars, this->TriangulationStrategy);
    vtkLog(TRACE, "Triangulated to produce: " << output->GetNumberOfCells() << " triangles");
  }

  // If the output style is other than default, delegate to vtkSurfaceNetsAtlas
  // to perform the boundary/selection filtering, then merge the per-label
  // partitions back into a single vtkPolyData via vtkAppendPolyData.
  if (this->OutputStyle != OUTPUT_STYLE_DEFAULT)
  {
    auto atlas = vtkSmartPointer<vtkSurfaceNetsAtlas>::New();
    atlas->SetBackgroundLabel(static_cast<vtkIdType>(this->BackgroundLabel));
    atlas->SetGeneratePatches(false);
    atlas->SetResolveNonManifoldPoints(false);
    if (this->OutputStyle == OUTPUT_STYLE_BOUNDARY)
    {
      atlas->SetExtractionModeToAll();
      atlas->SetOutputStyleToBoundary();
    }
    else // OUTPUT_STYLE_SELECTED
    {
      atlas->SetExtractionModeToLabelSet();
      atlas->SetOutputStyleToAll();
      for (const double label : this->SelectedLabels)
      {
        atlas->AddSelectedLabel(static_cast<vtkIdType>(label));
      }
    }
    atlas->SetInputDataObject(output);
    atlas->Update();
    auto datasets = vtkCompositeDataSet::GetDataSets(atlas->GetOutputDataObject(0));
    auto appender = vtkSmartPointer<vtkAppendPolyData>::New();
    for (vtkDataSet* dataset : datasets)
    {
      appender->AddInputDataObject(0, dataset);
    }
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
    vtkLog(TRACE, "Selected: " << output->GetNumberOfCells() << " cells");
  }

  // Flush the cache if caching is disabled.
  if (!this->DataCaching)
  {
    this->CacheData(nullptr, nullptr);
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkSurfaceNets3D::IsCacheEmpty()
{
  return (!this->StencilsCache || this->GeometryCache->GetNumberOfPoints() < 1);
}

//------------------------------------------------------------------------------
void vtkSurfaceNets3D::CacheData(vtkPolyData* pd, vtkCellArray* stencils)
{
  this->GeometryCache = pd;
  this->StencilsCache = stencils;
}

//------------------------------------------------------------------------------
int vtkSurfaceNets3D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkSurfaceNets3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Labels->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Background Label: " << this->BackgroundLabel << endl;
  os << indent << "Array Component: " << this->ArrayComponent << endl;
  os << indent << "Output Mesh Type: " << this->OutputMeshType << endl;

  os << indent << "Smoothing: " << (this->Smoothing ? "On\n" : "Off\n");
  os << indent << "Smoother: " << this->Smoother.Get() << endl;
  os << indent << "Automatic Smoothing Constraints: "
     << (this->AutomaticSmoothingConstraints ? "On\n" : "Off\n");
  os << indent << "ConstraintScale: " << this->ConstraintScale << endl;

  os << indent << "Output Style: " << this->OutputStyle << endl;
  os << indent << "Number of Selected Labels: " << this->SelectedLabels.size() << endl;

  os << indent << "Triangulation Strategy: " << this->TriangulationStrategy << endl;

  os << indent << "Data Caching: " << (this->DataCaching ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
