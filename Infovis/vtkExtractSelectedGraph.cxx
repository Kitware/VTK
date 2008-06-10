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

#include "vtkCellArray.h"
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

vtkCxxRevisionMacro(vtkExtractSelectedGraph, "1.25");
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

  // Invert the selection if necessary
  int inverse = selection->GetProperties()->Get(vtkSelection::INVERSE());
  vtkSmartPointer<vtkIdTypeArray> inverted = 0;
  if (inverse)
    {
    vtkIdType numVert = input->GetNumberOfVertices();
    inverted = vtkSmartPointer<vtkIdTypeArray>::New();
    for (vtkIdType i = 0; i < numVert; ++i)
      {
      if (selectArr->LookupValue(i) < 0)
        {
        inverted->InsertNextValue(i);
        }
      }
    selectArr = inverted;
    }
  vtkIdType selectSize = selectArr->GetNumberOfTuples();
  
  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder = 
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder = 
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  bool directed;
  vtkGraph* builder = 0;
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    directed = true;
    builder = dirBuilder;
    }
  else
    {
    directed = false;
    builder = undirBuilder;
    }

  // Use default selection type unless explicitly set to vertex or edge
  int selType = vtkSelection::VERTEX;
  if (selection->GetFieldType() == vtkSelection::EDGE ||
      selection->GetFieldType() == vtkSelection::VERTEX)
    {
    selType = selection->GetFieldType();
    }

  if (selType == vtkSelection::EDGE)
    {
    //
    // Edge selection
    //

    vtkDataSetAttributes *inputEdgeData = input->GetEdgeData();
    vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();
    builderEdgeData->CopyAllocate(inputEdgeData);

    // Handle the case where we are not outputing isolated vertices separately:
    if(this->RemoveIsolatedVertices)
      {
      vtkDataSetAttributes *inputVertexData = input->GetVertexData();
      vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
      builderVertexData->CopyAllocate(inputVertexData);
      vtkSmartPointer<vtkPoints> newPoints =
        vtkSmartPointer<vtkPoints>::New();

      vtkstd::vector<vtkIdType> outputId(input->GetNumberOfVertices(), -1);
      vtkSmartPointer<vtkEdgeListIterator> edgeIter =
        vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edgeIter);
      while (edgeIter->HasNext())
        {
        vtkEdgeType e = edgeIter->Next();
        if (selectArr->LookupValue(e.Id) >= 0)
          {
          vtkIdType u = outputId[e.Source];
          if (u == -1)
            {
            if (directed)
              {
              u = dirBuilder->AddVertex();
              }
            else
              {
              u = undirBuilder->AddVertex();
              }
            outputId[e.Source] = u;
            builderVertexData->CopyData(inputVertexData, e.Source, u);
            newPoints->InsertNextPoint(input->GetPoints()->GetPoint(e.Source));
            }
          vtkIdType v = outputId[e.Target];
          if (v == -1)
            {
            if (directed)
              {
              v = dirBuilder->AddVertex();
              }
            else
              {
              v = undirBuilder->AddVertex();
              }
            outputId[e.Target] = v;
            builderVertexData->CopyData(inputVertexData, e.Target, v);
            newPoints->InsertNextPoint(input->GetPoints()->GetPoint(e.Target));
            }
          vtkEdgeType f;
          if (directed)
            {
            f = dirBuilder->AddEdge(u, v);
            }
          else
            {
            f = undirBuilder->AddEdge(u, v);
            }
          builderEdgeData->CopyData(inputEdgeData, e.Id, f.Id);
          }
        }
      builder->SetPoints(newPoints);
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

        if (selectArr->LookupValue(e.Id) >= 0)
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
    vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();
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
        // Copy edge layout to the output.
        vtkIdType npts;
        double* pts;
        input->GetEdgePoints(e.Id, npts, pts);
        builder->SetEdgePoints(outputEdge.Id, npts, pts);
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
