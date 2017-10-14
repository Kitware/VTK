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
/**
 * @class   vtkVertexListIterator
 * @brief   Iterates all vertices in a graph.
 *
 *
 * vtkVertexListIterator iterates through all vertices in a graph.
 * Create an instance of this and call graph->GetVertices(it) to initialize
 * this iterator. You may alternately call SetGraph() to initialize the
 * iterator.
 *
 * @sa
 * vtkGraph
*/

#ifndef vtkVertexListIterator_h
#define vtkVertexListIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkGraph.h" // For edge type definitions

class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkVertexListIterator : public vtkObject
{
public:
  static vtkVertexListIterator *New();
  vtkTypeMacro(vtkVertexListIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setup the iterator with a graph.
   */
  virtual void SetGraph(vtkGraph *graph);

  //@{
  /**
   * Get the graph associated with this iterator.
   */
  vtkGetObjectMacro(Graph, vtkGraph);
  //@}

  //@{
  /**
   * Returns the next edge in the graph.
   */
  vtkIdType Next()
  {
    vtkIdType v = this->Current;
    ++this->Current;
    return v;
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
  vtkVertexListIterator();
  ~vtkVertexListIterator() override;

  vtkGraph *Graph;
  vtkIdType  Current;
  vtkIdType  End;

private:
  vtkVertexListIterator(const vtkVertexListIterator&) = delete;
  void operator=(const vtkVertexListIterator&) = delete;
};

#endif
