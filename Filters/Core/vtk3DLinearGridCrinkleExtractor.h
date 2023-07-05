// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3DLinearGridCrinkleExtractor
 * @brief   fast extraction of cells intersected by a plane
 *
 * vtk3DLinearGridCrinkleExtractor is a specialized filter that, given a
 * specified implicit function, extracts unstructured grid cells that
 * intersect the implicit function. (Since the surface of these cells roughly
 * follows the implicit function but is "bumpy", it is referred to as a
 * "crinkle" surface.) This filter operates on vtkUnstructuredGrids consisting
 * of 3D linear cells: tetrahedra, hexahedra, voxels, pyramids, and/or
 * wedges. (The cells are linear in the sense that each cell edge is a
 * straight line.)  The filter is designed for high-speed, specialized
 * operation. All other cell types are skipped and produce no output.
 *
 * To use this filter you must specify an input unstructured grid or
 * vtkCompositeDataSet (containing unstructured grids) and an implicit
 * function to cut with.
 *
 * The RemoveUnusedPoints data member controls whether the filter remaps the
 * input points to the output. Since the algorithm simply extracts a subset
 * of the original data (points and cells), it is possible simply to pass the
 * input points to the output, which is much faster (factor of ~2X) then
 * mapping the input points to the output. Of course, not removing the unused
 * points means extra points in the output dataset, but because the input
 * points are shallow copied to the output, no additional memory is consumed.
 *
 * @warning
 * When the input is of type vtkCompositeDataSet the filter will process the
 * unstructured grid(s) contained in the composite data set. As a result the
 * output of this filter is then a vtkMultiBlockDataSet containing multiple
 * vtkUnstructuredGrids. When a vtkUnstructuredGrid is provided as input the
 * output is a single vtkUnstructuredGrid.
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
 * @warning
 * The vtkExtractGeometry filter is similar to this filter when
 * ExtractOnlyBoundaryCells is enabled.
 *
 * @sa
 * vtk3DLinearGrid vtk3DLinearGridPlaneCutter vtkExtractGeometry
 */

#ifndef vtk3DLinearGridCrinkleExtractor_h
#define vtk3DLinearGridCrinkleExtractor_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;
class vtkImplicitFunction;

class VTKFILTERSCORE_EXPORT vtk3DLinearGridCrinkleExtractor : public vtkDataObjectAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static vtk3DLinearGridCrinkleExtractor* New();
  vtkTypeMacro(vtk3DLinearGridCrinkleExtractor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the implicit function which is used to select the output cell
   * faces. Note that the implicit function invocation must be thread
   * safe. Also, there is a fast path for vtkPlane implicit functions.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Indicate whether to copy input point data/attributes onto the output
   * points. By default this option is on.
   */
  vtkSetMacro(CopyPointData, bool);
  vtkGetMacro(CopyPointData, bool);
  vtkBooleanMacro(CopyPointData, bool);
  ///@}

  ///@{
  /**
   * Indicate whether to copy input cell data/attributes onto the output
   * cells. By default this option is off.
   */
  vtkSetMacro(CopyCellData, bool);
  vtkGetMacro(CopyCellData, bool);
  vtkBooleanMacro(CopyCellData, bool);
  ///@}

  ///@{
  /**
   * Indicate whether to eliminate unused output points. When this flag is
   * disabled, the input points and associated point data are simply shallow
   * copied to the output (which improves performance).  When enabled, any
   * points that are not used by the output cells are not sent to the output,
   * nor is associated point data copied. By default this option is disabled.
   * Removing unused points does have a significant performance impact.
   */
  vtkSetMacro(RemoveUnusedPoints, bool);
  vtkGetMacro(RemoveUnusedPoints, bool);
  vtkBooleanMacro(RemoveUnusedPoints, bool);
  ///@}

  /**
   * Overloaded GetMTime() because of delegation to the helper
   * vtkImplicitFunction.
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
   * Force sequential processing (i.e. single thread) of the crinkle cut
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
   * Returns true if the data object passed in is fully supported by this
   * filter, i.e., all cell types are linear. For composite datasets, this
   * means all dataset leaves have only linear cell types that can be processed
   * by this filter.
   */
  static bool CanFullyProcessDataObject(vtkDataObject* object);

protected:
  vtk3DLinearGridCrinkleExtractor();
  ~vtk3DLinearGridCrinkleExtractor() override;

  vtkImplicitFunction* ImplicitFunction;
  bool RemoveUnusedPoints;
  bool CopyPointData;
  bool CopyCellData;
  int OutputPointsPrecision;
  bool SequentialProcessing;
  int NumberOfThreadsUsed;

  // Process the data: input unstructured grid and output an unstructured grid
  int ProcessPiece(vtkUnstructuredGrid* input, vtkImplicitFunction* f, vtkUnstructuredGrid* grid);

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtk3DLinearGridCrinkleExtractor(const vtk3DLinearGridCrinkleExtractor&) = delete;
  void operator=(const vtk3DLinearGridCrinkleExtractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
