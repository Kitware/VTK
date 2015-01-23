/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchyIterator.h

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
// .NAME vtkLabelHierarchyIterator - iterator over vtkLabelHierarchy
//
// .SECTION Description
// Abstract superclass for iterators over vtkLabelHierarchy.

#ifndef vtkLabelHierarchyIterator_h
#define vtkLabelHierarchyIterator_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkStdString.h" // for std string
#include "vtkUnicodeString.h" // for unicode string

class vtkIdTypeArray;
class vtkLabelHierarchy;
class vtkPolyData;

class VTKRENDERINGLABEL_EXPORT vtkLabelHierarchyIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkLabelHierarchyIterator,vtkObject);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Initializes the iterator. lastLabels is an array holding labels
  // which should be traversed before any other labels in the hierarchy.
  // This could include labels placed during a previous rendering or
  // a label located under the mouse pointer. You may pass a null pointer.
  virtual void Begin( vtkIdTypeArray* ) { }

  // Description:
  // Advance the iterator.
  virtual void Next() { }

  // Description:
  // Returns true if the iterator is at the end.
  virtual bool IsAtEnd() { return true; }

  // Description:
  // Retrieves the current label location.
  virtual void GetPoint( double x[3] );

  // Description:
  // Retrieves the current label size.
  virtual void GetSize( double sz[2] );

  // Description:
  // Retrieves the current label maximum width in world coordinates.
  virtual void GetBoundedSize( double sz[2] );

  // Description:
  // Retrieves the current label type.
  virtual int GetType();

  // Description:
  // Retrieves the current label string.
  virtual vtkStdString GetLabel();

  // Description:
  // Retrieves the current label as a unicode string.
  virtual vtkUnicodeString GetUnicodeLabel();

  // Description:
  // Retrieves the current label orientation.
  virtual double GetOrientation();

  // Description:
  // Retrieves the current label id.
  virtual vtkIdType GetLabelId() { return -1; }

  // Description:
  // Get the label hierarchy associated with the current label.
  vtkGetObjectMacro(Hierarchy, vtkLabelHierarchy);

  // Description:
  // Sets a polydata to fill with geometry representing
  // the bounding boxes of the traversed octree nodes.
  virtual void SetTraversedBounds( vtkPolyData* );

  // Description:
  // Retrieve the coordinates of the center of the current hierarchy node
  // and the size of the node.
  // Nodes are n-cubes, so the size is the length of any edge of the cube.
  // This is used by BoxNode().
  virtual void GetNodeGeometry( double ctr[3], double& size ) = 0;

  // Description:
  // Add a representation to TraversedBounds for the current octree node.
  // This should be called by subclasses inside Next().
  // Does nothing if TraversedBounds is NULL.
  virtual void BoxNode();

  // Description:
  // Add a representation for all existing octree nodes to the specified polydata.
  // This is equivalent to setting TraversedBounds, iterating over the entire hierarchy,
  // and then resetting TraversedBounds to its original value.
  virtual void BoxAllNodes( vtkPolyData* );

  // Description:
  // Set/get whether all nodes in the hierarchy should be added to the TraversedBounds
  // polydata or only those traversed.
  // When non-zero, all nodes will be added.
  // By default, AllBounds is 0.
  vtkSetMacro(AllBounds,int);
  vtkGetMacro(AllBounds,int);

protected:
  vtkLabelHierarchyIterator();
  virtual ~vtkLabelHierarchyIterator();

  void BoxNodeInternal3( const double* ctr, double sz );
  void BoxNodeInternal2( const double* ctr, double sz );

  // Description:
  // The hierarchy being traversed by this iterator.
  virtual void SetHierarchy( vtkLabelHierarchy* h );

  vtkLabelHierarchy* Hierarchy;
  vtkPolyData* TraversedBounds;
  double BoundsFactor;
  int AllBounds;
  int AllBoundsRecorded;

private:
  vtkLabelHierarchyIterator( const vtkLabelHierarchyIterator& ); // Not implemented.
  void operator = ( const vtkLabelHierarchyIterator& ); // Not implemented.
};

#endif // vtkLabelHierarchyIterator_h
