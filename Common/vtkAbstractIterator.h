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

#include "vtkContainer.h"

#ifndef __vtkAbstractIterator_h
#define __vtkAbstractIterator_h

template<class KeyType, class DataType>
class VTK_COMMON_EXPORT vtkAbstractIterator 
{
  friend class vtkContainer;

public:
  // Description:
  // Return the class name as a string.
  virtual const char* GetClassName() { return "vtkAbstractIterator"; }

  // Description:
  // The counterpart to New(), Delete simply calls UnRegister to lower the
  // reference count by one. It is no different than calling UnRegister.
  void Delete() { this->UnRegister(); }
  
  // Description:
  // Increase the reference count of this container.
  void Register();
  void Register(vtkObject *) { this->Register(); }
  
  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  void UnRegister();
  void UnRegister(vtkObject *) { this->UnRegister(); }

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
  // Check if the iterator is at the end of the container. Return 
  // VTK_OK if it is.
  //virtual int IsDoneWithTraversal()=0;

  // Description:
  // Increment the iterator to the next location.
  // Return VTK_OK if everything is ok.
  //virtual int GoToNextItem() = 0;

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





