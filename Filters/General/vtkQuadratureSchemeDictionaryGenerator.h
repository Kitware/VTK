// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadratureSchemeDictionaryGenerator
 *
 *
 * Given an unstructured grid on its input this filter generates
 * for each data array in point data dictionary (ie an instance of
 * vtkInformationQuadratureSchemeDefinitionVectorKey). This filter
 * has been introduced to facilitate testing of the vtkQuadrature*
 * classes as these cannot operate with the dictionary. This class
 * is for testing and should not be used for application development.
 *
 * @sa
 * vtkQuadraturePointInterpolator, vtkQuadraturePointsGenerator, vtkQuadratureSchemeDefinition
 */

#ifndef vtkQuadratureSchemeDictionaryGenerator_h
#define vtkQuadratureSchemeDictionaryGenerator_h

#include "vtkDataSetAlgorithm.h"
#include "vtkDeprecation.h"          // For deprecation
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkInformation;
class vtkInformationVector;

class VTKFILTERSGENERAL_EXPORT vtkQuadratureSchemeDictionaryGenerator : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkQuadratureSchemeDictionaryGenerator, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkQuadratureSchemeDictionaryGenerator* New();

protected:
  int RequestData(
    vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output) override;
  vtkQuadratureSchemeDictionaryGenerator();
  ~vtkQuadratureSchemeDictionaryGenerator() override;

private:
  vtkQuadratureSchemeDictionaryGenerator(const vtkQuadratureSchemeDictionaryGenerator&) = delete;
  void operator=(const vtkQuadratureSchemeDictionaryGenerator&) = delete;

  ///@{
  /**
   * Generate definitions for each cell type found on the
   * input data set. The same definition will be used
   * for all point data arrays.
   */
  int Generate(vtkDataSet* usgOut);
  VTK_DEPRECATED_IN_9_4_0("Uses the vtkDataSet version instead.")
  int Generate(vtkUnstructuredGrid* usgOut);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
