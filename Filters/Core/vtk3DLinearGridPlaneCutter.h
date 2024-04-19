// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3DLinearGridPlaneCutter
 * @brief   fast plane cutting of vtkUnstructuredGrid containing 3D linear cells
 *
 * vtk3DLinearGridPlaneCutter is a specialized filter that cuts an input
 * vtkUnstructuredGrid consisting of 3D linear cells: tetrahedra, hexahedra,
 * voxels, pyramids, and/or wedges. (The cells are linear in the sense that
 * each cell edge is a straight line.)  The filter is designed for
 * high-speed, specialized operation. All other cell types are skipped and
 * produce no output.
 *
 * To use this filter you must specify an input unstructured grid or
 * vtkCompositeDataSet (containing unstructured grids) and a plane to cut with.
 *
 * The filter performance varies depending on optional output
 * information. Basically if point merging is required (when PointMerging is
 * set) a sorting process is required to eliminate duplicate output points in
 * the cut surface. Otherwise when point merging is not required, a fast path
 * process produces independent triangles representing the cut surface.
 *
 * This algorithm is fast because it is threaded, and may perform operations
 * (in a preprocessing step) to accelerate the plane cutting.
 *
 * Because this filter may build an initial data structure during a
 * preprocessing step, the first execution of the filter may take longer than
 * subsequent operations. Typically the first execution is still faster than
 * vtkCutter (especially with threading enabled), but for certain types of
 * data this may not be true. However if you are using the filter to cut a
 * dataset multiple times (as in an exploratory or interactive workflow) this
 * filter works well.
 *
 * @warning
 * When the input is of type vtkCompositeDataSet the filter will process the
 * unstructured grid(s) contained in the composite data set. As a result the
 * output of this filter is then a vtkMultiBlockDataSet containing multiple
 * vtkPolyData. When a vtkUnstructuredGrid is provided as input the
 * output is a single vtkPolyData.
 *
 * @warning
 * Input cells that are not of 3D linear type (tetrahedron, hexahedron,
 * wedge, pyramid, and voxel) are simply skipped and not processed.
 *
 * @warning
 * The filter is templated on types of input and output points, and input
 * scalar type. To reduce object file bloat, only real points (float,double) are
 * processed.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCutter vtkFlyingEdgesPlaneCutter vtkPlaneCutter vtkPlane
 * vtkContour3DLinearGrid
 */

#ifndef vtk3DLinearGridPlaneCutter_h
#define vtk3DLinearGridPlaneCutter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPlane;
class vtkUnstructuredGrid;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtk3DLinearGridPlaneCutter : public vtkDataObjectAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static vtk3DLinearGridPlaneCutter* New();
  vtkTypeMacro(vtk3DLinearGridPlaneCutter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of vtkPlane.
   */
  virtual void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane, vtkPlane);
  ///@}

  ///@{
  /**
   * Indicate whether to merge coincident points. Merging can take extra time
   * and produces fewer output points, creating a "watertight" output
   * surface. On the other hand, merging reduced output data size and may be
   * just as fast especially for smaller data. By default this is off.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  ///@}

  ///@{
  /**
   * Indicate whether to interpolate input attributes onto the cut
   * plane. By default this option is on.
   */
  vtkSetMacro(InterpolateAttributes, bool);
  vtkGetMacro(InterpolateAttributes, bool);
  vtkBooleanMacro(InterpolateAttributes, bool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. The normal, if generated, is defined by cell data
   * associated with the output polygons. By default computing of normals is
   * off.
   */
  vtkSetMacro(ComputeNormals, bool);
  vtkGetMacro(ComputeNormals, bool);
  vtkBooleanMacro(ComputeNormals, bool);
  ///@}

  /**
   * Overloaded GetMTime() because of delegation to the helper
   * vtkPlane.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/get the desired precision for the output points. See the
   * documentation for the vtkAlgorithm::Precision enum for an explanation of
   * the available precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
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
  vtkSetMacro(SequentialProcessing, bool);
  vtkGetMacro(SequentialProcessing, bool);
  vtkBooleanMacro(SequentialProcessing, bool);
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
   * by this filter.
   */
  static bool CanFullyProcessDataObject(vtkDataObject* object);

protected:
  vtk3DLinearGridPlaneCutter();
  ~vtk3DLinearGridPlaneCutter() override;

  vtkPlane* Plane;
  bool MergePoints;
  bool InterpolateAttributes;
  bool ComputeNormals;
  int OutputPointsPrecision;
  bool SequentialProcessing;
  int NumberOfThreadsUsed;
  bool LargeIds; // indicate whether integral ids are large(==true) or not

  // Process the data: input unstructured grid and output polydata
  int ProcessPiece(vtkUnstructuredGrid* input, vtkPlane* plane, vtkPolyData* output);

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtk3DLinearGridPlaneCutter(const vtk3DLinearGridPlaneCutter&) = delete;
  void operator=(const vtk3DLinearGridPlaneCutter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
