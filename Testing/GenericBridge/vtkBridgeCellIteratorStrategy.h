/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorStrategy - Interface used by vtkBridgeCellIterator
// vtkBridgeCellIterator has different behaviors depending on the way it is
// initialized. vtkBridgeCellIteratorStrategy is the interface for one of those
// behaviors. Concrete classes are vtkBridgeCellIteratorOnDataSet,
// vtkBridgeCellIteratorOnDataSetBoundaries,
// vtkBridgeCellIteratorOnCellBoundaries,
// vtkBridgeCellIteratorOnCellNeighbors,
// .SECTION See Also
// vtkGenericCellIterator, vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorOnDataSet, vtkBridgeCellIteratorOnDataSetBoundaries, vtkBridgeCellIteratorOnCellBoundaries, vtkBridgeCellIteratorOnCellNeighbors

#ifndef vtkBridgeCellIteratorStrategy_h
#define vtkBridgeCellIteratorStrategy_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericCellIterator.h"

class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorStrategy : public vtkGenericCellIterator
{
public:
  vtkTypeMacro(vtkBridgeCellIteratorStrategy,vtkGenericCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create an empty cell. NOT USED
  // \post result_exists: result!=0
  vtkGenericAdaptorCell *NewCell();

protected:
  vtkBridgeCellIteratorStrategy() {}
  virtual ~vtkBridgeCellIteratorStrategy() {}

private:
  vtkBridgeCellIteratorStrategy(const vtkBridgeCellIteratorStrategy&); // Not implemented
  void operator=(const vtkBridgeCellIteratorStrategy&); // Not implemented
};

#endif
