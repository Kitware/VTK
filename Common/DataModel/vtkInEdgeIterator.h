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
/**
 * @class   vtkInEdgeIterator
 * @brief   Iterates through all incoming edges to a vertex.
 *
 *
 * vtkInEdgeIterator iterates through all edges whose target is a particular
 * vertex. Instantiate this class directly and call Initialize() to traverse
 * the vertex of a graph. Alternately, use GetInEdges() on the graph to
 * initialize the iterator. it->Next() returns a vtkInEdgeType structure,
 * which contains Id, the edge's id, and Source, the edge's source vertex.
 *
 * @sa
 * vtkGraph vtkOutEdgeIterator
*/

#ifndef vtkInEdgeIterator_h
#define vtkInEdgeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkInEdgeIterator : public vtkObject
{
public:
  static vtkInEdgeIterator *New();
  vtkTypeMacro(vtkInEdgeIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize the iterator with a graph and vertex.
   */
  void Initialize(vtkGraph *g, vtkIdType v);

  //@{
  /**
   * Get the graph and vertex associated with this iterator.
   */
  vtkGetObjectMacro(Graph, vtkGraph);
  vtkGetMacro(Vertex, vtkIdType);
  //@}

  //@{
  /**
   * Returns the next edge in the graph.
   */
  inline vtkInEdgeType Next()
  {
    vtkInEdgeType e = *this->Current;
    ++this->Current;
    return e;
  }
  //@}

  /**
   * Just like Next(), but
   * returns heavy-weight vtkGraphEdge object instead of
   * the vtkEdgeType struct, for use with wrappers.
   * The graph edge is owned by this iterator, and changes
   * after each call to NextGraphEdge().
   */
  vtkGraphEdge *NextGraphEdge();

  /**
   * Whether this iterator has more edges.
   */
  bool HasNext()
  {
    return this->Current != this->End;
  }

protected:
  vtkInEdgeIterator();
  ~vtkInEdgeIterator() VTK_OVERRIDE;

  /**
   * Protected method for setting the graph used
   * by Initialize().
   */
  virtual void SetGraph(vtkGraph *graph);

  vtkGraph           *Graph;
  const vtkInEdgeType *Current;
  const vtkInEdgeType *End;
  vtkIdType            Vertex;
  vtkGraphEdge       *GraphEdge;

private:
  vtkInEdgeIterator(const vtkInEdgeIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInEdgeIterator&) VTK_DELETE_FUNCTION;
};

#endif
