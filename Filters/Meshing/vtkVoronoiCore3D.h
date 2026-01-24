// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVoronoiCore3D
 * @brief   provide core 3D Voronoi tessellation capabilities
 *
 * The Voronoi tessellation is a common computational tool used in a variety
 * of applications ranging from tessellating points, mesh generation, surface
 * reconstruction, materials anaysis, and contouring (surface nets).  It can
 * also be the basis for computing its dual construct, the Delaunay
 * triangulation, also used in wide-ranging applications with significant
 * impacts. This templated class provides core 3D Voronoi tessellation
 * capabilities, including implementation of fast parallel algorithms, which
 * can be used by other classes to create specialized Voronoi-based
 * algorithms.
 *
 * The Voronoi tessellation is a tessellation of space, where each Voronoi
 * n-dimensional tile (in 3D, a polyhedron, also referred to as a point hull,
 * or more simply, hull) represents the region nearest to one of the input
 * points.  Under the hood, the vtkVoronoiCore3D class depends on the
 * vtkVoronoiHull class to construct hulls. The vtkVoronoiCore3D provides a
 * framework for parallel, SMP shared memory algorithms, which can be
 * specialized to meet the needs of algorithms requiring Voronoi and/or
 * Delaunay-based capabilities. Specialization of the algorithm is via a
 * TCompositor template argument, which controls what information is
 * extracted from each hull as it is processed, and how the information is
 * combined (during a compositing process) to produce output.
 *
 * Publications are in preparation to describe the algorithm.  Conceptually,
 * the algorithm is meshless, meaning that each input point and its
 * associated hull is processed independently.  However, methods are provided
 * to transform the output into a fully-connected, valid, conformal mesh if
 * required. To summarize: in parallel, each (generating) input point is
 * associated with an initial Voronoi hull, which is simply the bounding box
 * of the input point set. A spatial locator is then used to identify nearby
 * points: each neighbor in turn generates a clipping plane positioned
 * halfway between the generating point and the neighboring point, and
 * orthogonal to the line connecting them. Clips are performed by evaluationg
 * the vertices of the convex Voronoi hull as being on either side
 * (inside,outside) of the clip plane. If an intersection with the Voronoi
 * hull is found, the portion of the hull "outside" the clip plane is
 * discarded, producing a new convex face, resulting in a new convex, Voronoi
 * hull. As each clip occurs, the Voronoi "Flower" error metric (the union of
 * Delunay spheres) is compared to the extent of the region containing the
 * neighboring clip points. The clip region (along with the points contained
 * in it) is grown by careful expansion, When the Voronoi circumflower is
 * contained within the clip region, the algorithm terminates and the Voronoi
 * hull is output. Once complete, it is possible to construct the Delaunay
 * triangulation from the Voronoi tessellation.  Note that topological and
 * geometric information can be used to generate a valid triangulation (e.g.,
 * merging coincident points and validating topology).
 *
 * This class can also construct a Voronoi adjacency graph, composed of edges
 * (the spokes) that connect each Voronoi hull generating point (the wheels)
 * with their face neighbors. The adjacency graph is a powerful data
 * representation that captures proximal neighborhood information. It has
 * many practical applications such as shortest path computation.
 *
 * An implementation note: this class is implemented using a parallel
 * algorithm. The output is invariant no matter what order the the threads
 * execute, i.e., the construction of geometric primitives (Voronoi cells,
 * adjacency graphs, etc.) is identical no matter the number of threads used,
 * or execution order. Also, note the correspondance between input generating
 * point of ptId and hullId, ptId produces hulls of hullId, where ptId ==
 * hullId.  This means for debugging purposes, picking output primitives with
 * POINT_IDS enabled provides a means to select the original generating hull.
 *
 * This class is templated, enabling specialized capabilities depending on
 * the using algorithm / filter. Using templating, it is possible to extract
 * different information from each Voronoi hull as it is constructed,
 * compositing this information later to produce different types of output.
 *
 * @warning
 * Coincident input points are discarded. The Voronoi tessellation requires
 * unique input points.
 *
 * @warning
 * This approach implements an embarrassingly parallel, meshless
 * algorithm. At the core of the algorithm a locator is used to determine
 * points close to a specified position. Currently, a vtkStaticPointLocator
 * is used because it is both threaded (when constructed) and supports
 * thread-safe queries. While other locators could be used in principal, they
 * must support thread-safe operations. This is done by defining and
 * implementing an iterator that enables traversal of neighborhood points
 * surrounding each generating point.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 * For example, in recent benchmarks using a 48-thread desktop, one million
 * points can be processed in well under one second elapsed time.
 *
 * @sa
 * vtkVoronoiHull vtkVoronoi3D vtkGeneralizedSurfaceNets3D
 */

#ifndef vtkVoronoiCore3D_h
#define vtkVoronoiCore3D_h

#include "vtkLocatorInterface.h"   // Interface to spatial locators
#include "vtkPoints.h"             // for input points
#include "vtkSMPTools.h"           // SMP parallel processing
#include "vtkShellBinIterator.h"   // Iteration over the bins of the locator
#include "vtkStaticPointLocator.h" // for locating nearby points
#include "vtkVoronoiCore.h"        // Core includes
#include "vtkVoronoiHull.h"        // Generate Voronoi convex hulls

VTK_ABI_NAMESPACE_BEGIN

/**
 * Implementation note about the compositor. Depending on which templated
 * functors are being used, the compositor must provide some methods and
 * classes for successful usage. This includes defining the composition
 * information to extract (vtkCompositeInfo); the vector of compositing
 * information across all point generators (vtkCompositeInformation); and
 * methods to initialize (Initialize()) and finalize (Finalize()) the
 * compositor. Also naming conventions are typically used when gathering
 * thread local data--for example Points and TopoCoords are local data
 * members that represent Voronoi tile/hull points, and topological
 * coordinates.  To learn more (by way of examples), view
 * TestVoronoiCore3D.cxx and the concrete VTK filters vtkVoronoi3D and
 * vtkGeneralizedSurfaceNets3D.
 */

/**
 * This is the default functor that classifies the spokes and associated
 * point regions as they are processed. (Spokes are edges that represent the
 * connection between Voronoi hull face neighbors.) This functor is used in
 * the vtkVoronoiCore3D classes as the default TClassification template
 * parameter.  It can be specialized (via templating) for more complex
 * spoke/face classification.  All classifiers must provide the methods
 * AddAdjacencyInformation(), IsInsideRegion(), IsSameRegion(), and
 * Initialize() as defined below. Note that the vtkSpokeClassification (found
 * in vtkVoronoiCore.h) specifies the possible classification values.
 */
struct vtkVoronoiClassifier3D
{
  // Optional regions ids for point classification.
  const int* Regions;

  // Constructors
  vtkVoronoiClassifier3D()
    : Regions(nullptr)
  {
  }
  vtkVoronoiClassifier3D(const int* regions)
    : Regions(regions)
  {
  }

  // Method required by vtkVoronoiCore3D.
  void Initialize(vtkVoronoiClassifier3D* c)
  {
    if (c)
    {
      this->Regions = c->Regions;
    }
  }

  // Method required by vtkVoronoiCore3D.
  const vtkVoronoiSpoke* AddAdjacencyInformation(vtkVoronoiHull& hull, vtkVoronoiWheelsType& wheels,
    vtkVoronoiSpokesType& spokes, int& numSpokes, int& maxPoints, int& maxFaces);

  // Method required by vtkVoronoiCore3D. By default, any region id >= 0 is
  // considered a valid inside region (<0 region values are reserved for
  // algorithm use). If no region ids have been specified, than the point
  // ptId is inside an interior region.
  bool IsInsideRegion(vtkIdType ptId)
  {
    if (!this->Regions)
    {
      return (ptId >= 0);
    }
    else
    {
      return (ptId >= 0 && this->Regions[ptId] >= 0);
    }
  }

  // Method required by vtkVoronoiCore3D. Determine if the two points
  // ptId and neiId (which form a spoke) are in the same region. It is
  // assumed that both (ptId,neiId >= 0), i.e., inside.
  bool IsSameRegion(vtkIdType ptId, vtkIdType neiId)
  {
    return (!this->Regions || this->Regions[ptId] == this->Regions[neiId]);
  }
}; // vtkVoronoiClassifier3D

/**
 * The following thread local data is used to process and keep track of
 * information on a per-thread basis.
 */
template <class TCompositor, class TClassifier>
struct vtkVoronoi3DLocalData
{
  vtkIdType ThreadId;                         // assign a thread id [0,NumThreadsUsed)
  int MaxPoints;                              // the maximum number of points in any hull
  int MaxFaces;                               // the maximum number of faces in any hull
  int NumPrunes;                              // total number of pruning operations
  vtkBatchIdsType LocalBatches;               // list of batches processed by this thread
  vtkVoronoiSpokesType LocalSpokes;           // connecting edges/spokes for each hull
  vtkShellBinIterator SIter;                  // iterator over static point locator bins
  vtkVoronoiHull Hull;                        // computational 3D Voronoi hull algorithm
  typename TCompositor::LocalData Compositor; // gather data from compositing operations
  TClassifier Classifier;                     // Used to classify spokes (based on regions)

  vtkVoronoi3DLocalData()
    : ThreadId(-1)
    , MaxPoints(0)
    , MaxFaces(0)
    , NumPrunes(0)
  {
    this->LocalBatches.reserve(2048);
    this->LocalSpokes.reserve(2048);
  }
}; // vtkVoronoi3DLocalData

/**
 * The thread map keeps track of the thread local data across all computing
 * threads. Accessed via thread id [0 <= threadId < NumberOfThreads).
 */
template <class TCompositor, class TClassifier>
using ThreadMapType = std::vector<vtkVoronoi3DLocalData<TCompositor, TClassifier>*>;

/** =========================================================================
 * The templated, core Voronoi class. It is a lightweight supporting
 * class (i.e., not a subclass of vtkObject) meant to be used by specialized
 * algorithms requiring Voronoi and/or Delaunay capabilities.
 *
 * Note: the template argument TCompositor is used to control what
 * information is extracted during tessellation. Different using filters will
 * define and extract information relevant to their application needs. This
 * is accomplished by defining different compositing classes. TClassifier is
 * used to classify the spokes connecting neighborhood points, which to the
 * the dual property, classifies the hull faces.
 */
template <class TCompositor, class TClassifier = vtkVoronoiClassifier3D>
class vtkVoronoiCore3D
{
public:
  /**
   * A factory method to conveniently instantiate and execute the algorithm.
   * This class should be executed using this Execute() method. It returns a
   * unique pointer to an instance of the VoronoiCore3D algorithm. After
   * execution, methods on the instance can be invoked to retrieve relevant
   * information. Note also that a Voronoi compositor should also be provided
   * to this Execute() method. It will contain output as well. Input to the
   * method includes the input (double) points, a prebuilt static point
   * locator, the initial hull bounds padding, a limit on the number of clips
   * each hull can perform, and an optional VTK filter (for controlling
   * execution abort).  An optional (non-null) compositor and/or classifier
   * can be provided which is used to initialize the compositors and/or
   * classifiers in the various threads (for example, providing region
   * ids).  Finally, methods to control degenerate faces (i.e., validation /
   * spoke pruning and tolerance) are provided.
   */
  static std::unique_ptr<vtkVoronoiCore3D<TCompositor, TClassifier>> Execute(vtkAlgorithm* filter,
    unsigned int batchSize, vtkStaticPointLocator* loc, vtkPoints* inPts, double padding,
    vtkIdType maxClips, bool validate, double pruneTol, TCompositor* comp, TClassifier* cl);

  ///@{
  /**
   * Access the local thread data produced by execution of the filter. This
   * includes the compositing data. The data is only available after the
   * Execute() method has been invoked.
   */
  int GetNumberOfThreads() { return this->NumberOfThreads; }
  vtkVoronoi3DLocalData<TCompositor, TClassifier>* GetThreadData(int threadNum)
  {
    return this->ThreadMap[threadNum];
  }
  ///@}

  ///@{
  /**
   * Obtain information about the execution of the Voronoi algorithm. This
   * includes the maximum number of faces found in any hull; the maximum
   * number of points found in any hull; and the number of prunes
   * performed to remove degeneracies.
   */
  int GetMaximumNumberOfPoints() { return this->MaximumNumberOfPoints; }
  int GetMaximumNumberOfFaces() { return this->MaximumNumberOfFaces; }
  int GetNumberOfPrunes() { return this->NumberOfPrunes; }
  ///@}

  /**
   * Obtain the adjacency graph (wheel & spokes data structure). This is
   * constructed during algorithm execution.
   */
  vtkVoronoiAdjacencyGraph& GetAdjacencyGraph() { return this->Graph; }

  /**
   * Constructor. This is typically not directly invoked by the user. The
   * Execute() method is preferred.
   */
  vtkVoronoiCore3D(vtkAlgorithm* filter, vtkVoronoiBatchManager& batcher,
    vtkStaticPointLocator* loc, vtkPoints* inPts, double padding, vtkIdType maxClips, bool validate,
    double pruneTol, TCompositor* comp, TClassifier* c);

  ///@{
  /**
   * Core vtkSMPTools methods to process hulls in parallel, and produce
   * thread local output data. Other compositing data is captured as well.
   * Note that the threading occurs over batches of points. (Note: this is
   * left public for access by vtkSMPTools. These are generally not invoked
   * directly by the user.)
   */
  void Initialize();
  void operator()(vtkIdType batchId, vtkIdType endBatchId);
  void Reduce();
  ///@}

  /**
   * The compositor enables this vtkVoronoiCore3D templated class to be used
   * in different applications. It supports parallel gather/compute of
   * specified information on a hull-by-hull basis, which can then
   * combined/composited to produce output. Users of this class must define
   * their own compositor.
   */
  TCompositor Compositor;

  /**
   * This templated class is used to extend the API of this vtkVoronoiCore3D
   * class, to implement the spoke classification process, to clone copies
   * in multiple threads, and to initialize the classification instances.
   */
  TClassifier Classifier;

  /**
   * Controls processing of batches of generating points. Thread local data is
   * is available after generating the hulls.
   */
  vtkVoronoiBatchManager Batcher;
  ThreadMapType<TCompositor, TClassifier> ThreadMap;

  /**
   * This is used to create the spokes and wheels adjacency graph used to
   * validate the tessellation and produce a Delaunay triangulation. Note
   * that if an "empty" classifier is used, the adjacency graph is empty.
   */
  vtkVoronoiAdjacencyGraph Graph;

  /**
   * Convenience methods to retrieve the number of input points, and
   * the raw double* points array. Invoke this only after execution.
   */
  vtkIdType GetNumberOfPoints() { return this->NPts; }
  const double* GetPoints() { return this->Points; }

  /**
   * Used for controlling filter abort and accessing filter information. If
   * nullptr, then filter abort checking is disabled.
   */
  vtkAlgorithm* Filter;

private:
  vtkIdType NPts;                 // The number of input points
  vtkPoints* InPoints;            // Input points as data array
  const double* Points;           // Input points as pointer to x-y-z AOS doubles
  vtkStaticPointLocator* Locator; // Used to (quickly) find nearby points
  double Padding;                 // The padding distance around the bounding box
  double Bounds[6];               // locator bounds
  double PaddedBounds[6];         // the domain over which Voronoi is calculated
  vtkIdType MaxClips;             // Limit the number of half-space clips

  // Enable pruning of spokes (equivalent to deletion of a degenerate hull face)
  bool Validate;            // Indicate whether to explicitly validate the mesh
  vtkIdType NumberOfPrunes; // If pruning is on, keep track of the number of prunes
  double PruneTolerance;    // Specify the spoke prune tolerance

  // High-level information captured during processing
  int NumberOfThreads;       // Keep track of the number of threads used durinf processing
  int MaximumNumberOfPoints; // Maximum numper of points in a generated Voronoi hull
  int MaximumNumberOfFaces;  // Maximum number of faces (i.e., spokes) in a generated Voronoi hull

  // Storage local to each thread, as well as working/scratch arrays. We
  // don't want to allocate working arrays on every thread invocation. Thread
  // local storage saves lots of new/delete (e.g. the locator tuples).
  vtkSMPThreadLocal<vtkDist2TupleArray> Dist2Tuples;
  vtkSMPThreadLocal<vtkVoronoi3DLocalData<TCompositor, TClassifier>> LocalData;

  /**
   * Driver method that coordinates the process of hull generation. It
   * integrates with the locator, and the vtkVoronoiHull class.
   */
  using vtkBinIterator = vtkShellBinIterator;
  bool BuildHull(vtkVoronoiHull& hull, vtkBinIterator* siter, const double* pts, vtkIdType maxClips,
    vtkDist2TupleArray& results, int& numPrunes);

public:
  /**
   * Produce the global adjacency graph / wheels and spokes data structure.
   * Implementation note: the wheels and spokes adjacency graph is always
   * generated in this implementation. In practice this is not always needed.
   * However it greatly simplifies the implementation. In the future,
   * building the data structure could be made optional. Note that the SMP
   * threading occurs over the local thread data.
   */
  struct ProduceWheelsAndSpokes
  {
    vtkVoronoiCore3D<TCompositor, TClassifier>* VC;
    ProduceWheelsAndSpokes(vtkVoronoiCore3D<TCompositor, TClassifier>* vc);
    void operator()(vtkIdType threadId, vtkIdType endThreadId);

    // Invoke the production of wheels and spokes
    static void Execute(vtkVoronoiCore3D<TCompositor, TClassifier>* vc);
  }; // ProduceWheelsAndSpokes

  /**
   * Functor class used to topologically merge (nearly) coincident points.
   * It basically sorts topological coordinates, and then assigns a global
   * point id to each run (of identical topological coordinates).  The
   * resulting merge map can then be used to generate merged point ids when
   * producing global output. The class depends on the compositor type,
   * which provides the topological coordinates and information about the
   * number of points produced by each hull; and merge tuples, which is the
   * global array that maps hull points into merged global points.  On
   * output, this class produces the total number of merged points, and a
   * merge map that maps the hull points into final point ids.
   */
  struct TopologicalMerge
  {
    vtkVoronoiCore3D<TCompositor, TClassifier>* VC;
    TopologicalMerge(vtkVoronoiCore3D<TCompositor, TClassifier>* vc);

    vtkMergeTuples3DType MergeTuples; // temporary array for merging points
    vtkMergeMapType MergeMap;         // maps tile/hull point ids to merged point ids
    vtkIdType NumMergedPts;           // after merging, the number of points remaining

    /**
     * Methods related to merging coincident points. The number of merged
     * points is the number of remaining points after merging (ie., after
     * removing duplicates). The MergeMap maps the hull point ids (which
     * contain duplicates) to global point ids (no duplicates).
     */
    vtkIdType GetNumberOfMergedPoints() { return this->NumMergedPts; }

    // Core Voronoi methods in support of vtkSMPTools
    void Initialize() {}
    void operator()(vtkIdType threadId, vtkIdType endThreadId);
    void Reduce();

    // Execute the topological point merge to produce a merge map.
    static std::unique_ptr<TopologicalMerge> Execute(
      vtkVoronoiCore3D<TCompositor, TClassifier>* vc);
  }; // TopologicalMerge

}; // vtkVoronoiCore3D

/**
 * These are convenience/demonstration classes for configuring the templated
 * 3D Voronoi classes.
 */

/**
 * Support Voronoi data compositing. This compositor class is responsible
 * for gathering data from each generated hull, which is later combined to
 * form a global output. The empty compositor illustrates the methods that
 * a Voronoi compositor must support, and does not actually gather any
 * information.
 */
struct vtkEmptyVoronoi3DCompositor
{
  /**
   * Prepare to accumulate compositing information: specify the total number
   * of generating points to be processed.
   */
  void Initialize(vtkIdType vtkNotUsed(numPts), vtkEmptyVoronoi3DCompositor*) {}
  void Finalize() {}

  /**
   * Thread local data may be needed.
   */
  struct LocalData
  {
    void Initialize(vtkEmptyVoronoi3DCompositor* vtkNotUsed(c)) {}
    void AddData(vtkVoronoiHull& vtkNotUsed(hull), int vtkNotUsed(numSpokes),
      const vtkVoronoiSpoke* vtkNotUsed(spokes))
    {
    }
  };
}; // vtkEmptyVoronoi3DCompositor

// Almost minimal classifier - just records the hull's number of points and faces. It also
// considers regions if any are defined -- this is optional in some cases.
struct vtkEmptyVoronoi3DClassifier
{
  // Optional region ids
  const int* Regions;

  // Constructor
  vtkEmptyVoronoi3DClassifier()
    : Regions(nullptr)
  {
  }
  vtkEmptyVoronoi3DClassifier(const int* regions)
    : Regions(regions)
  {
  }

  // Initialize
  void Initialize(vtkEmptyVoronoi3DClassifier* c)
  {
    if (c)
    {
      this->Regions = c->Regions;
    }
  }

  // Method required by vtkVoronoiCore3D. This vtkEmptyClassifier provides
  // the minimum information needed.
  const vtkVoronoiSpoke* AddAdjacencyInformation(vtkVoronoiHull& hull, vtkVoronoiWheelsType& wheels,
    vtkVoronoiSpokesType& vtkNotUsed(spokes), int& numSpokes, int& maxPoints, int& maxFaces)
  {
    wheels[hull.PtId] = hull.NumFaces; // numFaces == numSpokes
    maxPoints = (hull.NumPts > maxPoints ? hull.NumPts : maxPoints);
    maxFaces = (hull.NumFaces > maxFaces ? hull.NumFaces : maxFaces);
    numSpokes = 0;
    return nullptr;
  }

  // If no region ids are provided, all processed points are inside the same
  // region.
  bool IsInsideRegion(vtkIdType ptId)
  {
    if (!this->Regions)
    {
      return (ptId >= 0);
    }
    else
    {
      return (ptId >= 0 && this->Regions[ptId] >= 0);
    }
  }
  bool IsSameRegion(vtkIdType ptId, vtkIdType neiId)
  {
    return (!this->Regions || this->Regions[ptId] == this->Regions[neiId]);
  }
}; // vtkEmptyVoronoi3DClassifier

VTK_ABI_NAMESPACE_END
#include "vtkVoronoiCore3D.txx"

#endif
// VTK-HeaderTest-Exclude: vtkVoronoiCore3D.h
