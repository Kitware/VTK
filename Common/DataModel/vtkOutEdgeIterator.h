/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutEdgeIterator.h

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
// .NAME vtkOutEdgeIterator - Iterates through all outgoing edges from a vertex.
//
// .SECTION Description
// vtkOutEdgeIterator iterates through all edges whose source is a particular
// vertex. Instantiate this class directly and call Initialize() to traverse
// the vertex of a graph. Alternately, use GetInEdges() on the graph to
// initialize the iterator. it->Next() returns a vtkOutEdgeType structure,
// which contains Id, the edge's id, and Target, the edge's target vertex.
//
// .SECTION See Also
// vtkGraph vtkInEdgeIterator

#ifndef vtkOutEdgeIterator_h
#define vtkOutEdgeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkOutEdgeIterator : public vtkObject
{
public:
  static vtkOutEdgeIterator *New();
  vtkTypeMacro(vtkOutEdgeIterator, vtkObject);
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
  inline vtkOutEdgeType Next()
  {
    vtkOutEdgeType e = *this->Current;
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
  vtkOutEdgeIterator();
  ~vtkOutEdgeIterator();

  // Description:
  // Protected method for setting the graph used
  // by Initialize().
  virtual void SetGraph(vtkGraph *graph);

  vtkGraph            *Graph;
  const vtkOutEdgeType *Current;
  const vtkOutEdgeType *End;
  vtkIdType             Vertex;
  vtkGraphEdge        *GraphEdge;

private:
  vtkOutEdgeIterator(const vtkOutEdgeIterator&);  // Not implemented.
  void operator=(const vtkOutEdgeIterator&);  // Not implemented.
};

#endif
