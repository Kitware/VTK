// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight
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
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written and optimized by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight_h
#define vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight_h

#include "vtkHyperTreeGridNonOrientedSuperCursorLight.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight
  : public vtkHyperTreeGridNonOrientedSuperCursorLight
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight,
    vtkHyperTreeGridNonOrientedSuperCursorLight);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* New();

  /**
   * Initialize cursor at root of given tree index in grid.
   * "create" only applies to the central HT
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) override;

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight() = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight() override;

private:
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight(
    const vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
