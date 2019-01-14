/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdjacentVertexIterator.h

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
 * @class   vtkAdjacentVertexIterator
 * @brief   Iterates through adjacent vertices in a graph.
 *
 *
 * vtkAdjacentVertexIterator iterates through all vertices adjacent to a
 * vertex, i.e. the vertices which may be reached by traversing an out edge
 * of the source vertex. Use graph->GetAdjacentVertices(v, it) to initialize
 * the iterator.
 *
 *
 *
*/

#ifndef vtkAdjacentVertexIterator_h
#define vtkAdjacentVertexIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkAdjacentVertexIterator : public vtkObject
{
public:
  static vtkAdjacentVertexIterator *New();
  vtkTypeMacro(vtkAdjacentVertexIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  vtkIdType Next()
  {
    vtkOutEdgeType e = *this->Current;
    ++this->Current;
    return e.Target;
  }
  //@}

  /**
   * Whether this iterator has more edges.
   */
  bool HasNext()
  {
    return this->Current != this->End;
  }

protected:
  vtkAdjacentVertexIterator();
  ~vtkAdjacentVertexIterator() override;

  /**
   * Protected method for setting the graph used
   * by Initialize().
   */
  virtual void SetGraph(vtkGraph *graph);

  vtkGraph            *Graph;
  const vtkOutEdgeType *Current;
  const vtkOutEdgeType *End;
  vtkIdType             Vertex;

private:
  vtkAdjacentVertexIterator(const vtkAdjacentVertexIterator&) = delete;
  void operator=(const vtkAdjacentVertexIterator&) = delete;
};

#endif
