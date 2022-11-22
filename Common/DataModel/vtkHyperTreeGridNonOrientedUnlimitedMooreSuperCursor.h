/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor
 * @brief   Specific Moore super cursor that can subdivied neighborhood
 *
 * This supercursor behave like the Moore supercursor but relies on the
 * vtkHyperTreeGridNonOrientedUnlimitedSuperCursor so the neighborhood
 * can be refined to reach the level of the current cell in any case.
 *
 * @sa
 * vtkHyperTreeCursor vtkHyperTree vtkHyperTreeGrid vtkHyperTreeGridNonOrientedMooreSuperCursor
 */

#ifndef vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor_h
#define vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor_h

#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkHyperTreeGridNonOrientedUnlimitedSuperCursor.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkHyperTree;
class vtkHyperTreeGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor
  : public vtkHyperTreeGridNonOrientedUnlimitedSuperCursor
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor,
    vtkHyperTreeGridNonOrientedUnlimitedSuperCursor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* New();

  /**
   * Initialize cursor at root of given tree index in grid.
   * JB Le create ne s'applique que sur le HT central.
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) override;

  /**
   * Return the list of cursors pointing to the leaves touching a
   * given corner of the cell.
   * Return whether the considered cell is the owner of said corner.
   * JB Utilise aujourd'hui dans les filtres vtkHyperTreeGridContour et vtkHyperTreeGridPlaneCutter.
   */
  bool GetCornerCursors(unsigned int, unsigned int, vtkIdList*);

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor() = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor() override;

private:
  vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor(
    const vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
