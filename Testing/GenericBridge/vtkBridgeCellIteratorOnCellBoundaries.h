// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBridgeCellIteratorOnCellBoundaries
 * @brief   Iterate over boundary cells of
 * a cell.
 *
 * @sa
 * vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy
 */

#ifndef vtkBridgeCellIteratorOnCellBoundaries_h
#define vtkBridgeCellIteratorOnCellBoundaries_h

#include "vtkBridgeCellIteratorStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorOnCellBoundaries
  : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOnCellBoundaries* New();
  vtkTypeMacro(vtkBridgeCellIteratorOnCellBoundaries, vtkBridgeCellIteratorStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() override;

  /**
   * Is there no cell at iterator position? (exit condition).
   */
  vtkTypeBool IsAtEnd() override;

  /**
   * Cell at current position
   * \pre not_at_end: !IsAtEnd()
   * \pre c_exists: c!=0
   * THREAD SAFE
   */
  void GetCell(vtkGenericAdaptorCell* c) override;

  /**
   * Cell at current position.
   * NOT THREAD SAFE
   * \pre not_at_end: !IsAtEnd()
   * \post result_exits: result!=0
   */
  vtkGenericAdaptorCell* GetCell() override;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_at_end: !IsAtEnd()
   */
  void Next() override;

  /**
   * Used internally by vtkBridgeCell.
   * Iterate on boundary cells of a cell.
   * \pre cell_exists: cell!=0
   * \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
   */
  void InitWithCellBoundaries(vtkBridgeCell* cell, int dim);

protected:
  vtkBridgeCellIteratorOnCellBoundaries();
  ~vtkBridgeCellIteratorOnCellBoundaries() override;

  int Dim; // Dimension of cells over which to iterate (-1 to 3)

  vtkBridgeCell* DataSetCell; // the structure on which the object iterates.
  vtkIdType Id;               // the id at current position.
  vtkBridgeCell* Cell;        // cell at current position.
  vtkIdType NumberOfFaces;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfVertices;

private:
  vtkBridgeCellIteratorOnCellBoundaries(const vtkBridgeCellIteratorOnCellBoundaries&) = delete;
  void operator=(const vtkBridgeCellIteratorOnCellBoundaries&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
