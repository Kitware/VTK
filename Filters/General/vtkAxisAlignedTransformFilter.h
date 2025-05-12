// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAxisAlignedTransformFilter
 * @brief Applies an axis-aligned affine transformation (translation, scaling, and rotation)
 *
 * The Axis Aligned Transform filter operates on any type of data
 * set or hyper tree grid and applies a transformation that is
 * constrained to be axis-aligned. The output type is the same as the
 * input.
 */

#ifndef vtkAxisAlignedTransformFilter_h
#define vtkAxisAlignedTransformFilter_h

#include "vtkDataObjectAlgorithm.h"

#include "vtkFiltersGeneralModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkTransform;
class vtkImageData;
class vtkRectilinearGrid;

class VTKFILTERSGENERAL_EXPORT vtkAxisAlignedTransformFilter : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkAxisAlignedTransformFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAxisAlignedTransformFilter* New();

  /**
   * Enumeration for specifying the rotation axis.
   */
  enum Axis : int
  {
    X = 0,
    Y = 1,
    Z = 2
  };

  /**
   * Enumeration for specifying the rotation angle.
   */
  enum Angle : int
  {
    ROT0 = 0,
    ROT90 = 1,
    ROT180 = 2,
    ROT270 = 3
  };

  ///@{
  /**
   * Set/Get the translation vector.
   */
  vtkSetVector3Macro(Translation, double);
  vtkGetVectorMacro(Translation, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the scaling factors.
   */
  vtkSetVector3Macro(Scale, double);
  vtkGetVectorMacro(Scale, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the rotation angle enumeration.
   * This defines the rotation angle to be applied about the
   * chosen axis. Use Angle enum as value.
   */
  vtkSetClampMacro(RotationAngle, int, ROT0, ROT270);
  vtkGetMacro(RotationAngle, int);
  ///@}

  ///@{
  /**
   * Set/Get the axis along which the rotation is applied.
   * Use Axis enum as value.
   */
  vtkSetClampMacro(RotationAxis, int, X, Z);
  vtkGetMacro(RotationAxis, unsigned int);
  ///@}

protected:
  vtkAxisAlignedTransformFilter() = default;
  ~vtkAxisAlignedTransformFilter() override = default;

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * This creates an output that matches the type of the input data.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkAxisAlignedTransformFilter(const vtkAxisAlignedTransformFilter&) = delete;
  void operator=(const vtkAxisAlignedTransformFilter&) = delete;

  ///@{
  /**
   * Performs the actual transformation depending on the data type.
   */
  bool ProcessGeneric(vtkDataObject* inputDataObject, vtkDataObject* outputDataObject);
  bool ProcessHTG(vtkHyperTreeGrid* inputHTG, vtkHyperTreeGrid* outputHTG, int R[3][3]);
  bool ProcessImageData(vtkImageData* inputID, vtkImageData* outputID, int R[3][3]);
  bool ProcessRectilinearGrid(
    vtkRectilinearGrid* inputRG, vtkRectilinearGrid* outputRG, int R[3][3]);
  ///@}

  /**
   * Dispatch inputs based on their type
   */
  bool Dispatch(vtkDataObject* inputDataObject, vtkDataObject* outputDataObject);

  /**
   * Computes the rotation matrix part of the transformation based
   * on the selected axis and rotation angle.
   */
  void GetRotationMatrix(int axis, int rotation, int R[3][3]);

  /**
   * Based on a grid index, computes the corresponding index after rotation
   */
  int GetRotatedId(int id, int R[3][3], int newDims[3], int dims[3], int Tvec[3], bool transposed);

  /**
   * Translates the local transform parameters into a vtkTransform.
   */
  void GetTransform(vtkTransform* transform);

  /**
   * Helper function for HTG only. Applies recursively the new cell
   * scale to each cell.
   */
  void ComputeCellScale(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,
    int dims[3], double scales[3]);

  /**
   * Helper function for HTG only. Applies recursively the new cell
   * scale to each cell.
   */
  void ApplyCellScale(vtkHyperTreeGridNonOrientedCursor* cursor, double scales[3]);

  /**
   * Helper function for HTG only. For 2D HTG, find the axis
   * orthogonal to the plane the HTG is defined on.
   */
  vtkAxisAlignedTransformFilter::Axis FindNormalAxis(int dims[3]);

  /**
   * Helper function for HTG only. Computes a permutation vector that
   * represents how indices are swapped due to the rotation.
   */
  std::vector<unsigned int> ComputePermutation(
    unsigned int branchFactor, int axis, int normalAxis, int rotation, int dimension);

  /**
   * Helper function for HTG only. Recursively copy the input hyper tree
   * structure but rotated according to the provided permutation vector.
   */
  void CopyAndRotate(vtkHyperTree* input, vtkHyperTree* output, vtkIdType inputIndex,
    vtkIdType outputIndex, const std::vector<unsigned int>& permutation, unsigned int depth,
    vtkHyperTreeGridNonOrientedCursor* cursor);

  /**
   * Helper function for HTG only. Creates a new Hyper Tree similar
   * to the input but rotated according to the provided permutation vector.
   */
  vtkHyperTree* CreateNewRotatedHyperTree(
    vtkHyperTreeGrid* htg, vtkHyperTree* dest, const std::vector<unsigned int>& permutation);

  // Transformation parameters
  double Translation[3] = { 0.0, 0.0, 0.0 };
  double Scale[3] = { 1.0, 1.0, 1.0 };
  int RotationAngle = Angle::ROT0;
  int RotationAxis = Axis::X;
};

VTK_ABI_NAMESPACE_END
#endif
