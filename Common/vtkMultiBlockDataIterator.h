/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataIterator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockDataIterator - iterator to access datasets in a vtkMultiBlockDataSet
// .SECTION Description
// vtkMultiBlockDataIterator is a concrete implementation of 
// vtkCompositeDataIterator for vtkMultiBlockDataSet.

#ifndef __vtkMultiBlockDataIterator_h
#define __vtkMultiBlockDataIterator_h

#include "vtkCompositeDataIterator.h"

class vtkMultiBlockDataSet;
class vtkMultiBlockDataIteratorInternal;

class VTK_COMMON_EXPORT vtkMultiBlockDataIterator : public vtkCompositeDataIterator
{
public:
  static vtkMultiBlockDataIterator *New();

  vtkTypeRevisionMacro(vtkMultiBlockDataIterator,vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem();

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem();

  // Description:
  // Test whether the iterator is currently pointing to a valid
  // item. Returns 1 for yes, 0 for no.
  virtual int IsDoneWithTraversal();

  // Description:
  // Get the current item. Valid only when IsDoneWithTraversal()
  // returns 1.
  virtual vtkDataObject* GetCurrentDataObject();

  // Description:
  // Set the data object to iterator over.
  void SetDataSet(vtkMultiBlockDataSet* dataset);
  vtkGetObjectMacro(DataSet, vtkMultiBlockDataSet);

protected:
  vtkMultiBlockDataIterator(); 
  virtual ~vtkMultiBlockDataIterator(); 

  vtkMultiBlockDataSet* DataSet;
  vtkMultiBlockDataIteratorInternal* Internal;

private:
  vtkMultiBlockDataIterator(const vtkMultiBlockDataIterator&);  // Not implemented.
  void operator=(const vtkMultiBlockDataIterator&);  // Not implemented.
};

#endif

