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
// .NAME vtkBridgePointIteratorOnDataSet - Implementation of vtkGenericPointIterator.
// .SECTION Description
// It iterates over the points of a dataset (can be corner points of cells or
// isolated points)
// .SECTION See Also
// vtkGenericPointIterator, vtkBridgeDataSet

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move iterator to first position if any (loop initialization).
  void Begin();

  // Description:
  // Is there no point at iterator position? (exit condition).
  int IsAtEnd();

  // Description:
  // Move iterator to next position. (loop progression).
  // \pre not_off: !IsAtEnd()
  void Next();

  // Description:
  // Point at iterator position.
  // \pre not_off: !IsAtEnd()
  // \post result_exists: result!=0
  double *GetPosition();

  // Description:
  // Point at iterator position.
  // \pre not_off: !IsAtEnd()
  // \pre x_exists: x!=0
  void GetPosition(double x[3]);

  // Description:
  // Unique identifier for the point, could be non-contiguous
  // \pre not_off: !IsAtEnd()
  vtkIdType GetId();

  // Description:
  // Used internally by vtkBridgeDataSet.
  // Iterate over points of `ds'.
  // \pre ds_exists: ds!=0
  void InitWithDataSet(vtkBridgeDataSet *ds);

protected:
  // Description:
  // Default constructor.
  vtkBridgePointIteratorOnDataSet();

  // Description:
  // Destructor.
  virtual ~vtkBridgePointIteratorOnDataSet();

  vtkBridgeDataSet *DataSet; // the structure on which the objet iterates.
  vtkIdType Id; // the id at current position.
  int Size; // size of the structure.

private:
  vtkBridgePointIteratorOnDataSet(const vtkBridgePointIteratorOnDataSet&); // Not implemented
  void operator=(const vtkBridgePointIteratorOnDataSet&); // Not implemented
};

#endif
