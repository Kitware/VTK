/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractIterator.h
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
// .NAME vtkAbstractIterator - is an iterator for vtkContainer subclasses
// .SECTION Description
// vtkAbstractIterator is a superclass of all container iterators.

// .SECTION See Also
// vtkContainer

// .SECTION Caveates
// VTK Iterators are not reliable when adding or deleting elements 
// from the container. Use iterators for traversing only.

#ifndef __vtkAbstractIterator_h
#define __vtkAbstractIterator_h

#include "vtkObjectBase.h"

class vtkContainer;

template<class KeyType, class DataType>
class  vtkAbstractIterator : public vtkObjectBase
{
  friend class vtkContainer;

public:
  // Description:
  // Return the class name as a string.
  virtual const char* GetClassName() const { return "vtkAbstractIterator"; }

  // Description:
  // Retrieve the key from the iterator. For lists, the key is the
  // index of the element.
  // This method returns VTK_OK if key was retrieved correctly.
  //virtual int GetKey(KeyType&) = 0;
  
  // Description:
  // Retrieve the data from the iterator. 
  // This method returns VTK_OK if key was retrieved correctly.
  //virtual int GetData(DataType&) = 0;

  // Description:
  // Retrieve the key and data of the current element.
  // This method returns VTK_OK if key and data were retrieved correctly.
  // virtual int GetKeyAndData(KeyType&, DataType&) = 0;
  
  // Description:
  // Set the container for this iterator.
  void SetContainer(vtkContainer*);

  // Description:
  // Get the associated container.
  vtkContainer *GetContainer() { return this->Container; }

  // Description:
  // Initialize the traversal of the container. 
  // Set the iterator to the "beginning" of the container.
  //virtual void InitTraversal()=0;

  // Description:
  // Check if the iterator is at the end of the container. Return 1
  // for yes, 0 for no.
  //virtual int IsDoneWithTraversal()=0;

  // Description:
  // Increment the iterator to the next location.
  //virtual void GoToNextItem() = 0;

protected:
  vtkAbstractIterator();
  virtual ~vtkAbstractIterator();

  vtkContainer *Container;
  vtkIdType ReferenceCount;

private:
  vtkAbstractIterator(const vtkAbstractIterator&); // Not implemented
  void operator=(const vtkAbstractIterator&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkAbstractIterator.txx"
#endif

#endif





