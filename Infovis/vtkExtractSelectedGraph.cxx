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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkExtractSelectedGraph.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelection.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkVertexListIterator.h"

#include <vtksys/stl/map>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkExtractSelectedGraph, "1.22");
vtkStandardNewMacro(vtkExtractSelectedGraph);
//----------------------------------------------------------------------------
vtkExtractSelectedGraph::vtkExtractSelectedGraph()
{
  this->SetNumberOfInputPorts(2);
  this->RemoveIsolatedVertices = true;
}

//----------------------------------------------------------------------------
vtkExtractSelectedGraph::~vtkExtractSelectedGraph()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectedGraph::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedGraph::SetSelectionConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedGraph::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkGraph *output = vtkGraph::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    
    // Output a vtkDirectedGraph if the input is a tree.
    if (!output 
        || (vtkTree::SafeDownCast(input) && !vtkDirectedGraph::SafeDownCast(output)) 
        || (!vtkTree::SafeDownCast(input) && !output->IsA(input->GetClassName())))
      {
      if (vtkTree::SafeDownCast(input))
        {
        output = vtkDirectedGraph::New();
        }
      else
        {
        output = input->NewInstance();
        }
      output->SetPipelineInformation(info);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedGraph::RequestData(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkGraph* input = vtkGraph::GetData(inputVector[0]);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1]);

  // If there is no list, there is nothing to select.
  vtkAbstractArray* list = selection->GetSelectionList();
  if (!list)
    {
    return 1;
    }

  int inverse = 0;
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
      if (cur->GetFieldType() == vtkSelection::VERTEX)
        {
        child = cur;
        break;
        }
      else if (cur->GetFieldType() == vtkSelection::EDGE)
        {
        child = cur;
        }
      }
    selection = child;
    }
    
  // Convert the selection to an INDICES selection
  vtkSmartPointer<vtkSelection> indexSelection;
  indexSelection.TakeReference(
    vtkConvertSelection::ToIndexSelection(selection, input));
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
  
  bool directed = false;
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    directed = true;
    }

  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder = 
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder = 
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  if (selection->GetProperties()->Has(vtkSelection::FIELD_TYPE()) && 
      selection->GetProperties()->Get(vtkSelection::FIELD_TYPE()) == vtkSelection::EDGE)
    {
    //
    // Edge selection
    //

    vtkDataSetAttributes *inputEdgeData = input->GetEdgeData();
    vtkDataSetAttributes *builderEdgeData = 0;
    if (directed)
      {
      builderEdgeData = dirBuilder->GetEdgeData();
      }
    else
      {
      builderEdgeData = undirBuilder->GetEdgeData();
      }

    builderEdgeData->CopyAllocate(inputEdgeData);


    // Handle the case where we are not outputing isolated vertices separately:
    if(this->RemoveIsolatedVertices)
      {
      // Create and initialize vector that keeps track of which
      // vertices are connected by an edge (1) and which aren't (0).
      
      unsigned int idx;
      vtkstd::vector<int> connectedVertices;
      for(int j=0; j<input->GetNumberOfVertices(); ++j)
        {
        connectedVertices.push_back(0);
        }

      vtkSmartPointer<vtkEdgeListIterator> edgeIter =
        vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edgeIter);
      while (edgeIter->HasNext())
        {
        vtkEdgeType e = edgeIter->Next();

        if ((inverse && selectArr->LookupValue(e.Id) < 0) ||
            (!inverse && selectArr->LookupValue(e.Id) >= 0))
          {
          connectedVertices[e.Source] = 1;
          connectedVertices[e.Target] = 1;
          }
        }

      // Create a mapping between the input vertex id and the output vertex id.
      // Right now we rely on the fact that the input vertex ids are consecutive 
      // integers from 0 to NUMBER_OF_VERTICES -1. So the index into the 
      // vertexMap vector below actually represents the input vertex id.

      int numConnected = 0;
      vtkstd::vector<int> vertexMap;
      for(idx=0; idx<connectedVertices.size(); ++idx)
        {
        vertexMap.push_back(numConnected);

        if(connectedVertices[idx] == 1)
          {
          numConnected++;
          }
        }
    
      // Copy only the connected vertices
      for (int j = 0; j < numConnected; ++j)
        {
        if (directed)
          {
          dirBuilder->AddVertex();
          }
        else
          {
          undirBuilder->AddVertex();
          }
        }

      // Construct the output vertex data and vtkPoints using the new vertex ids

      vtkPoints* newPoints = vtkPoints::New();
      newPoints->SetNumberOfPoints(numConnected);
      vtkDataSetAttributes *newVertexData = vtkDataSetAttributes::New();
      newVertexData->CopyStructure(input->GetVertexData());

      if (directed)
        {
        for (idx = 0; idx<connectedVertices.size(); idx++)
          {
          if(connectedVertices[idx]==1)
            {
            newVertexData->InsertTuple(vertexMap[idx], idx, input->GetVertexData());
            newPoints->SetPoint(vertexMap[idx], input->GetPoints()->GetPoint(idx));
            }
          }
        dirBuilder->GetVertexData()->PassData(newVertexData);
        dirBuilder->SetPoints(newPoints);
        }
      else
        {
        for (idx = 0; idx<connectedVertices.size(); idx++)
          {
          if(connectedVertices[idx]==1)
            {
            newVertexData->InsertTuple(vertexMap[idx], idx, input->GetVertexData());
            newPoints->SetPoint(vertexMap[idx], input->GetPoints()->GetPoint(idx));
            }
          }
        undirBuilder->GetVertexData()->PassData(newVertexData);
        undirBuilder->SetPoints(newPoints);
        }

      newPoints->Delete();
      newVertexData->Delete();

      // Now actually copy the edges (only the selected ones) 
      // using the new vertex ids.

      vtkSmartPointer<vtkEdgeListIterator> edges =
        vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edges);
      while (edges->HasNext())
        {
        vtkEdgeType e = edges->Next();

        if ((inverse && selectArr->LookupValue(e.Id) < 0) ||
            (!inverse && selectArr->LookupValue(e.Id) >= 0))
          {
          vtkEdgeType outputEdge;
          if (directed)
            {
            outputEdge = dirBuilder->AddEdge(vertexMap[e.Source], vertexMap[e.Target]);
            }
          else
            {
            outputEdge = undirBuilder->AddEdge(vertexMap[e.Source], vertexMap[e.Target]);
            }
          builderEdgeData->CopyData(inputEdgeData, e.Id, outputEdge.Id);
          }
        }
      }
    else  // !this->RemoveIsolatedVertices
      {

      // Copy all vertices
      for (vtkIdType v = 0; v < input->GetNumberOfVertices(); ++v)
        {
        if (directed)
          {
          dirBuilder->AddVertex();
          }
        else
          {
          undirBuilder->AddVertex();
          }
        }

      // Copy unselected edges
      vtkSmartPointer<vtkEdgeListIterator> edges =
        vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edges);
      while (edges->HasNext())
        {
        vtkEdgeType e = edges->Next();

        if ((inverse && selectArr->LookupValue(e.Id) < 0) ||
            (!inverse && selectArr->LookupValue(e.Id) >= 0))
          {
          vtkEdgeType outputEdge;
          if (directed)
            {
            outputEdge = dirBuilder->AddEdge(e.Source, e.Target);
            }
          else
            {
            outputEdge = undirBuilder->AddEdge(e.Source, e.Target);
            }
          builderEdgeData->CopyData(inputEdgeData, e.Id, outputEdge.Id);
          }
        }

      if (directed)
        {
        dirBuilder->GetVertexData()->PassData(input->GetVertexData());
        dirBuilder->GetPoints()->ShallowCopy(input->GetPoints());
        }
      else
        {
        undirBuilder->GetVertexData()->PassData(input->GetVertexData());
        undirBuilder->GetPoints()->ShallowCopy(input->GetPoints());
        }
      }
    }
  else
    {
    //
    // Vertex selection
    //
   
    double pt[3];
    vtkPoints *inputPoints = input->GetPoints();
    vtkSmartPointer<vtkPoints> outputPoints = 
      vtkSmartPointer<vtkPoints>::New();
    vtkDataSetAttributes *inputVertexData = input->GetVertexData();
    vtkDataSetAttributes *builderVertexData = 0;
    if (directed)
      {
      builderVertexData = dirBuilder->GetVertexData();
      }
    else
      {
      builderVertexData = undirBuilder->GetVertexData();
      }
    builderVertexData->CopyAllocate(inputVertexData);
    vtksys_stl::map<vtkIdType, vtkIdType> idMap;

    // Copy unselected vertices
    if(inverse)
      {
      vtkSmartPointer<vtkVertexListIterator> vertices = 
        vtkSmartPointer<vtkVertexListIterator>::New();
      input->GetVertices(vertices);
      while (vertices->HasNext())
        {
        vtkIdType i = vertices->Next();
        if(selectArr->LookupValue(i) < 0)
          {
          vtkIdType outputVertex = -1;
          if (directed)
            {
            outputVertex = dirBuilder->AddVertex();
            }
          else
            {
            outputVertex = undirBuilder->AddVertex();
            }
          builderVertexData->CopyData(inputVertexData, i, outputVertex);
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
          vtkIdType outputVertex = -1;
          if (directed)
            {
            outputVertex = dirBuilder->AddVertex();
            }
          else
            {
            outputVertex = undirBuilder->AddVertex();
            }
          builderVertexData->CopyData(inputVertexData, inputVertex, outputVertex);
          idMap[inputVertex] = outputVertex;
          inputPoints->GetPoint(inputVertex, pt);
          outputPoints->InsertNextPoint(pt);
          }
        }
      }
    if (directed)
      {
      dirBuilder->SetPoints(outputPoints);
      }
    else
      {
      undirBuilder->SetPoints(outputPoints);
      }

    // Copy edges whose source and target are selected.
    vtkDataSetAttributes *inputEdgeData = input->GetEdgeData();
    vtkDataSetAttributes *builderEdgeData = 0;
    if (directed)
      {
      builderEdgeData = dirBuilder->GetEdgeData();
      }
    else
      {
      builderEdgeData = undirBuilder->GetEdgeData();
      }
    builderEdgeData->CopyAllocate(inputEdgeData);

    vtkSmartPointer<vtkEdgeListIterator> edges = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(edges);
    while (edges->HasNext())
      {
      vtkEdgeType e = edges->Next();
      if (idMap.count(e.Source) > 0 && idMap.count(e.Target) > 0)
        {
        vtkEdgeType outputEdge;
        if (directed)
          {
          outputEdge = dirBuilder->AddEdge(idMap[e.Source], idMap[e.Target]);
          }
        else
          {
          outputEdge = undirBuilder->AddEdge(idMap[e.Source], idMap[e.Target]);
          }
        builderEdgeData->CopyData(inputEdgeData, e.Id, outputEdge.Id);
        }
      }    
    }

  // Pass constructed graph to output.
  vtkGraph* output = vtkGraph::GetData(outputVector);
  if (directed)
    {
    if (!output->CheckedShallowCopy(dirBuilder))
      {
      vtkErrorMacro(<<"Invalid graph structure.");
      return 0;
      }
    }
  else
    {
    if (!output->CheckedShallowCopy(undirBuilder))
      {
      vtkErrorMacro(<<"Invalid graph structure.");
      return 0;
      }
    }
  output->GetFieldData()->PassData(input->GetFieldData());

  // Clean up
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoveIsolatedVertices: " 
     << (this->RemoveIsolatedVertices ? "on" : "off") << endl;
}
