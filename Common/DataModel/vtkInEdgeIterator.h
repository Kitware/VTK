/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInEdgeIterator.h

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
// .NAME vtkInEdgeIterator - Iterates through all incoming edges to a vertex.
//
// .SECTION Description
// vtkInEdgeIterator iterates through all edges whose target is a particular
// vertex. Instantiate this class directly and call Initialize() to traverse
// the vertex of a graph. Alternately, use GetInEdges() on the graph to
// initialize the iterator. it->Next() returns a vtkInEdgeType structure,
// which contains Id, the edge's id, and Source, the edge's source vertex.
//
// .SECTION See Also
// vtkGraph vtkOutEdgeIterator

#ifndef __vtkInEdgeIterator_h
#define __vtkInEdgeIterator_h

#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTK_FILTERING_EXPORT vtkInEdgeIterator : public vtkObject
{
public:
  static vtkInEdgeIterator *New();
  vtkTypeMacro(vtkInEdgeIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the iterator with a graph and vertex.
  void Initialize(vtkGraph *g, vtkIdType v);

  // Description:
  // Get the graph and vertex associated with this iterator.
  vtkGetObjectMacro(Graph, vtkGraph);
  vtkGetMacro(Vertex, vtkIdType);
  
  //BTX
  // Description:
  // Returns the next edge in the graph.
  inline vtkInEdgeType Next()
  {
    vtkInEdgeType e = *this->Current;
    ++this->Current;
    return e;
  }
  //ETX

  // Description:
  // Just like Next(), but
  // returns heavy-weight vtkGraphEdge object instead of
  // the vtkEdgeType struct, for use with wrappers.
  // The graph edge is owned by this iterator, and changes
  // after each call to NextGraphEdge().
  vtkGraphEdge *NextGraphEdge();

  // Description:
  // Whether this iterator has more edges.
  bool HasNext()
  {
    return this->Current != this->End;
  }

protected:
  vtkInEdgeIterator();
  ~vtkInEdgeIterator();

  // Description:
  // Protected method for setting the graph used
  // by Initialize().
  virtual void SetGraph(vtkGraph *graph);  

  vtkGraph           *Graph;
  const vtkInEdgeType *Current;
  const vtkInEdgeType *End;
  vtkIdType            Vertex;
  vtkGraphEdge       *GraphEdge;

private:
  vtkInEdgeIterator(const vtkInEdgeIterator&);  // Not implemented.
  void operator=(const vtkInEdgeIterator&);  // Not implemented.
};

#endif
