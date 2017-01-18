/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOnDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgePointIteratorOnDataSet
 * @brief   Implementation of vtkGenericPointIterator.
 *
 * It iterates over the points of a dataset (can be corner points of cells or
 * isolated points)
 * @sa
 * vtkGenericPointIterator, vtkBridgeDataSet
*/

#ifndef vtkBridgePointIteratorOnDataSet_h
#define vtkBridgePointIteratorOnDataSet_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericPointIterator.h"

class vtkBridgeDataSet;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgePointIteratorOnDataSet : public vtkGenericPointIterator
{
public:
  static vtkBridgePointIteratorOnDataSet *New();
  vtkTypeMacro(vtkBridgePointIteratorOnDataSet,vtkGenericPointIterator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() VTK_OVERRIDE;

  /**
   * Is there no point at iterator position? (exit condition).
   */
  int IsAtEnd() VTK_OVERRIDE;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_off: !IsAtEnd()
   */
  void Next() VTK_OVERRIDE;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \post result_exists: result!=0
   */
  double *GetPosition() VTK_OVERRIDE;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \pre x_exists: x!=0
   */
  void GetPosition(double x[3]) VTK_OVERRIDE;

  /**
   * Unique identifier for the point, could be non-contiguous
   * \pre not_off: !IsAtEnd()
   */
  vtkIdType GetId() VTK_OVERRIDE;

  /**
   * Used internally by vtkBridgeDataSet.
   * Iterate over points of `ds'.
   * \pre ds_exists: ds!=0
   */
  void InitWithDataSet(vtkBridgeDataSet *ds);

protected:
  /**
   * Default constructor.
   */
  vtkBridgePointIteratorOnDataSet();

  /**
   * Destructor.
   */
  ~vtkBridgePointIteratorOnDataSet() VTK_OVERRIDE;

  vtkBridgeDataSet *DataSet; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  int Size; // size of the structure.

private:
  vtkBridgePointIteratorOnDataSet(const vtkBridgePointIteratorOnDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBridgePointIteratorOnDataSet&) VTK_DELETE_FUNCTION;
};

#endif
