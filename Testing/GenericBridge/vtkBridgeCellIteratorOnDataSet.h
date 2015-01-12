/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOnDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOnDataSet - Iterate over cells of a dataset.
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#ifndef vtkBridgeCellIteratorOnDataSet_h
#define vtkBridgeCellIteratorOnDataSet_h

#include "vtkBridgeCellIteratorStrategy.h"

class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorOnDataSet : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOnDataSet *New();
  vtkTypeMacro(vtkBridgeCellIteratorOnDataSet,
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
  // Used internally by vtkBridgeDataSet.
  // Iterate over cells of `ds' of some dimension `dim'.
  // \pre ds_exists: ds!=0
  // \pre valid_dim_range: (dim>=-1) && (dim<=3)
  void InitWithDataSet(vtkBridgeDataSet *ds,
                       int dim);

protected:
  vtkBridgeCellIteratorOnDataSet();
  virtual ~vtkBridgeCellIteratorOnDataSet();

  int Dim; // Dimension of cells over which to iterate (-1 to 3)

  vtkBridgeDataSet *DataSet; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  vtkIdType Size; // size of the structure.
  vtkBridgeCell *Cell; // cell at current position.

private:
  vtkBridgeCellIteratorOnDataSet(const vtkBridgeCellIteratorOnDataSet&); // Not implemented
  void operator=(const vtkBridgeCellIteratorOnDataSet&); // Not implemented
};

#endif
