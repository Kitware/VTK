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

#ifndef __vtkLabelHierarchyIterator_h
#define __vtkLabelHierarchyIterator_h


#include "vtkObject.h"

class vtkIdTypeArray;
class vtkLabelHierarchy;
class vtkPolyData;

class VTK_RENDERING_EXPORT vtkLabelHierarchyIterator : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkLabelHierarchyIterator,vtkObject);

  // Description:
  // Initializes the iterator. lastLabels is an array holding the previous
  // list of labels.
  virtual void Begin( vtkIdTypeArray* lastLabels ) { }

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
  // Retrieves the current label type.
  virtual int GetType();

  // Description:
  // Retrieves the current label id.
  virtual vtkIdType GetLabelId() { return -1; }

  // Description:
  // Sets a polydata to fill with geometry representing
  // the bounding boxes of the traversed octree nodes.
  virtual void SetTraversedBounds( vtkPolyData* ) { }

protected:
  vtkLabelHierarchyIterator();
  virtual ~vtkLabelHierarchyIterator();

  // Description:
  // The hierarchy being traversed by this iterator.
  virtual void SetHierarchy( vtkLabelHierarchy* h );
  vtkLabelHierarchy* Hierarchy;
};

#endif // __vtkLabelHierarchyIterator_h
