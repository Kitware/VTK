/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkGraphInternals
 * @brief   Internal representation of vtkGraph
 *
 *
 * This is the internal representation of vtkGraph, used only in rare cases
 * where one must modify that representation.
*/

#ifndef vtkGraphInternals_h
#define vtkGraphInternals_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGraph.h"

#include <vector> // STL Header
#include <map>    // STL Header

//----------------------------------------------------------------------------
// class vtkVertexAdjacencyList
//----------------------------------------------------------------------------

class vtkVertexAdjacencyList
{
public:
  std::vector<vtkInEdgeType> InEdges;
  std::vector<vtkOutEdgeType> OutEdges;
};

//----------------------------------------------------------------------------
// class vtkGraphInternals
//----------------------------------------------------------------------------
class VTKCOMMONDATAMODEL_EXPORT vtkGraphInternals : public vtkObject
{
public:
  static vtkGraphInternals *New();

  vtkTypeMacro(vtkGraphInternals, vtkObject);
  std::vector<vtkVertexAdjacencyList> Adjacency;

  vtkIdType NumberOfEdges;

  vtkIdType LastRemoteEdgeId;
  vtkIdType LastRemoteEdgeSource;
  vtkIdType LastRemoteEdgeTarget;

  // Whether we have used pedigree IDs to refer to the vertices of the
  // graph, e.g., to add edges or vertices. In a distributed graph,
  // the pedigree-id interface is mutually exclusive with the
  // no-argument AddVertex() function in vtkMutableUndirectedGraph and
  // vtkMutableDirectedGraph.
  bool UsingPedigreeIds;

  /**
   * Convenience method for removing an edge from an out edge list.
   */
  void RemoveEdgeFromOutList(vtkIdType e, std::vector<vtkOutEdgeType>& outEdges);

  /**
   * Convenience method for removing an edge from an in edge list.
   */
  void RemoveEdgeFromInList(vtkIdType e, std::vector<vtkInEdgeType>& inEdges);

  /**
   * Convenience method for renaming an edge in an out edge list.
   */
  void ReplaceEdgeFromOutList(vtkIdType from, vtkIdType to, std::vector<vtkOutEdgeType>& outEdges);

  /**
   * Convenience method for renaming an edge in an in edge list.
   */
  void ReplaceEdgeFromInList(vtkIdType from, vtkIdType to, std::vector<vtkInEdgeType>& inEdges);

protected:
  vtkGraphInternals();
  ~vtkGraphInternals() VTK_OVERRIDE;

private:
  vtkGraphInternals(const vtkGraphInternals&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphInternals&) VTK_DELETE_FUNCTION;
};

#endif // vtkGraphInternals_h

// VTK-HeaderTest-Exclude: vtkGraphInternals.h
