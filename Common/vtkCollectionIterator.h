/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectionIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCollectionIterator - iterator through a vtkCollection.
// .SECTION Description
// vtkCollectionIterator provides an alternative way to traverse
// through the objects in a vtkCollection.  Unlike the collection's
// built in interface, this allows multiple iterators to
// simultaneously traverse the collection.  If items are removed from
// the collection, only the iterators currently pointing to those
// items are invalidated.  Other iterators will still continue to
// function normally.

#ifndef __vtkCollectionIterator_h
#define __vtkCollectionIterator_h

#include "vtkObject.h"

class vtkCollection;
class vtkCollectionElement;

class VTK_COMMON_EXPORT vtkCollectionIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkCollectionIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCollectionIterator* New();
  
  // Description:
  // Set/Get the collection over which to iterate.
  virtual void SetCollection(vtkCollection*);
  vtkGetObjectMacro(Collection, vtkCollection);
  
  // Description:
  // Position the iterator at the first item in the collection.
  void InitTraversal() { this->GoToFirstItem(); }
  
  // Description:
  // Position the iterator at the first item in the collection.
  void GoToFirstItem();
  
  // Description:
  // Move the iterator to the next item in the collection.
  void GoToNextItem();
  
  // Description:
  // Test whether the iterator is currently positioned at a valid item.
  // Returns 1 for yes, 0 for no.
  int IsDoneWithTraversal();

  // Description:
  // Get the item at the current iterator position.  Valid only when
  // IsDoneWithTraversal() returns 1.
  vtkObject* GetCurrentObject();

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# define GetObjectA GetObject
# define GetObjectW GetObject
#endif

  // Description:
  // @deprecated Replaced by vtkCollectionIterator::GetCurrentObject() as
  // of VTK 5.0.
  VTK_LEGACY(vtkObject* GetObject());

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetObjectW
# undef GetObjectA
  //BTX
  VTK_LEGACY(vtkObject* GetObjectA());
  VTK_LEGACY(vtkObject* GetObjectW());
  //ETX
#endif

protected:
  vtkCollectionIterator();
  ~vtkCollectionIterator();
  
  // The collection over which we are iterating.
  vtkCollection* Collection;
  
  // The current iterator position.
  vtkCollectionElement* Element;

  vtkObject* GetObjectInternal();
private:
  vtkCollectionIterator(const vtkCollectionIterator&); // Not implemented
  void operator=(const vtkCollectionIterator&); // Not implemented
};

#endif
