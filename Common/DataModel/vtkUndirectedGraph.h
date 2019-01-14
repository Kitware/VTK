/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUndirectedGraph.h

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
 * @class   vtkUndirectedGraph
 * @brief   An undirected graph.
 *
 *
 * vtkUndirectedGraph is a collection of vertices along with a collection of
 * undirected edges (they connect two vertices in no particular order).
 * ShallowCopy(), DeepCopy(), CheckedShallowCopy(), CheckedDeepCopy()
 * accept instances of vtkUndirectedGraph and vtkMutableUndirectedGraph.
 * GetOutEdges(v, it) and GetInEdges(v, it) return the same list of edges,
 * which is the list of all edges which have a v as an endpoint.
 * GetInDegree(v), GetOutDegree(v) and GetDegree(v) all return the full
 * degree of vertex v.
 *
 * vtkUndirectedGraph is read-only. To create an undirected graph,
 * use an instance of vtkMutableUndirectedGraph, then you may set the
 * structure to a vtkUndirectedGraph using ShallowCopy().
 *
 * @sa
 * vtkGraph vtkMutableUndirectedGraph
*/

#ifndef vtkUndirectedGraph_h
#define vtkUndirectedGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGraph.h"

class VTKCOMMONDATAMODEL_EXPORT vtkUndirectedGraph : public vtkGraph
{
public:
  static vtkUndirectedGraph *New();
  vtkTypeMacro(vtkUndirectedGraph, vtkGraph);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override {return VTK_UNDIRECTED_GRAPH;}

  /**
   * Returns the full degree of the vertex.
   */
  vtkIdType GetInDegree(vtkIdType v) override;

  /**
   * Random-access method for retrieving the in edges of a vertex.
   * For an undirected graph, this is the same as the out edges.
   */
  vtkInEdgeType GetInEdge(vtkIdType v, vtkIdType i) override;

  /**
   * Random-access method for retrieving incoming edges to vertex v.
   * The method fills the vtkGraphEdge instance with the id, source, and
   * target of the edge. This method is provided for wrappers,
   * GetInEdge(vtkIdType, vtkIdType) is preferred.
   */
  void GetInEdge(vtkIdType v, vtkIdType i, vtkGraphEdge* e) override
    { this->Superclass::GetInEdge(v, i, e); }

  //@{
  /**
   * Retrieve a graph from an information vector.
   */
  static vtkUndirectedGraph *GetData(vtkInformation *info);
  static vtkUndirectedGraph *GetData(vtkInformationVector *v, int i=0);
  //@}

  /**
   * Initialize the iterator to get the incoming edges to a vertex.
   * For an undirected graph, this is all incident edges.
   */
  void GetInEdges(vtkIdType v, vtkInEdgeIterator *it) override
    { Superclass::GetInEdges(v, it); }

  /**
   * Check the structure, and accept it if it is a valid
   * undirected graph. This is public to allow
   * the ToDirected/UndirectedGraph to work.
   */
  bool IsStructureValid(vtkGraph *g) override;

protected:
  vtkUndirectedGraph();
  ~vtkUndirectedGraph() override;

  /**
   * For iterators, returns the same edge list as GetOutEdges().
   */
  void GetInEdges(vtkIdType v, const vtkInEdgeType *& edges,
                  vtkIdType & nedges) override;

private:
  vtkUndirectedGraph(const vtkUndirectedGraph&) = delete;
  void operator=(const vtkUndirectedGraph&) = delete;
};

#endif
