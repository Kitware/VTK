/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataIterator.h
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
// .NAME vtkHierarchicalDataIterator - iterator to access datasets in a vtkHierarchicalDataIterator
// .SECTION Description
// vtkMultiBlockDataIterator is a concrete implementation of 
// vtkCompositeDataIterator for vtkMultiBlockDataSet. It allows flat
// and forward access to the datasets in the hierchical dataset.

#ifndef __vtkHierarchicalDataIterator_h
#define __vtkHierarchicalDataIterator_h

#include "vtkCompositeDataIterator.h"

class vtkHierarchicalDataSet;
class vtkHierarchicalDataIteratorInternal;

class VTK_COMMON_EXPORT vtkHierarchicalDataIterator : public vtkCompositeDataIterator
{
public:
  static vtkHierarchicalDataIterator *New();

  vtkTypeRevisionMacro(vtkHierarchicalDataIterator,vtkCompositeDataIterator);
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
  void SetDataSet(vtkHierarchicalDataSet* dataset);
  vtkGetObjectMacro(DataSet, vtkHierarchicalDataSet);

protected:
  vtkHierarchicalDataIterator(); 
  virtual ~vtkHierarchicalDataIterator(); 

  vtkHierarchicalDataSet* DataSet;
  vtkHierarchicalDataIteratorInternal* Internal;

private:
  vtkHierarchicalDataIterator(const vtkHierarchicalDataIterator&);  // Not implemented.
  void operator=(const vtkHierarchicalDataIterator&);  // Not implemented.
};

#endif

