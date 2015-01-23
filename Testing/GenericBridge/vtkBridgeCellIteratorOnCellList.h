/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOnCellList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOnCellList - Iterate over a list of cells defined
// on a dataset.
// See InitWithCells().
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#ifndef vtkBridgeCellIteratorOnCellList_h
#define vtkBridgeCellIteratorOnCellList_h

#include "vtkBridgeCellIteratorStrategy.h"

class vtkBridgeCell;
class vtkIdList;
class vtkBridgeDataSet;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorOnCellList : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOnCellList *New();
  vtkTypeMacro(vtkBridgeCellIteratorOnCellList,
                       vtkBridgeCellIteratorStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move iterator to first position if any (loop initialization).
  void Begin();

  // Description:
  // Is there no cell at iterator position? (exit condition).
  int IsAtEnd();

  // Description:
  // Cell at current position
  // \pre not_at_end: !IsAtEnd()
  // \pre c_exists: c!=0
  // THREAD SAFE
  void GetCell(vtkGenericAdaptorCell *c);

  // Description:
  // Cell at current position.
  // NOT THREAD SAFE
  // \pre not_at_end: !IsAtEnd()
  // \post result_exits: result!=0
  vtkGenericAdaptorCell *GetCell();

  // Description:
  // Move iterator to next position. (loop progression).
  // \pre not_at_end: !IsAtEnd()
  void Next();

  // Description:
  // Used internally by vtkBridgeCell.
  // Iterate on neighbors defined by `cells' over the dataset `ds'.
  // \pre cells_exist: cells!=0
  // \pre ds_exists: ds!=0
  void InitWithCells(vtkIdList *cells,
                     vtkBridgeDataSet *ds);

protected:
  vtkBridgeCellIteratorOnCellList();
  virtual ~vtkBridgeCellIteratorOnCellList();

  vtkIdList *Cells; // cells traversed by the iterator.
  vtkBridgeDataSet *DataSet;
  vtkIdType Id; // the id at current position.
  vtkBridgeCell *Cell; // cell at current position.

private:
  vtkBridgeCellIteratorOnCellList(const vtkBridgeCellIteratorOnCellList&); // Not implemented
  void operator=(const vtkBridgeCellIteratorOnCellList&); // Not implemented
};

#endif
