/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeGraphs.cxx

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

#include "vtkMergeGraphs.h"

#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeTables.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkMutableGraphHelper.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <map>

vtkStandardNewMacro(vtkMergeGraphs);
//---------------------------------------------------------------------------
vtkMergeGraphs::vtkMergeGraphs()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->UseEdgeWindow = false;
  this->EdgeWindowArrayName = 0;
  this->SetEdgeWindowArrayName("time");
  this->EdgeWindow = 10000;
}

//---------------------------------------------------------------------------
vtkMergeGraphs::~vtkMergeGraphs()
{
  this->SetEdgeWindowArrayName(0);
}

//---------------------------------------------------------------------------
int vtkMergeGraphs::FillInputPortInformation(int port, vtkInformation *info)
{
  if(port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    }
  else if(port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//---------------------------------------------------------------------------
// Fills array_map with matching arrays from data1 to data2
void vtkMergeGraphsCreateArrayMapping(std::map<vtkAbstractArray*, vtkAbstractArray*>& array_map, vtkDataSetAttributes* data1, vtkDataSetAttributes* data2)
{
  vtkIdType narr1 = data1->GetNumberOfArrays();
  for (int arr1 = 0; arr1 < narr1; ++arr1)
    {
    vtkAbstractArray* a1 = data1->GetAbstractArray(arr1);
    vtkAbstractArray* a2 = data2->GetAbstractArray(a1->GetName());
    if (a2 && a1->GetDataType() == a2->GetDataType() && a1->GetNumberOfComponents() == a2->GetNumberOfComponents())
      {
      array_map[a1] = a2;
      }
    }
  // Force pedigree id array to match
  if (data1->GetPedigreeIds() && data2->GetPedigreeIds())
  array_map[data1->GetPedigreeIds()] = data2->GetPedigreeIds();
}

//---------------------------------------------------------------------------
// Uses array_map to append a row to data1 corresponding to
// row index2 of mapped arrays (which came from data2)
void vtkMergeGraphsAddRow(vtkDataSetAttributes* data1, vtkIdType index2, std::map<vtkAbstractArray*, vtkAbstractArray*>& array_map)
{
  int narr1 = data1->GetNumberOfArrays();
  for (int arr1 = 0; arr1 < narr1; ++arr1)
    {
    vtkAbstractArray* a1 = data1->GetAbstractArray(arr1);
    if (array_map.find(a1) != array_map.end())
      {
      vtkAbstractArray* a2 = array_map[a1];
      a1->InsertNextTuple(index2, a2);
      }
    else
      {
      vtkIdType num_values = a1->GetNumberOfTuples()*a1->GetNumberOfComponents();
      for (vtkIdType i = 0; i < a1->GetNumberOfComponents(); ++i)
        {
        a1->InsertVariantValue(num_values + i, vtkVariant());
        }
      }
    }
}

//---------------------------------------------------------------------------
int vtkMergeGraphs::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* graph1_info = inputVector[0]->GetInformationObject(0);
  vtkGraph* graph1 = vtkGraph::SafeDownCast(
    graph1_info->Get(vtkDataObject::DATA_OBJECT()));

  // Copy structure into output graph.
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkGraph* output = vtkGraph::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* graph2_info = inputVector[1]->GetInformationObject(0);
  if(!graph2_info)
    {
    // If no second graph provided, we're done
    output->CheckedShallowCopy(graph1);
    return 1;
    }

  vtkGraph* graph2 = vtkGraph::SafeDownCast(
    graph2_info->Get(vtkDataObject::DATA_OBJECT()));

  // Make a copy of the graph
  vtkSmartPointer<vtkMutableGraphHelper> builder = vtkSmartPointer<vtkMutableGraphHelper>::New();
  if (vtkDirectedGraph::SafeDownCast(output))
    {
    vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();
    builder->SetGraph(g);
    }
  else
    {
    vtkSmartPointer<vtkMutableUndirectedGraph> g = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
    builder->SetGraph(g);
    }
  builder->GetGraph()->DeepCopy(graph1);

  if (!this->ExtendGraph(builder, graph2))
    {
    return 0;
    }

  if (!output->CheckedShallowCopy(builder->GetGraph()))
    {
    vtkErrorMacro("Output graph format invalid.");
    return 0;
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkMergeGraphs::ExtendGraph(vtkMutableGraphHelper* builder, vtkGraph* graph2)
{
  vtkAbstractArray* ped_ids1 = builder->GetGraph()->GetVertexData()->GetPedigreeIds();
  if (!ped_ids1)
    {
    vtkErrorMacro("First graph must have pedigree ids");
    return 0;
    }

  vtkAbstractArray* ped_ids2 = graph2->GetVertexData()->GetPedigreeIds();
  if (!ped_ids1)
    {
    vtkErrorMacro("Second graph must have pedigree ids");
    return 0;
    }

  // Find matching vertex arrays
  std::map<vtkAbstractArray*, vtkAbstractArray*> vert_array_map;
  vtkDataSetAttributes* vert_data1 = builder->GetGraph()->GetVertexData();
  vtkMergeGraphsCreateArrayMapping(vert_array_map, vert_data1, graph2->GetVertexData());

  // Find graph1 vertices matching graph2's pedigree ids
  vtkIdType n2 = graph2->GetNumberOfVertices();
  std::vector<vtkIdType> graph2_to_graph1(n2);
  for (vtkIdType vert2 = 0; vert2 < n2; ++vert2)
    {
    vtkIdType vert1 = ped_ids1->LookupValue(ped_ids2->GetVariantValue(vert2));
    if (vert1 == -1)
      {
      vert1 = builder->AddVertex();
      vtkMergeGraphsAddRow(vert_data1, vert2, vert_array_map);
      }
    graph2_to_graph1[vert2] = vert1;
    }

  // Find matching edge arrays
  std::map<vtkAbstractArray*, vtkAbstractArray*> edge_array_map;
  vtkDataSetAttributes* edge_data1 = builder->GetGraph()->GetEdgeData();
  vtkMergeGraphsCreateArrayMapping(edge_array_map, edge_data1, graph2->GetEdgeData());

  // For each edge in graph2, add it to the output
  vtkSmartPointer<vtkEdgeListIterator> it = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph2->GetEdges(it);
  while (it->HasNext())
    {
    vtkEdgeType e = it->Next();
    vtkIdType source = graph2_to_graph1[e.Source];
    vtkIdType target = graph2_to_graph1[e.Target];
    if (source != -1 && target != -1)
      {
      builder->AddEdge(source, target);
      vtkMergeGraphsAddRow(edge_data1, e.Id, edge_array_map);
      }
    }

  // Remove edges if using an edge window.
  if (this->UseEdgeWindow)
    {
    if (!this->EdgeWindowArrayName)
      {
      vtkErrorMacro("EdgeWindowArrayName must not be null if using edge window.");
      return 0;
      }
    vtkDataArray* windowArr = vtkDataArray::SafeDownCast(
      builder->GetGraph()->GetEdgeData()->GetAbstractArray(this->EdgeWindowArrayName));
    if (!windowArr)
      {
      vtkErrorMacro("EdgeWindowArrayName not found or not a numeric array.");
      return 0;
      }
    double range[2];
    range[0] = VTK_DOUBLE_MAX;
    range[1] = VTK_DOUBLE_MIN;
    vtkIdType numEdges = builder->GetGraph()->GetNumberOfEdges();
    for (vtkIdType i = 0; i < numEdges; ++i)
      {
      double val = windowArr->GetTuple1(i);
      if (val < range[0])
        {
        range[0] = val;
        }
      if (val > range[1])
        {
        range[1] = val;
        }
      }
    double cutoff = range[1] - this->EdgeWindow;
    if (range[0] < cutoff)
      {
      vtkSmartPointer<vtkIdTypeArray> edgesToRemove =
        vtkSmartPointer<vtkIdTypeArray>::New();
      for (vtkIdType i = 0; i < numEdges; ++i)
        {
        if (windowArr->GetTuple1(i) < cutoff)
          {
          edgesToRemove->InsertNextValue(i);
          }
        }
      builder->RemoveEdges(edgesToRemove);
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkMergeGraphs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseEdgeWindow: " << this->UseEdgeWindow << endl;
  os << indent << "EdgeWindowArrayName: "
     << (this->EdgeWindowArrayName ? this->EdgeWindowArrayName : "(none)") << endl;
  os << indent << "EdgeWindow: " << this->EdgeWindow << endl;
}
