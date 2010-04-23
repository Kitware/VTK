/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLShortestPaths.cxx

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
#include "vtkPBGLShortestPaths.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
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
#include "vtkType.h"
#include "vtkUndirectedGraph.h"
#include "vtkVertexListIterator.h"

#include <boost/graph/use_mpi.hpp>

#include <boost/graph/distributed/delta_stepping_shortest_paths.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>

#include <vtksys/stl/utility> // for pair

using namespace boost;

vtkStandardNewMacro(vtkPBGLShortestPaths);

// Function object used to reduce (vertex, distance) pairs to find
// the furthest vertex. This ordering favors vertices on processors
// with a lower rank. Used only for the Parallel BGL shortest-paths.
class furthest_vertex_double
{
public:
  furthest_vertex_double() : graph(0) { }

  furthest_vertex_double(vtkGraph *g) : graph(g) { }

  vtkstd::pair<vtkIdType, double>
  operator()(vtkstd::pair<vtkIdType, double> x,
             vtkstd::pair<vtkIdType, double> y) const
  {
    vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();
    if (x.second > y.second
        || (x.second == y.second
            && helper->GetVertexOwner(x.first) < helper->GetVertexOwner(y.first))
        || (x.second == y.second
            && helper->GetVertexOwner(x.first) == helper->GetVertexOwner(y.first)
            && helper->GetVertexIndex(x.first) < helper->GetVertexIndex(y.first)))
      {
      return x;
      }
    else
      {
      return y;
      }
  }

private:
  vtkGraph *graph;
};

// Constructor/Destructor
vtkPBGLShortestPaths::vtkPBGLShortestPaths()
{
  // Default values for the origin vertex
  this->OriginVertexIndex = 0;
  this->InputArrayName = 0;
  this->EdgeWeightArrayName = 0;
  this->Delta = 0.0;
  this->PredecessorArrayName = 0;
  this->PathLengthArrayName = 0;
  this->OutputSelectionType = 0;
  this->SetOutputSelectionType("MAX_DIST_FROM_ROOT");
  this->OriginValue = -1;
  this->OutputSelection = false;
  this->OriginFromSelection = false;
  this->UseUniformEdgeWeights = false;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

vtkPBGLShortestPaths::~vtkPBGLShortestPaths()
{
  this->SetInputArrayName(0);
  this->SetEdgeWeightArrayName(0);
  this->SetPredecessorArrayName(0);
  this->SetPathLengthArrayName(0);
}

void vtkPBGLShortestPaths::SetOriginSelection(vtkSelection* s)
{
  this->SetOriginSelectionConnection(s->GetProducerPort());
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void vtkPBGLShortestPaths::SetOriginVertex(vtkIdType index)
{
  this->OriginVertexIndex = index;
  this->InputArrayName = NULL; // Reset any origin set by another method
  this->Modified();
}

// Description:
// Set the breadth first search 'origin' vertex.
// This method is basically the same as above
// but allows the application to simply specify
// an array name and value, instead of having to
// know the specific index of the vertex.
void vtkPBGLShortestPaths::SetOriginVertex(
  vtkStdString arrayName, vtkVariant value)
{
  this->SetInputArrayName(arrayName);
  this->OriginValue = value;
  this->Modified();
}

void vtkPBGLShortestPaths::SetOriginVertexString(
  char* arrayName, char* value)
{
  this->SetOriginVertex(arrayName, value);
}

vtkIdType vtkPBGLShortestPaths::GetVertexIndex(
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


int vtkPBGLShortestPaths::RequestData(
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

  // Sanity check
  // The Boost BFS likes to crash on empty datasets
  if (input->GetNumberOfVertices() == 0)
    {
    //vtkWarningMacro("Empty input into " << this->GetClassName());
    return 1;
    }

  if (this->OriginFromSelection)
    {
    vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
    if (selection == NULL)
      {
      vtkErrorMacro("OriginFromSelection set but selection input undefined.");
      return 0;
      }
    vtkSmartPointer<vtkIdTypeArray> idArr =
      vtkSmartPointer<vtkIdTypeArray>::New();
    vtkConvertSelection::GetSelectedVertices(selection, input, idArr);
    if (idArr->GetNumberOfTuples() == 0)
      {
      vtkErrorMacro("Origin selection is empty.");
      return 0;
      }
    this->OriginVertexIndex = idArr->GetValue(0);
    }
  else
    {
    // Now figure out the origin vertex of the
    // breadth first search
    if (this->InputArrayName)
      {
      vtkAbstractArray* abstract = input->GetVertexData()->GetAbstractArray(this->InputArrayName);

      // Does the array exist at all?
      if (abstract == NULL)
        {
        vtkErrorMacro("Could not find array named " << this->InputArrayName);
        return 0;
        }

      this->OriginVertexIndex = this->GetVertexIndex(abstract,this->OriginValue);
      }
    }

  vtkAbstractArray * abstractEdgeWeightArray = NULL;
  vtkDoubleArray   * edgeWeightArray = NULL;
  bool edgeWeightArrayIsTemporary = false;

  if(!this->UseUniformEdgeWeights)
    {
    // Retrieve the edge-weight array.
    if (!this->EdgeWeightArrayName)
      {
      vtkErrorMacro("Edge-weight array name is required");
      return 1;
      }
    abstractEdgeWeightArray = input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName);

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

    edgeWeightArray = vtkDoubleArray::SafeDownCast(abstractEdgeWeightArray);
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
    }
  else
    {
    // Create a default edge weight array with uniform edge weights.
    edgeWeightArray = vtkDoubleArray::New();
    edgeWeightArray->SetNumberOfTuples(output->GetNumberOfEdges());
    edgeWeightArrayIsTemporary = true;

    for(vtkIdType i=0; i<output->GetNumberOfEdges(); ++i)
      {
      edgeWeightArray->SetTuple1(i, 1.0);
      }
    }

  // Create the predecessor array
  vtkIdTypeArray* PredecessorArray = vtkIdTypeArray::New();
  if (this->PredecessorArrayName)
    {
    PredecessorArray->SetName(this->PredecessorArrayName);
    }
  else
    {
    PredecessorArray->SetName("Predecessor");
    }
  PredecessorArray->SetNumberOfTuples(output->GetNumberOfVertices());

  // Create the path-length array
  vtkDoubleArray* PathLengthArray = vtkDoubleArray::New();
  if (this->PathLengthArrayName)
    {
    PathLengthArray->SetName(this->PathLengthArrayName);
    }
  else
    {
    PathLengthArray->SetName("PathLength");
    }
  PathLengthArray->SetNumberOfTuples(output->GetNumberOfVertices());

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
    vtkErrorMacro("Can only perform parallel shortest-paths on a Parallel BGL distributed graph");
    return 1;
    }

  int myRank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  // Distributed predecessor map
  typedef vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type
    DistributedPredecessorMap;
  DistributedPredecessorMap predecessorMap
    = MakeDistributedVertexPropertyMap(output, PredecessorArray);

  // Distributed path-length map
  typedef vtkDistributedVertexPropertyMapType<vtkDoubleArray>::type
    DistributedPathLengthMap;
  DistributedPathLengthMap pathLengthMap
    = MakeDistributedVertexPropertyMap(output, PathLengthArray);

  // Distributed edge-weight map
  typedef vtkDistributedEdgePropertyMapType<vtkDoubleArray>::type
    DistributedEdgeWeightMap;
  DistributedEdgeWeightMap edgeWeightMap
    = MakeDistributedEdgePropertyMap(output, edgeWeightArray);

  if (vtkDirectedGraph::SafeDownCast(output))
    {
    vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(output);
    if (this->Delta > 0.0)
      {
      boost::graph::distributed::delta_stepping_shortest_paths
        (g, this->OriginVertexIndex,
         predecessorMap, pathLengthMap, edgeWeightMap, this->Delta);
      }
    else
      {
      boost::graph::distributed::delta_stepping_shortest_paths
        (g, this->OriginVertexIndex,
         predecessorMap, pathLengthMap, edgeWeightMap);
      }
    }
  else
    {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(output);
    if (this->Delta > 0.0)
      {
      boost::graph::distributed::delta_stepping_shortest_paths
        (g, this->OriginVertexIndex,
         predecessorMap, pathLengthMap, edgeWeightMap, this->Delta);
      }
    else
      {
      boost::graph::distributed::delta_stepping_shortest_paths
        (g, this->OriginVertexIndex,
         predecessorMap, pathLengthMap, edgeWeightMap);
      }
    }

  // Since we know PredecessorArray will contain DistributedIds, we can flag
  // it by adding the key, DISTRIBUTEDIDS(), so that CollectGraph will know
  // to recalculate the values when collecting to 1 node.
  // This might also be used for repartitioning as well.
  PredecessorArray->GetInformation()->Set(
        vtkDistributedGraphHelper::DISTRIBUTEDVERTEXIDS(), 1);

  // Add output arrays to the output
  output->GetVertexData()->AddArray(PredecessorArray);
  PredecessorArray->Delete();
  output->GetVertexData()->AddArray(PathLengthArray);
  PathLengthArray->Delete();

  if (edgeWeightArrayIsTemporary)
    {
    edgeWeightArray->Delete();
    }

  if (this->OutputSelection)
    {
    vtkSelection* sel = vtkSelection::GetData(outputVector, 1);
    vtkSmartPointer<vtkIdTypeArray> ids =
      vtkSmartPointer<vtkIdTypeArray>::New();
    vtkSmartPointer<vtkSelectionNode> node =
      vtkSmartPointer<vtkSelectionNode>::New();

    // Set the output based on the output selection type
    if (!strcmp(OutputSelectionType,"MAX_DIST_FROM_ROOT"))
      {
      // Compute the vertex that is furthest from the root (but still
      // reachable).
      vtkIdType maxFromRootVertex = this->OriginVertexIndex;
      double maxDistance = 0.0;
      vtkSmartPointer<vtkVertexListIterator> vertices
        = vtkSmartPointer<vtkVertexListIterator>::New();
      output->GetVertices(vertices);
      while (vertices->HasNext())
        {
        vtkIdType v = vertices->Next();
        double dist = PathLengthArray->GetTuple1(helper->GetVertexIndex(v));
        if (dist != VTK_DOUBLE_MAX && dist > maxDistance)
          {
          maxFromRootVertex = v;
          maxDistance = dist;
          }
        }

      // Combine the furthest-from-root information from all
      // processes.
      using namespace boost::parallel;
      maxFromRootVertex = all_reduce(pbglHelper->GetProcessGroup(),
                                     vtkstd::make_pair(maxFromRootVertex,
                                                       maxDistance),
                                     furthest_vertex_double(output)).first;

      ids->InsertNextValue(maxFromRootVertex);
      }

    node->SetSelectionList(ids);
    node->SetContentType(vtkSelectionNode::INDICES);
    node->SetFieldType(vtkSelectionNode::POINT);
    sel->AddNode(node);
    }

  return 1;
}

void vtkPBGLShortestPaths::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "InputArrayName: "
     << (this->InputArrayName ? this->InputArrayName : "(none)") << endl;

  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)")
     << endl;

  os << indent << "Delta: " << this->Delta << endl;

  os << indent << "PredecessorArrayName: "
     << (this->PredecessorArrayName ? this->PredecessorArrayName : "(none)")
     << endl;

  os << indent << "PathLengthArrayName: "
     << (this->PathLengthArrayName ? this->PathLengthArrayName : "(none)")
     << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "OutputSelection: "
     << (this->OutputSelection ? "on" : "off") << endl;

  os << indent << "OriginFromSelection: "
     << (this->OriginFromSelection ? "on" : "off") << endl;

  os << indent << "UseUniformEdgeWeights: "
     << (this->UseUniformEdgeWeights ? "on" : "off") << endl;

  os << indent << "OutputSelectionType: "
     << (this->OutputSelectionType ? this->OutputSelectionType : "(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkPBGLShortestPaths::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPBGLShortestPaths::FillOutputPortInformation(
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

