/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataIterator - abstract superclass for composite data iterators
// .SECTION Description
// vtkCompositeDataIterator provides an interface for accessing datasets
// in a collection (vtkCompositeDataIterator). Sub-classes provide the
// actual implementation.

#ifndef __vtkCompositeDataIterator_h
#define __vtkCompositeDataIterator_h

#include "vtkObject.h"

class vtkDataObject;

class VTK_COMMON_EXPORT vtkCompositeDataIterator : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the iterator to the beginning of the collection.
  void InitTraversal() { this->GoToFirstItem(); }

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem() = 0;

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem() = 0;

  // Description:
  // Test whether the iterator is currently pointing to a valid
  // item. Returns 1 for yes, 0 for no.
  virtual int IsDoneWithTraversal() = 0;

  // Description:
  // Get the current item. Valid only when IsDoneWithTraversal()
  // returns 1.
  virtual vtkDataObject* GetCurrentDataObject() = 0;

protected:
  vtkCompositeDataIterator(); 
  virtual ~vtkCompositeDataIterator(); 

private:
  vtkCompositeDataIterator(const vtkCompositeDataIterator&);  // Not implemented.
  void operator=(const vtkCompositeDataIterator&);  // Not implemented.
};

#endif

