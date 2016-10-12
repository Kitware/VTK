/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectedGraph.h

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
 * @class   vtkDirectedGraph
 * @brief   A directed graph.
 *
 *
 * vtkDirectedGraph is a collection of vertices along with a collection of
 * directed edges (edges that have a source and target). ShallowCopy()
 * and DeepCopy() (and CheckedShallowCopy(), CheckedDeepCopy())
 * accept instances of vtkTree and vtkMutableDirectedGraph.
 *
 * vtkDirectedGraph is read-only. To create an undirected graph,
 * use an instance of vtkMutableDirectedGraph, then you may set the
 * structure to a vtkDirectedGraph using ShallowCopy().
 *
 * @sa
 * vtkGraph vtkMutableDirectedGraph
*/

#ifndef vtkDirectedGraph_h
#define vtkDirectedGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGraph.h"

class VTKCOMMONDATAMODEL_EXPORT vtkDirectedGraph : public vtkGraph
{
public:
  static vtkDirectedGraph *New();
  vtkTypeMacro(vtkDirectedGraph, vtkGraph);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_DIRECTED_GRAPH;}

  //@{
  /**
   * Retrieve a graph from an information vector.
   */
  static vtkDirectedGraph *GetData(vtkInformation *info);
  static vtkDirectedGraph *GetData(vtkInformationVector *v, int i=0);
  //@}

  /**
   * Check the storage, and accept it if it is a valid
   * undirected graph. This is public to allow
   * the ToDirected/UndirectedGraph to work.
   */
  bool IsStructureValid(vtkGraph *g) VTK_OVERRIDE;

protected:
  vtkDirectedGraph();
  ~vtkDirectedGraph() VTK_OVERRIDE;

private:
  vtkDirectedGraph(const vtkDirectedGraph&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDirectedGraph&) VTK_DELETE_FUNCTION;
};

#endif
