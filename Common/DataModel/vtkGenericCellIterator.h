/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCellIterator - iterator used to traverse cells
// .SECTION Description
// This class (and subclasses) are used to iterate over cells. Use it
// only in conjunction with vtkGenericDataSet (i.e., the adaptor framework).
//
// Typical use is:
// <pre>
// vtkGenericDataSet *dataset;
// vtkGenericCellIterator *it = dataset->NewCellIterator(2);
// for (it->Begin(); !it->IsAtEnd(); it->Next());
//   {
//   spec=it->GetCell();
//   }
// </pre>

#ifndef __vtkGenericCellIterator_h
#define __vtkGenericCellIterator_h

#include "vtkObject.h"

class vtkGenericAdaptorCell;

class VTK_FILTERING_EXPORT vtkGenericCellIterator : public vtkObject
{
public:
  // Description:
  // Standard VTK construction and type macros.
  vtkTypeMacro(vtkGenericCellIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move iterator to first position if any (loop initialization).
  virtual void Begin() = 0;

  // Description:
  // Is the iterator at the end of traversal?
  virtual int IsAtEnd() = 0;
  
  // Description:
  // Create an empty cell. The user is responsible for deleting it.
  // \post result_exists: result!=0
  virtual vtkGenericAdaptorCell *NewCell() = 0;
  
  // Description:
  // Get the cell at current position. The cell should be instantiated
  // with the NewCell() method.
  // \pre not_at_end: !IsAtEnd()
  // \pre c_exists: c!=0
  // THREAD SAFE
  virtual void GetCell(vtkGenericAdaptorCell *c) = 0;
  
  // Description:
  // Get the cell at the current traversal position.
  // NOT THREAD SAFE
  // \pre not_at_end: !IsAtEnd()
  // \post result_exits: result!=0
  virtual vtkGenericAdaptorCell *GetCell() = 0;
  
  // Description:
  // Move the iterator to the next position in the list.
  // \pre not_at_end: !IsAtEnd()
  virtual void Next() = 0;

protected:
  vtkGenericCellIterator();
  virtual ~vtkGenericCellIterator();
  
private:
  vtkGenericCellIterator(const vtkGenericCellIterator&);  // Not implemented.
  void operator=(const vtkGenericCellIterator&);  // Not implemented.
};

#endif
