/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridGradient
 * @brief   Compute the gradient of a scalar field
 * on a Hyper Tree Grid.
 *
 * This filter compute the gradient of the cell scalars
 * on a Hyper Tree Grid data set. This result in a new array
 * attached to the original input.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkGradientFilter
 *
 * @par Thanks:
 * This class was modified by Charles Gueunet, 2022
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGradient_h
#define vtkHyperTreeGridGradient_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vector> // For STL

class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGradient : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGradient* New();
  vtkTypeMacro(vtkHyperTreeGridGradient, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHyperTreeGridGradient();
  ~vtkHyperTreeGridGradient() override;

  /**
   * Main routine to generate gradient of hyper tree grid.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   *
   */
  bool RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedCursor*);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedMooreSuperCursor*);

  /**
   * Keep track of selected input scalars
   */
  vtkDataArray* InScalars;

private:
  vtkHyperTreeGridGradient(const vtkHyperTreeGridGradient&) = delete;
  void operator=(const vtkHyperTreeGridGradient&) = delete;
};

#endif // vtkHyperTreeGridGradient_h
