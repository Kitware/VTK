// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVoronoiFlower3D
 * @brief   create a 3D Voronoi tessellation of input points
 *
 * vtkVoronoiFlower3D is a filter that constructs a 3D Voronoi tessellation of a
 * list of input points. The points are presumed to lie within 3D-space and
 * non-coincident. These points may be represented by any dataset of type
 * vtkPointSet and subclasses. Multiple different outputs of the filter are
 * produced depending on the output types selected (as described shortly);
 * for example, an unstructured grid consisting of polyhedral cells, an
 * unstructured grid consisting of a Delaunay tetrahedralization, an
 * adjacency graph, a polygonal complex, exterior boundary of the
 * tessellation, and/or a generalized surface net can be produced.
 *
 * The 3D Voronoi tessellation is a tessellation of space, where each Voronoi
 * n-dimensional tile (in 3D, a polyhedron, also referred to as a point hull,
 * or more simply, hull) represents the region nearest to one of the input
 * points. Voronoi tessellations are important in computational geometry (and
 * many other fields), and form the dual of Delaunay triangulations. Note
 * that in this implementation, points may be labeled (or segmented) with an
 * optional region id, this is useful for specifiying inside / outside, so as
 * to control the tessellation, as well as creating contours between regions
 * (a surface net). (Please note that region ids < 0 mean that the associated
 * point is considered "outside" or "exterior"; this can be used to carve
 * away areas that should not be tessellated.)
 *
 * This filter is a reference implementation written with simplicity in
 * mind. Additional methods are available for debugging / instructional
 * purposes. This includes producing a single hull under various stages of
 * creation, as well as the Voronoi flower and circumflower, the error metric
 * controlling Voronoi's point insertion / half-space clipping process.
 *
 * Publications are in preparation to describe the algorithm. A brief summary
 * is as follows. In parallel, each (generating) input point is associated
 * with an initial Voronoi hull, which is simply the bounding box of the
 * input point set. A locator is then used to identify nearby points: each
 * neighbor in turn generates a clipping plane positioned halfway between the
 * generating point and the neighboring point, and orthogonal to the line
 * connecting them. Clips are readily performed by evaluationg the vertices
 * of the convex Voronoi hull as being on either side (inside,outside) of the
 * clip plane. If an intersection with the Voronoi 3D hull is found, the
 * portion of the hull "outside" the clip line is discarded, resulting in a
 * new convex, Voronoi hull. As each clip occurs, the Voronoi "Flower" error
 * metric (the union of error spheres) is compared to the extent of the
 * region containing the neighboring clip points. The clip region (along with
 * the points contained in it) is grown by careful expansion, When the
 * Voronoi circumflower is contained within the clip region, the algorithm
 * terminates and the Voronoi hull is output. Once complete, it is possible
 * to construct the Delaunay triangulation from the Voronoi tessellation, or
 * extract a generalized surface net between regions. Note that topological
 * and geometric information is used to generate a valid triangulation (e.g.,
 * merging points and validating topology).
 *
 * The filter produces different outputs depending on how the filter is
 * configured. The filter accepts any vtkPointSet (and derived classes) as
 * input. vtkUnstructuredGrid output is produced when the requested output
 * type is a Voronoi tessellation or Delaunay tetrahedralization. vtkPolyData
 * is produced when all other types of output are requested (e.g., adjacency
 * graph, boundary faces, surface net contours, etc.  (Use the methods
 * GetUnstructuredGridOutput() or GetPolyDataOutput() to retrieve the
 * appropriate output.)
 *
 * A useful, optional feature of this filter is its ability to control the
 * tessellation process and/or generation of a surface net via an input
 * region ids array. The optional region ids array is a single component,
 * signed integer array (vtkIntArray) that labels each input point as part of
 * a segmented region, with value <0 meaning outside. As a result, this
 * filter can be used to tessellate different regions using convex polygons
 * (i.e., Voronoi hulls), or create holes in Voronoi tessellations, using
 * this supplemental input single-component, scalar data array (the region
 * ids array). The size of the region ids array must match the number of
 * input points (the region ids must be provided as input point data).
 *
 * This input region ids array can also be used to create a generalized 3D
 * surface net. (See vtkSurfaceNets3D and vtkSurfaceNets2D for surface nets
 * algorithms specialized to image data; and vtkGeneralizedSurfaceNets3D for
 * a general surface nets algorithm based on this Voronoi approach.) A
 * surface net is a type of contour that partitions segmented portions of
 * the input volumetric space into separate regions. (The segmentation
 * process requires the labeling of the input points as a member of a
 * specified region. The segmentation is represented by integral, input
 * scalar point data "region ids".) The resulting surface contour is not
 * necessarily manifold, and typically contains inner structures or "bubbles"
 * demarcating separate portions of the input. (Note that the output surface
 * net can be smoothed by the filters like the vtkConstrainedSmoothingFilter,
 * or vtkWindowedSincPolyDataFilter.)
 *
 * This class can also construct an adjacency graph, composed of edges (the
 * spokes) that connect each Voronoi hull generating point (the wheels) with
 * their face neigbors. The adjacency graph is a powerful data representation
 * that captures proximal neighborhood information. It has many practical
 * applications such as shortest path computation.
 *
 * There are two common use cases when using this filter. The first case
 * simply produces output for the purposes of visualization. In this case the
 * resulting output meshes are not watertight and cannot be smoothed
 * (so-called meshless complex of polygons or Voronoi polyhedra).  The
 * second use case produces connected, watertight surface meshes or
 * polyhedra which can be processed via downstream filters. Note that this
 * second case requires a fair amount of work to merge nearly coincident
 * points to produce the watertight surfaces. (Note: a built-in
 * topologically-based point merging process is used. Alternatively, users
 * can disable the built in point merging process, and use subsequent filters
 * like vtkStaticCleanPolyData to merge coincident points, remove degenerate
 * face primitives, etc, and otherwise process the surfaces with smoothing
 * etc. vtkStaticCleanPolyData uses a proximal geometric point merging
 * process requiring a tolerance, this can cause problems in some cases.)
 *
 * An implementation note: this filter is implemented using a parallel
 * algorithm, but produces invariant output in terms of the construction of
 * the geometric primitives (Voronoi cells, adjacency graphs, etc.) Each
 * input generating point of ptId, produces convex Voronoi hulls of hullId,
 * where ptId == hullId.  This means for debugging purposes, picking output
 * primitives with POINT_IDS enabled provides a means to select the original
 * generating hull.
 *
 * @note
 * Note that the class vtkGeneralizedSurfaceNets3D can also generate a
 * surface net. However the difference is that in vtkGeneralizedSurfaceNets3D
 * the label values are explicitly specified, only specified labels may
 * generate surfaces. The class vtkVoronoiFlower3D will generate surfaces for each
 * non-negative region id enumerated in the region ids array. In addition,
 * vtkGeneralizedSurfaceNets3D has the ability to smooth the resulting
 * surfaces, as well as perform auxiliary functions such as triangulating the
 * output polygons.
 *
 * @note
 * There are several utility classes that can be used with vtkVoronoiFlower3D to
 * massage data and improve performance.
 * ```
 * vtkFillPointCloud can add points to a set of input points P. These points
 * are labeled "outside" of P, placed in areas where no existing points exist
 * in P. Adding these outside points can markedly improve performance and
 * improve the quality of the output mesh.
 *
 * vtkLabeledImagePointSampler can be used to transform a segmented image
 * into a point cloud suitable for processing by vtkVoronoiFlower3D (and related
 * filters). By using this filter along with image crop filters it is
 * possible to "snip" out areas of interest, producing a sample of points
 * and processing them as an input point cloud.
 *
 * vtkJogglePoints can be used to improve the performance and quality of the
 * output mesh. Voronoi and Delaunay methods are known for their sensitivity
 * to numerical degeneracies (e.g., more than n+1 points cospherical to a
 * n-dimensional simplex in a n-dimensional Delaunay trianglulation). The
 * filter randomly perturbs (i.e., joggles or jitters) a point set thereby
 * removing degeneracies.
 * ```
 *
 * @warning
 * Coincident input points are discarded. The Voronoi tessellation requires
 * unique input points.
 *
 * @warning
 * This is a meshless Voronoi approach that implements an embarrassingly
 * parallel algorithm. At the core of the algorithm a locator is used to
 * determine points close to a specified position. Currently, a
 * vtkStaticPointLocator is used because it is both threaded (when
 * constructed) and supports thread-safe queries. While other locators could
 * be used in principal, they must support thread-safe operations. This is
 * done by defining and implementing an iterator that enables traversal of
 * neighborhood points surrounding each generating point.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 * For example, in recent benchmarks using a 48-thread desktop, one million
 * points can be processed in well under one second elapsed time.
 *
 * @sa
 * vtkVoronoiCore3D vtkGeneralizedSurfaceNets3D vtkShellBinIterator
 * vtkVoronoiFlower2D vtkDelaunay2D vtkDelaunay3D vtkSurfaceNets3D
 * vtkSurfaceNets2D vtkConstrainedSmoothingFilter
 * vtkWindowedSincPolyDataFilter vtkJogglePoints
 * vtkLabeledImagePointSampler vtkFillPointCloud
 */

#ifndef vtkVoronoiFlower3D_h
#define vtkVoronoiFlower3D_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkIdTypeArray.h"          // for PointsOfInterest
#include "vtkSmartPointer.h"         // For self-destructing data members
#include "vtkStaticPointLocator.h"   // For point locator
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSMESHING_EXPORT VTK_MARSHALAUTO vtkVoronoiFlower3D : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkVoronoiFlower3D* New();
  vtkTypeMacro(vtkVoronoiFlower3D, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Used to control the filter output. Note that output types VORONOI and
   * DELAUNAY produce a vtkUnstructuredGrid output. All other output types
   * produce vtkPolyData. Depending on the selected output type, use the
   * methods GetUnstructuredGridOutput() and GetPolyDataOutput() as
   * appropriate.
   */
  enum OutputTypeOptions
  {
    VORONOI = 0,         // 3D Voronoi tessellation; output cells are polyhedra (vtkPolyhedron)
    DELAUNAY = 1,        // 3D Delaunay tesselation; cells are tetraheda (vtkTetra)
    ADJACENCY_GRAPH = 2, // the graph edges connecting neighboring Voronoi hulls
    POLYGONAL_COMPLEX =
      3,          // all polygonal faces including interior. Duplicate faces are not produced.
    BOUNDARY = 4, // produce polygonal faces on the boundary of the Voronoi tessellation
    SURFACE_NET =
      5,            // faces forming the surface net (i.e., faces on the boundaries between regions)
    SPEED_TEST = 6, // no output, just compute Voronoi hulls (for performance testing)
  };

  /**
   * Specify the type of output the filter creates. Output types VORONOI and
   * DELAUNAY produce vtkUnstructuredGrid; all other types create
   * vtkPolyData. The SPEED_TEST simply computes Voronoi hulls, no
   * compositing is performed / output generated (this is for performance
   * benchmarking).
   */
  vtkSetClampMacro(OutputType, int, VORONOI, SPEED_TEST);
  vtkGetMacro(OutputType, int);
  void SetOutputTypeToVoronoi() { this->SetOutputType(VORONOI); }
  void SetOutputTypeToDelaunay() { this->SetOutputType(DELAUNAY); }
  void SetOutputTypeToAdjacencyGraph() { this->SetOutputType(ADJACENCY_GRAPH); }
  void SetOutputTypeToPolygonalComplex() { this->SetOutputType(POLYGONAL_COMPLEX); }
  void SetOutputTypeToBoundary() { this->SetOutputType(BOUNDARY); }
  void SetOutputTypeToSurfaceNet() { this->SetOutputType(SURFACE_NET); }
  void SetOutputTypeToSpeedTest() { this->SetOutputType(SPEED_TEST); }
  ///@}

  ///@{
  /**
   * Specify a padding for the bounding box of the points. A >0 padding is
   * necessary in order to create valid Voronoi hulls on the boundary of the
   * tessellation. The padding is specified as a fraction of the diagonal
   * length of the bounding box of the points. Large padding values can
   * markedly degrade performance.
   */
  vtkSetClampMacro(Padding, double, 0.0001, 0.25);
  vtkGetMacro(Padding, double);
  ///@}

  ///@{
  /**
   * Indicate whether to pass input point data through to the filter output.
   * If enabled, then the input point data is passed (for DELAUNAY output
   * type) as point data; and passed (for ADJACENCY_GRAPH output type) as
   * point data). In addition, if the number of output cells is equal to the
   * number of input points (a common use case), then input point data is
   * passed through (for VORONOI output type) as cell data. By default,
   * passing input attribute point data is enabled.
   */
  vtkSetMacro(PassPointData, vtkTypeBool);
  vtkGetMacro(PassPointData, vtkTypeBool);
  vtkBooleanMacro(PassPointData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify how to generate cell scalars for the outputs. Note that some
   * output styles (e.g., BOUNDARY) may produce multiple output primitives
   * for each Voronoi hull processed, each of these primitives will take on
   * the same cell scalar value as the generating hull (unless RANDOM is
   * specified). So what's effectively happening here is that a scalar value
   * is assigned to each generating Voronoi hull, and any surface primitives
   * (surface primitives are convex polygons) generated by the hull assume
   * that scalar value. On the other hand, the PRIM_ID option assigns
   * different cell scalar values based on the primitive number (i.e., so
   * primitive faces etc. will have different cell scalar values even if they
   * originate from the same hull). Finally, random produces up to 64 random
   * integer values for each output primitive.
   */
  enum GenerateCellScalarsStrategy
  {
    NO_CELL_SCALARS = 0, // Don't produce any cell scalars
    POINT_IDS = 1,       // Output cell scalars are the generating point id (default)
    REGION_IDS = 2,      // The region the cell primitives originated from (if region ids available)
    NUMBER_FACES = 3,    // The number of faces in the Voronoi hull
    PRIM_IDS = 4,        // The ids of the hull face primitives
    THREAD_IDS =
      5,       // Scalars are the thread id used to produce output. This may change between runs.
    RANDOM = 6 // Scalars are pseudo random numbers between [0,64).
  };
  ///@}

  ///@{
  /**
   * Indicate whether to create a cell scalar array as part of the
   * output. Options include generating no scalars; using input point ids
   * (and hence output hulls); using input region ids; using the number of
   * faces produced by each Voronoi hull; defining scalars by execution thread
   * ids; using primitive (i.e., output polygon id); or generating a random
   * scalar value [0<=s<64] for each output primitive. By default point ids
   * cell scalars are generated.
   */
  vtkSetMacro(GenerateCellScalars, int);
  vtkGetMacro(GenerateCellScalars, int);
  void SetGenerateCellScalarsToNone() { this->SetGenerateCellScalars(NO_CELL_SCALARS); }
  void SetGenerateCellScalarsToPointIds() { this->SetGenerateCellScalars(POINT_IDS); }
  void SetGenerateCellScalarsToRegionIds() { this->SetGenerateCellScalars(REGION_IDS); }
  void SetGenerateCellScalarsToNumberFaces() { this->SetGenerateCellScalars(NUMBER_FACES); }
  void SetGenerateCellScalarsToPrimIds() { this->SetGenerateCellScalars(PRIM_IDS); }
  void SetGenerateCellScalarsToThreadIds() { this->SetGenerateCellScalars(THREAD_IDS); }
  void SetGenerateCellScalarsToRandom() { this->SetGenerateCellScalars(RANDOM); }
  ///@}

  ///@{
  /**
   * Specify whether to merge (nearly) concident points in order to produce
   * compatible output meshes. Visualization of the output is possible
   * without point merging; however subsequent operations that require
   * compatible, connected meshes will not work. Note that point merging does
   * require significant time to compute. By default this is on.
   */
  vtkGetMacro(MergePoints, vtkTypeBool);
  vtkSetMacro(MergePoints, vtkTypeBool);
  vtkBooleanMacro(MergePoints, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The following method--FindHull()--can be used to locate/query the
   * Voronoi hull containing a point x (i.e., given that a Voronoi hull Vi is
   * a region of closest proximity to the generating point x).  FindHull()
   * returns the tile id/point id of a query location x.  Note that if the
   * query point x is outside of the bounds of the input point set, an id
   * value <0 is returned.
   *
   * @note This method is only valid after the filter executes.
   */
  vtkIdType FindHull(double x[3]);
  ///@}

  ///@{
  /**
   * Specify a relative tolerance to determine which spokes (i.e., small hull
   * facets) to prune. See vtkVoronoiHull for more information.
   */
  vtkSetClampMacro(PruneTolerance, double, 0.0, 0.5);
  vtkGetMacro(PruneTolerance, double);
  ///@}

  ///@{
  /**
   * Retrieve the internal locator to manually configure it, for example
   * specifying the number of points per bucket. This method is generally
   * used for debugging or testing purposes.
   */
  vtkStaticPointLocator* GetLocator() { return this->Locator; }
  ///@}

  ///@{
  /**
   * Enable the validation of the Voronoi tesselation (which also affects the
   * Delaunay triangulation and other output types if requested). Enabling
   * validation increases computation time. By default, validation is
   * off. Validation is a necessary condition that must be satisfied to
   * produce a valid output tessellation.
   */
  vtkSetMacro(Validate, vtkTypeBool);
  vtkGetMacro(Validate, vtkTypeBool);
  vtkBooleanMacro(Validate, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the number of input generating points in a batch, where a batch
   * defines a contiguous subset of the input points operated on during
   * threaded execution. Generally this is only used for debugging or
   * performance studies (since batch size affects the thread workload).
   *
   * Default is 1000.
   */
  vtkSetClampMacro(BatchSize, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(BatchSize, unsigned int);
  ///@}

  ///@{
  /**
   * Specify whether to cap the surface net along the domain boundary. This
   * only applies if the OutputType==SURFACE_NET.
   */
  vtkGetMacro(BoundaryCapping, vtkTypeBool);
  vtkSetMacro(BoundaryCapping, vtkTypeBool);
  vtkBooleanMacro(BoundaryCapping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * These methods are for debugging or instructional purposes. When the
   * point of interest is specified (i.e., set to a non-negative number) then
   * the algorithm will process this single point (whose id is the
   * PointOfInterest). When PointsOfInterest is specified through a supplied
   * vtkIdTypeArray (this is in addition to the PointOfInterest), then only
   * those hulls in the set (PointOfInterest + PointsOfInterestArray) will be
   * produced. The maximum number of clips (the MaximumNumberOfHullClips) can
   * be specified. If MaximumNumberOfHullClips=0, then the initial tile
   * (single point within the bounding box) is produced; if =1 then the split
   * with the closest point is produced; and so on. By default the
   * PointOfInterest is set to (-1), and the number of clips is unlimited
   * (i.e., MaximumNumberOfHullClips=VTK_ID_MAX and therefore automatically
   * limited by the algorithm).
   */
  vtkSetClampMacro(PointOfInterest, vtkIdType, -1, VTK_ID_MAX);
  vtkGetMacro(PointOfInterest, vtkIdType);
  vtkSetObjectMacro(PointsOfInterest, vtkIdTypeArray);
  vtkGetObjectMacro(PointsOfInterest, vtkIdTypeArray);
  vtkSetClampMacro(MaximumNumberOfHullClips, vtkIdType, 1, VTK_ID_MAX);
  vtkGetMacro(MaximumNumberOfHullClips, vtkIdType);
  ///@}

  ///@{
  /**
   * Used internally to generate point scalars for the output. When a
   * point of interest is defined, then additional point scalars which
   * are the radii of the Voronoi flower are produced, which is useful
   * for debugging or instructional purposes.
   */
  enum GeneratePointScalarsStrategy
  {
    NO_POINT_SCALARS = 0, // Don't produce any point scalars (default)
    FLOWER_RADII = 1      // Output point scalars are the Voronoi flower radii
  };
  vtkGetMacro(GeneratePointScalars, int);
  ///@}

  /**
   *  Return the maximum number of points in any Voronoi hull.
   *
   * @note This method is only valid after the filter executes.
   */
  int GetMaximumNumberOfPoints() { return this->MaximumNumberOfPoints; }

  /**
   *  Return the maximum number of faces in any Voronoi hull.
   *
   * @note This method is only valid after the filter executes.
   */
  int GetMaximumNumberOfFaces() { return this->MaximumNumberOfFaces; }

  /**
   *  Return the number of threads actually used during execution.
   *
   * @note This method is only valid after the filter executes.
   */
  int GetNumberOfThreads() { return this->NumberOfThreads; }

  /**
   *  Return the number of prunes performed during execution.
   *
   * @note This method is only valid after the filter executes.
   */
  int GetNumberOfPrunes() { return this->NumberOfPrunes; }

  /**
   * Get the MTime of this object also considering the locator.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Method used to update this filter's execution parameters after the
   * internal, templated instance of vtkVoronoiCore3D completes execution.
   */
  template <typename T>
  void UpdateExecutionInformation(T* voro);

protected:
  vtkVoronoiFlower3D();
  ~vtkVoronoiFlower3D() override = default;

  // Satisfy pipeline-related API
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkVoronoiFlower3D(const vtkVoronoiFlower3D&) = delete;
  void operator=(const vtkVoronoiFlower3D&) = delete;

  int OutputType;            // specification of the filter output
  double Padding;            // amount to pad out input points bounding box
  vtkTypeBool Validate;      // Choose to validate and repair output
  vtkTypeBool PassPointData; // indicate whether to pass input point data to output
  int GeneratePointScalars;  // indicate whether point scalars are to be produced
  int GenerateCellScalars;   // indicate whether cell scalars are to be produced
  vtkTypeBool MergePoints;   // merge near coincident points or not
  vtkIdType PointOfInterest; // specify a single input point to process
  vtkSmartPointer<vtkIdTypeArray> PointsOfInterest; // list of points of interest
  vtkIdType MaximumNumberOfHullClips;               // limit the number of hull clips
  vtkSmartPointer<vtkStaticPointLocator> Locator;   // locator for finding proximal points
  double PruneTolerance;                            // the prune spokes tolerance
  unsigned int BatchSize;                           // process data in batches of specified size
  vtkTypeBool BoundaryCapping; // cap the domain boundary if OutputType is SURFACE_NET

  /**
   * Execution parameters. Updated after the internal vtkVoronoiCore3D executes.
   */
  int NumberOfThreads;       // report on the number of threads used during processing
  int MaximumNumberOfPoints; // maximum number of points found in any hull
  int MaximumNumberOfFaces;  // maximum number of faces found in any hull
  int NumberOfPrunes;        // If spoke pruning is enabled, report number of pruning operations
};

//------------------------------------------------------------------------------
template <typename T>
void vtkVoronoiFlower3D::UpdateExecutionInformation(T* voro)
{
  this->NumberOfThreads = voro->GetNumberOfThreads();
  this->MaximumNumberOfPoints = voro->GetMaximumNumberOfPoints();
  this->MaximumNumberOfFaces = voro->GetMaximumNumberOfFaces();
  this->NumberOfPrunes = voro->GetNumberOfPrunes();
}

VTK_ABI_NAMESPACE_END
#endif
