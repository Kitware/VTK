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

//@Note: This piece of code is copied from boost graph library.
// In the initial algorithm the weight map was not used and this
// modified version allows the user to pass edge weight map.
namespace boost
{
  /** Graph clustering based on edge betweenness centrality.
   *
   * This algorithm implements graph clustering based on edge
   * betweenness centrality. It is an iterative algorithm, where in each
   * step it compute the edge betweenness centrality (via @ref
   * brandes_betweenness_centrality) and removes the edge with the
   * maximum betweenness centrality. The @p done function object
   * determines when the algorithm terminates (the edge found when the
   * algorithm terminates will not be removed).
   *
   * @param g The graph on which clustering will be performed. The type
   * of this parameter (@c MutableGraph) must be a model of the
   * VertexListGraph, IncidenceGraph, EdgeListGraph, and Mutable Graph
   * concepts.
   *
   * @param done The function object that indicates termination of the
   * algorithm. It must be a ternary function object thats accepts the
   * maximum centrality, the descriptor of the edge that will be
   * removed, and the graph @p g.
   *
   * @param edge_centrality (UTIL/OUT) The property map that will store
   * the betweenness centrality for each edge. When the algorithm
   * terminates, it will contain the edge centralities for the
   * graph. The type of this property map must model the
   * ReadWritePropertyMap concept. Defaults to an @c
   * iterator_property_map whose value type is
   * @c Done::centrality_type and using @c get(edge_index, g) for the
   * index map.
   *
   * @param vertex_index (IN) The property map that maps vertices to
   * indices in the range @c [0, num_vertices(g)). This type of this
   * property map must model the ReadablePropertyMap concept and its
   * value type must be an integral type. Defaults to
   * @c get(vertex_index, g).
   */
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
      brandes_betweenness_centrality(g,
                                     edge_centrality_map(edge_centrality)
                                     .vertex_index_map(vertex_index).weight_map(edge_weight_map));
      std::pair<edge_iterator, edge_iterator> edges_iters = edges(g);
      edge_descriptor e = *max_element(edges_iters.first, edges_iters.second, cmp);
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
}

//-----------------------------------------------------------------------------
vtkBoostBetweennessClustering::~vtkBoostBetweennessClustering()
{
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
    os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl :
    os << indent << "EdgeWeightArrayName: NULL" << endl;

  (EdgeCentralityArrayName) ?
    os << indent << "EdgeCentralityArrayName: " << this->EdgeCentralityArrayName << endl :
    os << indent << "EdgeCentralityArrayName: NULL" << endl;
}

//-----------------------------------------------------------------------------
int vtkBoostBetweennessClustering::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if(!inInfo)
    {
    vtkErrorMacro("Failed to get input information.")
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro("Failed to get output information.")
    return 1;
    }

  // Get the input and output
  vtkGraph* input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(!input)
    {
    vtkErrorMacro("Failed to get input graph.")
    return 1;
    }

  vtkGraph* output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(!output)
    {
    vtkErrorMacro("Failed to get output graph.")
    return 1;
    }

  vtkFloatArray* edgeCM = vtkFloatArray::New();
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

      edgeWeight.TakeReference(
        vtkDataArray::CreateDataArray(weights->GetDataType()));

      double range[2];
      weights->GetRange(range);

      if(weights->GetNumberOfComponents() > 1)
        {
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
                    << " is set but not found.\n");
      return 1;
      }
    }

  if(vtkDirectedGraph::SafeDownCast(input))
    {
    vtkMutableDirectedGraph* out  = vtkMutableDirectedGraph::New();
    // Send the data to output.
    out->DeepCopy(input);

    if(edgeWeight)
      {
      boost::vtkGraphEdgePropertyMapHelper<vtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(
        out, boost::bc_clustering_threshold<double>(
        this->Threshold, out, false), helper, helper2,
        boost::get(boost::vertex_index, out));
      }
    else
      {
      boost::betweenness_centrality_clustering(
        out, boost::bc_clustering_threshold<double>(
          this->Threshold, out, false), helper);
      }
    out->GetEdgeData()->AddArray(edgeCM);
    output->ShallowCopy(out);
    out->Delete();
    }
  else
    {
    vtkMutableUndirectedGraph* out = vtkMutableUndirectedGraph::New();
    // Send the data to output.
    out->DeepCopy(input);

    if(edgeWeight)
      {
      boost::vtkGraphEdgePropertyMapHelper<vtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(out,
        boost::bc_clustering_threshold<double>(this->Threshold, out,false),
        helper, helper2, boost::get(boost::vertex_index, out));
      }
    else
      {
      boost::betweenness_centrality_clustering(
        out, boost::bc_clustering_threshold<double>(this->Threshold, out,
                                                    false), helper);
      }
    out->GetEdgeData()->AddArray(edgeCM);
    output->ShallowCopy(out);
    out->Delete();
    }

  edgeCM->Delete();
  return 1;
}
