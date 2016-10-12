/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineGraphSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArray.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotationLink.h"
#include "vtkArrayData.h"
#include "vtkCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPipelineGraphSource.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"

#include <map>
#include <sstream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkPipelineGraphSource);

// ----------------------------------------------------------------------

vtkPipelineGraphSource::vtkPipelineGraphSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Sinks = vtkCollection::New();
}

// ----------------------------------------------------------------------

vtkPipelineGraphSource::~vtkPipelineGraphSource()
{
  if (this->Sinks)
  {
    this->Sinks->Delete();
    this->Sinks = NULL;
  }
}

// ----------------------------------------------------------------------

void vtkPipelineGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

void vtkPipelineGraphSource::AddSink(vtkObject* sink)
{
  if (sink != NULL && !this->Sinks->IsItemPresent(sink))
  {
    this->Sinks->AddItem(sink);
    this->Modified();
  }
}

void vtkPipelineGraphSource::RemoveSink(vtkObject* sink)
{
  if (sink != NULL && this->Sinks->IsItemPresent(sink))
  {
    this->Sinks->RemoveItem(sink);
    this->Modified();
  }
}

// ----------------------------------------------------------------------

static void InsertObject(
  vtkObject* object,
  std::map<vtkObject*, vtkIdType>& object_map,
  vtkMutableDirectedGraph* builder,
  vtkStringArray* vertex_class_name_array,
  vtkVariantArray* vertex_object_array,
  vtkStringArray* edge_output_port_array,
  vtkStringArray* edge_input_port_array,
  vtkStringArray* edge_class_name_array,
  vtkVariantArray* edge_object_array)
{
  if(!object)
    return;

  if(object_map.count(object))
    return;

  // Insert pipeline algorithms ...
  if(vtkAlgorithm* const algorithm = vtkAlgorithm::SafeDownCast(object))
  {
    object_map[algorithm] = builder->AddVertex();
    vertex_class_name_array->InsertNextValue(algorithm->GetClassName());
    vertex_object_array->InsertNextValue(algorithm);

    // Recursively insert algorithm inputs ...
    for(int i = 0; i != algorithm->GetNumberOfInputPorts(); ++i)
    {
      for(int j = 0; j != algorithm->GetNumberOfInputConnections(i); ++j)
      {
        vtkAlgorithm* const input_algorithm = algorithm->GetInputConnection(i, j)->GetProducer();
        InsertObject(input_algorithm, object_map, builder, vertex_class_name_array, vertex_object_array, edge_output_port_array, edge_input_port_array, edge_class_name_array, edge_object_array);

        builder->AddEdge(object_map[input_algorithm], object_map[algorithm]);

        vtkDataObject* input_data = input_algorithm->GetOutputDataObject(algorithm->GetInputConnection(i, j)->GetIndex());
        edge_output_port_array->InsertNextValue(vtkVariant(algorithm->GetInputConnection(i, j)->GetIndex()).ToString());
        edge_input_port_array->InsertNextValue(vtkVariant(i).ToString());
        edge_class_name_array->InsertNextValue(input_data ? input_data->GetClassName() : "");
        edge_object_array->InsertNextValue(input_data);
      }
    }
  }
}

int vtkPipelineGraphSource::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  // Setup the graph data structure ...
  VTK_CREATE(vtkMutableDirectedGraph, builder);

  vtkStringArray* vertex_class_name_array = vtkStringArray::New();
  vertex_class_name_array->SetName("class_name");
  builder->GetVertexData()->AddArray(vertex_class_name_array);
  vertex_class_name_array->Delete();

  vtkVariantArray* vertex_object_array = vtkVariantArray::New();
  vertex_object_array->SetName("object");
  builder->GetVertexData()->AddArray(vertex_object_array);
  vertex_object_array->Delete();

  vtkStringArray* edge_output_port_array = vtkStringArray::New();
  edge_output_port_array->SetName("output_port");
  builder->GetEdgeData()->AddArray(edge_output_port_array);
  edge_output_port_array->Delete();

  vtkStringArray* edge_input_port_array = vtkStringArray::New();
  edge_input_port_array->SetName("input_port");
  builder->GetEdgeData()->AddArray(edge_input_port_array);
  edge_input_port_array->Delete();

  vtkStringArray* edge_class_name_array = vtkStringArray::New();
  edge_class_name_array->SetName("class_name");
  builder->GetEdgeData()->AddArray(edge_class_name_array);
  edge_class_name_array->Delete();

  vtkVariantArray* edge_object_array = vtkVariantArray::New();
  edge_object_array->SetName("object");
  builder->GetEdgeData()->AddArray(edge_object_array);
  edge_object_array->Delete();

  // Recursively insert pipeline components into the graph ...
  std::map<vtkObject*, vtkIdType> object_map;
  for(vtkIdType i = 0; i != this->Sinks->GetNumberOfItems(); ++i)
  {
    InsertObject(this->Sinks->GetItemAsObject(i), object_map, builder, vertex_class_name_array, vertex_object_array, edge_output_port_array, edge_input_port_array, edge_class_name_array, edge_object_array);
  }

  // Finish creating the output graph ...
  vtkDirectedGraph* const output_graph = vtkDirectedGraph::GetData(outputVector);
  if(!output_graph->CheckedShallowCopy(builder))
  {
    vtkErrorMacro(<<"Invalid graph structure");
    return 0;
  }

  return 1;
}


void vtkPipelineGraphSource::PipelineToDot(vtkAlgorithm* sink, ostream& output, const vtkStdString& graph_name)
{
  vtkSmartPointer<vtkCollection> sinks = vtkSmartPointer<vtkCollection>::New();
  sinks->AddItem(sink);

  PipelineToDot(sinks, output, graph_name);
}

namespace {

void replace_all(std::string& str, std::string oldStr, std::string newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
    str.replace(pos, oldStr.length(), newStr);
    pos += newStr.length();
  }
}

}

void vtkPipelineGraphSource::PipelineToDot(vtkCollection* sinks, ostream& output, const vtkStdString& graph_name)
{
  // Create a graph representation of the pipeline ...
  vtkSmartPointer<vtkPipelineGraphSource> pipeline = vtkSmartPointer<vtkPipelineGraphSource>::New();
  for(vtkIdType i = 0; i != sinks->GetNumberOfItems(); ++i)
  {
    pipeline->AddSink(sinks->GetItemAsObject(i));
  }
  pipeline->Update();
  vtkGraph* const pipeline_graph = pipeline->GetOutput();

  vtkAbstractArray* const vertex_object_array = pipeline_graph->GetVertexData()->GetAbstractArray("object");
  vtkAbstractArray* const edge_output_port_array = pipeline_graph->GetEdgeData()->GetAbstractArray("output_port");
  vtkAbstractArray* const edge_input_port_array = pipeline_graph->GetEdgeData()->GetAbstractArray("input_port");
  vtkAbstractArray* const edge_object_array = pipeline_graph->GetEdgeData()->GetAbstractArray("object");

  output << "digraph \"" << graph_name << "\"\n";
  output << "{\n";

  // Do some standard formatting ...
  output << "  node [ fontname=\"helvetica\" fontsize=\"10\" shape=\"record\" style=\"filled\" ]\n";
  output << "  edge [ fontname=\"helvetica\" fontsize=\"9\" ]\n\n";

  // Write-out vertices ...
  for(vtkIdType i = 0; i != pipeline_graph->GetNumberOfVertices(); ++i)
  {
    vtkObjectBase* const object = vertex_object_array->GetVariantValue(i).ToVTKObject();

    std::stringstream buffer;
    object->PrintSelf(buffer, vtkIndent());

    std::string line;
    std::string object_state;
    while(std::getline(buffer, line))
    {
      replace_all(line, "\"", "'");
      replace_all(line, "\r", "");
      replace_all(line, "\n", "");

      if(0 == line.find("Debug:"))
        continue;
      if(0 == line.find("Modified Time:"))
        continue;
      if(0 == line.find("Reference Count:"))
        continue;
      if(0 == line.find("Registered Events:"))
        continue;
      if(0 == line.find("Executive:"))
        continue;
      if(0 == line.find("ErrorCode:"))
        continue;
      if(0 == line.find("Information:"))
        continue;
      if(0 == line.find("AbortExecute:"))
        continue;
      if(0 == line.find("Progress:"))
        continue;
      if(0 == line.find("Progress Text:"))
        continue;
      if(0 == line.find("  "))
        continue;

      object_state += line + "\\n";
    }

    std::string fillcolor = "#ccffcc";
    if(vtkAnnotationLink::SafeDownCast(object))
    {
      fillcolor = "#ccccff";
    }

    output << "  " << "node_" << object << " [ fillcolor=\"" << fillcolor << "\" label=\"{" << object->GetClassName() << "|" << object_state << "}\" vtk_class_name=\"" << object->GetClassName() << "\" ]\n";
  }

  // Write-out edges ...
  vtkSmartPointer<vtkEdgeListIterator> edges = vtkSmartPointer<vtkEdgeListIterator>::New();
  edges->SetGraph(pipeline_graph);
  while(edges->HasNext())
  {
    vtkEdgeType edge = edges->Next();
    vtkObjectBase* const source = vertex_object_array->GetVariantValue(edge.Source).ToVTKObject();
    vtkObjectBase* const target = vertex_object_array->GetVariantValue(edge.Target).ToVTKObject();
    const vtkStdString output_port = edge_output_port_array->GetVariantValue(edge.Id).ToString();
    const vtkStdString input_port = edge_input_port_array->GetVariantValue(edge.Id).ToString();
    vtkObjectBase* const object = edge_object_array->GetVariantValue(edge.Id).ToVTKObject();

    std::string color = "black";
    if(vtkTree::SafeDownCast(object))
    {
      color = "#00bb00";
    }
    else if(vtkTable::SafeDownCast(object))
    {
      color = "blue";
    }
    else if(vtkArrayData* const array_data = vtkArrayData::SafeDownCast(object))
    {
      if(array_data->GetNumberOfArrays())
      {
        color = "";
        for(vtkIdType i = 0; i != array_data->GetNumberOfArrays(); ++i)
        {
          if(i)
            color += ":";

          if(array_data->GetArray(i)->IsDense())
            color += "purple";
          else
            color += "red";
        }
      }
    }
    else if(vtkGraph::SafeDownCast(object))
    {
      color = "#cc6600";
    }

    output << "  " << "node_" << source << " -> " << "node_" << target;
    output << " [";
    output << " color=\"" << color << "\" fontcolor=\"" << color << "\"";
    output << " label=\"" << (object ? object->GetClassName() : "") << "\"";
    output << " headlabel=\"" << input_port << "\"";
    output << " taillabel=\"" << output_port << "\"";
    output << " ]\n";
  }

  output << "}\n";
}
