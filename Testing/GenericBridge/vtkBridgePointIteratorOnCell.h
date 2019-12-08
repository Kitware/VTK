/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOnCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgePointIteratorOnCell
 * @brief   Implementation of vtkGenericPointIterator.
 *
 * It iterates over the corner points of a cell.
 * @sa
 * vtkGenericPointIterator, vtkBridgeDataSet
 */

#ifndef vtkBridgePointIteratorOnCell_h
#define vtkBridgePointIteratorOnCell_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericPointIterator.h"

class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkIdList;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgePointIteratorOnCell : public vtkGenericPointIterator
{
public:
  static vtkBridgePointIteratorOnCell* New();
  vtkTypeMacro(vtkBridgePointIteratorOnCell, vtkGenericPointIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() override;

  /**
   * Is there no point at iterator position? (exit condition).
   */
  vtkTypeBool IsAtEnd() override;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_off: !IsAtEnd()
   */
  void Next() override;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \post result_exists: result!=0
   */
  double* GetPosition() override;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \pre x_exists: x!=0
   */
  void GetPosition(double x[3]) override;

  /**
   * Unique identifier for the point, could be non-contiguous
   * \pre not_off: !IsAtEnd()
   */
  vtkIdType GetId() override;

  /**
   * The iterator will iterate over the point of a cell
   * \pre cell_exists: cell!=0
   */
  void InitWithCell(vtkBridgeCell* cell);

protected:
  /**
   * Default constructor.
   */
  vtkBridgePointIteratorOnCell();

  /**
   * Destructor.
   */
  ~vtkBridgePointIteratorOnCell() override;

  vtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  vtkIdType Cursor;          // current position

  vtkIdList* PtIds; // list of points of the cell

private:
  vtkBridgePointIteratorOnCell(const vtkBridgePointIteratorOnCell&) = delete;
  void operator=(const vtkBridgePointIteratorOnCell&) = delete;
};

#endif
