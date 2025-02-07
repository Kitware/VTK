// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAxisAlignedReflectionFilter
 * @brief   Reflects the input over an axis-aligned plane
 *
 * The Axis Aligned Reflection filter reflects the input dataset across the
 * specified plane. This filter operates on any type of data
 * set or hyper tree grid and produces a Partitioned DataSet Collection
 * containing partitions of the same type as the input (the reflection
 * and the input if CopyInput is enabled).
 * Data arrays are also reflected (if ReflectAllInputArrays is false,
 * only Vectors, Normals and Tensors will be reflected, otherwise,
 * all 3, 6 and 9-component data arrays are reflected).
 *
 * @sa vtkReflectionFilter vtkHyperTreeGridAxisReflection
 * The main difference between vtkReflectionFilter and vtkAxisAlignedReflectionFilter
 * is the output type (vtkReflectionFilter produces an Unstructured Grid no matter
 * the input type).
 * Also, vtkAxisAlignedReflectionFilter supports Hyper Tree Grids (vtkReflectionFilter does not).
 */

#ifndef vtkAxisAlignedReflectionFilter_h
#define vtkAxisAlignedReflectionFilter_h

#include "vtkCompositeDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // for export macro
#include "vtkPlane.h"                // for vtkPlane class
#include "vtkSmartPointer.h"         // for smart pointer

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkExplicitStructuredGrid;
class vtkHyperTreeGrid;
class vtkImageData;
class vtkPartitionedDataSetCollection;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;

class VTKFILTERSGENERAL_EXPORT vtkAxisAlignedReflectionFilter : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkAxisAlignedReflectionFilter* New();

  vtkTypeMacro(vtkAxisAlignedReflectionFilter, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum PlaneAxis
  {
    X_PLANE = 0,
    Y_PLANE = 1,
    Z_PLANE = 2
  };

  enum PlaneModes
  {
    PLANE = 0,
    X_MIN = 1,
    Y_MIN = 2,
    Z_MIN = 3,
    X_MAX = 4,
    Y_MAX = 5,
    Z_MAX = 6,
  };

  ///@{
  /**
   * Specify the vtkPlane to perform the reflection when using PLANE mode.
   * Default is AxisAligned true, Offset 0, Normal (1.0, 0.0, 0.0) and Origin (0.0, 0.0, 0.0)
   */
  vtkSetSmartPointerMacro(ReflectionPlane, vtkPlane);
  vtkGetSmartPointerMacro(ReflectionPlane, vtkPlane);
  ///@}

  ///@{
  /**
   * Determines which plane to reflect across.
   * If the value is PLANE, the plane is defined by the ReflectionPlane.
   * The other six options (X Min, X Max, etc.) place the reflection plane
   * at the specified face of the bounding box of the input dataset.
   * Default is PLANE.
   */
  vtkSetClampMacro(PlaneMode, int, 0, 6);
  vtkGetMacro(PlaneMode, int);
  void SetPlaneModeToPlane() { this->SetPlaneMode(PLANE); }
  void SetPlaneModeToXMin() { this->SetPlaneMode(X_MIN); }
  void SetPlaneModeToYMin() { this->SetPlaneMode(Y_MIN); }
  void SetPlaneModeToZMin() { this->SetPlaneMode(Z_MIN); }
  void SetPlaneModeToXMax() { this->SetPlaneMode(X_MAX); }
  void SetPlaneModeToYMax() { this->SetPlaneMode(Y_MAX); }
  void SetPlaneModeToZMax() { this->SetPlaneMode(Z_MAX); }
  ///@}

  ///@{
  /**
   * Copy the input geometry to the output. If false,
   * the output will only contain the reflection.
   * Default is true.
   */
  vtkSetMacro(CopyInput, bool);
  vtkGetMacro(CopyInput, bool);
  vtkBooleanMacro(CopyInput, bool);
  ///@}

  ///@{
  /**
   * If false, only Vectors, Normals and Tensors will be reflected.
   * If true, all 3-component data arrays ( considered as 3D vectors),
   * 6-component data arrays (considered as symmetric tensors),
   * 9-component data arrays (considered as tensors ) of signed type will be reflected.
   * All other won't be reflected and will only be copied.
   * Default is false.
   */
  vtkSetMacro(ReflectAllInputArrays, bool);
  vtkGetMacro(ReflectAllInputArrays, bool);
  vtkBooleanMacro(ReflectAllInputArrays, bool);
  ///@}

  /**
   * Get the last modified time of this filter.
   * This time also depends on the modified
   * time of the internal ReflectionFunction instance.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAxisAlignedReflectionFilter() = default;
  ~vtkAxisAlignedReflectionFilter() override = default;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Compute the bounds of the input data object.
   * It has to be virtual protected because in a distributed context,
   * the bounds will need to be computed differently.
   */
  virtual void ComputeBounds(vtkDataObject* input, double bounds[6]);

private:
  vtkAxisAlignedReflectionFilter(const vtkAxisAlignedReflectionFilter&) = delete;
  void operator=(const vtkAxisAlignedReflectionFilter&) = delete;

  /**
   * Find all the reflectable arrays in the input, then reflect them to the output
   */
  void FindAndReflectArrays(vtkDataSet* input, vtkDataSet* output, int mirrorDir[3],
    int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);

  /**
   * Add `dObj` as a new partitioned dataset of `outputPDSC` and update the assembly.
   */
  void AddPartitionedDataSet(vtkPartitionedDataSetCollection* outputPDSC, vtkDataObject* dObj,
    vtkInformation* inputMetadata, int nodeId, bool isParentMultiblock, bool isInputCopy);

  /**
   * Process composite inputs.
   * A "Composite" node is added as child of reflectionNodeId, and for each child of the composite
   * input, a node with the same name is added to the "Composite" node. If CopyInput is on, the same
   * process is applied as child of inputNodeId, and the prefix "Input_" is added to each child's
   * name.
   */
  bool ProcessComposite(vtkPartitionedDataSetCollection* outputPDSC, vtkCompositeDataSet* inputCD,
    double bounds[6], int inputNodeId, int reflectionNodeId);
  /**
   * Process non-composite inputs (datasets and hyper tree grids).
   */
  bool ProcessLeaf(
    vtkDataObject* inputDataObject, vtkDataObject* outputDataObject, double bounds[6]);

  ///@{
  /**
   * Performs the actual reflection depending on the data type.
   */
  void ProcessExplicitStructuredGrid(vtkExplicitStructuredGrid* input,
    vtkExplicitStructuredGrid* output, double constant[3], int mirrorDir[3],
    int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  void ProcessHtg(vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, int mirrorDir[3],
    int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  void ProcessImageData(vtkImageData* input, vtkImageData* output, double constant[3],
    int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  void ProcessPolyData(vtkPolyData* input, vtkPolyData* output, double constant[3],
    int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  void ProcessRectilinearGrid(vtkRectilinearGrid* input, vtkRectilinearGrid* output,
    double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  void ProcessStructuredGrid(vtkStructuredGrid* input, vtkStructuredGrid* output,
    double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9]);
  ///@}

  bool CopyInput = true;
  bool ReflectAllInputArrays = false;
  int PlaneMode = PLANE;
  vtkSmartPointer<vtkPlane> ReflectionPlane;

  PlaneAxis PlaneAxisInternal = X_PLANE;
  double PlaneOriginInternal[3] = { 0.0, 0.0, 0.0 };

  // For naming purposes
  int InputCount = 0;
  int ReflectionCount = 0;

  int PartitionIndex = 0;
};

VTK_ABI_NAMESPACE_END
#endif
