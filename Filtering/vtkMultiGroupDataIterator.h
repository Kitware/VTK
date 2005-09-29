/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataIterator - iterator to access datasets in a vtkMultiGroupDataIterator
// .SECTION Description
// vtkMultiGroupDataIterator is a concrete implementation of 
// vtkCompositeDataIterator for vtkMultiGroupDataSet. It allows flat
// and forward access to the datasets in the hierchical dataset.

#ifndef __vtkMultiGroupDataIterator_h
#define __vtkMultiGroupDataIterator_h

#include "vtkCompositeDataIterator.h"

class vtkMultiGroupDataSet;
class vtkMultiGroupDataIteratorInternal;

class VTK_FILTERING_EXPORT vtkMultiGroupDataIterator : public vtkCompositeDataIterator
{
public:
  static vtkMultiGroupDataIterator *New();

  vtkTypeRevisionMacro(vtkMultiGroupDataIterator,vtkCompositeDataIterator);
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
  void SetDataSet(vtkMultiGroupDataSet* dataset);
  vtkMultiGroupDataSet* GetDataSet()
    {
      return this->DataSet;
    }

protected:
  vtkMultiGroupDataIterator(); 
  virtual ~vtkMultiGroupDataIterator(); 

  void GoToNextNonEmptyGroup();

  vtkMultiGroupDataSet* DataSet;
  vtkMultiGroupDataIteratorInternal* Internal;

private:
  vtkMultiGroupDataIterator(const vtkMultiGroupDataIterator&);  // Not implemented.
  void operator=(const vtkMultiGroupDataIterator&);  // Not implemented.
};

#endif

