/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgeCellIterator
 * @brief   Implementation of vtkGenericCellIterator.
 * It is just an example that show how to implement the Generic API. It is also
 * used for testing and evaluating the Generic framework.
 * @sa
 * vtkGenericCellIterator, vtkBridgeDataSet
*/

#ifndef vtkBridgeCellIterator_h
#define vtkBridgeCellIterator_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericCellIterator.h"

class vtkBridgeCell;
class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;
class vtkBridgeDataSet;
class vtkPoints;

class vtkBridgeCellIteratorStrategy;
class vtkBridgeCellIteratorOnDataSet;
class vtkBridgeCellIteratorOne;
class vtkBridgeCellIteratorOnCellBoundaries;
class vtkBridgeCellIteratorOnCellList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCellIterator : public vtkGenericCellIterator
{
public:
  static vtkBridgeCellIterator *New();
  vtkTypeMacro(vtkBridgeCellIterator,vtkGenericCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() VTK_OVERRIDE;

  /**
   * Is there no cell at iterator position? (exit condition).
   */
  int IsAtEnd() VTK_OVERRIDE;

  /**
   * Create an empty cell.
   * \post result_exists: result!=0
   */
  vtkGenericAdaptorCell *NewCell() VTK_OVERRIDE;

  /**
   * Cell at current position
   * \pre not_at_end: !IsAtEnd()
   * \pre c_exists: c!=0
   * THREAD SAFE
   */
  void GetCell(vtkGenericAdaptorCell *c) VTK_OVERRIDE;

  /**
   * Cell at current position.
   * NOT THREAD SAFE
   * \pre not_at_end: !IsAtEnd()
   * \post result_exits: result!=0
   */
  vtkGenericAdaptorCell *GetCell() VTK_OVERRIDE;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_at_end: !IsAtEnd()
   */
  void Next() VTK_OVERRIDE;

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate over cells of `ds' of some dimension `dim'.
   * \pre ds_exists: ds!=0
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   */
  void InitWithDataSet(vtkBridgeDataSet *ds,
                       int dim);

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate over boundary cells of `ds' of some dimension `dim'.
   * \pre ds_exists: ds!=0
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   */
  void InitWithDataSetBoundaries(vtkBridgeDataSet *ds,
                                 int dim,
                                 int exterior_only);

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate on one cell `id' of `ds'.
   * \pre ds_exists: ds!=0
   * \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
   */
  void InitWithOneCell(vtkBridgeDataSet *ds,
                       vtkIdType cellid);

  /**
   * Used internally by vtkBridgeCell.
   * Iterate on one cell `c'.
   * \pre c_exists: c!=0
   */
  void InitWithOneCell(vtkBridgeCell *c);

  /**
   * Used internally by vtkBridgeCell.
   * Iterate on boundary cells of a cell.
   * \pre cell_exists: cell!=0
   * \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
   */
  void InitWithCellBoundaries(vtkBridgeCell *cell,
                              int dim);

  /**
   * Used internally by vtkBridgeCell.
   * Iterate on neighbors defined by `cells' over the dataset `ds'.
   * \pre cells_exist: cells!=0
   * \pre ds_exists: ds!=0
   */
  void InitWithCells(vtkIdList *cells,
                     vtkBridgeDataSet *ds);

  /**
   * Used internally by vtkBridgeCell.
   * Iterate on a boundary cell (defined by its points `pts' with coordinates
   * `coords', dimension `dim' and unique id `cellid') of a cell.
   * \pre coords_exist: coords!=0
   * \pre pts_exist: pts!=0
   * \pre valid_dim: dim>=0 && dim<=2
   * \pre valid_points: pts->GetNumberOfIds()>dim
   */
  void InitWithPoints(vtkPoints *coords,
                      vtkIdList *pts,
                      int dim,
                      vtkIdType cellid);

protected:
  vtkBridgeCellIterator();
  ~vtkBridgeCellIterator() VTK_OVERRIDE;

  vtkBridgeCellIteratorStrategy *CurrentIterator;
  vtkBridgeCellIteratorOnDataSet *IteratorOnDataSet;
  vtkBridgeCellIteratorOne *IteratorOneCell;
  vtkBridgeCellIteratorOnCellBoundaries * IteratorOnCellBoundaries;
  vtkBridgeCellIteratorOnCellList *IteratorOnCellList;

  vtkBridgeDataSet *DataSet; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  int OneCell; // Is in one cell mode?
  vtkIdType Size; // size of the structure.
  vtkBridgeCell *Cell; // cell at current position.

private:
  vtkBridgeCellIterator(const vtkBridgeCellIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBridgeCellIterator&) VTK_DELETE_FUNCTION;
};

#endif
