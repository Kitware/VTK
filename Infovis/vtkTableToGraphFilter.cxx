/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToGraphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTableToGraphFilter.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkGraph.h"
#include "vtkGraphIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/map>

vtkCxxRevisionMacro(vtkTableToGraphFilter, "1.5");
vtkStandardNewMacro(vtkTableToGraphFilter);

vtkTableToGraphFilter::vtkTableToGraphFilter()
{
  this->Directed = 0;
  this->CollapseEdges = 0;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->Adj = vtkGraphIdList::New();
  this->Incident = vtkGraphIdList::New();

  // Set default input arrays
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "source");
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "target");
  this->SetInputArrayToProcess(2, 1, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "id");
  this->SetInputArrayToProcess(3, 1, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "x");
  this->SetInputArrayToProcess(4, 1, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "y");
  this->SetInputArrayToProcess(5, 1, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "z");
}

vtkTableToGraphFilter::~vtkTableToGraphFilter()
{
  if (this->Adj != NULL)
    {
    this->Adj->Delete();
    }
  if (this->Incident != NULL)
    {
    this->Incident->Delete();
    }
}

int vtkTableToGraphFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
    }
  return 0;
}

void vtkTableToGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "CollapseEdges: " << (this->CollapseEdges ? "On" : "Off") << endl;
}

vtkIdType vtkTableToGraphFilter::AppendEdge(
  vtkGraph* output, 
  vtkIdType sourceVertex, 
  vtkIdType targetVertex, 
  vtkIdTypeArray* weightArr)
{
  vtkIdType edge;
  output->GetAdjacentVertices(sourceVertex, this->Adj);
  if (this->Adj->IsId(targetVertex) != -1)
    {
    output->GetIncidentEdges(sourceVertex, this->Incident);
    edge = this->Incident->GetId(this->Adj->IsId(targetVertex));
    vtkIdType existingWeight = weightArr->GetValue(edge);
    weightArr->SetValue(edge, existingWeight + 1);
    }
  else
    {
    edge = output->AddEdge(sourceVertex, targetVertex);
    weightArr->InsertNextValue(1);
    }
  return edge;
}

int vtkTableToGraphFilter::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{

  // Extract edge and (possibly) vertex tables
  vtkInformation* edgeTableInfo = inputVector[0]->GetInformationObject(0);
  vtkTable* edgeTable = vtkTable::SafeDownCast(
    edgeTableInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation* vertexTableInfo = inputVector[1]->GetInformationObject(0);
  vtkTable* vertexTable = NULL;
  if (vertexTableInfo != NULL)
    {
    vertexTable = vtkTable::SafeDownCast(
      vertexTableInfo->Get(vtkDataObject::DATA_OBJECT()));
    }

  // Extract output graph
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkGraph* output = vtkGraph::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->SetDirected(this->Directed);

  // Extract edge source/target input arrays
  vtkAbstractArray* sourceArray = 
    this->GetInputAbstractArrayToProcess(0, inputVector);
  if (sourceArray == NULL)
    {
    vtkErrorMacro("Source array must be defined in input array 0");
    return 0;
    }
  vtkAbstractArray* targetArray = 
    this->GetInputAbstractArrayToProcess(1, inputVector);
  if (targetArray == NULL)
    {
    vtkErrorMacro("Target array must be defined in input array 1");
    return 0;
    }

  vtkIdTypeArray* weightArr = NULL;
  vtkAbstractArray* outputSourceArr = NULL;
  vtkStringArray* outputSourceArrStr = NULL;
  vtkDataArray* outputSourceArrData = NULL;
  vtkAbstractArray* outputTargetArr = NULL;
  vtkStringArray* outputTargetArrStr = NULL;
  vtkDataArray* outputTargetArrData = NULL;
  if (this->CollapseEdges)
    {
    // Create weight array; do not pass edge data
    weightArr = vtkIdTypeArray::New();
    weightArr->SetName("weight");
    output->GetEdgeData()->AddArray(weightArr);
    weightArr->Delete();

    outputSourceArr = vtkAbstractArray::CreateArray(sourceArray->GetDataType());
    outputSourceArr->SetName(sourceArray->GetName());
    output->GetEdgeData()->AddArray(outputSourceArr);
    outputSourceArr->Delete();
    outputSourceArrStr = vtkStringArray::SafeDownCast(outputSourceArr);
    outputSourceArrData = vtkDataArray::SafeDownCast(outputSourceArr);

    outputTargetArr = vtkAbstractArray::CreateArray(targetArray->GetDataType());
    outputTargetArr->SetName(targetArray->GetName());
    output->GetEdgeData()->AddArray(outputTargetArr);
    outputTargetArr->Delete();
    outputTargetArrStr = vtkStringArray::SafeDownCast(outputTargetArr);
    outputTargetArrData = vtkDataArray::SafeDownCast(outputTargetArr);
    }
  else
    {
    // Pass edge data to output graph
    output->GetEdgeData()->PassData(edgeTable->GetFieldData());
    }

  if (vertexTable != NULL)
    {
    // Extract vertex id input array
    vtkAbstractArray* vertexIdArray =
      this->GetInputAbstractArrayToProcess(2, inputVector);

    // Pass vertex data to output graph
    output->GetVertexData()->PassData(vertexTable->GetFieldData());

    // If the vertex table exists, first add vertices to the graph,
    // then add edges
    if ((vtkStringArray::SafeDownCast(sourceArray) != NULL) &&
        (vtkStringArray::SafeDownCast(targetArray) != NULL) &&
        (vtkStringArray::SafeDownCast(vertexIdArray) != NULL))
      {
      vtkStringArray* sourceArr = vtkStringArray::SafeDownCast(sourceArray);
      vtkStringArray* targetArr = vtkStringArray::SafeDownCast(targetArray);
      vtkStringArray* vertexIdArr = vtkStringArray::SafeDownCast(vertexIdArray);
      vtksys_stl::map<vtkStdString, vtkIdType> vertices;
      for (vtkIdType vertex = 0; vertex < vertexIdArr->GetNumberOfValues(); vertex++)
        {
        vertices[vertexIdArr->GetValue(vertex)] = output->AddVertex();
        }
      for (vtkIdType edge = 0; edge < sourceArr->GetNumberOfTuples(); edge++)
        {
        
        // Check to see if the source and target vertices exist
        if ((vertices.count(sourceArr->GetValue(edge)) == 0) ||
            (vertices.count(targetArr->GetValue(edge)) == 0))
            {
            continue;
            }   
         
        // Okay both the source and target exist so go forth   
        if (this->CollapseEdges)
          {
          vtkIdType a = this->AppendEdge(
            output, 
            vertices[sourceArr->GetValue(edge)], 
            vertices[targetArr->GetValue(edge)], 
            weightArr);
          outputSourceArrStr->InsertValue(a, sourceArr->GetValue(edge));
          outputTargetArrStr->InsertValue(a, targetArr->GetValue(edge));
          }
        else
          {
          output->AddEdge(vertices[sourceArr->GetValue(edge)], vertices[targetArr->GetValue(edge)]);
          }
        this->UpdateProgress(0.5 * static_cast<double>(edge) / 
                             static_cast<double>(sourceArr->GetNumberOfValues()));
        }
      }
    else if ((vtkDataArray::SafeDownCast(sourceArray) != NULL) &&
             (vtkDataArray::SafeDownCast(targetArray) != NULL) &&
             (vtkDataArray::SafeDownCast(vertexIdArray) != NULL))
      {
      vtkDataArray* sourceArr = vtkDataArray::SafeDownCast(sourceArray);
      vtkDataArray* targetArr = vtkDataArray::SafeDownCast(targetArray);
      vtkDataArray* vertexIdArr = vtkDataArray::SafeDownCast(vertexIdArray);
      vtksys_stl::map<double, vtkIdType> vertices;
      for (vtkIdType vertex = 0; vertex < vertexIdArr->GetNumberOfTuples(); vertex++)
        {
        vertices[vertexIdArr->GetTuple1(vertex)] = output->AddVertex();
        }
      for (vtkIdType edge = 0; edge < sourceArr->GetNumberOfTuples(); edge++)
        {
        
        // Check to see if the source and target vertices exist
        if ((vertices.count(sourceArr->GetTuple1(edge)) == 0) ||
            (vertices.count(targetArr->GetTuple1(edge)) == 0))
            {
            continue;
            }   
            
        // Okay both the source and target exist so go forth        
        if (this->CollapseEdges)
          {
          vtkIdType a = this->AppendEdge(
            output, 
            vertices[sourceArr->GetTuple1(edge)], 
            vertices[targetArr->GetTuple1(edge)], 
            weightArr);
          outputSourceArrData->InsertTuple1(a, sourceArr->GetTuple1(edge));
          outputTargetArrData->InsertTuple1(a, targetArr->GetTuple1(edge));
          }
        else
          {
          output->AddEdge(vertices[sourceArr->GetTuple1(edge)], vertices[targetArr->GetTuple1(edge)]);
          }
        this->UpdateProgress(0.5 * static_cast<double>(edge) / 
                             static_cast<double>(sourceArr->GetNumberOfTuples()));

        }
      }
    else
      {
      vtkErrorMacro(<< "Invalid source/target array type; may be a data array or string array");
      return 0;
      }

    // Load point data (if it exists)
    vtkDataArray* xArr = this->GetInputArrayToProcess(3, inputVector);
    if (xArr != NULL)
      {
      vtkDataArray* yArr = this->GetInputArrayToProcess(4, inputVector);
      vtkDataArray* zArr = NULL;
      zArr = this->GetInputArrayToProcess(5, inputVector);
      vtkPoints* points = vtkPoints::New();
      for (vtkIdType vertex = 0; vertex < output->GetNumberOfVertices(); vertex++)
        {
        double x = xArr->GetTuple1(vertex);
        double y = yArr->GetTuple1(vertex);
        double z;
        if (zArr)
          {
          z = zArr->GetTuple1(vertex);
          }
        else
          {
          z = 0;
          }
        points->InsertNextPoint(x, y, z);
        this->UpdateProgress(0.5 + 0.5 * (static_cast<double>(vertex) / 
                                    static_cast<double>(output->GetNumberOfVertices())));
        }
      output->SetPoints(points);
      points->Delete();
      }
    }
  else
    {
    // If no vertex table, vertices are implicit in edge table
    if ((vtkStringArray::SafeDownCast(sourceArray) != NULL) &&
        (vtkStringArray::SafeDownCast(targetArray) != NULL))
      {
      vtkStringArray* sourceArr = vtkStringArray::SafeDownCast(sourceArray);
      vtkStringArray* targetArr = vtkStringArray::SafeDownCast(targetArray);
      vtkStringArray* idArr = vtkStringArray::New();
      idArr->SetName("id");
      vtksys_stl::map<vtkStdString, vtkIdType> vertices;
      for (vtkIdType edge = 0; edge < sourceArr->GetNumberOfTuples(); edge++)
        {
        vtkIdType source, target;
        if (vertices.count(sourceArr->GetValue(edge)) > 0)
          {
          source = vertices[sourceArr->GetValue(edge)];
          }
        else
          {
          source = output->AddVertex();
          idArr->InsertNextValue(sourceArr->GetValue(edge));
          vertices[sourceArr->GetValue(edge)] = source;
          }
        if (vertices.count(targetArr->GetValue(edge)) > 0)
          {
          target = vertices[targetArr->GetValue(edge)];
          }
        else
          {
          target = output->AddVertex();
          idArr->InsertNextValue(targetArr->GetValue(edge));
          vertices[targetArr->GetValue(edge)] = target;
          }
        if (this->CollapseEdges)
          {
          vtkIdType a = this->AppendEdge(output, source, target, weightArr);
          outputSourceArrStr->InsertValue(a, sourceArr->GetValue(edge));
          outputTargetArrStr->InsertValue(a, targetArr->GetValue(edge));
          }
        else
          {
          output->AddEdge(source, target);
          }
        this->UpdateProgress(static_cast<double>(edge) / 
                             static_cast<double>(sourceArr->GetNumberOfTuples()));
        }
      output->GetVertexData()->AddArray(idArr);
      idArr->Delete();
      }
    else if ((vtkDataArray::SafeDownCast(sourceArray) != NULL)&&
             (vtkDataArray::SafeDownCast(targetArray) != NULL))
      {
      vtkDataArray* sourceArr = vtkDataArray::SafeDownCast(sourceArray);
      vtkDataArray* targetArr = vtkDataArray::SafeDownCast(targetArray);
      vtkDataArray* idArr = vtkDataArray::CreateDataArray(sourceArr->GetDataType());
      idArr->SetName("id");
      vtksys_stl::map<double, vtkIdType> vertices;
      for (vtkIdType edge = 0; edge < sourceArr->GetNumberOfTuples(); edge++)
        {
        vtkIdType source, target;
        if (vertices.count(sourceArr->GetTuple1(edge)) > 0)
          {
          source = vertices[sourceArr->GetTuple1(edge)];
          }
        else
          {
          source = output->AddVertex();
          idArr->InsertNextTuple(edge, sourceArr);
          vertices[sourceArr->GetTuple1(edge)] = source;
          }
        if (vertices.count(targetArr->GetTuple1(edge)) > 0)
          {
          target = vertices[targetArr->GetTuple1(edge)];
          }
        else
          {
          target = output->AddVertex();
          idArr->InsertNextTuple(edge, targetArr);
          vertices[targetArr->GetTuple1(edge)] = target;
          }
        if (this->CollapseEdges)
          {
          vtkIdType a = this->AppendEdge(output, source, target, weightArr);
          outputSourceArrData->InsertTuple1(a, source);
          outputTargetArrData->InsertTuple1(a, target);
          }
        else
          {
          output->AddEdge(source, target);
          }
        this->UpdateProgress(static_cast<double>(edge) / 
                             static_cast<double>(sourceArr->GetNumberOfTuples()));
        }
      output->GetVertexData()->AddArray(idArr);
      idArr->Delete();
      }
    else
      {
      vtkErrorMacro(<< "Invalid source/target array type; may be a data array or string array");
      return 0;
      }
    }

  return 1;
}

