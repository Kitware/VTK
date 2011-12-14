/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollapseGraph.cxx

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
#include "vtkCollapseGraph.h"

#include <vtkConvertSelection.h>
#include <vtkDataSetAttributes.h>
#include <vtkEdgeListIterator.h>
#include <vtkIdTypeArray.h>
#include <vtkInEdgeIterator.h>
#include <vtkInformation.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkObjectFactory.h>
#include <vtkSelection.h>
#include <vtkSmartPointer.h>

//#include <iterator>
#include <vector>

/// Defines storage for a collection of edges
typedef std::vector<vtkEdgeType> EdgeListT;

///////////////////////////////////////////////////////////////////////////////////
// BuildGraph

template<typename GraphT>
static void BuildGraph(vtkGraph* input_graph, const std::vector<vtkIdType>& vertex_map, const EdgeListT& edge_list, vtkGraph* destination_graph)
{
  vtkSmartPointer<GraphT> output_graph = vtkSmartPointer<GraphT>::New();

  output_graph->GetFieldData()->ShallowCopy(input_graph->GetFieldData());

  vtkDataSetAttributes* const input_vertex_data = input_graph->GetVertexData();
  vtkDataSetAttributes* const output_vertex_data = output_graph->GetVertexData();
  output_vertex_data->CopyAllocate(input_vertex_data);
  for(std::vector<vtkIdType>::size_type i = 0; i != vertex_map.size(); ++i)
    {
    if(vertex_map[i] == -1)
      continue;
      
    output_graph->AddVertex();
    output_vertex_data->CopyData(input_vertex_data, i, vertex_map[i]);
    }

  vtkDataSetAttributes* const input_edge_data = input_graph->GetEdgeData();
  vtkDataSetAttributes* const output_edge_data = output_graph->GetEdgeData();
  output_edge_data->CopyAllocate(input_edge_data);
  for(EdgeListT::const_iterator input_edge = edge_list.begin(); input_edge != edge_list.end(); ++input_edge)
    {
    vtkEdgeType output_edge = output_graph->AddEdge(vertex_map[input_edge->Source], vertex_map[input_edge->Target]);
    output_edge_data->CopyData(input_edge_data, input_edge->Id, output_edge.Id);
    }
 
  destination_graph->ShallowCopy(output_graph); 
}

///////////////////////////////////////////////////////////////////////////////////
// vtkCollapseGraph

vtkStandardNewMacro(vtkCollapseGraph);

vtkCollapseGraph::vtkCollapseGraph()
{
  this->SetNumberOfInputPorts(2);
}


vtkCollapseGraph::~vtkCollapseGraph()
{
}

void vtkCollapseGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCollapseGraph::SetGraphConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(0, input);
}

void vtkCollapseGraph::SetSelectionConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}

int vtkCollapseGraph::FillInputPortInformation(int port, vtkInformation* info)
{
  if(port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
    }
  else if(port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
    }
    
  return 0;
}


int vtkCollapseGraph::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Ensure we have valid inputs ...
  vtkGraph* const input_graph = vtkGraph::GetData(inputVector[0]);
  vtkGraph* const output_graph = vtkGraph::GetData(outputVector);

  vtkSmartPointer<vtkIdTypeArray> input_indices =
    vtkSmartPointer<vtkIdTypeArray>::New();
  vtkConvertSelection::GetSelectedVertices(
    vtkSelection::GetData(inputVector[1]),
    input_graph,
    input_indices);
  
  // Convert the input selection into an "expanding" array that contains "true" for each
  // vertex that is expanding (i.e. its neighbors are collapsing into it)
  std::vector<bool> expanding(input_graph->GetNumberOfVertices(), false);

  for(vtkIdType i = 0; i != input_indices->GetNumberOfTuples(); ++i)
    {
    expanding[input_indices->GetValue(i)] = true;
    }

  // Create a mapping from each child vertex to its expanding neighbor (if any)
  std::vector<vtkIdType> parent(input_graph->GetNumberOfVertices());
  vtkSmartPointer<vtkInEdgeIterator> in_edge_iterator = vtkSmartPointer<vtkInEdgeIterator>::New();
  for(vtkIdType vertex = 0; vertex != input_graph->GetNumberOfVertices(); ++vertex)
    {
    // By default, vertices map to themselves, i.e: they aren't collapsed
    parent[vertex] = vertex;

    if(expanding[vertex])
      continue;

    input_graph->GetInEdges(vertex, in_edge_iterator);
    while(in_edge_iterator->HasNext())
      {
      const vtkIdType adjacent_vertex = in_edge_iterator->Next().Source;
      if(expanding[adjacent_vertex])
        {
        parent[vertex] = adjacent_vertex;
        break;
        }
      }
    }

  // Create a mapping from vertex IDs in the original graph to vertex IDs in the output graph
  std::vector<vtkIdType> vertex_map(input_graph->GetNumberOfVertices(), -1);
  for(vtkIdType old_vertex = 0, new_vertex = 0; old_vertex != input_graph->GetNumberOfVertices(); ++old_vertex)
    {
    if(parent[old_vertex] != old_vertex)
      continue;

    vertex_map[old_vertex] = new_vertex++;
    }

  // Create a new edge list, mapping each edge from children to parents, eliminating duplicates as we go
  EdgeListT edge_list;

  vtkSmartPointer<vtkEdgeListIterator> edge_iterator = vtkSmartPointer<vtkEdgeListIterator>::New();
  input_graph->GetEdges(edge_iterator);
  while(edge_iterator->HasNext())
    {
    vtkEdgeType edge = edge_iterator->Next();
    
    edge.Source = parent[edge.Source];
    edge.Target = parent[edge.Target];
    if(edge.Source == edge.Target)
      continue;

    edge_list.push_back(edge);
    }

  // Build the new output graph, based on the graph type ...
  if(vtkDirectedGraph::SafeDownCast(input_graph))
    {
    BuildGraph<vtkMutableDirectedGraph>(input_graph, vertex_map, edge_list, output_graph);
    }
  else if(vtkUndirectedGraph::SafeDownCast(input_graph))
    {
    BuildGraph<vtkMutableUndirectedGraph>(input_graph, vertex_map, edge_list, output_graph);
    }
  else
    {
    vtkErrorMacro(<< "Unknown input graph type");
    return 0;
    }
    
  return 1;
}

