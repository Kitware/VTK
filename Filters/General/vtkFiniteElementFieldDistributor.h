// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkFiniteElementFieldDistributor.h
 * @brief Distribute cell-centered finite element fields from the input dataset onto vtk cell
 * points.
 *
 */

#ifndef vtkFiniteElementFieldDistributor_h
#define vtkFiniteElementFieldDistributor_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkFiniteElementFieldDistributor
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkFiniteElementFieldDistributor* New();
  vtkTypeMacro(vtkFiniteElementFieldDistributor, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkFiniteElementFieldDistributor();
  ~vtkFiniteElementFieldDistributor() override;

  // vtkAlgorithm interface
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkFiniteElementFieldDistributor(const vtkFiniteElementFieldDistributor&) = delete;
  void operator=(const vtkFiniteElementFieldDistributor&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
