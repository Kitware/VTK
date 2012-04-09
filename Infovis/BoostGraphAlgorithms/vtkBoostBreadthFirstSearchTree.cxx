/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBreadthFirstSearchTree.cxx

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
#include "vtkBoostBreadthFirstSearchTree.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include "vtkBoostGraphAdapter.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkUndirectedGraph.h"
#include "vtkTree.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/pending/queue.hpp>

using namespace boost;

vtkStandardNewMacro(vtkBoostBreadthFirstSearchTree);


#if BOOST_VERSION >= 104800      // Boost 1.48.x
namespace {
  vtkIdType unwrap_edge_id(vtkEdgeType const &e)
  {
    return e.Id;
  }
  vtkIdType unwrap_edge_id(boost::detail::reverse_graph_edge_descriptor<vtkEdgeType> const &e)
  {
# if BOOST_VERSION == 104800
    return e.underlying_desc.Id;
# else
    return e.underlying_descx.Id;
# endif
  }
}
#endif

// Redefine the bfs visitor, the only visitor we
// are using is the tree_edge visitor.
template <typename IdMap>
class bfs_tree_builder : public default_bfs_visitor
{
public:
  bfs_tree_builder()
  { }

  bfs_tree_builder(IdMap& g2t, IdMap& t2g, vtkGraph* g, vtkMutableDirectedGraph* t, vtkIdType root)
    : graph_to_tree(g2t), tree_to_graph(t2g), tree(t), graph(g)
  {
    double x[3];
    graph->GetPoints()->GetPoint(root, x);
    tree->GetPoints()->InsertNextPoint(x);
    vtkIdType tree_root = t->AddVertex();
    put(graph_to_tree, root, tree_root);
    put(tree_to_graph, tree_root, root);
    tree->GetVertexData()->CopyData(graph->GetVertexData(), root, tree_root);
  }

  template <typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g) const
  {
    typename graph_traits<Graph>::vertex_descriptor u, v;
    u = source(e, g);
    v = target(e, g);

    // Get the source vertex id (it has already been visited).
    vtkIdType tree_u = get(graph_to_tree, u);

    // Add the point before the vertex so that
    // points match the number of vertices, so that GetPoints()
    // doesn't reallocate and zero-out points.
    double x[3];
    graph->GetPoints()->GetPoint(v, x);
    tree->GetPoints()->InsertNextPoint(x);

    // Create the target vertex in the tree.
    vtkIdType tree_v = tree->AddVertex();
    vtkEdgeType tree_e = tree->AddEdge(tree_u, tree_v);

    // Store the mapping from graph to tree.
    put(graph_to_tree, v, tree_v);
    put(tree_to_graph, tree_v, v);

    // Copy the vertex and edge data from the graph to the tree.
    tree->GetVertexData()->CopyData(graph->GetVertexData(), v, tree_v);
#if BOOST_VERSION < 104800      // Boost 1.48.x
    tree->GetEdgeData()->CopyData(graph->GetEdgeData(), e.Id, tree_e.Id);
#else
    tree->GetEdgeData()->CopyData(graph->GetEdgeData(),
                                  unwrap_edge_id(e), tree_e.Id);
#endif
  }

private:
  IdMap graph_to_tree;
  IdMap tree_to_graph;
  vtkMutableDirectedGraph *tree;
  vtkGraph *graph;
};

// Constructor/Destructor
vtkBoostBreadthFirstSearchTree::vtkBoostBreadthFirstSearchTree()
{
  // Default values for the origin vertex
  this->OriginVertexIndex = 0;
  this->ArrayName = 0;
  this->SetArrayName("Not Set");
  this->ArrayNameSet = false;
  this->OriginValue = 0;
  this->CreateGraphVertexIdArray = false;
  this->ReverseEdges = false;
}

vtkBoostBreadthFirstSearchTree::~vtkBoostBreadthFirstSearchTree()
{
  this->SetArrayName(NULL);
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void vtkBoostBreadthFirstSearchTree::SetOriginVertex(vtkIdType index)
{
  this->OriginVertexIndex = index;
  this->ArrayNameSet = false;
  this->Modified();
}

// Description:
// Set the breadth first search 'origin' vertex.
// This method is basically the same as above
// but allows the application to simply specify
// an array name and value, instead of having to
// know the specific index of the vertex.
void vtkBoostBreadthFirstSearchTree::SetOriginVertex(
  vtkStdString arrayName, vtkVariant value)
{
  this->SetArrayName(arrayName);
  this->ArrayNameSet = true;
  this->OriginValue = value;
  this->Modified();
}

vtkIdType vtkBoostBreadthFirstSearchTree::GetVertexIndex(
  vtkAbstractArray *abstract,vtkVariant value)
{

  // Okay now what type of array is it
  if (abstract->IsNumeric())
    {
    vtkDataArray *dataArray = vtkDataArray::SafeDownCast(abstract);
    int intValue = value.ToInt();
    for(int i=0; i<dataArray->GetNumberOfTuples(); ++i)
      {
      if (intValue == static_cast<int>(dataArray->GetTuple1(i)))
        {
        return i;
        }
      }
    }
  else
    {
    vtkStringArray *stringArray = vtkStringArray::SafeDownCast(abstract);
    vtkStdString stringValue(value.ToString());
    for(int i=0; i<stringArray->GetNumberOfTuples(); ++i)
      {
      if (stringValue == stringArray->GetValue(i))
        {
        return i;
        }
      }
    }

  // Failed
  vtkErrorMacro("Did not find a valid vertex index...");
  return 0;
}

int vtkBoostBreadthFirstSearchTree::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

int vtkBoostBreadthFirstSearchTree::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Now figure out the origin vertex of the
  // breadth first search
  if (this->ArrayNameSet)
    {
    vtkAbstractArray* abstract = input->GetVertexData()->GetAbstractArray(this->ArrayName);

    // Does the array exist at all?
    if (abstract == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->ArrayName);
      return 0;
      }

    this->OriginVertexIndex = this->GetVertexIndex(abstract,this->OriginValue);
   }

  // Create tree to graph id map array
  vtkIdTypeArray* treeToGraphIdMap = vtkIdTypeArray::New();

  // Create graph to tree id map array
  vtkIdTypeArray* graphToTreeIdMap = vtkIdTypeArray::New();

  // Create a color map (used for marking visited nodes)
  vector_property_map<default_color_type> color;

  // Create a queue to hand off to the BFS
  queue<int> q;

  // Create the mutable graph to build the tree
  vtkSmartPointer<vtkMutableDirectedGraph> temp =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  // Initialize copying data into tree
  temp->GetFieldData()->PassData(input->GetFieldData());
  temp->GetVertexData()->CopyAllocate(input->GetVertexData());
  temp->GetEdgeData()->CopyAllocate(input->GetEdgeData());

  // Create the visitor which will build the tree
  bfs_tree_builder<vtkIdTypeArray*> builder(
      graphToTreeIdMap, treeToGraphIdMap, input, temp, this->OriginVertexIndex);

  // Run the algorithm
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(input);
    if (this->ReverseEdges)
      {
#if BOOST_VERSION < 104100      // Boost 1.41.x
      vtkErrorMacro("ReverseEdges requires Boost 1.41.x or higher");
      return 0;
#else
      boost::reverse_graph<vtkDirectedGraph*> r(g);
      breadth_first_search(r, this->OriginVertexIndex, q, builder, color);
#endif
      }
    else
      {
      breadth_first_search(g, this->OriginVertexIndex, q, builder, color);
      }
    }
  else
    {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(input);
    breadth_first_search(g, this->OriginVertexIndex, q, builder, color);
    }

  // If the user wants it, store the mapping back to graph vertices
  if (this->CreateGraphVertexIdArray)
    {
    treeToGraphIdMap->SetName("GraphVertexId");
    temp->GetVertexData()->AddArray(treeToGraphIdMap);
    }

  // Copy the builder graph structure into the output tree
  vtkTree *output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output->CheckedShallowCopy(temp))
    {
    vtkErrorMacro(<<"Invalid tree.");
    return 0;
    }

  // Clean up
  output->Squeeze();
  graphToTreeIdMap->Delete();
  treeToGraphIdMap->Delete();

  return 1;
}

void vtkBoostBreadthFirstSearchTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "ArrayName: "
     << (this->ArrayName ? this->ArrayName : "(none)") << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "ArrayNameSet: "
     << (this->ArrayNameSet ? "true" : "false") << endl;

  os << indent << "CreateGraphVertexIdArray: "
     << (this->CreateGraphVertexIdArray ? "on" : "off") << endl;

  os << indent << "ReverseEdges: "
     << (this->ReverseEdges ? "on" : "off") << endl;
}

