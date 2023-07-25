// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContour3DLinearGrid
 * @brief   fast generation of isosurface from 3D linear cells
 *
 * vtkContour3DLinearGrid is a specialized filter that generates isocontours
 * from an input vtkUnstructuredGrid consisting of 3D linear cells:
 * tetrahedra, hexahedra, voxels, pyramids, and/or wedges. (The cells are
 * linear in the sense that each cell edge is a straight line.) The filter is
 * designed for high-speed, specialized operation. All other cell types are
 * skipped and produce no output. (Note: the filter will also process
 * input vtkCompositeDataSets containing vtkUnstructuredGrids.)
 *
 * To use this filter you must specify an input unstructured grid or
 * vtkCompositeDataSet, and one or more contour values.  You can either use
 * the method SetValue() to specify each contour value, or use
 * GenerateValues() to generate a series of evenly spaced contours.
 *
 * The filter performance varies depending on optional output
 * information. Basically if point merging is required (when PointMerging,
 * InterpolateAttributes, and/or ComputeNormals is enabled), a sorting
 * process is required to eliminate duplicate output points in the
 * isosurface. Otherwise when point merging is not required, a fast path
 * process produces independent triangles representing the isosurface. In
 * many situations the results of the fast path are quite good and do not
 * require additional processing.
 *
 * Note that another performance option exists, using a vtkScalarTree, which
 * is an object that accelerates isosurface extraction, at the initial cost
 * of building the scalar tree. (This feature is useful for exploratory
 * isosurface extraction when the isovalue is frequently changed.) In some
 * cases this can improve performance, however this algorithm is so highly
 * tuned that random memory jumps (due to random access of cells provided by
 * the scalar tree) can actually negatively impact performance, especially if
 * the input dataset type consists of homogeneous cell types.
 *
 * @warning
 * When the input is of type vtkCompositeDataSet the filter will process the
 * unstructured grid(s) contained in the composite data set. As a result the
 * output of this filter is then a composite data set (same as input) containing
 * multiple vtkPolyData. When a vtkUnstructuredGrid is provided as input the
 * output is a single vtkPolyData.
 *
 * @warning
 * The fast path simply produces output points and triangles (the fast path
 * executes when MergePoints if off; InterpolateAttributes is off; and
 * ComputeNormals is off). Since the fast path does not merge points, it
 * produces many more output points, typically on the order of 5-6x more than
 * when MergePoints is enabled. Adding in the other options point merging,
 * field interpolation, and normal generation results in additional
 * performance impacts. By default the fast path is enabled.
 *
 * @warning
 * When a vtkCompositeDataSet is provided as input, and UseScalarTree is
 * enabled and a ScalarTree specified, then the specified scalar tree is
 * cloned to create new ones for each dataset in the composite
 * dataset. Otherwise (i.e., when vtkUnstructuredGrid input) the specified
 * scalar tree is directly used (no cloning required).
 *
 * @warning
 * Internal to this filter, a caching iterator is used to traverse the cells
 * that compose the vtkUnstructuredGrid. Maximum performance is obtained if
 * the cells are all of one type (i.e., input grid of homogeneous cell
 * types); repeated switching from different types may have detrimental
 * effects on performance.
 *
 * @warning
 * For unstructured data, gradients are not computed. Normals are computed if
 * requested; they are "pseudo-normals" in that the normals of output
 * triangles that use a common point are averaged at the point. Alternatively
 * use vtkPolyDataNormals to compute the surface normals.
 *
 * @warning
 * The output of this filter is subtly different than the more general filter
 * vtkContourGrid. vtkContourGrid eliminates small, degenerate triangles with
 * concident points which are consequently not sent to the output. In
 * practice this makes little impact on visual appearance but may have
 * repercussions if the output is used for modelling and/or analysis.
 *
 * @warning
 * Input cells that are not of 3D linear type (tetrahedron, hexahedron,
 * wedge, pyramid, and voxel) are simply skipped and not processed.
 *
 * @warning
 * The filter is templated on types of input and output points, and input
 * scalar type. To reduce object file bloat, only real points (float,double) are
 * processed, and a limited subset of scalar types.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkContourGrid vtkContourFilter vtkFlyingEdges3D vtkMarchingCubes
 * vtkPolyDataNormals vtkStaticEdgeLocatorTemplate.h vtkScalarTree
 * vtkSpanSpace
 */

#ifndef vtkContour3DLinearGrid_h
#define vtkContour3DLinearGrid_h

#include "vtkContourValues.h" // Needed for inline methods
#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkScalarTree;
struct vtkScalarTreeMap;

class VTKFILTERSCORE_EXPORT vtkContour3DLinearGrid : public vtkDataObjectAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static vtkContour3DLinearGrid* New();
  vtkTypeMacro(vtkContour3DLinearGrid, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  vtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  ///@}

  ///@{
  /**
   * Indicate whether to merge coincident points. This takes extra time and
   * produces fewer output points, creating a "watertight" contour
   * surface. By default this is off.
   */
  vtkSetMacro(MergePoints, vtkTypeBool);
  vtkGetMacro(MergePoints, vtkTypeBool);
  vtkBooleanMacro(MergePoints, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether to interpolate input attributes onto the isosurface. By
   * default this option is off.
   */
  vtkSetMacro(InterpolateAttributes, vtkTypeBool);
  vtkGetMacro(InterpolateAttributes, vtkTypeBool);
  vtkBooleanMacro(InterpolateAttributes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether to compute output point normals. An averaging method is
   * used to average shared triangle normals. By default this if off. This is
   * a relatively expensive option so use judiciously.
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get flag to compute scalars. When enabled, and when the
    InterpolateAttributes option is on, vtkContour3DLinearGrid will add an
    array corresponding to the array used to compute the contour and
    populate it with values.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  /**
   * Overloaded GetMTime() because of delegation to the internal
   * vtkContourValues class.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Enable the use of a scalar tree to accelerate contour extraction. By
   * default this is off. If enabled, and a scalar tree is not specified, then
   * a vtkSpanSpace instance will be constructed and used.
   */
  vtkSetMacro(UseScalarTree, vtkTypeBool);
  vtkGetMacro(UseScalarTree, vtkTypeBool);
  vtkBooleanMacro(UseScalarTree, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the scalar tree to use. By default a vtkSpanSpace scalar tree is
   * used.
   */
  virtual void SetScalarTree(vtkScalarTree*);
  vtkGetObjectMacro(ScalarTree, vtkScalarTree);
  ///@}

  ///@{
  /**
   * Force sequential processing (i.e. single thread) of the contouring
   * process. By default, sequential processing is off. Note this flag only
   * applies if the class has been compiled with VTK_SMP_IMPLEMENTATION_TYPE
   * set to something other than Sequential. (If set to Sequential, then the
   * filter always runs in serial mode.) This flag is typically used for
   * benchmarking purposes.
   */
  vtkSetMacro(SequentialProcessing, vtkTypeBool);
  vtkGetMacro(SequentialProcessing, vtkTypeBool);
  vtkBooleanMacro(SequentialProcessing, vtkTypeBool);
  ///@}

  /**
   *  Return the number of threads actually used during execution. This is
   *  valid only after algorithm execution.
   */
  int GetNumberOfThreadsUsed() { return this->NumberOfThreadsUsed; }

  /**
   * Inform the user as to whether large ids were used during filter
   * execution. This flag only has meaning after the filter has executed.
   * Large ids are used when the id of the larges cell or point is greater
   * than signed 32-bit precision. (Smaller ids reduce memory usage and speed
   * computation. Note that LargeIds are only available on 64-bit
   * architectures.)
   */
  bool GetLargeIds() { return this->LargeIds; }

  /**
   * Returns true if the data object passed in is fully supported by this
   * filter, i.e., all cell types are linear. For composite datasets, this
   * means all dataset leaves have only linear cell types that can be processed
   * by this filter. The second array is the name of the array to process.
   */
  static bool CanFullyProcessDataObject(vtkDataObject* object, const char* scalarArrayName);

protected:
  vtkContour3DLinearGrid();
  ~vtkContour3DLinearGrid() override;

  vtkContourValues* ContourValues;
  int OutputPointsPrecision;
  vtkTypeBool MergePoints;
  vtkTypeBool InterpolateAttributes;
  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeScalars;
  vtkTypeBool SequentialProcessing;
  int NumberOfThreadsUsed;
  bool LargeIds; // indicate whether integral ids are large(==true) or not

  // Manage scalar trees, including mapping scalar tree to input dataset
  vtkTypeBool UseScalarTree;
  vtkScalarTree* ScalarTree;
  struct vtkScalarTreeMap* ScalarTreeMap;

  // Process the data: input unstructured grid and output polydata
  void ProcessPiece(vtkUnstructuredGrid* input, vtkDataArray* inScalars, vtkPolyData* output);

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkContour3DLinearGrid(const vtkContour3DLinearGrid&) = delete;
  void operator=(const vtkContour3DLinearGrid&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkContour3DLinearGrid::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double vtkContour3DLinearGrid::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* vtkContour3DLinearGrid::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkContour3DLinearGrid::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkContour3DLinearGrid::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline vtkIdType vtkContour3DLinearGrid::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkContour3DLinearGrid::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkContour3DLinearGrid::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

VTK_ABI_NAMESPACE_END
#endif
