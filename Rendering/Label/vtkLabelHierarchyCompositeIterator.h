/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchyCompositeIterator.h

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
// .NAME vtkLabelHierarchyCompositeIterator - Iterator over sub-iterators
//
// .SECTION Description
// Iterates over child iterators in a round-robin order. Each iterator may
// have its own count, which is the number of times it is repeated until
// moving to the next iterator.
// 
// For example, if you initialize the iterator with
// <pre>
// it->AddIterator(A, 1);
// it->AddIterator(B, 3);
// </pre>
// The order of iterators will be A,B,B,B,A,B,B,B,...

#ifndef __vtkLabelHierarchyCompositeIterator_h
#define __vtkLabelHierarchyCompositeIterator_h


#include "vtkLabelHierarchyIterator.h"

class vtkIdTypeArray;
class vtkLabelHierarchy;
class vtkPolyData;

class VTK_RENDERING_EXPORT vtkLabelHierarchyCompositeIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchyCompositeIterator, vtkLabelHierarchyIterator);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  static vtkLabelHierarchyCompositeIterator* New();

  // Description:
  // Adds a label iterator to this composite iterator.
  // The second optional argument is the number of times to repeat the iterator
  // before moving to the next one round-robin style. Default is 1.
  virtual void AddIterator(vtkLabelHierarchyIterator* it)
    { this->AddIterator(it, 1); }
  virtual void AddIterator(vtkLabelHierarchyIterator* it, int count);

  // Description:
  // Remove all iterators from this composite iterator.
  virtual void ClearIterators();

  // Description:
  // Initializes the iterator. lastLabels is an array holding labels
  // which should be traversed before any other labels in the hierarchy.
  // This could include labels placed during a previous rendering or
  // a label located under the mouse pointer. You may pass a null pointer.
  virtual void Begin( vtkIdTypeArray* );

  // Description:
  // Advance the iterator.
  virtual void Next();

  // Description:
  // Returns true if the iterator is at the end.
  virtual bool IsAtEnd();

  // Description:
  // Retrieves the current label id.
  virtual vtkIdType GetLabelId();

  // Description:
  // Retrieve the current label hierarchy.
  virtual vtkLabelHierarchy* GetHierarchy();

  // Description:
  // Retrieve the coordinates of the center of the current hierarchy node
  // and the size of the node.
  // Nodes are n-cubes, so the size is the length of any edge of the cube.
  // This is used by BoxNode().
  virtual void GetNodeGeometry( double ctr[3], double& size );

  // Description:
  // Not implemented.
  virtual void BoxNode() { }

  // Description:
  // Not implemented.
  virtual void BoxAllNodes( vtkPolyData* ) { }

protected:
  vtkLabelHierarchyCompositeIterator();
  virtual ~vtkLabelHierarchyCompositeIterator();

  //BTX
  class Internal;
  Internal* Implementation;
  //ETX

private:
  vtkLabelHierarchyCompositeIterator( const vtkLabelHierarchyCompositeIterator& ); // Not implemented.
  void operator = ( const vtkLabelHierarchyCompositeIterator& ); // Not implemented.
};

#endif // __vtkLabelHierarchyCompositeIterator_h
