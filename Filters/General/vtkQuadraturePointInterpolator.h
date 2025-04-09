// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadraturePointInterpolator
 *
 *
 * Interpolates each scalar/vector field in a vtkDataSet
 * on its input to a specific set of quadrature points. The
 * set of quadrature points is specified per array via a
 * dictionary (ie an instance of vtkInformationQuadratureSchemeDefinitionVectorKey).
 * contained in the array. The interpolated fields are placed
 * in FieldData along with a set of per cell indexes, that allow
 * random access to a given cells quadrature points.
 *
 * @sa
 * vtkQuadratureSchemeDefinition, vtkQuadraturePointsGenerator,
 * vtkInformationQuadratureSchemeDefinitionVectorKey
 */

#ifndef vtkQuadraturePointInterpolator_h
#define vtkQuadraturePointInterpolator_h

#include "vtkDataSetAlgorithm.h"
#include "vtkDeprecation.h"          // For deprecation
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkUnstructuredGrid;
class vtkInformation;
class vtkInformationVector;

class VTKFILTERSGENERAL_EXPORT vtkQuadraturePointInterpolator : public vtkDataSetAlgorithm
{
public:
  static vtkQuadraturePointInterpolator* New();
  vtkTypeMacro(vtkQuadraturePointInterpolator, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  int RequestData(
    vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output) override;
  vtkQuadraturePointInterpolator();
  ~vtkQuadraturePointInterpolator() override = default;

private:
  vtkQuadraturePointInterpolator(const vtkQuadraturePointInterpolator&) = delete;
  void operator=(const vtkQuadraturePointInterpolator&) = delete;

  ///@{
  /**
   * Generate field data arrays that have all scalar/vector
   * fields interpolated to the quadrature points. The type
   * of quadrature used is found in the dictionary stored as
   * meta data in each data array.
   */
  int InterpolateFields(vtkDataSet* datasetOut);
  VTK_DEPRECATED_IN_9_4_0("Uses the vtkDataSet version instead.")
  int InterpolateFields(vtkUnstructuredGrid* usgOut);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
