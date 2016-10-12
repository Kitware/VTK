/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBreadthFirstSearch.cxx

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
#include "vtkBoostBreadthFirstSearch.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkBoostGraphAdapter.h"
#include "vtkDirectedGraph.h"
#include "vtkUndirectedGraph.h"

#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/pending/queue.hpp>

#include <utility> // for pair

using namespace boost;

vtkStandardNewMacro(vtkBoostBreadthFirstSearch);

// Redefine the bfs visitor, the only visitor we
// are using is the tree_edge visitor.
template <typename DistanceMap>
class my_distance_recorder : public default_bfs_visitor
{
public:
  my_distance_recorder() { }
  my_distance_recorder(DistanceMap dist, vtkIdType* far)
    : d(dist), far_vertex(far), far_dist(-1) { *far_vertex = -1; }

  template <typename Vertex, typename Graph>
  void examine_vertex(Vertex v, const Graph& vtkNotUsed(g))
  {
    if (get(d, v) > far_dist)
    {
      *far_vertex = v;
      far_dist = get(d, v);
    }
  }

  template <typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g)
  {
    typename graph_traits<Graph>::vertex_descriptor
    u = source(e, g), v = target(e, g);
    put(d, v, get(d, u) + 1);
  }

private:
  DistanceMap d;
  vtkIdType* far_vertex;
  vtkIdType far_dist;
};

// Constructor/Destructor
vtkBoostBreadthFirstSearch::vtkBoostBreadthFirstSearch()
{
  // Default values for the origin vertex
  this->OriginVertexIndex = 0;
  this->InputArrayName = 0;
  this->OutputArrayName = 0;
  this->OutputSelectionType = 0;
  this->SetOutputSelectionType("MAX_DIST_FROM_ROOT");
  this->OriginValue = -1;
  this->OutputSelection = false;
  this->OriginFromSelection = false;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

vtkBoostBreadthFirstSearch::~vtkBoostBreadthFirstSearch()
{
  this->SetInputArrayName(0);
  this->SetOutputArrayName(0);
  this->SetOutputSelectionType(0);
}

void vtkBoostBreadthFirstSearch::SetOriginSelection(vtkSelection* s)
{
  this->SetInputData(1, s);
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void vtkBoostBreadthFirstSearch::SetOriginVertex(vtkIdType index)
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
void vtkBoostBreadthFirstSearch::SetOriginVertex(
  vtkStdString arrayName, vtkVariant value)
{
  this->SetInputArrayName(arrayName);
  this->OriginValue = value;
  this->Modified();
}

void vtkBoostBreadthFirstSearch::SetOriginVertexString(
  char* arrayName, char* value)
{
  this->SetOriginVertex(arrayName, value);
}

vtkIdType vtkBoostBreadthFirstSearch::GetVertexIndex(
  vtkAbstractArray *abstract,vtkVariant value)
{

  // Okay now what type of array is it
  if (abstract->IsNumeric())
  {
    vtkDataArray *dataArray = vtkArrayDownCast<vtkDataArray>(abstract);
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
    vtkStringArray *stringArray = vtkArrayDownCast<vtkStringArray>(abstract);
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


int vtkBoostBreadthFirstSearch::RequestData(
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

  // Create the attribute array
  vtkIntArray* BFSArray = vtkIntArray::New();
  if (this->OutputArrayName)
  {
    BFSArray->SetName(this->OutputArrayName);
  }
  else
  {
    BFSArray->SetName("BFS");
  }
  BFSArray->SetNumberOfTuples(output->GetNumberOfVertices());

  // Initialize the BFS array to all 0's
  for(int i=0;i< BFSArray->GetNumberOfTuples(); ++i)
  {
      BFSArray->SetValue(i, VTK_INT_MAX);
  }

  vtkIdType maxFromRootVertex = this->OriginVertexIndex;

  // Create a color map (used for marking visited nodes)
  vector_property_map<default_color_type> color(output->GetNumberOfVertices());

  // Set the distance to the source vertex to zero
  BFSArray->SetValue(this->OriginVertexIndex, 0);

  // Create a queue to hand off to the BFS
  boost::queue<int> Q;

  my_distance_recorder<vtkIntArray*> bfsVisitor(BFSArray, &maxFromRootVertex);

  // Is the graph directed or undirected
  if (vtkDirectedGraph::SafeDownCast(output))
  {
    vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(output);
    breadth_first_search(g, this->OriginVertexIndex, Q, bfsVisitor, color);
  }
  else
  {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(output);
    breadth_first_search(g, this->OriginVertexIndex, Q, bfsVisitor, color);
  }

  // Add attribute array to the output
  output->GetVertexData()->AddArray(BFSArray);
  BFSArray->Delete();

  if (this->OutputSelection)
  {
    vtkSelection* sel = vtkSelection::GetData(outputVector, 1);
    vtkIdTypeArray* ids = vtkIdTypeArray::New();

    // Set the output based on the output selection type
    if (!strcmp(OutputSelectionType,"MAX_DIST_FROM_ROOT"))
    {
      ids->InsertNextValue(maxFromRootVertex);
    }

    vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
    sel->AddNode(node);
    node->SetSelectionList(ids);
    node->GetProperties()->Set(vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
    node->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::VERTEX);
    ids->Delete();
  }

  return 1;
}

void vtkBoostBreadthFirstSearch::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "InputArrayName: "
     << (this->InputArrayName ? this->InputArrayName : "(none)") << endl;

  os << indent << "OutputArrayName: "
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "OutputSelection: "
     << (this->OutputSelection ? "on" : "off") << endl;

  os << indent << "OriginFromSelection: "
     << (this->OriginFromSelection ? "on" : "off") << endl;

  os << indent << "OutputSelectionType: "
     << (this->OutputSelectionType ? this->OutputSelectionType : "(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkBoostBreadthFirstSearch::FillInputPortInformation(
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
int vtkBoostBreadthFirstSearch::FillOutputPortInformation(
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

