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

vtkCxxRevisionMacro(vtkExtractSelectedGraph, "1.8");
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

int vtkExtractSelectedGraph::ConvertToIndexSelection(
  vtkSelection* selection, 
  vtkDataSet* input, 
  vtkSelection* outputSelection)
{
  // Change the selection to preserve topology
  vtkSelection* selTemp = vtkSelection::New();
  selTemp->ShallowCopy(selection);
  selTemp->GetProperties()->Set(vtkSelection::PRESERVE_TOPOLOGY(), true);
  
  // Use the extraction filter to create an insidedness array.
  vtkExtractSelection* const extract = vtkExtractSelection::New();
  extract->SetInput(0, input);
  extract->SetInput(1, selTemp);
  extract->Update();
  vtkDataSet* const extracted = extract->GetOutput();
  selTemp->Delete();
  
  outputSelection->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  int type = selection->GetProperties()->Get(vtkSelection::FIELD_TYPE());
  outputSelection->GetProperties()->Set(vtkSelection::FIELD_TYPE(), type);
  vtkSignedCharArray* insidedness = 0;
  if (type == vtkSelection::CELL)
    {
    insidedness = vtkSignedCharArray::SafeDownCast(
      extracted->GetCellData()->GetAbstractArray("vtkInsidedness"));
    }
  else if (type == vtkSelection::POINT)
    {
    insidedness = vtkSignedCharArray::SafeDownCast(
      extracted->GetPointData()->GetAbstractArray("vtkInsidedness"));
    }
  else
    {
    vtkErrorMacro("Unknown field type");
    extract->Delete();
    return 0;
    }
  
  if (!insidedness)
    {
    vtkErrorMacro("Did not find expected vtkInsidedness array.");
    extract->Delete();
    return 0;
    }
  
  // Convert the insidedness array into an index selection.
  vtkIdTypeArray* indexArray = vtkIdTypeArray::New();
  for (vtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
    {
    if (insidedness->GetValue(i) == 1)
      {
      indexArray->InsertNextValue(i);
      }
    }
  outputSelection->SetSelectionList(indexArray);
  indexArray->Delete();
  extract->Delete();
  return 1;
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
  
  vtkSelection* indexSelection = vtkSelection::New();
  indexSelection->ShallowCopy(selection);
  
  int content = selection->GetProperties()->Get(selection->CONTENT_TYPE());
  if (content != vtkSelection::INDICES)
    {
    // Convert the selection to an INDICES selection
    int ret = this->ConvertToIndexSelection(selection, input, indexSelection);
    if (ret != 1)
      {
      vtkErrorMacro("Selection conversion to INDICES failed.");
      indexSelection->Delete();
      return 0;
      }
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
        
    // Copy selected edges
    vtkCellData* inputEdgeData = input->GetEdgeData();
    vtkCellData* outputEdgeData = output->GetEdgeData();
    outputEdgeData->CopyAllocate(inputEdgeData);
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
    
    // Copy selected vertices
    double pt[3];
    vtkPoints* inputPoints = input->GetPoints();
    vtkPoints* outputPoints = vtkPoints::New();
    vtkPointData* inputVertexData = input->GetVertexData();
    vtkPointData* outputVertexData = output->GetVertexData();
    outputVertexData->CopyAllocate(inputVertexData);
    map<vtkIdType, vtkIdType> idMap;
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
    output->SetPoints(outputPoints);
    outputPoints->Delete();
  
    // Copy any edges that connect selected vertices
    vtkCellData* inputEdgeData = input->GetEdgeData();
    vtkCellData* outputEdgeData = output->GetEdgeData();
    outputEdgeData->CopyAllocate(inputEdgeData);
    vtkGraphIdList* edgeList = vtkGraphIdList::New();
    for (vtkIdType i = 0; i < selectSize; i++)
      {
      vtkIdType inputVertex = selectArr->GetValue(i);
      if (idMap.count(inputVertex) > 0)
        {
        vtkIdType outputVertex = idMap[inputVertex];
        input->GetOutEdges(inputVertex, edgeList);
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
    edgeList->Delete();
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

