/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataIterator - iterator to access datasets in a 
// vtkHierarchicalDataIterator
// .SECTION Description
//  Iterates over leaves in a vtkHierarchicalDataSet. User can control whether
//  the levels are iterated in ascending (0-higher) or descending (highest-0)
//  order using the flag AscendingLevels. 
//  Note that the order of datasets within a level is not affected by this flag.
// .SECTION See Also
// vtkMultiGroupDataIterator

#ifndef __vtkHierarchicalDataIterator_h
#define __vtkHierarchicalDataIterator_h

#include "vtkMultiGroupDataIterator.h"

class vtkHierarchicalDataSet;
class vtkHierarchicalDataIteratorInternal;

class VTK_FILTERING_EXPORT vtkHierarchicalDataIterator : public vtkMultiGroupDataIterator
{
public:
  static vtkHierarchicalDataIterator *New();

  vtkTypeRevisionMacro(vtkHierarchicalDataIterator,vtkMultiGroupDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the data object to iterator over.
  vtkHierarchicalDataSet* GetDataSet();

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
  // Get the information object associated with the current
  // data object.
  virtual vtkInformation* GetCurrentInformationObject();

  // Description:
  // Get the current item. Valid only when IsDoneWithTraversal()
  // returns 1.
  virtual vtkDataObject* GetCurrentDataObject();

  // Description:
  // When set to 1, the levels are iterated in ascending order, otherwise, they
  // are iterated in descending order starting with the highest level available
  // when InitTraversal() or GoToFirstItem() is called.
  // Set to 1 by default.
  vtkSetMacro(AscendingLevels, int);
  vtkGetMacro(AscendingLevels, int);

  // Description:
  // Get current level number.
  unsigned int GetCurrentLevel();

  // Description:
  // Get current dataset index within the current level.
  unsigned int GetCurrentIndex();

protected:
  vtkHierarchicalDataIterator(); 
  virtual ~vtkHierarchicalDataIterator(); 

private:
  class vtkInternal;
  vtkInternal* Internal;

  int AscendingLevels;
private:
  vtkHierarchicalDataIterator(const vtkHierarchicalDataIterator&);  // Not implemented.
  void operator=(const vtkHierarchicalDataIterator&);  // Not implemented.
};

#endif

