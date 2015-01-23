/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeBFSIterator.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTreeBFSIterator - breadth first search iterator through a vtkTree
//
// .SECTION Description
// vtkTreeBFSIterator performs a breadth first search traversal of a tree.
//
// After setting up the iterator, the normal mode of operation is to
// set up a <code>while(iter->HasNext())</code> loop, with the statement
// <code>vtkIdType vertex = iter->Next()</code> inside the loop.
//
// .SECTION Thanks
// Thanks to David Doria for submitting this class.

#ifndef vtkTreeBFSIterator_h
#define vtkTreeBFSIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkTreeIterator.h"

class vtkTreeBFSIteratorInternals;
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeBFSIterator : public vtkTreeIterator
{
public:
  static vtkTreeBFSIterator* New();
  vtkTypeMacro(vtkTreeBFSIterator, vtkTreeIterator);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTreeBFSIterator();
  ~vtkTreeBFSIterator();

  virtual void Initialize();
  virtual vtkIdType NextInternal();

  vtkTreeBFSIteratorInternals* Internals;
  vtkIntArray* Color;

  //BTX
  enum ColorType
    {
    WHITE,
    GRAY,
    BLACK
    };
  //ETX

private:
  vtkTreeBFSIterator(const vtkTreeBFSIterator &);  // Not implemented.
  void operator=(const vtkTreeBFSIterator &);        // Not implemented.
};

#endif
