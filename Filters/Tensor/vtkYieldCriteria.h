// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkYieldCriteria
 * @brief   Compute principal stress and yield criteria from symmetric tensor
 *
 * This filter computes various yield criteria from symmetric tensors including:
 *   - Principal stress values and vectors
 *   - Tresca criterion
 *   - Von Mises criterion
 *
 * The principal values are ordered from largest to smallest.
 *   - sigmaN value: Nth principal stress eigenvalue
 *   - sigmaN vector: Nth principal stress vector (can be scaled with the value)
 *   - Tresca criterion : |sigma3 - sigma1|
 *   - Von Mises criterion:
 *     sqrt( (sigma1 - sigma2)^2 + (sigma2 - sigma3)^2 + (sigma1 - sigma3)^2 ) / sqrt(2)
 */

#ifndef vtkYieldCriteria_h
#define vtkYieldCriteria_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersTensorModule.h" // For export macro
#include "vtkNew.h"                 // For vtkNew

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataArraySelection;
class vtkDataSet;
class vtkTensorPrincipalInvariants;

class VTKFILTERSTENSOR_EXPORT vtkYieldCriteria : public vtkDataSetAlgorithm
{
public:
  static vtkYieldCriteria* New();
  vtkTypeMacro(vtkYieldCriteria, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Enum of supported yield criteria.
   */
  enum class Criterion
  {
    PrincipalStress = 0,
    Tresca,
    VonMises
  };

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
   * should have their yield criteria computed.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  /**
   * Access the cell data array selection that specifies which cell data arrays
   * should have their yield criteria computed.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  /**
   * Access the selection of yield criteria to compute.
   */
  vtkGetObjectMacro(CriteriaSelection, vtkDataArraySelection);

protected:
  vtkYieldCriteria();
  ~vtkYieldCriteria() override = default;

  /**
   * Compute and append to the given dataset the yield criteria derived from the
   * given array.
   */
  bool ComputeYieldCriteria(vtkDataSet* output, vtkDataArray* array, const std::string& arrayName,
    vtkIdType nbTuples, bool isPointData) const;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<vtkTensorPrincipalInvariants> InvariantsFilter;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> CriteriaSelection;
  bool ScaleVectors = false;

private:
  vtkYieldCriteria(const vtkYieldCriteria&) = delete;
  void operator=(const vtkYieldCriteria&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
