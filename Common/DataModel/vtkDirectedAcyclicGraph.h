/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectedAcyclicGraph.h

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
// .NAME vtkDirectedAcyclicGraph - A rooted tree data structure.
//
// .SECTION Description
// vtkDirectedAcyclicGraph is a connected directed graph with no cycles. A tree is a type of
// directed graph, so works with all graph algorithms.
//
// vtkDirectedAcyclicGraph is a read-only data structure.
// To construct a tree, create an instance of vtkMutableDirectedGraph.
// Add vertices and edges with AddVertex() and AddEdge(). You may alternately
// start by adding a single vertex as the root then call graph->AddChild(parent)
// which adds a new vertex and connects the parent to the child.
// The tree MUST have all edges in the proper direction, from parent to child.
// After building the tree, call tree->CheckedShallowCopy(graph) to copy the
// structure into a vtkDirectedAcyclicGraph. This method will return false if the graph is
// an invalid tree.
//
// vtkDirectedAcyclicGraph provides some convenience methods for obtaining the parent and
// children of a vertex, for finding the root, and determining if a vertex
// is a leaf (a vertex with no children).
//
// .SECTION See Also
// vtkDirectedGraph vtkMutableDirectedGraph vtkGraph

#ifndef vtkDirectedAcyclicGraph_h
#define vtkDirectedAcyclicGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDirectedGraph.h"

class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT vtkDirectedAcyclicGraph : public vtkDirectedGraph
{
public:
  static vtkDirectedAcyclicGraph *New();
  vtkTypeMacro(vtkDirectedAcyclicGraph, vtkDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  virtual int GetDataObjectType() {return VTK_DIRECTED_ACYCLIC_GRAPH;}

  //BTX
  // Description:
  // Retrieve a graph from an information vector.
  static vtkDirectedAcyclicGraph *GetData(vtkInformation *info);
  static vtkDirectedAcyclicGraph *GetData(vtkInformationVector *v, int i=0);
  //ETX

protected:
  vtkDirectedAcyclicGraph();
  ~vtkDirectedAcyclicGraph();

  // Description:
  // Check the storage, and accept it if it is a valid
  // tree.
  virtual bool IsStructureValid(vtkGraph *g);

private:
  vtkDirectedAcyclicGraph(const vtkDirectedAcyclicGraph&);  // Not implemented.
  void operator=(const vtkDirectedAcyclicGraph&);  // Not implemented.
};

#endif
