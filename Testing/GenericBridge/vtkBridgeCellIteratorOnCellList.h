// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBridgeCellIteratorOnCellList
 * @brief   Iterate over a list of cells defined
 * on a dataset.
 * See InitWithCells().
 * @sa
 * vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy
 */

#ifndef vtkBridgeCellIteratorOnCellList_h
#define vtkBridgeCellIteratorOnCellList_h

#include "vtkBridgeCellIteratorStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBridgeCell;
class vtkIdList;
class vtkBridgeDataSet;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorOnCellList
  : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOnCellList* New();
  vtkTypeMacro(vtkBridgeCellIteratorOnCellList, vtkBridgeCellIteratorStrategy);
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
   * Iterate on neighbors defined by `cells' over the dataset `ds'.
   * \pre cells_exist: cells!=0
   * \pre ds_exists: ds!=0
   */
  void InitWithCells(vtkIdList* cells, vtkBridgeDataSet* ds);

protected:
  vtkBridgeCellIteratorOnCellList();
  ~vtkBridgeCellIteratorOnCellList() override;

  vtkIdList* Cells; // cells traversed by the iterator.
  vtkBridgeDataSet* DataSet;
  vtkIdType Id;        // the id at current position.
  vtkBridgeCell* Cell; // cell at current position.

private:
  vtkBridgeCellIteratorOnCellList(const vtkBridgeCellIteratorOnCellList&) = delete;
  void operator=(const vtkBridgeCellIteratorOnCellList&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
