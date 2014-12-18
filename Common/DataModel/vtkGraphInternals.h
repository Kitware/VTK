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
// .NAME vtkGraphInternals - Internal representation of vtkGraph
//
// .SECTION Description
// This is the internal representation of vtkGraph, used only in rare cases
// where one must modify that representation.

#ifndef vtkGraphInternals_h
#define vtkGraphInternals_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGraph.h"

#include <vtksys/stl/vector> // STL Header
#include <vtksys/stl/map>    // STL Header

//----------------------------------------------------------------------------
// class vtkVertexAdjacencyList
//----------------------------------------------------------------------------
//BTX
class vtkVertexAdjacencyList
{
public:
  vtksys_stl::vector<vtkInEdgeType> InEdges;
  vtksys_stl::vector<vtkOutEdgeType> OutEdges;
};
//ETX

//----------------------------------------------------------------------------
// class vtkGraphInternals
//----------------------------------------------------------------------------
class VTKCOMMONDATAMODEL_EXPORT vtkGraphInternals : public vtkObject
{
public:
  static vtkGraphInternals *New();
  //BTX
  vtkTypeMacro(vtkGraphInternals, vtkObject);
  vtksys_stl::vector<vtkVertexAdjacencyList> Adjacency;
  //ETX
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

  //BTX
  // Description:
  // Convenience method for removing an edge from an out edge list.
  void RemoveEdgeFromOutList(vtkIdType e, vtksys_stl::vector<vtkOutEdgeType>& outEdges);

  // Description:
  // Convenience method for removing an edge from an in edge list.
  void RemoveEdgeFromInList(vtkIdType e, vtksys_stl::vector<vtkInEdgeType>& inEdges);

  // Description:
  // Convenience method for renaming an edge in an out edge list.
  void ReplaceEdgeFromOutList(vtkIdType from, vtkIdType to, vtksys_stl::vector<vtkOutEdgeType>& outEdges);

  // Description:
  // Convenience method for renaming an edge in an in edge list.
  void ReplaceEdgeFromInList(vtkIdType from, vtkIdType to, vtksys_stl::vector<vtkInEdgeType>& inEdges);
  //ETX

protected:
  vtkGraphInternals();
  ~vtkGraphInternals();

private:
  vtkGraphInternals(const vtkGraphInternals&);  // Not implemented.
  void operator=(const vtkGraphInternals&);  // Not implemented.
};

#endif // vtkGraphInternals_h

// VTK-HeaderTest-Exclude: vtkGraphInternals.h
