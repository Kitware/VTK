/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexListIterator.h

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
// .NAME vtkVertexListIterator - Iterates all vertices in a graph.
//
// .SECTION Description
// vtkVertexListIterator iterates through all vertices in a graph.
// Create an instance of this and call graph->GetVertices(it) to initialize
// this iterator. You may alternately call SetGraph() to initialize the
// iterator.
//
// .SECTION See Also
// vtkGraph

#ifndef __vtkVertexListIterator_h
#define __vtkVertexListIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkVertexListIterator : public vtkObject
{
public:
  static vtkVertexListIterator *New();
  vtkTypeMacro(vtkVertexListIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Setup the iterator with a graph.
  virtual void SetGraph(vtkGraph *graph);

  // Description:
  // Get the graph associated with this iterator.
  vtkGetObjectMacro(Graph, vtkGraph);

  // Description:
  // Returns the next edge in the graph.
  vtkIdType Next()
  {
    vtkIdType v = this->Current;
    ++this->Current;
    return v;
  }

  // Description:
  // Whether this iterator has more edges.
  bool HasNext()
  {
    return this->Current != this->End;
  }

protected:
  vtkVertexListIterator();
  ~vtkVertexListIterator();

  vtkGraph *Graph;
  vtkIdType  Current;
  vtkIdType  End;

private:
  vtkVertexListIterator(const vtkVertexListIterator&);  // Not implemented.
  void operator=(const vtkVertexListIterator&);  // Not implemented.
};

#endif
