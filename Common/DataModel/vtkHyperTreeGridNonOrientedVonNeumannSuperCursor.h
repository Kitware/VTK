// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedVonNeumannSuperCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * This supercursor allows to traverse neighbors attached to coface of
 * the current position.
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written and optimized by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedVonNeumannSuperCursor_h
#define vtkHyperTreeGridNonOrientedVonNeumannSuperCursor_h

#include "vtkHyperTreeGridNonOrientedSuperCursor.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedVonNeumannSuperCursor
  : public vtkHyperTreeGridNonOrientedSuperCursor
{
public:
  vtkTypeMacro(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor, vtkHyperTreeGridNonOrientedSuperCursor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* New();

  /**
   * Initialize cursor at root of given tree index in grid.
   * "create" only applies to the central HT
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) override;

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor() = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedVonNeumannSuperCursor() override;

private:
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor(
    const vtkHyperTreeGridNonOrientedVonNeumannSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedVonNeumannSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
