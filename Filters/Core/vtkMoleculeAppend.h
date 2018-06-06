/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeAppend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

/**
 * @class vtkMoleculeAppend
 * @brief Appends one or more molecules into a single molecule
 *
 * vtkMoleculeAppend appends molecule into a single molecule. It also appends
 * the associated atom data and edge data.
 * Note that input data arrays should match (same number of arrays with same names in each input)
 *
 * Option MergeCoincidentAtoms specifies if coincident atoms should be merged or not.
 * This may be useful in Parallel mode to remove ghost atoms when gather molecule on a rank.
 * When merging, use the data of the non ghost atom. If none, use the data of the last coincident atom.
 * This option is active by default.
 */

#ifndef vtkMoleculeAppend_h
#define vtkMoleculeAppend_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkMoleculeAppend : public vtkMoleculeAlgorithm
{
public:
  static vtkMoleculeAppend* New();
  vtkTypeMacro(vtkMoleculeAppend, vtkMoleculeAlgorithm);

  //@{
  /**
   * Get one input to this filter. This method is only for support of
   * old-style pipeline connections.  When writing new code you should
   * use vtkAlgorithm::GetInputConnection(0, num).
   */
  vtkDataObject* GetInput(int num);
  vtkDataObject* GetInput() { return this->GetInput(0); }
    //@}

  //@{
  /**
   * Specify if coincident atoms (atom with exactly the same position)
   * should be merged into one.
   * True by default.
   */
  vtkGetMacro(MergeCoincidentAtoms, bool);
  vtkSetMacro(MergeCoincidentAtoms, bool);
  vtkBooleanMacro(MergeCoincidentAtoms, bool);
  // @}

protected:
  vtkMoleculeAppend();
  ~vtkMoleculeAppend() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // see vtkAlgorithm for docs.
  int FillInputPortInformation(int, vtkInformation*) override;

  // Check arrays information : name, type and number of components.
  bool CheckArrays(vtkDataArray* array1, vtkDataArray* array2);

  bool MergeCoincidentAtoms;

private:
  vtkMoleculeAppend(const vtkMoleculeAppend&) = delete;
  void operator=(const vtkMoleculeAppend&) = delete;
};

#endif
