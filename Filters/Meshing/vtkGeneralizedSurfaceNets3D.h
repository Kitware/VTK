// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeneralizedSurfaceNets3D
 * @brief   create a surface net from an unorganized set of segmented (i.e., labeled) points
 *
 * vtkGeneralizedSurfaceNets3D is a filter that constructs a surface net from
 * a labeled / segmented list of input points. The points are presumed to lie
 * within 3D-space. These points may be represented by any dataset of type
 * vtkPointSet and subclasses. The output of the filter is a complex of
 * convex polygons represented by a vtkPolyData. Additionally the output
 * contains cell data consisting of a 2-component tuples which record the
 * regions on either side of the polygonal faces composing the surface
 * net. The algorithm uses a novel 3D Voronoi tessellation algorithm, and
 * extracts surface net faces between Voronoi hulls which lie in specified
 * separate regions.
 *
 * Besides the list of input points, the filter requires an input region ids
 * array which labels the points as belonging to different regions. This
 * point data region ids array should be of integer type (vtkIntArray), with
 * region id values>=0 (although a region id < 0 indicates that the
 * associated point is "outside"; consequently the point will not produce
 * output but will affect neighboring points in terms of producing boundary
 * faces). In addition, the filter requires the specification of the
 * segmented regions to extract by specifying one or more labels. (If labels
 * are not specified, than all non-negative region ids are assumed to be a
 * label. Automatically generating labels can be slow, it is preferred that
 * labels are specified.)
 *
 * The surface net algorithm can also (optionally) smooth the output
 * polygonal surface. To be faithful to the original algorithm, a
 * vtkConstrainedSmoothingFilter is used; however other smoothing algorithms
 * such as vtkWindowedSincPolyDataFilter may be used (by disabling smoothing,
 * enabling point merging, and passing the output of the filter to subsequent
 * smoothing filter).
 *
 * Note that the class vtkVoronoi3D can also generate a surface net. However
 * the difference is that in vtkGeneralizedSurfaceNets3D the label values
 * are explicitly specified, only specified labels may generate surfaces. The
 * class vtkVoronoi3D will generate surfaces for each non-negative region id
 * enumerated in the region ids array. In addition, vtkGeneralizedSurfaceNets3D
 * has the ability to smooth the resulting surfaces, as well as perform
 * auxiliary functions such as triangulating the output polygons.
 *
 * There are two common use cases when using this filter. The first case
 * simply produces output surface net faces for the purposes of
 * visualization. In this case the resulting surfaces are not watertight and
 * cannot be smoothed (so-called meshless complex of polygons). (Note that
 * non-smoothed surface nets tend to be choppy depending on the input point
 * cloud resolution.) The second case produces connected, watertight surface
 * meshes which can be smoothed. Note that this second case requires a fair
 * amount of work to merge nearly coincident points to produce the watertight
 * surfaces. (Note: a built-in topologically-based point merging process is
 * used. Users can disable the built in point merging process, and use
 * subsequent filters like vtkStaticCleanPolyData to merge coincident points,
 * remove degenerate face primitives, etc, and otherwise process the surfaces
 * with smoothing etc. vtkStaticCleanPolyData uses a proximal geometric point
 * merging process requiring a tolerance, this can cause problems in some
 * cases.)
 *
 * Finally, another important option to the filter is that capping surfaces
 * corresponding to the domain boundary can be generated. In some cases it is
 * useful to provide the boundary as context to the surface net
 * contours. (The domain boundary is determined from Voronoi edges that
 * connect to the domain edges, or connect to points with region ids < 0).
 *
 * See the following reference for more details about surface nets:
 * W. Schroeder, S. Tsalikis, M. Halle, S. Frisken. A High-Performance
 * SurfaceNets Discrete Isocontouring Algorithm. arXiv:2401.14906. 2024.
 * (http://arxiv.org/abs/2401.14906).
 *
 * The Surface Nets algorithm was first proposed by Sarah Frisken.  Two
 * important papers include the description of surface nets for binary
 * objects (i.e., extracting just one segmented object from a volume) and
 * multi-label (multiple object extraction).
 *
 * S. Frisken (Gibson), “Constrained Elastic SurfaceNets: Generating Smooth
 * Surfaces from Binary Segmented Data”, Proc. MICCAI, 1998, pp. 888-898.
 *
 * S. Frisken, “SurfaceNets for Multi-Label Segmentations with Preservation
 * of Sharp Boundaries”, J. Computer Graphics Techniques, 2022.
 *
 * These techniques referenced above are specialized to input 3D labeled (or
 * segmented) volumes. This filter implementes a generalized version of
 * surface nets for labeled, unorganized point clouds.
 *
 * @note
 * There are several utility classes that can be used with
 * vtkGeneralizedSurfaceNets3D (and related classes such as vtkVoronoi3D) to
 * prepare data and improve performance.
 * ```
 * vtkFillPointCloud can add points to a set of input points P. These points
 * are labeled "outside" of P, placed in areas where no existing points exist
 * in P. Adding these outside points can markedly improve performance and
 * improve the quality of the output mesh.
 *
 * vtkLabeledImagePointSampler can be used to transform a segmented image
 * into a point cloud suitable for processing by vtkVoronoi3D (and related
 * filters). By using this filter along with image crop filters it is
 * possible to "snip" out areas of interest, producing a sample of points
 * and processing them as an input point cloud.
 *
 * vtkJitterPoints can be used to improve the performance and quality of
 * the output mesh. Voronoi and Delaunay methods are known for their
 * sensitivity to numerical degeneracies (e.g., more than n+1 points
 * cospherical to a n-dimensional simplex in a n-dimensional Delaunay
 * trianglulation). The filter randomly perturbs (i.e., jitters) a point
 * set thereby removing degeneracies.
 * ```
 *
 * @warning
 * Coincident input points are discarded. The Voronoi tessellation requires
 * unique input points.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkSurfaceNets2D vtkSurfaceNets3D vtkVoronoi3D vtkVoronoiCore3D
 * vtkShellBinIterator vtkConstrainedSmoothingFilter
 * vtkWindowedSincPolyDataFilter
 */

#ifndef vtkGeneralizedSurfaceNets3D_h
#define vtkGeneralizedSurfaceNets3D_h

#include "vtkConstrainedSmoothingFilter.h" // Perform surface smoothing
#include "vtkContourValues.h"              // Manage countour values
#include "vtkFiltersMeshingModule.h"       // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkStaticPointLocator.h" // For point locator
#include "vtkWrappingHints.h"      // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSMESHING_EXPORT VTK_MARSHALAUTO vtkGeneralizedSurfaceNets3D
  : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkGeneralizedSurfaceNets3D* New();
  vtkTypeMacro(vtkGeneralizedSurfaceNets3D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  //---------------The following are methods used to set/get and generate
  //---------------contour labels. Contour labels are required to be
  //---------------specified by this filter.
  ///@{
  /**
   * Set a particular label value at label number i. The index i ranges
   * between (0 <= i < NumberOfLabels). (Note: while labels values are
   * expressed as doubles, the underlying scalar data may be a different
   * type. During execution the label values are cast to the type of the
   * scalar data.)  Note the use of "Value" and "Label" when specifying
   * regions to extract. The use of "Value" is consistent with other VTK
   * continuous-scalar field isocontouring algorithms; however the term
   * "Label" is more consistent with label maps.  Warning: make sure that the
   * label value >= 0 as any label < 0 is considered a background, i.e.,
   * outside, label.
   */
  void SetValue(int i, double value) { this->Labels->SetValue(i, value); }
  void SetLabel(int i, double value) { this->Labels->SetValue(i, value); }
  ///@}

  ///@{
  /**
   * Get the ith label value.
   */
  double GetValue(int i) { return this->Labels->GetValue(i); }
  double GetLabel(int i) { return this->Labels->GetValue(i); }
  ///@}

  ///@{
  /**
   * Get a pointer to an array of labels. There will be
   * GetNumberOfLabels() values in the list.
   */
  double* GetValues() { return this->Labels->GetValues(); }
  double* GetLabels() { return this->Labels->GetValues(); }
  ///@}

  ///@{
  /**
   * Fill a supplied list with label values. There will be
   * GetNumberOfLabels() values in the list. Make sure you allocate enough
   * memory to hold the list.
   */
  void GetValues(double* contourValues) { this->Labels->GetValues(contourValues); }
  void GetLabels(double* contourValues) { this->Labels->GetValues(contourValues); }
  ///@}

  ///@{
  /**
   * Set the number of labels to place into the list. You only really need to
   * use this method to reduce list size. The method SetValue() will
   * automatically increase list size as needed. Note that for consistency
   * with other isocountoring-related algorithms, some methods use
   * "Labels" and "Contours" interchangeably.
   */
  void SetNumberOfLabels(int number) { this->Labels->SetNumberOfContours(number); }
  void SetNumberOfContours(int number) { this->Labels->SetNumberOfContours(number); }
  ///@}

  ///@{
  /**
   * Get the number of labels in the list of label values.
   */
  vtkIdType GetNumberOfLabels() { return this->Labels->GetNumberOfContours(); }
  vtkIdType GetNumberOfContours() { return this->Labels->GetNumberOfContours(); }
  ///@}

  ///@{
  /**
   * Generate numLabels equally spaced labels between the specified
   * range. The labels will include the min/max range values.
   */
  void GenerateLabels(int numLabels, double range[2])
  {
    this->Labels->GenerateValues(numLabels, range);
  }
  void GenerateValues(int numContours, double range[2])
  {
    this->Labels->GenerateValues(numContours, range);
  }
  void GenerateLabels(int numLabels, double rangeStart, double rangeEnd)
  {
    this->Labels->GenerateValues(numLabels, rangeStart, rangeEnd);
  }
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->Labels->GenerateValues(numContours, rangeStart, rangeEnd);
  }
  ///@}

  ///@{
  /**
   * This value specifies the label value to use when indicating that a region
   * is outside. That is, the output 2-tuple cell data array indicates which
   * segmented region is on either side of it. To indicate that one side is
   * on the boundary, the BackgroundLabel value is used. By default the
   * background label is (-100).
   */
  vtkSetMacro(BackgroundLabel, int);
  vtkGetMacro(BackgroundLabel, int);
  ///@}

  //---------------Done defining label-related methods.
  ///@{
  /**
   * Specify whether to cap the surface net along the boundary.  By default
   * this is off.
   */
  vtkGetMacro(BoundaryCapping, vtkTypeBool);
  vtkSetMacro(BoundaryCapping, vtkTypeBool);
  vtkBooleanMacro(BoundaryCapping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify whether to merge nearly concident points in order to produce
   * watertight output surfaces. Enabling merging is necessary to perform
   * smoothing. However it does require significant time to compute. By
   * default this is on.
   */
  vtkGetMacro(MergePoints, vtkTypeBool);
  vtkSetMacro(MergePoints, vtkTypeBool);
  vtkBooleanMacro(MergePoints, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether smoothing should be enabled. By default, after the
   * surface net is extracted, smoothing occurs using the built-in internal
   * smoother, and MergePoints is enabled.  To disable smoothing, simply invoke
   * SmoothingOff(). (Note: disabling smoothing can be useful to visualize
   * the initial surface net, or a different smoother is to be used later
   * in the downstream visualization pipeline.)
   */
  vtkSetMacro(Smoothing, vtkTypeBool);
  vtkGetMacro(Smoothing, vtkTypeBool);
  vtkBooleanMacro(Smoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Convenience methods that delegate to the internal smoothing filter
   * follow below. See the documentation for vtkConstrainedSmoothingFilter
   * for more information.
   */
  void SetNumberOfIterations(int n) { this->Smoother->SetNumberOfIterations(n); }
  int GetNumberOfIterations() { return this->Smoother->GetNumberOfIterations(); }
  void SetRelaxationFactor(double f) { this->Smoother->SetRelaxationFactor(f); }
  double GetRelaxationFactor() { return this->Smoother->GetRelaxationFactor(); }
  void SetConstraintDistance(double d) { this->Smoother->SetConstraintDistance(d); }
  double GetConstraintDistance() { return this->Smoother->GetConstraintDistance(); }
  ///@}

  ///@{
  /**
   * Indicate whether this filter should produce smoothing stencils. This
   * only applies if Smoothing is enabled. By default (On), this filter will
   * compute the stencils; otherwise (Off) an internal smoothing filter of
   * type vtkConstrainedSmoothingFilter is used, and this internal filter
   * generates the stencils used to perform smoothing iterations.  Note that
   * generating the smoothing stencils uses an evaluation of the topological
   * coordinates to distinguish between fixed, edge, face, and unconstrained
   * connections; whereas the internal vtkConstrainedSmoothingFilter simply
   * joins all edge connected points and does not distinguish between
   * different types of connections. Generating smoothing stencils (On) is
   * typically faster compared to the internal vtkConstrainedSmoothingFilter
   * or using other smoothing filters downstream in the visualization
   * pipeline.
   */
  vtkSetMacro(GenerateSmoothingStencils, vtkTypeBool);
  vtkGetMacro(GenerateSmoothingStencils, vtkTypeBool);
  vtkBooleanMacro(GenerateSmoothingStencils, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If GenerateSmoothingStencils is on, this provides some control over
   * each point's stencil creation. Four values can be set (0/1) corresponding
   * to the type of constraint as determined by the topological coordinates.
   * The first value [0] controls fixed points (topological coordinate spans
   * four regions, e.g., a corner); edge points [1] (topological coordinate
   * spans three regions along sharp edge); face points [2] (topological
   * coordinates span two regions along face); and unconstrained [3] (topological
   * coordinates span a single region). By default, the smoothing constraints are
   * all disabled ==[0,0,0,0]. Other common options are all unconstrained
   * ==[1,1,1,1]; and edges unconstrained ==[1,0,1,1].
   */
  vtkSetVector4Macro(SmoothingConstraints, unsigned char);
  vtkGetVector4Macro(SmoothingConstraints, unsigned char);
  void AllSmoothingConstraintsOn() { this->SetSmoothingConstraints(1, 1, 1, 1); }
  void AllSmoothingConstraintsOff() { this->SetSmoothingConstraints(0, 0, 0, 0); }
  void EdgeSmoothingConstraintOff() { this->SetSmoothingConstraints(1, 0, 1, 1); }
  ///@}

  ///@{
  /**
   * Get the internal instance of vtkConstrainedSmoothingFilter used to smooth
   * the extracted surface net. To control smoothing, access this instance and
   * specify its parameters such as number of smoothing iterations and
   * constraint distance. If you wish to disable smoothing, set
   * SmoothingOff(). Note also that by default vtkConstrainedSmoothingFilter
   * will compute smoothing stencils; however if GenerateSmoothingStencils
   * is on, then this filter will provide the stencils.
   */
  vtkGetSmartPointerMacro(Smoother, vtkConstrainedSmoothingFilter);
  ///@}

  /**
   * This enum is used to control the type of the output polygonal mesh.
   * vtkGeneralizedSurfaceNets3D creates convex polygons; but for smoothing
   * and subsequent processing, triangles are preferred (triangles are
   * default).
   */
  enum MeshType
  {
    MESH_TYPE_DEFAULT = 0,
    MESH_TYPE_TRIANGLES,
    MESH_TYPE_POLYGONS
  };

  ///@{
  /**
   * Control the type of output mesh. By default, if smoothing is off, the
   * output mesh is a polygonal mesh consisting of convex polygons.
   * However, if smoothing is enabled, then the output mesh type is
   * a polygonal mesh consisting of triangles. It is possible to force the
   * output mesh type to be of a certain type (triangles, or convex polygons)
   * regardless whether smoothing is enabled or not. Note that if an output
   * mesh is forced to be polygons, and smoothing is enabled, the resulting
   * smoothed polygons may not be planar.
   */
  vtkSetClampMacro(OutputMeshType, int, MESH_TYPE_DEFAULT, MESH_TYPE_POLYGONS);
  vtkGetMacro(OutputMeshType, int);
  void SetOutputMeshTypeToDefault() { this->SetOutputMeshType(MESH_TYPE_DEFAULT); }
  void SetOutputMeshTypeToTriangles() { this->SetOutputMeshType(MESH_TYPE_TRIANGLES); }
  void SetOutputMeshTypeToPolygons() { this->SetOutputMeshType(MESH_TYPE_POLYGONS); }
  ///@}

  ///@{
  /**
   * Specify a padding for the bounding box of the input points. A >0 padding
   * is necessary in order to create valid Voronoi hulls on the boundary of
   * the tessellation. The padding is specified as a fraction of the diagonal
   * length of the bounding box of the points.
   */
  vtkSetClampMacro(Padding, double, 0.001, 0.25);
  vtkGetMacro(Padding, double);
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
   * Enable the validation of the Voronoi tesselation. Enabling validation
   * increases computation time. By default, validation is off. Validation is
   * a necessary condition that must be satisfied to produce a valid output
   * tessellation.
   */
  vtkSetMacro(Validate, vtkTypeBool);
  vtkGetMacro(Validate, vtkTypeBool);
  vtkBooleanMacro(Validate, vtkTypeBool);
  ///@}

  ///@{
  /**
   * These methods are for debugging or instructional purposes. When the
   * point of interest is specified (i.e., set to a non-negative number) then
   * the algorithm will process this single point (whose id is the
   * PointOfInterest). When PointsOfInterest is specified through a supplied
   * vtkIdTypeArray (this is in addition to the PointOfInterest), then only
   * those hulls in the PointOfInterest + PointsOfInterestArray will be
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

  /**
   *  Return the number of threads actually used during execution. This is
   *  valid only after algorithm execution.
   */
  int GetNumberOfThreadsUsed() { return this->NumberOfThreadsUsed; }

  /**
   *  Return the number of hull prunes performed during execution. This is
   *  valid only after algorithm execution.
   */
  int GetNumberOfPrunes() { return this->NumberOfPrunes; }

  /**
   * The modified time is also a function of the built in locator, smoothing
   * filter, and label values.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkGeneralizedSurfaceNets3D();
  ~vtkGeneralizedSurfaceNets3D() override = default;

  // Support the contouring operation by defining labels.
  vtkSmartPointer<vtkContourValues> Labels;
  int BackgroundLabel;

  // Algorithm control
  vtkTypeBool BoundaryCapping; // produce boundary surfaces or not
  vtkTypeBool MergePoints;     // merge near coincident points or not
  vtkTypeBool Smoothing;       // enable built-in smoothing process

  // Internal classes Related to point location and smoothing control
  vtkSmartPointer<vtkConstrainedSmoothingFilter> Smoother; // built in smoother
  vtkSmartPointer<vtkStaticPointLocator> Locator;          // locator for finding proximal points

  // Control the type of output mesh. Triangles by default.
  int OutputMeshType;

  // Related to internal Voronoi methods
  double Padding;                        // amount to pad out input points bounding box
  vtkTypeBool Validate;                  // Choose to validate and repair output
  vtkTypeBool GenerateSmoothingStencils; // Produce smoothing stencils
  unsigned char SmoothingConstraints[4]; // Specify which smoothing constraints are active
  vtkIdType PointOfInterest;             // specify a single input point to process
  vtkSmartPointer<vtkIdTypeArray> PointsOfInterest; // list of points of interest
  vtkIdType MaximumNumberOfHullClips;               // limit the number of hull clips
  unsigned int BatchSize;                           // process data in batches of specified size
  int NumberOfThreadsUsed; // report on the number of threads used during processing
  int NumberOfPrunes;      // If spoke pruning is enabled, report number of pruning operations

  // Satisfy pipeline-related API
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGeneralizedSurfaceNets3D(const vtkGeneralizedSurfaceNets3D&) = delete;
  void operator=(const vtkGeneralizedSurfaceNets3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
