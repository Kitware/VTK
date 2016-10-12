/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLMinimumSpanningTree.cxx

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
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
#include "vtkPBGLMinimumSpanningTree.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkBoostGraphAdapter.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUndirectedGraph.h"

#include <boost/graph/use_mpi.hpp>

#include <boost/graph/distributed/dehne_gotz_min_spanning_tree.hpp>
#include <boost/graph/distributed/vertex_list_adaptor.hpp>
#include <boost/property_map/parallel/global_index_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/pending/queue.hpp>

#include <utility> // for pair
#include <iterator> // for back_inserter

using namespace boost;

vtkStandardNewMacro(vtkPBGLMinimumSpanningTree);

// Constructor/Destructor
vtkPBGLMinimumSpanningTree::vtkPBGLMinimumSpanningTree()
{
  // Default values for the origin vertex
  this->EdgeWeightArrayName = 0;
  this->OutputSelectionType = 0;
  this->SetOutputSelectionType("MINIMUM_SPANNING_TREE_EDGES");
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);

  VTK_LEGACY_BODY(vtkPBGLMinimumSpanningTree::vtkPBGLMinimumSpanningTree, "VTK 6.2");
}

vtkPBGLMinimumSpanningTree::~vtkPBGLMinimumSpanningTree()
{
  this->SetEdgeWeightArrayName(0);
}

int vtkPBGLMinimumSpanningTree::RequestData(
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
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  vtkDistributedGraphHelper *helper = output->GetDistributedGraphHelper();
  if (!helper)
  {
    vtkErrorMacro("Distributed vtkGraph is required.");
    return 1;
  }

  // We can only deal with Parallel BGL-distributed graphs.
  vtkPBGLDistributedGraphHelper *pbglHelper
    = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
  if (!pbglHelper)
  {
    vtkErrorMacro("Can only perform parallel breadth-first-search on a Parallel BGL distributed graph");
    return 1;
  }

  // Retrieve the edge-weight array.
  if (!this->EdgeWeightArrayName)
  {
    vtkErrorMacro("Edge-weight array name is required");
    return 1;
  }
  vtkAbstractArray* abstractEdgeWeightArray
    = input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName);

  // Does the edge-weight array exist at all?
  if (abstractEdgeWeightArray == NULL)
  {
    vtkErrorMacro("Could not find edge-weight array named "
                  << this->EdgeWeightArrayName);
    return 1;
  }

  // Does the edge-weight array have enough values in it?
  if (abstractEdgeWeightArray->GetNumberOfTuples() < output->GetNumberOfEdges())
  {
    vtkErrorMacro("Edge-weight array named " << this->EdgeWeightArrayName
                  << " has too few values in it.");
    return 1;
  }

  bool edgeWeightArrayIsTemporary = false;
  vtkDoubleArray *edgeWeightArray
    = vtkArrayDownCast<vtkDoubleArray>(abstractEdgeWeightArray);
  if (edgeWeightArray == 0)
  {
    // Edge-weight array does not contain "double" values. We will
    // need to build a temporary array, or fail.
      if (abstractEdgeWeightArray->IsNumeric())
      {
        // Allocate a new edge-weight array.
        edgeWeightArray = vtkDoubleArray::New();
        edgeWeightArray->SetNumberOfTuples(output->GetNumberOfEdges());
        edgeWeightArrayIsTemporary = true;

        // Convert the values in the given array into doubles.
        for (vtkIdType i = 0; i < output->GetNumberOfEdges(); ++i)
        {
          vtkVariant value = abstractEdgeWeightArray->GetVariantValue(i);
          edgeWeightArray->SetTuple1(i, value.ToDouble());
        }
      }
      else
      {
        vtkErrorMacro("Edge-weight array named " << this->EdgeWeightArrayName
                      << " does not contain numeric values.")
        return 1;
      }
  }

  // Execute the algorithm
  if (vtkUndirectedGraph::SafeDownCast(output))
  {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(output);

    // Distributed edge-weight map
    typedef vtkDistributedEdgePropertyMapType<vtkDoubleArray>::type
      DistributedEdgeWeightMap;
    DistributedEdgeWeightMap edgeWeightMap
      = MakeDistributedEdgePropertyMap(g, edgeWeightArray);

    // Create a global index map, which takes vertex descriptors and
    // turns them into numbers in the range [0, N), where N is the
    // total number of vertices in the distributed graph.
    typedef boost::parallel::global_index_map<vtkGraphDistributedVertexIndexMap,
                                              vtkVertexGlobalMap>
      GlobalIndexMap;

    GlobalIndexMap globalIndexMap(pbglHelper->GetProcessGroup(),
                                  g->GetNumberOfVertices(),
                                  MakeDistributedVertexIndexMap(g),
                                  get(boost::vertex_global, g));

    // Run the minimum spanning tree algorithm
    std::vector<vtkEdgeType> mstEdges;
    boost::graph::distributed::boruvka_mixed_merge
      (boost::graph::make_vertex_list_adaptor(g, globalIndexMap),
       edgeWeightMap,
       std::back_inserter(mstEdges),
       globalIndexMap);

    // Select the minimum spanning tree edges.
    if (!strcmp(OutputSelectionType,"MINIMUM_SPANNING_TREE_EDGES"))
    {
      vtkSelection* sel = vtkSelection::GetData(outputVector, 1);
      vtkSmartPointer<vtkIdTypeArray> ids =
        vtkSmartPointer<vtkIdTypeArray>::New();
      vtkSmartPointer<vtkSelectionNode> node =
        vtkSmartPointer<vtkSelectionNode>::New();

      // Add the ids of each MST edge.
      for (std::vector<vtkEdgeType>::iterator i = mstEdges.begin();
           i != mstEdges.end(); ++i)
      {
        ids->InsertNextValue(i->Id);
      }

      node->SetSelectionList(ids);
      node->GetProperties()->Set(vtkSelectionNode::CONTENT_TYPE(),
                                 vtkSelectionNode::INDICES);
      node->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(),
                                 vtkSelectionNode::EDGE);
      sel->AddNode(node);
    }
  }
  else
  {
    vtkErrorMacro("Minimum spanning tree can only be computed on an undirected vtkGraph");
    return 1;
  }

  return 1;
}

void vtkPBGLMinimumSpanningTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)")
     << endl;

  os << indent << "OutputSelectionType: "
     << (this->OutputSelectionType ? this->OutputSelectionType : "(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkPBGLMinimumSpanningTree::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPBGLMinimumSpanningTree::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
  }
  return 1;
}

#endif //VTK_LEGACY_REMOVE
