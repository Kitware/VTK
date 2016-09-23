/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeListIterator.h

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
 * @class   vtkEdgeListIterator
 * @brief   Iterates through all edges in a graph.
 *
 *
 * vtkEdgeListIterator iterates through all the edges in a graph, by traversing
 * the adjacency list for each vertex. You may instantiate this class directly
 * and call SetGraph() to traverse a certain graph. You may also call the graph's
 * GetEdges() method to set up the iterator for a certain graph.
 *
 * Note that this class does NOT guarantee that the edges will be processed in
 * order of their ids (i.e. it will not necessarily return edge 0, then edge 1,
 * etc.).
 *
 * @sa
 * vtkGraph
*/

#ifndef vtkEdgeListIterator_h
#define vtkEdgeListIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkGraph;
class vtkGraphEdge;

struct vtkEdgeType;
struct vtkOutEdgeType;

class VTKCOMMONDATAMODEL_EXPORT vtkEdgeListIterator : public vtkObject
{
public:
  static vtkEdgeListIterator *New();
  vtkTypeMacro(vtkEdgeListIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkGetObjectMacro(Graph, vtkGraph);
  virtual void SetGraph(vtkGraph *graph);

  /**
   * Returns the next edge in the graph.
   */
  vtkEdgeType Next();

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
  bool HasNext();

protected:
  vtkEdgeListIterator();
  ~vtkEdgeListIterator() VTK_OVERRIDE;

  void Increment();

  vtkGraph *Graph;
  const vtkOutEdgeType *Current;
  const vtkOutEdgeType *End;
  vtkIdType             Vertex;
  bool                  Directed;
  vtkGraphEdge        *GraphEdge;

private:
  vtkEdgeListIterator(const vtkEdgeListIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEdgeListIterator&) VTK_DELETE_FUNCTION;
};

#endif
