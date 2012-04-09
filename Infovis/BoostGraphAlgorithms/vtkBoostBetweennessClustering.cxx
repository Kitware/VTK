/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBoostBetweennessClustering.h"

#include "vtkBoostConnectedComponents.h"
#include "vtkBoostGraphAdapter.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUndirectedGraph.h"

#include <boost/graph/bc_clustering.hpp>

// @note: This piece of code is modification of algorithm from boost graph
// library. This modified version allows the user to pass edge weight map
namespace boost
{
   // Graph clustering based on edge betweenness centrality.
   //
   // This algorithm implements graph clustering based on edge
   // betweenness centrality. It is an iterative algorithm, where in each
   // step it compute the edge betweenness centrality (via @ref
   // brandes_betweenness_centrality) and removes the edge with the
   // maximum betweenness centrality. The @p done function object
   // determines when the algorithm terminates (the edge found when the
   // algorithm terminates will not be removed).
   //
   // @param g The graph on which clustering will be performed. The type
   // of this parameter (@c MutableGraph) must be a model of the
   // VertexListGraph, IncidenceGraph, EdgeListGraph, and Mutable Graph
   // concepts.
   //
   // @param done The function object that indicates termination of the
   // algorithm. It must be a ternary function object thats accepts the
   // maximum centrality, the descriptor of the edge that will be
   // removed, and the graph @p g.
   //
   // @param edge_centrality (UTIL/out2) The property map that will store
   // the betweenness centrality for each edge. When the algorithm
   // terminates, it will contain the edge centralities for the
   // graph. The type of this property map must model the
   // ReadWritePropertyMap concept. Defaults to an @c
   // iterator_property_map whose value type is
   // @c Done::centrality_type and using @c get(edge_index, g) for the
   // index map.
   //
   // @param vertex_index (IN) The property map that maps vertices to
   // indices in the range @c [0, num_vertices(g)). This type of this
   // property map must model the ReadablePropertyMap concept and its
   // value type must be an integral type. Defaults to
   // @c get(vertex_index, g).
  template<typename MutableGraph, typename Done, typename EdgeCentralityMap,
           typename EdgeWeightMap, typename VertexIndexMap>
  void
  betweenness_centrality_clustering(MutableGraph& g, Done done,
                                    EdgeCentralityMap edge_centrality,
                                    EdgeWeightMap edge_weight_map,
                                    VertexIndexMap vertex_index)
  {
    typedef typename property_traits<EdgeCentralityMap>::value_type
      centrality_type;
    typedef typename graph_traits<MutableGraph>::edge_iterator edge_iterator;
    typedef typename graph_traits<MutableGraph>::edge_descriptor edge_descriptor;
    typedef typename graph_traits<MutableGraph>::vertices_size_type
      vertices_size_type;

    if (has_no_edges(g)) return;

    // Function object that compares the centrality of edges
    indirect_cmp<EdgeCentralityMap, std::less<centrality_type> >
      cmp(edge_centrality);

    bool is_done;
    do {
      brandes_betweenness_centrality(g,edge_centrality_map(edge_centrality)
                                     .vertex_index_map(vertex_index)
                                     .weight_map(edge_weight_map));
      std::pair<edge_iterator, edge_iterator> edges_iters = edges(g);
      edge_descriptor e = *max_element(edges_iters.first, edges_iters.second,
                                       cmp);
      is_done = done(get(edge_centrality, e), e, g);
      if (!is_done) remove_edge(e, g);
    } while (!is_done && !has_no_edges(g));
  }
}

vtkStandardNewMacro(vtkBoostBetweennessClustering);

//-----------------------------------------------------------------------------
vtkBoostBetweennessClustering::vtkBoostBetweennessClustering() :
  vtkGraphAlgorithm       (),
  Threshold               (0),
  UseEdgeWeightArray      (false),
  InvertEdgeWeightArray   (false),
  EdgeWeightArrayName     (0),
  EdgeCentralityArrayName (0)
{
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
vtkBoostBetweennessClustering::~vtkBoostBetweennessClustering()
{
  this->SetEdgeWeightArrayName(0);
  this->SetEdgeCentralityArrayName(0);
}

//-----------------------------------------------------------------------------
void vtkBoostBetweennessClustering::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Threshold: " << this->Threshold << endl;
  os << indent << "UseEdgeWeightArray: " << this->UseEdgeWeightArray << endl;
  os << indent << "InvertEdgeWeightArray: " << this->InvertEdgeWeightArray
    << endl;
  (EdgeWeightArrayName) ?
    os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName
      << endl :
    os << indent << "EdgeWeightArrayName: NULL" << endl;

  (EdgeCentralityArrayName) ?
    os << indent << "EdgeCentralityArrayName: " << this->EdgeCentralityArrayName
      << endl :
    os << indent << "EdgeCentralityArrayName: NULL" << endl;
}

//-----------------------------------------------------------------------------
int vtkBoostBetweennessClustering::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Helpful vars.
  bool isDirectedGraph (false);

  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if(!inInfo)
    {
    vtkErrorMacro("Failed to get input information.")
    return 1;
    }

  vtkInformation* outInfo1 = outputVector->GetInformationObject(0);
  if(!outInfo1)
    {
    vtkErrorMacro("Failed get output1 on information first port.")
    }

  vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
  if(!outInfo2)
    {
    vtkErrorMacro("Failed to get output2 information on second port.")
    return 1;
    }

  // Get the input, output1 and output2.
  vtkGraph* input = vtkGraph::SafeDownCast(inInfo->Get(
    vtkDataObject::DATA_OBJECT()));
  if(!input)
    {
    vtkErrorMacro("Failed to get input graph.")
    return 1;
    }

  if(vtkDirectedGraph::SafeDownCast(input))
    {
    isDirectedGraph = true;
    }

  vtkGraph* output1 = vtkGraph::SafeDownCast(
    outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  if(!output1)
    {
    vtkErrorMacro("Failed to get output1 graph.")
    return 1;
    }

  vtkGraph* output2 = vtkGraph::SafeDownCast(
    outInfo2->Get(vtkDataObject::DATA_OBJECT()));
  if(!output2)
    {
    vtkErrorMacro("Failed to get output2 graph.")
    return 1;
    }

  vtkSmartPointer<vtkFloatArray> edgeCM = vtkSmartPointer<vtkFloatArray>::New();
  if(this->EdgeCentralityArrayName)
    {
    edgeCM->SetName(this->EdgeCentralityArrayName);
    }
  else
    {
    edgeCM->SetName("edge_centrality");
    }

  boost::vtkGraphEdgePropertyMapHelper<vtkFloatArray*> helper(edgeCM);

  vtkSmartPointer<vtkDataArray> edgeWeight (0);
  if(this->UseEdgeWeightArray && this->EdgeWeightArrayName)
    {
    if(!this->InvertEdgeWeightArray)
      {
      edgeWeight = input->GetEdgeData()->GetArray(this->EdgeWeightArrayName);
      }
    else
      {
      vtkDataArray* weights =
        input->GetEdgeData()->GetArray(this->EdgeWeightArrayName);

      if(!weights)
        {
        vtkErrorMacro(<<"Error: Edge weight array " << this->EdgeWeightArrayName
                      << " is set but not found or not a data array.\n");
        return 1;
        }

      edgeWeight.TakeReference(
        vtkDataArray::CreateDataArray(weights->GetDataType()));

      double range[2];
      weights->GetRange(range);

      if(weights->GetNumberOfComponents() > 1)
        {
        vtkErrorMacro("Expecting single component array.");
        return 1;
        }

      for(int i=0; i < weights->GetDataSize(); ++i)
        {
        edgeWeight->InsertNextTuple1(range[1] - weights->GetTuple1(i));
        }
      }

    if(!edgeWeight)
      {
      vtkErrorMacro(<<"Error: Edge weight array " << this->EdgeWeightArrayName
                    << " is set but not found or not a data array.\n");
      return 1;
      }
    }

  // First compute the second output and the result will be used
  // as input for the first output.
  if(isDirectedGraph)
    {
    vtkMutableDirectedGraph* out2  = vtkMutableDirectedGraph::New();

    // Copy the data to the second output (as this algorithm most likely
    // going to removed edges (and hence modifies the graph).
    out2->DeepCopy(input);

    if(edgeWeight)
      {
      boost::vtkGraphEdgePropertyMapHelper<vtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(this->Threshold, out2, false),
        helper, helper2, boost::get(boost::vertex_index, out2));
      }
    else
      {
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(
        this->Threshold, out2, false), helper);
      }
    out2->GetEdgeData()->AddArray(edgeCM);

    // Finally copy the results to the output.
    output2->ShallowCopy(out2);

    out2->Delete();
    }
  else
    {
    vtkMutableUndirectedGraph* out2 = vtkMutableUndirectedGraph::New();

    // Send the data to output2.
    out2->DeepCopy(input);

    if(edgeWeight)
      {
      boost::vtkGraphEdgePropertyMapHelper<vtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(this->Threshold, out2,false),
        helper, helper2, boost::get(boost::vertex_index, out2));
      }
    else
      {
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(this->Threshold, out2,false),
        helper);
      }
    out2->GetEdgeData()->AddArray(edgeCM);

    // Finally copy the results to the output.
    output2->ShallowCopy(out2);

    out2->Delete();
    }

  // Now take care of the first output.
  vtkSmartPointer<vtkBoostConnectedComponents> bcc (
    vtkSmartPointer<vtkBoostConnectedComponents>::New());

  vtkSmartPointer<vtkGraph> output2Copy(0);

  if(isDirectedGraph)
    {
    output2Copy = vtkSmartPointer<vtkDirectedGraph>::New();
    }
  else
    {
    output2Copy = vtkSmartPointer<vtkUndirectedGraph>::New();
    }

  output2Copy->ShallowCopy(output2);

  bcc->SetInputData(0, output2Copy);
  bcc->Update();

  vtkSmartPointer<vtkGraph> bccOut = bcc->GetOutput(0);

  vtkSmartPointer<vtkAbstractArray> compArray (0);
  if(isDirectedGraph)
    {
    vtkSmartPointer<vtkDirectedGraph> out1
      (vtkSmartPointer<vtkDirectedGraph>::New());
    out1->ShallowCopy(input);

    compArray = bccOut->GetVertexData()->GetAbstractArray("component");

    if(!compArray)
      {
      vtkErrorMacro("Unable to get component array.")
      return 1;
      }

    out1->GetVertexData()->AddArray(compArray);

    // Finally copy the output to the algorithm output.
    output1->ShallowCopy(out1);
    }
  else
    {
    vtkSmartPointer<vtkUndirectedGraph> out1
      (vtkSmartPointer<vtkUndirectedGraph>::New());
    out1->ShallowCopy(input);

    compArray = bccOut->GetVertexData()->GetAbstractArray("component");

    if(!compArray)
      {
      vtkErrorMacro("Unable to get component array.")
      return 1;
      }

    out1->GetVertexData()->AddArray(compArray);

    // Finally copy the output to the algorithm output.
    output1->ShallowCopy(out1);
    }

  // Also add the components array to the second output.
  output2->GetVertexData()->AddArray(compArray);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkBoostBetweennessClustering::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if(port == 0 || port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
    }
  return 1;
}
