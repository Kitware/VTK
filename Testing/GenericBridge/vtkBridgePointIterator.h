/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgePointIterator
 * @brief   Implementation of vtkGenericPointIterator.
 *
 * It is just an example that show how to implement the Generic API. It is also
 * used for testing and evaluating the Generic framework.
 * @sa
 * vtkGenericPointIterator, vtkBridgeDataSet
*/

#ifndef vtkBridgePointIterator_h
#define vtkBridgePointIterator_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericPointIterator.h"

class vtkBridgeDataSet;
class vtkBridgeCell;
class vtkBridgePointIteratorOnDataSet;
class vtkBridgePointIteratorOne;
class vtkBridgePointIteratorOnCell;


class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgePointIterator : public vtkGenericPointIterator
{
public:
  static vtkBridgePointIterator *New();
  vtkTypeMacro(vtkBridgePointIterator,vtkGenericPointIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin();

  /**
   * Is there no point at iterator position? (exit condition).
   */
  int IsAtEnd();

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_off: !IsAtEnd()
   */
  void Next();

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \post result_exists: result!=0
   */
  double *GetPosition();

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \pre x_exists: x!=0
   */
  void GetPosition(double x[3]);

  /**
   * Unique identifier for the point, could be non-contiguous
   * \pre not_off: !IsAtEnd()
   */
  vtkIdType GetId();

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate over points of `ds'.
   * \pre ds_exists: ds!=0
   */
  void InitWithDataSet(vtkBridgeDataSet *ds);

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate over one point of identifier `id' on dataset `ds'.
   * \pre ds_can_be_null: ds!=0 || ds==0
   * \pre valid_id: vtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
   */
  void InitWithOnePoint(vtkBridgeDataSet *ds,
                        vtkIdType id);

  /**
   * The iterator will iterate over the point of a cell
   * \pre cell_exists: cell!=0
   */
  void InitWithCell(vtkBridgeCell *cell);

protected:
  /**
   * Default constructor.
   */
  vtkBridgePointIterator();

  /**
   * Destructor.
   */
  virtual ~vtkBridgePointIterator();

  vtkGenericPointIterator *CurrentIterator;
  vtkBridgePointIteratorOnDataSet *IteratorOnDataSet;
  vtkBridgePointIteratorOne *IteratorOne;
  vtkBridgePointIteratorOnCell *IteratorOnCell;

private:
  vtkBridgePointIterator(const vtkBridgePointIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBridgePointIterator&) VTK_DELETE_FUNCTION;
};

#endif
