// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkOutEdgeIterator
 * @brief   Iterates through all outgoing edges from a vertex.
 *
 *
 * vtkOutEdgeIterator iterates through all edges whose source is a particular
 * vertex. Instantiate this class directly and call Initialize() to traverse
 * the vertex of a graph. Alternately, use GetInEdges() on the graph to
 * initialize the iterator. it->Next() returns a vtkOutEdgeType structure,
 * which contains Id, the edge's id, and Target, the edge's target vertex.
 *
 * @sa
 * vtkGraph vtkInEdgeIterator
 */

#ifndef vtkOutEdgeIterator_h
#define vtkOutEdgeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

VTK_ABI_NAMESPACE_BEGIN
class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkOutEdgeIterator : public vtkObject
{
public:
  static vtkOutEdgeIterator* New();
  vtkTypeMacro(vtkOutEdgeIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the iterator with a graph and vertex.
   */
  void Initialize(vtkGraph* g, vtkIdType v);

  ///@{
  /**
   * Get the graph and vertex associated with this iterator.
   */
  vtkGetObjectMacro(Graph, vtkGraph);
  vtkGetMacro(Vertex, vtkIdType);
  ///@}

  ///@{
  /**
   * Returns the next edge in the graph.
   */
  vtkOutEdgeType Next()
  {
    vtkOutEdgeType e = *this->Current;
    ++this->Current;
    return e;
  }
  ///@}

  /**
   * Just like Next(), but
   * returns heavy-weight vtkGraphEdge object instead of
   * the vtkEdgeType struct, for use with wrappers.
   * The graph edge is owned by this iterator, and changes
   * after each call to NextGraphEdge().
   */
  vtkGraphEdge* NextGraphEdge();

  /**
   * Whether this iterator has more edges.
   */
  bool HasNext() { return this->Current != this->End; }

protected:
  vtkOutEdgeIterator();
  ~vtkOutEdgeIterator() override;

  /**
   * Protected method for setting the graph used
   * by Initialize().
   */
  virtual void SetGraph(vtkGraph* graph);

  vtkGraph* Graph;
  const vtkOutEdgeType* Current;
  const vtkOutEdgeType* End;
  vtkIdType Vertex;
  vtkGraphEdge* GraphEdge;

private:
  vtkOutEdgeIterator(const vtkOutEdgeIterator&) = delete;
  void operator=(const vtkOutEdgeIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
