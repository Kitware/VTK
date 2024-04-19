// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTensorPrincipalInvariants
 * @brief   Compute principal values and directions from symmetric tensor
 *
 * This filter computes principal values and vectors of symmetric tensors.
 * The principal values are ordered from largest to smallest.
 * The principal vectors can be scaled with the principal values.
 */

#ifndef vtkTensorPrincipalInvariants_h
#define vtkTensorPrincipalInvariants_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersTensorModule.h" // For export macro
#include "vtkNew.h"                 // For vtkNew

#include <array>  // for std::array
#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataArraySelection;
class vtkDataSet;

class VTKFILTERSTENSOR_EXPORT vtkTensorPrincipalInvariants : public vtkDataSetAlgorithm
{
public:
  static vtkTensorPrincipalInvariants* New();
  vtkTypeMacro(vtkTensorPrincipalInvariants, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get whether the principal vectors should be scaled with the principal values.
   * Default is false.
   */
  vtkGetMacro(ScaleVectors, bool);
  vtkSetMacro(ScaleVectors, bool);
  ///@}

  /**
   * Access the point data array selection that specifies which point data arrays
   * should have their principal invariants computed.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  /**
   * Access the cell data array selection that specifies which cell data arrays
   * should have their principal invariants computed.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  /**
   * Return the given array name with a suffix for principal values attached.
   */
  static std::string GetSigmaValueArrayName(const std::string& baseName, int index);

  /**
   * Return the given array name with a suffix for principal vectors attached.
   */
  static std::string GetSigmaVectorArrayName(const std::string& baseName, int index);

protected:
  vtkTensorPrincipalInvariants();
  ~vtkTensorPrincipalInvariants() override = default;

  /**
   * Determine whether the given array corresponds to a symmetric 3D tensor
   * (6 components) or 2D tensor (3 components). 2D tensors are differentiated
   * from vectors by checking the components names to see if they match "XX",
   * "YY" and "XY".
   */
  bool IsSymmetricTensor(vtkDataArray* array) const;

  /**
   * Compute and append to the given dataset the principal values and directions
   * from the given array.
   */
  bool ComputePrincipalInvariants(vtkDataSet* output, vtkDataArray* array,
    const std::string& arrayName, vtkIdType nbTuples, bool isPointData) const;

  /**
   * Return the indices that sort the input values from largest to smallest.
   */
  std::array<int, 3> GetDecreasingOrder(double values[3]) const;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  bool ScaleVectors = false;

private:
  vtkTensorPrincipalInvariants(const vtkTensorPrincipalInvariants&) = delete;
  void operator=(const vtkTensorPrincipalInvariants&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
