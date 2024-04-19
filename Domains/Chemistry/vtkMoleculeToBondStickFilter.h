// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMoleculeToBondStickFilter
 * @brief   Generate polydata with cylinders
 * representing bonds
 */

#ifndef vtkMoleculeToBondStickFilter_h
#define vtkMoleculeToBondStickFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeToPolyDataFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToBondStickFilter : public vtkMoleculeToPolyDataFilter
{
public:
  vtkTypeMacro(vtkMoleculeToBondStickFilter, vtkMoleculeToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkMoleculeToBondStickFilter* New();

protected:
  vtkMoleculeToBondStickFilter();
  ~vtkMoleculeToBondStickFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMoleculeToBondStickFilter(const vtkMoleculeToBondStickFilter&) = delete;
  void operator=(const vtkMoleculeToBondStickFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
