// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkGraph.h"                 // For edge types
#include "vtkObject.h"

#include <map>    // STL Header
#include <vector> // STL Header

//----------------------------------------------------------------------------
// class vtkVertexAdjacencyList
//----------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
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
  static vtkGraphInternals* New();

  vtkTypeMacro(vtkGraphInternals, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  ~vtkGraphInternals() override;

private:
  vtkGraphInternals(const vtkGraphInternals&) = delete;
  void operator=(const vtkGraphInternals&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkGraphInternals_h
