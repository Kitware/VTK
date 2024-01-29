// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSurfaceNets2D
 * @brief   generate smoothed constours from segmented 2D image data (i.e., "label maps")
 *
 * vtkSurfaceNets2D creates boundary/isocontour lines from a label map (e.g.,
 * a segmented image) using a threaded, 2D version of the multiple
 * regions/labels surface nets algorithm. The input is a 2D image where each
 * pixel is labeled (integer labels are preferred to real values), and the
 * output data is polygonal data separating labeled regions via line
 * segments. (Note that on output each region [corresponding to a different
 * segmented object] will share points/edges on a common boundary, i.e., two
 * objects next to each other will share the boundary that separates them.)
 *
 * While this filter is similar to a contouring operation, classic contouring
 * methods assume a continuous scalar field. In comparison, label maps are
 * not continuous in scalar function value, meaning that usual data
 * interpolation (e.g., along edges) is not possible. Instead, when the edge
 * endpoint pixels are labeled in differing regions, the edge is split and
 * transected by a line segment that connects the center points of the squares
 * on either side of the edge. Later, using a energy minimization smoothing
 * process, these split edges will be adjusted to produce a smoother
 * result. (Constraints on smoothing displacements may be specified to
 * prevent excessive shrinkage and/or object distortion.)
 *
 * The smoothing process is controlled by setting a convergence measure, the
 * number of smoothing iterations, the step size, and the allowed
 * (constraint) distance that points may move.  These can be adjusted to
 * provide the desired result. This class provides a method to access an
 * internal instance of vtkConstrainedSmoothingFilter, through which these
 * smoothing parameters may be specified, and which actually performs the
 * smoothing operation. (Note: it is possible to skip the smoothing process
 * altogether by disabling smoothing (e.g., invoking SmoothingOff()) or
 * setting the number of smoothing iterations to zero. This can
 * be useful when using a different smoothing filter like
 * vtkWindowedSincPolyDataFilter; or if an unsmoothed, aliased output is
 * desired. The reason the smoothing is built in to this filter is to remain
 * faithful to the published literature describing the surface nets
 * algorithm.)
 *
 * See the following reference for more details about the implementation:
 * W. Schroeder, S. Tsalikis, M. Halle, S. Frisken. A High-Performance
 * SurfaceNets Discrete Isocontouring Algorithm. arXiv:2401.14906. 2024.
 * (http://arxiv.org/abs/2401.14906).
 *
 * The SurfaceNets algorithm was first proposed by Sarah Frisken.  Two
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
 * Note that one nice feature of this filter is that algorithm execution
 * occurs only once no matter the number of object labels / contour
 * values. In many contouring-like algorithms, each separate contour value
 * requires an additional algorithm execution with a new contour value. So in
 * this filter large numbers of contour values do not significantly affect
 * overall speed. The user can specify which objects (i.e., labels) are to be
 * output to the filter. (Unspecified labels are treated as background and
 * not output.)
 *
 * The filter can optionally output a two-component, cell data array
 * indicating the labels/regions on either side of the line segments
 * composing the output vtkPolyData. This can be used for advanced operations
 * like extracting shared/contacting boundaries between two objects. The name
 * of this cell data array is "BoundaryLabels".
 *
 * Implementation note: For performance reasons, this filter is internally
 * implemented quite differently than described in the literature.  The main
 * difference is that concepts from the Flying Edges parallel isocontouring
 * algorithm are used. Namely, parallel, edge-by-edge processing is used to
 * define cell cases, generate smoothing stencils, and produce points and
 * output lines. The smoothing process is also threaded using a
 * double-buffering approach.
 *
 * @warning
 * This filter is specialized to 2D images.
 *
 * @warning
 * Subtle differences in the output may result when the number of objects /
 * labels changes. This is because the smoothing operation operates on all of
 * the boundaries simultaneously. If the boundaries change due to a
 * difference in the number of regions / labels, then the smoothing operation
 * can produce different results.
 *
 * @warning
 * The filters vtkDiscreteMarchingCubes, vtkDiscreteFlyingEdges2D,
 * vtkDiscreteFlyingEdges3D, and vtkDiscreteFlyingEdgesClipper2D also
 * perform isocontour extraction. However these filters produce output
 * that may not share common boundary cells, and may produce "gaps"
 * between segmented regions. For example, vtkDiscreteMarchingCubes will
 * share points between adjacent regions, but not triangle cells (which
 * will be coincident). Also, no center point is inserted into voxels,
 * meaning that intermittent gaps may form between regions.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * See also vtkPackLabels which is a utility class for renumbering the labels
 * found in the input segmentation mask to contiguous forms of smaller type.
 *
 * @sa
 * vtkSurfaceNets3D vtkDiscreteFlyingEdges2D vtkDiscreteFlyingEdgesClipper2D
 * vtkConstrainedSmoothingFilter vtkFlyingEdges2D vtkFlyingEdges3D
 * vtkWindowedSincPolyDataFilter vtkPackLabels
 */

#ifndef vtkSurfaceNets2D_h
#define vtkSurfaceNets2D_h

#include "vtkConstrainedSmoothingFilter.h" // Perform mesh smoothing
#include "vtkContourValues.h"              // Needed for direct access to ContourValues
#include "vtkFiltersCoreModule.h"          // For export macro
#include "vtkPolyData.h"                   // To support data caching
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkSurfaceNets2D : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static vtkSurfaceNets2D* New();
  vtkTypeMacro(vtkSurfaceNets2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * The modified time is also a function of the label values and
   * the smoothing filter.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set a particular label value at label number i. The index i ranges
   * between 0 <= i <NumberOfLabels. (Note: while labels values are expressed
   * as doubles, the underlying scalar data may be a different type. During
   * execution the label values are cast to the type of the scalar data.)
   * Note the use of "Value" and "Label" when specifying regions to
   * extract. The use of "Value" is continuous with other VTK
   * continuous-scalar field isocontouring algorithms; however the term
   * "Label" is more consistent with label maps.
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
   * Enable/disable an option to generate cell scalars in the output. The
   * cell scalars are a two-tuple that indicates which labels (i.e.,
   * segmented regions) are on either side of each (line) cell (l0,l1) with
   * l0<l1.  If an object is one-sided (meaning the background is on one side
   * of the line) then l1=(OutsideLabel). The name of the output cell scalars is
   * "BoundaryLabels". By default this flag is on.
   */
  vtkSetMacro(ComputeScalars, bool);
  vtkGetMacro(ComputeScalars, bool);
  vtkBooleanMacro(ComputeScalars, bool);
  ///@}

  ///@{
  /**
   * If computing cell scalars, specify the label to use when referencing a
   * labeled region outside any of the specified regions (i.e., when
   * referencing the background region). By default this value is zero. Be
   * very careful of the value being used here, it should not overlap an
   * extracted label value, and because it is the same type as the input
   * image scalars, make sure the value can be properly represented (i.e., if
   * the input scalars are an unsigned type, then BackgroundLabel should not
   * be negative).
   */
  vtkSetMacro(BackgroundLabel, double);
  vtkGetMacro(BackgroundLabel, double);
  ///@}

  ///@{
  /**
   * Set/get which component of a input multi-component scalar array to
   * contour with; defaults to component 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  ///@}

  ///@{
  /**
   * Indicate whether smoothing should be enabled. By default, after the
   * isoline is extracted, smoothing occurs using the built-in smoother.
   * To disable smoothing, invoke SmoothingOff().
   */
  vtkSetMacro(Smoothing, bool);
  vtkGetMacro(Smoothing, bool);
  vtkBooleanMacro(Smoothing, bool);
  ///@}

  ///@{
  /**
   * Get the instance of vtkConstrainedSmoothingFilter used to smooth the
   * extracted surface net. To control smoothing, access this instance and
   * specify its parameters such as number of smoothing iterations and
   * constraint distance. If you wish to disable smoothing, set
   * SmoothingOff().
   */
  vtkGetSmartPointerMacro(Smoother, vtkConstrainedSmoothingFilter);
  ///@}

  ///@{
  /**
   * Enable caching of intermediate data. A common workflow using this filter
   * requires extracting object boundaries, and then repeatedly rerunning the
   * smoothing process with different parameters. To improve performance by
   * avoiding repeated extraction of the boundary, the filter can cache
   * intermediate data prior to the smoothing process. In this way, the
   * boundary is only extracted once, and as long as only the internal
   * constrained smoothing filter is modified, then boundary extraction will
   * not be reexecuted. By default this is enabled.
   */
  vtkSetMacro(DataCaching, bool);
  vtkGetMacro(DataCaching, bool);
  vtkBooleanMacro(DataCaching, bool);
  ///@}

protected:
  vtkSurfaceNets2D();
  ~vtkSurfaceNets2D() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkSmartPointer<vtkContourValues> Labels;
  bool ComputeScalars;
  double BackgroundLabel;
  int ArrayComponent;

  bool Smoothing;
  vtkSmartPointer<vtkConstrainedSmoothingFilter> Smoother;

  // Support data caching of the extracted surface nets. This is used to
  // avoid repeated surface extraction when only smoothing filter
  // parameters are modified.
  bool DataCaching;
  vtkSmartPointer<vtkPolyData> GeometryCache;
  vtkSmartPointer<vtkCellArray> StencilsCache;
  vtkTimeStamp SmoothingTime;
  bool IsCacheEmpty();
  void CacheData(vtkPolyData* pd, vtkCellArray* ca);

private:
  vtkSurfaceNets2D(const vtkSurfaceNets2D&) = delete;
  void operator=(const vtkSurfaceNets2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
