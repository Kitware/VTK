/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeDFSIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeDFSIterator - depth first iterator through a vtkGraph
//
// .SECTION Description
// vtkTreeDFSIterator performs a depth first search traversal of a tree.
//
// First, you must set the tree on which you are going to iterate, and then
// optionally set the starting vertex and mode. The mode is either
// DISCOVER (default), in which case vertices are visited as they are first
// reached, or FINISH, in which case vertices are visited when they are
// done, i.e. all adjacent vertices have been discovered already.
//
// After setting up the iterator, the normal mode of operation is to
// set up a <code>while(iter->HasNext())</code> loop, with the statement
// <code>vtkIdType vertex = iter->Next()</code> inside the loop.


#ifndef vtkTreeDFSIterator_h
#define vtkTreeDFSIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkTreeIterator.h"

class vtkTreeDFSIteratorInternals;
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeDFSIterator : public vtkTreeIterator
{
public:
  static vtkTreeDFSIterator* New();
  vtkTypeMacro(vtkTreeDFSIterator, vtkTreeIterator);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum ModeType
    {
    DISCOVER,
    FINISH
    };
  //ETX

  // Description:
  // Set the visit mode of the iterator.  Mode can be
  //   DISCOVER (0): Order by discovery time
  //   FINISH   (1): Order by finish time
  // Default is DISCOVER.
  // Use DISCOVER for top-down algorithms where parents need to be processed before children.
  // Use FINISH for bottom-up algorithms where children need to be processed before parents.
  void SetMode(int mode);
  vtkGetMacro(Mode, int);

protected:
  vtkTreeDFSIterator();
  ~vtkTreeDFSIterator();

  virtual void Initialize();
  virtual vtkIdType NextInternal();

  int Mode;
  vtkIdType CurRoot;
  vtkTreeDFSIteratorInternals* Internals;
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
  vtkTreeDFSIterator(const vtkTreeDFSIterator &);  // Not implemented.
  void operator=(const vtkTreeDFSIterator &);        // Not implemented.
};


#endif

