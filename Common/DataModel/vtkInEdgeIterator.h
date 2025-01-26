// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

VTK_ABI_NAMESPACE_BEGIN
class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkInEdgeIterator : public vtkObject
{
public:
  static vtkInEdgeIterator* New();
  vtkTypeMacro(vtkInEdgeIterator, vtkObject);
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
  vtkInEdgeType Next()
  {
    vtkInEdgeType e = *this->Current;
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
  vtkInEdgeIterator();
  ~vtkInEdgeIterator() override;

  /**
   * Protected method for setting the graph used
   * by Initialize().
   */
  virtual void SetGraph(vtkGraph* graph);

  vtkGraph* Graph;
  const vtkInEdgeType* Current;
  const vtkInEdgeType* End;
  vtkIdType Vertex;
  vtkGraphEdge* GraphEdge;

private:
  vtkInEdgeIterator(const vtkInEdgeIterator&) = delete;
  void operator=(const vtkInEdgeIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
