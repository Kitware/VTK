/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOne.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOne - Iterate over one cell only of a dataset.
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#ifndef __vtkBridgeCellIteratorOne_h
#define __vtkBridgeCellIteratorOne_h

#include "vtkBridgeCellIteratorStrategy.h"

class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;
class vtkPoints;
class vtkCell;

class VTK_BRIDGE_EXPORT vtkBridgeCellIteratorOne : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOne *New();
  vtkTypeMacro(vtkBridgeCellIteratorOne,
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
  // Iterate on one cell `id' of `ds'.
  // \pre ds_exists: ds!=0
  // \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
  void InitWithOneCell(vtkBridgeDataSet *ds,
                       vtkIdType cellid);
  
  // Description:
  // Used internally by vtkBridgeCell.
  // Iterate on one cell `c'.
  // \pre c_exists: c!=0
  void InitWithOneCell(vtkBridgeCell *c);
  
  // Description:
  // Used internally by vtkBridgeCell.
  // Iterate on a boundary cell (defined by its points `pts' with coordinates
  // `coords', dimension `dim' and unique id `cellid') of a cell.
  // \pre coords_exist: coords!=0
  // \pre pts_exist: pts!=0
  // \pre valid_dim: dim>=0 && dim<=2
  // \pre valid_points: pts->GetNumberOfIds()>dim
  void InitWithPoints(vtkPoints *coords,
                      vtkIdList *pts,
                      int dim,
                      vtkIdType cellid);
  
protected:
  vtkBridgeCellIteratorOne();
  virtual ~vtkBridgeCellIteratorOne();
  
  int cIsAtEnd;
  vtkBridgeDataSet *DataSet; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  vtkBridgeCell *Cell; // cell at current position.
  vtkCell *InternalCell;
  
private:
  vtkBridgeCellIteratorOne(const vtkBridgeCellIteratorOne&); // Not implemented
  void operator=(const vtkBridgeCellIteratorOne&); // Not implemented
};

#endif
