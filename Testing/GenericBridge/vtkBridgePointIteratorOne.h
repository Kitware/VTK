/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOne.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgePointIteratorOne
 * @brief   Iterate over one point of a dataset.
 * @sa
 * vtkGenericPointIterator, vtkBridgeDataSet
 */

#ifndef vtkBridgePointIteratorOne_h
#define vtkBridgePointIteratorOne_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericPointIterator.h"

class vtkBridgeDataSet;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgePointIteratorOne : public vtkGenericPointIterator
{
public:
  static vtkBridgePointIteratorOne* New();
  vtkTypeMacro(vtkBridgePointIteratorOne, vtkGenericPointIterator);
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
   * Used internally by vtkBridgeDataSet.
   * Iterate over one point of identifier `id' on dataset `ds'.
   * \pre ds_can_be_null: ds!=0 || ds==0
   * \pre valid_id: vtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
   */
  void InitWithOnePoint(vtkBridgeDataSet* ds, vtkIdType id);

protected:
  /**
   * Default constructor.
   */
  vtkBridgePointIteratorOne();

  /**
   * Destructor.
   */
  ~vtkBridgePointIteratorOne() override;

  vtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  vtkIdType Id;              // the id at current position.
  int cIsAtEnd;

private:
  vtkBridgePointIteratorOne(const vtkBridgePointIteratorOne&) = delete;
  void operator=(const vtkBridgePointIteratorOne&) = delete;
};

#endif
