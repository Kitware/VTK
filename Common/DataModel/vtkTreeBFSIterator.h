/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeBFSIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTreeBFSIterator
 * @brief   breadth first search iterator through a vtkTree
 *
 *
 * vtkTreeBFSIterator performs a breadth first search traversal of a tree.
 *
 * After setting up the iterator, the normal mode of operation is to
 * set up a <code>while(iter->HasNext())</code> loop, with the statement
 * <code>vtkIdType vertex = iter->Next()</code> inside the loop.
 *
 * @par Thanks:
 * Thanks to David Doria for submitting this class.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkTreeBFSIterator();
  ~vtkTreeBFSIterator() VTK_OVERRIDE;

  void Initialize() VTK_OVERRIDE;
  vtkIdType NextInternal() VTK_OVERRIDE;

  vtkTreeBFSIteratorInternals* Internals;
  vtkIntArray* Color;

  enum ColorType
  {
    WHITE,
    GRAY,
    BLACK
  };

private:
  vtkTreeBFSIterator(const vtkTreeBFSIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeBFSIterator &) VTK_DELETE_FUNCTION;
};

#endif
