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

#include "vtkAbstractPointLocator.h" // for the neighbor iterator
#include "vtkAlgorithm.h"            // check abort status if embedded in filter
#include "vtkDoubleArray.h"          // for the petals of the neighbor iterator
#include "vtkIntArray.h"             // for representing segmented region ids
#include "vtkKdTreePointLocator.h"   // for the neighbor iterator
#include "vtkLocatorInterface.h"     // for vtkDist2TupleArray
#include "vtkSMPTools.h"             // SMP parallel processing
#include "vtkSmartPointer.h"         // for the neighbor iterator
#include "vtkStaticPointLocator.h"   // for the neighbor iterator
#include "vtkStaticPointLocator2D.h" // for the neighbor iterator

#include <algorithm> // for std::sort
#include <iostream>
#include <limits>
#include <random> // random generation of colors
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
    this->LocalSpokes =
      ((size_t)this->Wheels[id] < this->Spokes.size() ? &(this->Spokes[this->Wheels[id]])
                                                      : nullptr);
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

    ValidateAdjacencyGraph(vtkVoronoiAdjacencyGraph& graph)
      : Graph(graph)
      , NumInvalid(0)
    {
    }

    // Keep track whether threads are non-degenerate.
    vtkSMPThreadLocal<vtkIdType> ThreadNumInvalid;

    // vtkSMPTools threaded interface
    void Initialize();
    void operator()(vtkIdType wheelId, vtkIdType endWheelId);
    void Reduce();
  }; // ValidateAdjacencyGraph
}; // vtkVoronoiAdjacencyGraph

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
  vtkVoronoiRandomColors() { this->Dist.param(decltype(this->Dist)::param_type(0, 64)); }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  vtkIdType Next() { return this->Dist(RNG); }
};

// Use system <random> - create a simple convenience class. This generates
// random real values [0,1).
struct vtkVoronoiRandom01Range
{
  std::mt19937 RNG;
  std::uniform_real_distribution<double> Dist;
  vtkVoronoiRandom01Range() { this->Dist.param(decltype(this->Dist)::param_type(0.0, 1.0)); }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  double Next() { return this->Dist(RNG); }
};

// A convenience class and methods to randomly perturb (alternatively:
// joggle, jitter, or jiggle) point positions. Such jittering (even if very
// small) significantly improves the numerical stability of Voronoi and
// Delaunay computations. Terminology note: while joggle is not a widely used
// term (typically in computer graphics the words perturb, jitter, and jiggle
// are found more frequently), it is used by the popular QuickHull (and
// associated implementation QHull qhull.org)--so the terminology was
// adopted by this work. Implementation note: these methods might be better
// added to vtkMath since they can be used by other classes. Once they are
// demonstrated to be stable, they may be moved.
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

  // Joggle a 3D vector. Specify an input normalized vector (vecIn), a angle
  // of rotation theta (in radians, 0<theta<Pi/2), and a random
  // sequence. Produce on output a normalized vector (vecOut) -- vecIn and
  // vecOut may be computed in place. Note that if this method is invoked in
  // a thread, separate sequence instantiations (one per thread) should be
  // provided. The approach used here is to define a disk located at the tip
  // of the (normalized) vecIn (the disc is normal to vecIn), and randomly
  // select a vector from the edge/along the perimeter of the disc.
  static void JoggleNormal(
    double vecIn[3], double vecOut[3], double theta, vtkVoronoiRandom01Range& sequence)
  {
    // Find a vector orthogonal to the input vector.
    double perp[3];
    int iMax = (std::fabs(vecIn[0]) > std::fabs(vecIn[1]) ? 0 : 1);
    iMax = (std::fabs(vecIn[iMax]) > std::fabs(vecIn[2]) ? iMax : 2);
    perp[(iMax + 1) % 3] = 1.0;
    perp[(iMax + 2) % 3] = 0.0;
    perp[iMax] = -(vecIn[(iMax + 1) % 3] / vecIn[iMax]);
    vtkMath::Normalize(perp);

    // Now find third orthogonal vector. Since vecIn and perp are normalized and
    // orthogonal, the resultant cross also a unit vector.
    double cross[3];
    vtkMath::Cross(vecIn, perp, cross);

    // Compute a random vector
    double radius = theta / (2.0 * vtkMath::Pi()); // small angle approximation
    double t = 2.0 * vtkMath::Pi() * sequence.Next();
    double x = radius * cos(t);
    double y = radius * sin(t);

    // Construct a vector along the disc/cone edge
    vecOut[0] = vecIn[0] + x * perp[0] + y * cross[0];
    vecOut[1] = vecIn[1] + x * perp[1] + y * cross[1];
    vecOut[2] = vecIn[2] + x * perp[2] + y * cross[2];
    vtkMath::Normalize(vecOut);
  }
}; // vtkVoronoiJoggle

//------------------------------------------------------------------------------
/**
 * Iterate over the neighbors of a Voronoi generator in spherical shells (the
 * annuli of a 2D tile), with any point locator, of dimension DIM (2 or 3).
 * BuildHull() (or BuildTile()) requests batches of points around a generator,
 * nearest first, until it can prove that no point not yet returned can modify
 * the hull. The first batch is the shell of
 * vtkAbstractPointLocator::FindNPointsInShell(), which holds the nearest
 * points, and the next ones are the shells of FindPointsInShell(), which grow
 * geometrically, up to the circumflower. Next() can cull the shells with the
 * petals of the Voronoi flower, spheres (x, y, z, r^2) in 3D, or circles
 * (x, y, r^2) in 2D: the points outside of all of them are skipped. Used by
 * both vtkVoronoiCore3D (DIM = 3) and vtkVoronoiCore2D (DIM = 2).
 *
 * The petals have DIM + 1 components, which the locator expects when it is
 * of dimension DIM: that of vtkStaticPointLocator is 3, that of
 * vtkStaticPointLocator2D is 2, and that of vtkKdTreePointLocator is 2 if its
 * points are in a plane of constant z and 3 otherwise. When the dimension of
 * the locator is not DIM, the petals are not used (see IsLocatorSuited()).
 */
template <int DIM>
struct vtkVoronoiNeighborIterator
{
  static_assert(DIM == 2 || DIM == 3, "The dimension of the Voronoi tessellation is 2 or 3.");

  // The ratio of the squared radii of consecutive shells. Small shells let the
  // circumflower and the petals shrink with the hull between two shells.
  static constexpr double ShellGrowth2 = 2.0;

  // The shells of at most this many points are sorted, nearest first, so that
  // the nearest points clip the hull first. Larger shells, found around dense
  // clusters of points, are left in the order of the locator: sorting them
  // costs more than it saves.
  static constexpr std::size_t MaxSortedShellSize = 128;

  vtkAbstractPointLocator* Locator = nullptr;
  int FirstShellSize = 16; // the number of nearest points of the first shell
  vtkIdType PtId = -1;
  double X[3] = { 0.0, 0.0, 0.0 };
  double MinD2 = 0.0;
  bool Done = false;
  bool UsePetals = true;

  // The dimension of the locator (2 or 3), or 0 for a locator that accepts the
  // petals of any dimension.
  static int GetLocatorDimension(vtkAbstractPointLocator* loc)
  {
    if (auto* kdLoc = vtkKdTreePointLocator::SafeDownCast(loc))
    {
      return kdLoc->GetDimension();
    }
    if (vtkStaticPointLocator2D::SafeDownCast(loc))
    {
      return 2;
    }
    if (vtkStaticPointLocator::SafeDownCast(loc))
    {
      return 3;
    }
    return 0;
  }

  // Return false if the locator is not of dimension DIM: for example, a
  // vtkKdTreePointLocator of 3D points, for a 2D tessellation. It must be
  // built. The petals can't be used then: the filters warn about it.
  static bool IsLocatorSuited(vtkAbstractPointLocator* loc)
  {
    const int locDim = GetLocatorDimension(loc);
    return locDim == 0 || locDim == DIM;
  }

  // A Voronoi cell has about 15.5 neighbors on average in 3D, and 6 in 2D: the
  // first shell holds about that many by default (12 in 2D, 16 in 3D), which
  // was measured to be the fastest with a vtkKdTreePointLocator.
  void Initialize(vtkAbstractPointLocator* loc, int firstShellSize = 0)
  {
    this->Locator = loc;
    constexpr int defaultShellSize = (DIM == 2 ? 12 : 16);
    this->FirstShellSize = firstShellSize > 0 ? firstShellSize : defaultShellSize;
    this->UsePetals = IsLocatorSuited(loc);
  }

  bool Begin(vtkIdType ptId, double x[3], vtkDist2TupleArray& results)
  {
    this->PtId = ptId;
    std::copy_n(x, 3, this->X);

    // The first shell holds the nearest points, and all the points as close
    // as the farthest of them. It is the shell that FindPointsInShell() would
    // return, so that the next shells, which start at its radius, return
    // every point once. (A negative minimum distance keeps points coincident
    // with the generator.)
    const double firstD2 = this->Locator->FindNPointsInShell(
      this->FirstShellSize, this->X, results, -1.0, /*sort=*/true, /*petals=*/nullptr);
    if (static_cast<int>(results.size()) < this->FirstShellSize)
    {
      // Fewer points than requested: there are no more points, or the locator
      // does not support FindNPointsInShell(). A single shell has them all.
      this->Done = true;
      this->FindShell(-1.0, std::numeric_limits<double>::max(), nullptr, results);
      return true;
    }
    this->Done = false;
    this->RemoveGenerator(results);
    this->MinD2 = firstD2;
    return true;
  }

  bool Next(double radius2, vtkDoubleArray* petals, vtkDist2TupleArray& results)
  {
    if (this->Done)
    {
      return false;
    }
    // The last shell reaches the circumflower: no point is left after it.
    double maxD2 = this->MinD2 * ShellGrowth2;
    if (!(maxD2 > this->MinD2) || maxD2 >= radius2)
    {
      maxD2 = radius2;
      this->Done = true;
    }
    this->FindShell(this->MinD2, maxD2, this->UsePetals ? petals : nullptr, results);
    return true;
  }

  // Every point not yet returned is farther than this (squared) distance.
  double GetMinD2() const { return this->MinD2; }

private:
  void FindShell(double minD2, double maxD2, vtkDoubleArray* petals, vtkDist2TupleArray& results)
  {
    this->Locator->FindPointsInShell(this->X, results, minD2, maxD2, /*sort=*/false, petals);
    if (results.size() <= MaxSortedShellSize)
    {
      std::sort(results.begin(), results.end());
    }
    this->RemoveGenerator(results);
    this->MinD2 = this->Done ? std::numeric_limits<double>::max() : maxD2;
  }

  void RemoveGenerator(vtkDist2TupleArray& results)
  {
    results.erase(std::remove_if(results.begin(), results.end(),
                    [this](const vtkDist2Tuple& t) { return t.Id == this->PtId; }),
      results.end());
  }
}; // vtkVoronoiNeighborIterator

VTK_ABI_NAMESPACE_END
#include "vtkVoronoiCore.txx"

#endif
// VTK-HeaderTest-Exclude: vtkVoronoiCore.h
