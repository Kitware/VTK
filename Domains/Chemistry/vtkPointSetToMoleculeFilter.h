// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*
 * @class vtkPointSetToMoleculeFilter
 * @brief Converts a pointset into a molecule.
 *
 * vtkPointSetToMoleculeFilter is a filter that takes a vtkPointSet as input
 * and generates a vtkMolecule.
 * Each point of the given vtkPointSet will become an atom of the vtkMolecule.
 * The vtkPointSet should provide a point data array (default is scalar one)
 * to specify the atomic number of each atom.
 */

#ifndef vtkPointSetToMoleculeFilter_h
#define vtkPointSetToMoleculeFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKDOMAINSCHEMISTRY_EXPORT vtkPointSetToMoleculeFilter : public vtkMoleculeAlgorithm
{
public:
  static vtkPointSetToMoleculeFilter* New();
  vtkTypeMacro(vtkPointSetToMoleculeFilter, vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set if the filter should look for lines in input cells and convert them
   * into bonds.
   * default is ON.
   */
  vtkGetMacro(ConvertLinesIntoBonds, bool);
  vtkSetMacro(ConvertLinesIntoBonds, bool);
  vtkBooleanMacro(ConvertLinesIntoBonds, bool);
  ///@}

protected:
  vtkPointSetToMoleculeFilter();
  ~vtkPointSetToMoleculeFilter() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool ConvertLinesIntoBonds;

private:
  vtkPointSetToMoleculeFilter(const vtkPointSetToMoleculeFilter&) = delete;
  void operator=(const vtkPointSetToMoleculeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
