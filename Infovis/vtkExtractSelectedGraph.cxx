/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedGraph.cxx

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

#include "vtkExtractSelectedGraph.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelection.h"
#include "vtkGraph.h"
#include "vtkGraphIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkCxxRevisionMacro(vtkExtractSelectedGraph, "1.12");
vtkStandardNewMacro(vtkExtractSelectedGraph);

vtkExtractSelectedGraph::vtkExtractSelectedGraph()
{
  this->SetNumberOfInputPorts(2);
  this->RemoveIsolatedVertices = true;
}

vtkExtractSelectedGraph::~vtkExtractSelectedGraph()
{
}

int vtkExtractSelectedGraph::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAbstractGraph");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
    }
  return 0;
}

void vtkExtractSelectedGraph::SetSelectionConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

int vtkExtractSelectedGraph::RequestData(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkAbstractGraph* input = vtkAbstractGraph::GetData(inputVector[0]);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1]);

  // If there is nothing in the list, there is nothing to select.
  vtkAbstractArray* list = selection->GetSelectionList();
  if (!list || list->GetNumberOfTuples() == 0)
    {
    return 1;
    }

  bool inverse = false;
  if(selection->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    inverse = selection->GetProperties()->Get(vtkSelection::INVERSE());
    }
  
  // If it is a selection with multiple parts, find a point or cell
  // child selection, with preference to points.
  if (selection->GetContentType() == vtkSelection::SELECTIONS)
    {
    vtkSelection* child = 0;
    for (unsigned int i = 0; i < selection->GetNumberOfChildren(); i++)
      {
      vtkSelection* cur = selection->GetChild(i);
      if (cur->GetFieldType() == vtkSelection::POINT)
        {
        child = cur;
        break;
        }
      else if (cur->GetFieldType() == vtkSelection::CELL)
        {
        child = cur;
        }
      }
    selection = child;
    }
    
  // Convert the selection to an INDICES selection
  vtkSelection* indexSelection = 
    vtkConvertSelection::ToIndexSelection(selection, input);
  if (!indexSelection)
    {
    vtkErrorMacro("Selection conversion to INDICES failed.");
    indexSelection->Delete();
    return 0;
    }
  
  vtkAbstractArray* arr = indexSelection->GetSelectionList();
  if (arr == NULL)
    {
    vtkErrorMacro("Selection list not found.");
    return 0;
    }
  vtkIdTypeArray* selectArr = vtkIdTypeArray::SafeDownCast(arr);
  if (selectArr == NULL)
    {
    vtkErrorMacro("Selection list must be of type vtkIdTypeArray.");
    return 0;
    }
  vtkIdType selectSize = selectArr->GetNumberOfTuples();
  
  vtkGraph* output = vtkGraph::GetData(outputVector);

  output->SetDirected(input->GetDirected());
  output->GetFieldData()->PassData(input->GetFieldData());


  if (selection->GetProperties()->Has(vtkSelection::FIELD_TYPE()) && 
      selection->GetProperties()->Get(vtkSelection::FIELD_TYPE()) == vtkSelection::CELL)
    {
    //
    // Edge selection
    //
    
    // Copy all vertices
    output->SetNumberOfVertices(input->GetNumberOfVertices());
        
    vtkCellData* inputEdgeData = input->GetEdgeData();
    vtkCellData* outputEdgeData = output->GetEdgeData();
    outputEdgeData->CopyAllocate(inputEdgeData);

    // Copy unselected edges
    if(inverse)
      {
      for (vtkIdType i = 0; i < inputEdgeData->GetNumberOfTuples(); i++)
        {
        if(selectArr->LookupValue(i) < 0 )
          {
          vtkIdType source = input->GetSourceVertex(i);
          vtkIdType target = input->GetTargetVertex(i);
          vtkIdType outputEdge = output->AddEdge(source, target);
          outputEdgeData->CopyData(inputEdgeData, i, outputEdge);
          }
        }
      }
    // Copy selected edges
    else
      {
      for (vtkIdType i = 0; i < selectSize; i++)
        {
        vtkIdType inputEdge = selectArr->GetValue(i);
        if (inputEdge < input->GetNumberOfEdges())
          {
          vtkIdType source = input->GetSourceVertex(inputEdge);
          vtkIdType target = input->GetTargetVertex(inputEdge);
          vtkIdType outputEdge = output->AddEdge(source, target);
          outputEdgeData->CopyData(inputEdgeData, inputEdge, outputEdge);
          }
        }
      }

    // Remove isolated vertices
    if (this->RemoveIsolatedVertices)
      {
      output->GetVertexData()->DeepCopy(input->GetVertexData());
      output->GetPoints()->DeepCopy(input->GetPoints());
      vtkIdTypeArray* isolated = vtkIdTypeArray::New();
      for (vtkIdType i = 0; i < output->GetNumberOfVertices(); i++)
        {
        if (output->GetDegree(i) == 0)
          {
          isolated->InsertNextValue(i);
          }
        }
      output->RemoveVertices(isolated->GetPointer(0), isolated->GetNumberOfTuples());
      isolated->Delete();
      }
    else
      {
      output->GetVertexData()->PassData(input->GetVertexData());
      output->GetPoints()->ShallowCopy(input->GetPoints());
      }
    }
  else
    {
    //
    // Vertex selection
    //
    
    double pt[3];
    vtkPoints* inputPoints = input->GetPoints();
    vtkPoints* outputPoints = vtkPoints::New();
    vtkPointData* inputVertexData = input->GetVertexData();
    vtkPointData* outputVertexData = output->GetVertexData();
    outputVertexData->CopyAllocate(inputVertexData);
    map<vtkIdType, vtkIdType> idMap;

    // Copy unselected vertices
    if(inverse)
      {
      for (vtkIdType i = 0; i < inputVertexData->GetNumberOfTuples(); i++)
        {
        if(selectArr->LookupValue(i) < 0)
          {
          vtkIdType outputVertex = output->AddVertex();
          outputVertexData->CopyData(inputVertexData, i, outputVertex);
          idMap[i] = outputVertex;
          inputPoints->GetPoint(i, pt);
          outputPoints->InsertNextPoint(pt);
          }
        }
      }
    // Copy selected vertices
    else
      {
      for (vtkIdType i = 0; i < selectSize; i++)
        {
        vtkIdType inputVertex = selectArr->GetValue(i);
        if (inputVertex < input->GetNumberOfVertices())
          {
          vtkIdType outputVertex = output->AddVertex();
          outputVertexData->CopyData(inputVertexData, inputVertex, outputVertex);
          idMap[inputVertex] = outputVertex;
          inputPoints->GetPoint(inputVertex, pt);
          outputPoints->InsertNextPoint(pt);
          }
        }
      }
    output->SetPoints(outputPoints);
    outputPoints->Delete();
    
    // Make a directed shallow copy of the graph so GetOutEdges()
    // returns every edge no more than once.
    vtkAbstractGraph* copy = input;
    bool madeCopy = false;
    if (vtkGraph::SafeDownCast(input) && !input->GetDirected())
      {
      copy = vtkGraph::New();
      copy->ShallowCopy(input);
      vtkGraph::SafeDownCast(copy)->SetDirected(true);
      madeCopy = true;
      }
  
    vtkCellData* inputEdgeData = input->GetEdgeData();
    vtkCellData* outputEdgeData = output->GetEdgeData();
    outputEdgeData->CopyAllocate(inputEdgeData);
    vtkGraphIdList* edgeList = vtkGraphIdList::New();

    // Copy any edges that connect UNSELECTED vertices
    if(inverse)
      {
      map<vtkIdType, vtkIdType>::iterator mapIter = idMap.begin();
      while(mapIter != idMap.end())
        {
        vtkIdType inputVertex = mapIter->first;
        vtkIdType outputVertex = mapIter->second;
        copy->GetOutEdges(inputVertex, edgeList);
        for (vtkIdType j = 0; j < edgeList->GetNumberOfIds(); j++)
          {
          vtkIdType inputEdge = edgeList->GetId(j);
          vtkIdType oppInputVertex = input->GetOppositeVertex(inputEdge, inputVertex);
          if (idMap.count(oppInputVertex) > 0)
            {
            vtkIdType oppOutputVertex = idMap[oppInputVertex];
            vtkIdType outputEdge = output->AddEdge(outputVertex, oppOutputVertex);
            outputEdgeData->CopyData(inputEdgeData, inputEdge, outputEdge);
            }
          }
        mapIter++;
        }
      }
    // Copy any edges that connect SELECTED vertices
    else
      {
      for (vtkIdType i = 0; i < selectSize; i++)
        {
        vtkIdType inputVertex = selectArr->GetValue(i);
        if (idMap.count(inputVertex) > 0)
          {
          vtkIdType outputVertex = idMap[inputVertex];
          copy->GetOutEdges(inputVertex, edgeList);
          for (vtkIdType j = 0; j < edgeList->GetNumberOfIds(); j++)
            {
            vtkIdType inputEdge = edgeList->GetId(j);
            vtkIdType oppInputVertex = input->GetOppositeVertex(inputEdge, inputVertex);
            if (idMap.count(oppInputVertex) > 0)
              {
              vtkIdType oppOutputVertex = idMap[oppInputVertex];
              vtkIdType outputEdge = output->AddEdge(outputVertex, oppOutputVertex);
              outputEdgeData->CopyData(inputEdgeData, inputEdge, outputEdge);
              }
            }
          }
        }
      }
    edgeList->Delete();
    
    // Clean up
    if (madeCopy)
      {
      copy->Delete();
      }
    }

  // Clean up
  output->Squeeze();
  indexSelection->Delete();

  return 1;
}

void vtkExtractSelectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoveIsolatedVertices: " 
     << (this->RemoveIsolatedVertices ? "on" : "off") << endl;
}

