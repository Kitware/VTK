/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOnCellBoundaries.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOnCellBoundaries - Iterate over boundary cells of
// a cell.
//
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#ifndef vtkBridgeCellIteratorOnCellBoundaries_h
#define vtkBridgeCellIteratorOnCellBoundaries_h

#include "vtkBridgeCellIteratorStrategy.h"

class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIteratorOnCellBoundaries : public vtkBridgeCellIteratorStrategy
{
public:
  static vtkBridgeCellIteratorOnCellBoundaries *New();
  vtkTypeMacro(vtkBridgeCellIteratorOnCellBoundaries,
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
  // Iterate on boundary cells of a cell.
  // \pre cell_exists: cell!=0
  // \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
  void InitWithCellBoundaries(vtkBridgeCell *cell,
                              int dim);

protected:
  vtkBridgeCellIteratorOnCellBoundaries();
  virtual ~vtkBridgeCellIteratorOnCellBoundaries();

  int Dim; // Dimension of cells over which to iterate (-1 to 3)

  vtkBridgeCell *DataSetCell; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  vtkBridgeCell *Cell; // cell at current position.
  vtkIdType NumberOfFaces;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfVertices;
private:
  vtkBridgeCellIteratorOnCellBoundaries(const vtkBridgeCellIteratorOnCellBoundaries&); // Not implemented
  void operator=(const vtkBridgeCellIteratorOnCellBoundaries&); // Not implemented
};

#endif
