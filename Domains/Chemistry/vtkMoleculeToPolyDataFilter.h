// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMoleculeToPolyDataFilter
 * @brief   abstract filter class
 *
 * vtkMoleculeToPolyDataFilter is an abstract filter class whose
 * subclasses take as input datasets of type vtkMolecule and
 * generate polygonal data on output.
 */

#ifndef vtkMoleculeToPolyDataFilter_h
#define vtkMoleculeToPolyDataFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToPolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeToPolyDataFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMolecule* GetInput();

protected:
  vtkMoleculeToPolyDataFilter();
  ~vtkMoleculeToPolyDataFilter() override;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkMoleculeToPolyDataFilter(const vtkMoleculeToPolyDataFilter&) = delete;
  void operator=(const vtkMoleculeToPolyDataFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
