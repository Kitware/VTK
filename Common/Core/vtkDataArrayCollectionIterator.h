/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayCollectionIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArrayCollectionIterator - iterator through a vtkDataArrayCollection.
// .SECTION Description
// vtkDataArrayCollectionIterator provides an implementation of
// vtkCollectionIterator which allows the items to be retrieved with
// the proper subclass pointer type for vtkDataArrayCollection.

#ifndef __vtkDataArrayCollectionIterator_h
#define __vtkDataArrayCollectionIterator_h

#include "vtkCollectionIterator.h"

class vtkDataArray;
class vtkDataArrayCollection;

class VTK_COMMON_EXPORT vtkDataArrayCollectionIterator : public vtkCollectionIterator
{
public:
  vtkTypeMacro(vtkDataArrayCollectionIterator,vtkCollectionIterator);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDataArrayCollectionIterator* New();  
  
  // Description:
  // Set the collection over which to iterate.
  virtual void SetCollection(vtkCollection*);
  void SetCollection(vtkDataArrayCollection*);
  
  // Description:
  // Get the item at the current iterator position.  Valid only when
  // IsDoneWithTraversal() returns 1.
  vtkDataArray* GetDataArray();
  
protected:
  vtkDataArrayCollectionIterator();
  ~vtkDataArrayCollectionIterator();  
  
private:
  vtkDataArrayCollectionIterator(const vtkDataArrayCollectionIterator&); // Not implemented
  void operator=(const vtkDataArrayCollectionIterator&); // Not implemented
};

#endif
