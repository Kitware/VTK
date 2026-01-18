// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Classes, structs, and typedefs in support of Voronoi processing. Names
 * have been chosen to avoid namespace collisions when mixing both 2D and 3D
 * Voronoi algorithms in the same scope.
 *
 * Implementation note: perceptive reviewers will recognize that it is
 * possible to template this Voronoi implmentation by the dimension of the
 * points being processed (e.g., 2D,3D) including the tile/hull generation
 * process. However, certain properties (like the angle of 2D tiles summing
 * to 360, and optimal Delaunay triangulation properties) suggest differing
 * approaches in some situations (as compared to a general n-D
 * approach). Also the clarity of the code is somewhat improved by creating
 * separate 2D and 3D Voronoi-related classes. This of course may be changed
 * in the future.
 */

#ifndef vtkVoronoiCore_h
#define vtkVoronoiCore_h

#include "vtkAlgorithm.h" // check abort status if embedded in filter
#include "vtkIntArray.h"  // for representing segmented region ids
#include "vtkSMPTools.h"  // SMP parallel processing

#include <algorithm> // for std::sort
#include <random>    // random generation of colors
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**
 * Return values from a line/plane clip operation. Besides reporting no
 * intersection, or a valid intersection, rare degenerate cases may also be
 * reported-- this can results in a prune, or a numeric condition.
 */
enum class ClipIntersectionStatus
{
  NoIntersection = 0,
  Intersection = 1,
  Pruned = 2,
  Numeric = 3
};

/**
 * Classification for Voronoi spokes (and associated faces). Different
 * classifications may be used in different Voronoi instantiations. Since
 * the enums are cast, we do not use enum class.
 */
enum vtkSpokeClassification
{
  BACKWARD_SPOKE = 0,  // Bit 0: Backward spoke
  FORWARD_SPOKE = 1,   // Bit 0: Forward spoke
  REGION_BOUNDARY = 2, // Bit 1: Region boundary spoke
  DOMAIN_BOUNDARY = 4, // Bit 2: Domain boundary spoke
  PRUNED = 8,          // Bit 3: Spoke is pruned (deleted)
};

/**
 * Typedefs and classes in support of the adjacency graph.
 */
struct vtkVoronoiSpoke
{
  vtkIdType NeiId;              // Id of the wheel that the spoke is connected to (wheelId,NeiId)
  unsigned char Classification; // Indicate the classification of this spoke
  vtkVoronoiSpoke()
    : NeiId(-1)
    , Classification(0)
  {
  }
  vtkVoronoiSpoke(vtkIdType neiId, unsigned char classification)
    : NeiId(neiId)
    , Classification(classification)
  {
  }
}; // vtkVoronoiSpoke

/**
 * The vtkVoronoiWheelsType vector is used to keep track of the number of
 * spokes (and equivalently, the number of faces) in each Voronoi tile/hull
 * (due to dual property, there is a one-to-one correspondance between spoke
 * and a tile edge/hull face). The vtkVoronoiWheelsType vector is basically
 * an array of offsets into the spokes vector.
 */
using vtkVoronoiSpokesType = std::vector<vtkVoronoiSpoke>;
using vtkVoronoiSpokesIterator = vtkVoronoiSpokesType::iterator;
using vtkVoronoiWheelsType = std::vector<vtkIdType>;

// Gather spokes into a wheel. Define some basic operators.  Note that every
// wheel is associated with an input (tile/hull generating) point. So access
// to the wheel and its associated spokes is via point id.
struct vtkVoronoiWheel
{
  vtkVoronoiWheelsType& Wheels; // The composited array of wheels
  vtkVoronoiSpokesType& Spokes; // The composited array of spokes
  vtkIdType Id;                 // The associated point/tile id: with wheelId == pointId
  int NumSpokes;                // The number of emanating spokes
  vtkVoronoiSpoke* LocalSpokes; // The array of spokes connected to this wheel

  // Default instantiation.
  vtkVoronoiWheel(vtkVoronoiWheelsType& wheels, vtkVoronoiSpokesType& spokes)
    : Wheels(wheels)
    , Spokes(spokes)
    , Id(-1)
    , NumSpokes(0)
    , LocalSpokes(nullptr)
  {
  }

  // Setup the wheel for queries: an efficient form that does not require
  // repeated wheel instantiation.
  vtkVoronoiSpoke* Initialize(vtkIdType id, int& numSpokes)
  {
    this->Id = id;
    this->NumSpokes = numSpokes = (this->Wheels[id + 1] - this->Wheels[id]);
    this->LocalSpokes = &(this->Spokes[this->Wheels[id]]);
    return this->LocalSpokes;
  }
}; // vtkVoronoiWheel

/**
 * The adjacency graph, a collection of wheels and spokes, is a topological
 * construct that connects Voronoi hull face neighbors.  Each n-dimensional
 * Voronoi hull is represented by a set of spokes, which correspond to the
 * (n-1) faces of the hull. Spokes are classified, and are a dual construct
 * of the hull faces.
 */
struct vtkVoronoiAdjacencyGraph
{
  vtkVoronoiWheelsType Wheels; // Wheel/spokes data structure: offset array into spokes
  vtkVoronoiSpokesType Spokes; // Spokes / edges with classification

  void Initialize(vtkIdType numWheels, vtkIdType numSpokes);
  vtkVoronoiWheelsType& GetWheels() { return this->Wheels; }
  vtkVoronoiSpokesType& GetSpokes() { return this->Spokes; }
  vtkIdType GetNumberOfWheels() { return (this->Wheels.size() - 1); }
  vtkIdType GetNumberOfSpokes() { return this->Spokes.size(); }
  bool IsSpoke(vtkIdType pt0, vtkIdType pt1);
  vtkIdType GetNumberOfSpokes(vtkIdType ptId); // #spokes for a specified wheel
  vtkVoronoiSpoke* GetSpokes(vtkIdType ptId, vtkIdType& numSpokes);
  vtkIdType GetWheelOffset(vtkIdType ptId) { return this->Wheels[ptId]; }
  static void CountFaces(const vtkVoronoiSpoke* spokes, int numSpokes, int& numDomainBoundaryFaces,
    int& numRegionBoundaryFaces, int& numForwardFaces);

  /**
   * Return true if the graph meets the conditions necessary to form a valid
   * Voronoi tessellation. That is, each interior spoke is used exactly twice;
   * each domain boundary edge is used exactly once. Note that this method will
   * classify each invalid spoke as "PRUNED" (which can be used subsequently to
   * repair the tessellation).
   */
  bool Validate();

  /**
   * Threaded perform validity checking.
   */
  struct ValidateAdjacencyGraph
  {
    vtkVoronoiAdjacencyGraph& Graph;
    vtkIdType NumInvalid;
    bool AllValid;

    ValidateAdjacencyGraph(vtkVoronoiAdjacencyGraph& graph)
      : Graph(graph)
      , NumInvalid(0)
      , AllValid(false)
    {
    }

    // Keep track whether threads are non-degenerate.
    vtkSMPThreadLocal<vtkIdType> ThreadInvalid;
    vtkSMPThreadLocal<unsigned char> ThreadAllValid;

    // vtkSMPTools threaded interface
    void Initialize();
    void operator()(vtkIdType wheelId, vtkIdType endWheelId);
    void Reduce();
  }; // ValidateAdjacencyGraph
};   // vtkVoronoiAdjacencyGraph

/**
 * Define hull/tile vertex types. This is to represent generated points,
 * including topological coordinates.
 */
struct vtkVoronoiHullVertex // 3D hull vertices
{
  double X[3];
  vtkVoronoiHullVertex(double x, double y, double z)
    : X{ x, y, z }
  {
  }
  vtkVoronoiHullVertex(const double x[3])
    : X{ x[0], x[1], x[2] }
  {
  }
};
using vtkVoronoiHullVertexType = std::vector<vtkVoronoiHullVertex>;

struct vtkVoronoiTileVertex // 2D tile vertices
{
  double X[2];
  vtkVoronoiTileVertex(double x, double y)
    : X{ x, y }
  {
  }
  vtkVoronoiTileVertex(const double x[2])
    : X{ x[0], x[1] }
  {
  }
};
using vtkVoronoiTileVertexType = std::vector<vtkVoronoiTileVertex>;

/**
 * A topological coordinate of dimension N are the N+1 ids of neighboring N+1
 * Voronoi generator points whose spokes form half spaces intersecting at a
 * tile (2D) / hull (3D) vertex. In non-degenerate situations, the
 * topological coordinate enumerates a Delaunay simplex (triangle 2D or
 * tetrahedron 3D). The topological coordinate is used to topological merge
 * coincident points, generate a Delaunay triangulation, characterize local
 * topology (e.g., for smoothing), and verify the correctness of the
 * resulting Voronoi tessellation or its dual Delaunay triangulation.
 */
struct vtkVoronoiTopoCoord3D
{
  /**
   * Points defining a topological coord tuple / Delaunay simplex. std::array
   * has some nice properties including being easily sortable.
   */
  std::array<vtkIdType, 4> Ids;

  /**
   * Various flavors of constructors.
   */
  vtkVoronoiTopoCoord3D()
    : Ids{ 0 }
  {
  }
  /**
   * Define with the N+1 point generators: the N generators producing
   * the hull vertex, plus the current point generator.
   */
  vtkVoronoiTopoCoord3D(vtkIdType p0, vtkIdType p1, vtkIdType p2, vtkIdType ptId)
    : Ids{ p0, p1, p2, ptId }
  {
    std::sort(this->Ids.data(), this->Ids.data() + this->Ids.size());
  }
  /**
   * Copy constructor assumes that tuple ids are already sorted.
   */
  vtkVoronoiTopoCoord3D(const vtkVoronoiTopoCoord3D& tt) { this->Ids = tt.Ids; }

  /**
   * Operator< used to support a subsequent sort operation of the n-tuples
   * (used for uniquely identifying and producing a topologcally coincident
   * point).
   */
  bool operator<(const vtkVoronoiTopoCoord3D& tuple) const { return this->Ids < tuple.Ids; }
};
using vtkVoronoiTopoCoords3DType = std::vector<vtkVoronoiTopoCoord3D>;

struct vtkVoronoiTopoCoord2D
{
  /**
   * Points defining a topological coord tuple / Delaunay simplex. std::array
   * has some nice properties including being easily sortable.
   */
  std::array<vtkIdType, 3> Ids;

  /**
   * Various flavors of constructors.
   */
  vtkVoronoiTopoCoord2D()
    : Ids{ 0 }
  {
  }
  /**
   * Define with the N+1 point generators: the N generators producing
   * the hull vertex, plus the current point generator.
   */
  vtkVoronoiTopoCoord2D(vtkIdType p0, vtkIdType p1, vtkIdType ptId)
    : Ids{ p0, p1, ptId }
  {
    std::sort(this->Ids.data(), this->Ids.data() + this->Ids.size());
  }
  /**
   * Copy constructor assumes that tuple ids are already sorted.
   */
  vtkVoronoiTopoCoord2D(const vtkVoronoiTopoCoord2D& tt) { this->Ids = tt.Ids; }

  /**
   * Operator< used to support a subsequent sort operation of the n-tuples
   * (used for uniquely identifying and producing a topologcally coincident
   * point).
   */
  bool operator<(const vtkVoronoiTopoCoord2D& tuple) const { return this->Ids < tuple.Ids; }
};
using vtkVoronoiTopoCoords2DType = std::vector<vtkVoronoiTopoCoord2D>;

/**
 * Merge tuples contain an additional point id, which is the global id of a
 * tile/hull point, with most of these points being duplicates. Later, these
 * duplicate points are topologically merged to produce a final,
 * non-duplicate point id (suitable for producing connected, conformal
 * output).
 */
struct vtkVoronoiMergeTuple3D : public vtkVoronoiTopoCoord3D
{
  vtkIdType PtId; // the id of the hull vertex

  vtkVoronoiMergeTuple3D()
    : PtId(-1)
  {
  }
  bool operator!=(const vtkVoronoiMergeTuple3D& mt) const { return (this->Ids != mt.Ids); }
}; // vtkVoronoiMergeTuple3D

struct vtkVoronoiMergeTuple2D : public vtkVoronoiTopoCoord2D
{
  vtkIdType PtId; // the id of the tile vertex

  vtkVoronoiMergeTuple2D()
    : PtId(-1)
  {
  }
  bool operator!=(const vtkVoronoiMergeTuple2D& mt) const { return (this->Ids != mt.Ids); }
}; // vtkVoronoiMergeTuple2D

/**
 * Global tile/hull vertices, with duplicates, that are assigned a global id
 * (if point merging is performed). Duplicate vertices are sorted to group
 * them, and a merge map is built to assign global point ids without
 * duplicates (i.e., a topological merge is performed).
 */
using vtkMergeTupleOffsets = std::vector<vtkIdType>; // offsets into merged tuples
using vtkMergeTuples3DType = std::vector<vtkVoronoiMergeTuple3D>;
using vtkMergeTuples2DType = std::vector<vtkVoronoiMergeTuple2D>;

/**
 * When merging points, the merge map is a vector that maps
 * global tile/hull vertex ids (which contain duplicates) into
 * global point ids (which have duplicate points merged).
 */
using vtkMergeMapType = std::vector<vtkIdType>;

/**
 * Convenience type for representing cell connectivity during compositing.
 */
using vtkVoronoiCellConnType = std::vector<vtkIdType>;

/**
 * Class to manage batches of points. This is used to improve threaded
 * performance and reduce memory.
 */
struct vtkVoronoiBatchManager
{
  vtkIdType Num;        // Number of total items (e.g., points) to process
  vtkIdType BatchSize;  // The desired batch size (clamped by Num)
  vtkIdType NumBatches; // The total number of batches to process
  vtkVoronoiBatchManager(vtkIdType num, vtkIdType batchSize)
    : Num(num)
    , BatchSize(batchSize)
  {
    this->NumBatches = static_cast<vtkIdType>(ceil(static_cast<double>(num) / batchSize));
  }
  vtkIdType GetNumberOfBatches() const { return this->NumBatches; }
  vtkIdType GetBatchItemRange(vtkIdType batchNum, vtkIdType& startId, vtkIdType& endId) const
  {
    startId = batchNum * this->BatchSize;
    endId = startId + this->BatchSize;
    endId = (endId > this->Num ? this->Num : endId);
    return (endId - startId);
  }
}; // vtkVoronoiBatchManager

/**
 * Keep track of batches of generating points. The threading occurs
 * over contiguous batches of points.
 */
using vtkBatchIdsType = std::vector<vtkIdType>;

// Convenience function: convert input labels/region ids/scalars to signed int.
// The Voronoi classes expect signed int region labels.
inline vtkSmartPointer<vtkIntArray> ConvertRegionLabels(vtkDataArray* inScalars)
{
  vtkNew<vtkIntArray> rIds;
  rIds->SetNumberOfTuples(inScalars->GetNumberOfTuples());
  rIds->DeepCopy(inScalars);

  return rIds;
}

/**
 * Convenience class to check and interrupt processing aborts during
 * threaded processing.
 */
struct vtkVoronoiAbortCheck
{
  vtkAlgorithm* Filter;
  bool IsFirst;
  vtkIdType CheckAbortInterval;

  vtkVoronoiAbortCheck(vtkIdType start, vtkIdType end, vtkAlgorithm* filter)
    : Filter(filter)
  {
    this->IsFirst = vtkSMPTools::GetSingleThread();
    this->CheckAbortInterval = std::min((end - start) / 10 + 1, (vtkIdType)1000);
  }

  bool operator()(vtkIdType id)
  {
    if (this->Filter && this->IsFirst && !(id % this->CheckAbortInterval))
    {
      this->Filter->CheckAbort();
      return (this->Filter->GetAbortOutput() ? true : false);
    }
    return false;
  }
}; // vtkVoronoiAbortCheck

// Use system <random> - create a simple convenience class. This generates
// random color indices [0,64).
struct vtkVoronoiRandomColors
{
  std::mt19937 RNG;
  std::uniform_int_distribution<int> Dist;
  vtkVoronoiRandomColors() { this->Dist.param(typename decltype(this->Dist)::param_type(0, 64)); }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  vtkIdType Next() { return this->Dist(RNG); }
};

// Use system <random> - create a simple convenience class. This generates
// random real values [0,1).
struct vtkVoronoiRandom01Range
{
  std::mt19937 RNG;
  std::uniform_real_distribution<double> Dist;
  vtkVoronoiRandom01Range()
  {
    this->Dist.param(typename decltype(this->Dist)::param_type(0.0, 1.0));
  }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  double Next() { return this->Dist(RNG); }
};

// A convenience class and methods to randomly perturb (joggle or jitter)
// point positions. Such jittering (even if very small) significantly
// improves the numerical stability of Voronoi and Delaunay computations.
struct vtkVoronoiJoggle
{
  // Joggle a single point at input position xIn to produce the output position
  // xOut (xIn and xOut may be computed in-place). The radius is the allowable
  // range of joggle in the sphere. A sequence is provided, assumed properly
  // initialized, to produce random (0,1) values. Note that if this method is
  // invoked in a thread, separate sequence instantiations (one per thread)
  // should be provided.
  static void JoggleXYZ(
    double xIn[3], double xOut[3], double radius, vtkVoronoiRandom01Range& sequence)
  {
    double cosphi = 1 - 2 * sequence.Next();
    double sinphi = sqrt(1 - cosphi * cosphi);
    double rho = radius * pow(sequence.Next(), 0.33333333);
    double R = rho * sinphi;
    double theta = 2.0 * vtkMath::Pi() * sequence.Next();
    xOut[0] = xIn[0] + R * cos(theta);
    xOut[1] = xIn[1] + R * sin(theta);
    xOut[2] = xIn[2] + rho * cosphi;
  }

  // Joggle a single point at input position xIn to produce the output
  // position xOut (xIn and xOut may be computed in-place). The radius is the
  // allowable range of joggle in the circle in the x-y plane. A sequence is
  // provided, assumed properly initialized, to produce random (0,1) values.
  // Note that if this method is invoked in a thread, separate sequence
  // instantiations (one per thread) should be provided.
  static void JoggleXY(
    double xIn[3], double xOut[3], double radius, vtkVoronoiRandom01Range& sequence)
  {
    double R = radius * sequence.Next();
    double theta = 2.0 * vtkMath::Pi() * sequence.Next();
    xOut[0] = xIn[0] + R * cos(theta);
    xOut[1] = xIn[1] + R * sin(theta);
    xOut[2] = xIn[2];
  }

  // Joggle a single point at input position xIn to produce the output
  // position xOut (xIn and xOut may be computed in-place). The radius is the
  // allowable range of joggle in the circle in the x-z plane. A sequence is
  // provided, assumed properly initialized, to produce random (0,1) values.
  // Note that if this method is invoked in a thread, separate sequence
  // instantiations (one per thread) should be provided.
  static void JoggleXZ(
    double xIn[3], double xOut[3], double radius, vtkVoronoiRandom01Range& sequence)
  {
    double R = radius * sequence.Next();
    double theta = 2.0 * vtkMath::Pi() * sequence.Next();
    xOut[0] = xIn[0] + R * cos(theta);
    xOut[1] = xIn[1];
    xOut[2] = xIn[2] + R * sin(theta);
  }

  // Joggle a single point at input position xIn to produce the output
  // position xOut (xIn and xOut may be computed in-place). The radius is the
  // allowable range of joggle in the circle in the y-z plane. A sequence is
  // provided, assumed properly initialized, to produce random (0,1) values.
  // Note that if this method is invoked in a thread, separate sequence
  // instantiations (one per thread) should be provided.
  static void JoggleYZ(
    double xIn[3], double xOut[3], double radius, vtkVoronoiRandom01Range& sequence)
  {
    double R = radius * sequence.Next();
    double theta = 2.0 * vtkMath::Pi() * sequence.Next();
    xOut[0] = xIn[0];
    xOut[1] = xIn[1] + R * cos(theta);
    xOut[2] = xIn[2] + R * sin(theta);
  }
}; // vtkVoronoiJoggle

VTK_ABI_NAMESPACE_END
#include "vtkVoronoiCore.txx"

#endif
// VTK-HeaderTest-Exclude: vtkVoronoiCore.h
