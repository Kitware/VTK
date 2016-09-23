/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractSelectedTree.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkIdTypeArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkConvertSelection.h"
#include "vtkEdgeListIterator.h"
#include "vtkNew.h"

#include <map>

vtkStandardNewMacro(vtkExtractSelectedTree);


vtkExtractSelectedTree::vtkExtractSelectedTree()
{
  this->SetNumberOfInputPorts(2);
}

vtkExtractSelectedTree::~vtkExtractSelectedTree()
{
}

//----------------------------------------------------------------------------
void vtkExtractSelectedTree::SetSelectionConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);

}
//----------------------------------------------------------------------------
int vtkExtractSelectedTree::FillInputPortInformation(int port, vtkInformation *info)
{
  if(port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    return 1;
  }
  else if(port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedTree::BuildTree( vtkTree * inputTree, vtkIdTypeArray * selectedVerticesList,
         vtkMutableDirectedGraph * builder )
{
  // Get the input and builder vertex and edge data.
  vtkDataSetAttributes * inputVertexData = inputTree->GetVertexData();
  vtkDataSetAttributes * inputEdgeData = inputTree->GetEdgeData();

  vtkDataSetAttributes * builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes * builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  //Add selected vertices and set up a  map between the input tree vertex id
  //and the output tree vertex id
  std::map<vtkIdType, vtkIdType> vertexMap;
  for (vtkIdType j = 0; j < selectedVerticesList->GetNumberOfTuples();j++)
  {
    vtkIdType inVert = selectedVerticesList->GetValue(j);
    vtkIdType outVert = builder->AddVertex();

    builderVertexData->CopyData(inputVertexData, inVert, outVert);
    vertexMap[inVert] = outVert;
  }


  // Add edges connecting selected vertices
  vtkSmartPointer<vtkEdgeListIterator> edges = vtkSmartPointer<vtkEdgeListIterator>::New();
  inputTree->GetEdges(edges);
  while (edges->HasNext())
  {
    vtkEdgeType e = edges->Next();
    if (vertexMap.find(e.Source) != vertexMap.end() &&
      vertexMap.find(e.Target) != vertexMap.end())
    {
      vtkIdType source = vertexMap[e.Source];
      vtkIdType target = vertexMap[e.Target];
      vtkEdgeType f = builder->AddEdge(source, target);
      builderEdgeData->CopyData(inputEdgeData, e.Id, f.Id);

      vtkIdType npts;
      double* pts;
      inputTree->GetEdgePoints(e.Id, npts, pts);
      builder->SetEdgePoints(f.Id, npts, pts);
    }
  }

  return 1;
}



int vtkExtractSelectedTree::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  vtkTree * inputTree = vtkTree::GetData(inputVector[0]);
  vtkSelection * selection = vtkSelection::GetData(inputVector[1]);
  vtkTree * outputTree = vtkTree::GetData(outputVector);

  if(!selection)
  {
    vtkErrorMacro("No vtkSelection provided as input.");
    return 0;
  }

  //obtain a vertex selection list from the input vtkSelection
  // Convert the selection to an INDICES selection
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToIndexSelection(selection, inputTree));
  if (!converted.GetPointer())
  {
    vtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
  }
  vtkNew<vtkIdTypeArray> selectedVerticesList;

  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode * node = converted->GetNode(i);

    // Append the selectedVerticesList
    vtkIdTypeArray * curList = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
    if (curList)
    {
      int inverse = node->GetProperties()->Get(vtkSelectionNode::INVERSE());
      if (inverse)
      {//selection is to be removed
        if (node->GetFieldType() == vtkSelectionNode::VERTEX)
        {//keep all the other vertices
          vtkIdType num = inputTree->GetNumberOfVertices();
          for (vtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0 && selectedVerticesList->LookupValue(j) < 0)
            {
              selectedVerticesList->InsertNextValue(j);
            }
          }
        }
        else if (node->GetFieldType() == vtkSelectionNode ::EDGE)
        {// keep all the other edges
          vtkIdType num = inputTree->GetNumberOfEdges();
          for (vtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0 )
            {
              vtkIdType s = inputTree->GetSourceVertex(j);
              vtkIdType t = inputTree->GetTargetVertex(j);
              if (selectedVerticesList->LookupValue(s) < 0)
              {
                selectedVerticesList->InsertNextValue(s);
              }
              if (selectedVerticesList->LookupValue(t) < 0)
              {
                selectedVerticesList->InsertNextValue(t);
              }
            }
          }
        }
      }// end of if(!inverse)
      else
      {//selection is to be extracted
        vtkIdType numTuples = curList->GetNumberOfTuples();
        for (vtkIdType j = 0; j < numTuples; ++j)
        {
          if (node->GetFieldType() == vtkSelectionNode::VERTEX )
          {
            vtkIdType curVertexId = curList->GetValue(j);
            if (selectedVerticesList->LookupValue(curVertexId) < 0)
            {
              selectedVerticesList->InsertNextValue(curVertexId);
            }
          }
          else if (node->GetFieldType() == vtkSelectionNode::EDGE)
          {//if an edge is selected to be extracted,
            //keep both source and target vertices
            vtkIdType curEdgeId = curList->GetValue(j);
            vtkIdType t = inputTree->GetTargetVertex(curEdgeId);
            vtkIdType s = inputTree->GetSourceVertex(curEdgeId);
            if (selectedVerticesList->LookupValue(s) < 0)
            {
              selectedVerticesList->InsertNextValue(s);
            }
            if (selectedVerticesList->LookupValue(t) < 0)
            {
              selectedVerticesList->InsertNextValue(t);
            }
          }
        }
      }
    } // end if (curList)
  } // end for each selection node


  vtkNew<vtkMutableDirectedGraph> builder;
  // build the tree recursively
  this->BuildTree(inputTree, selectedVerticesList.GetPointer(), builder.GetPointer());

  // Copy the structure into the output.
  if (!outputTree->CheckedShallowCopy(builder.GetPointer()))
  {
    vtkErrorMacro( <<"Invalid tree structure." << outputTree->GetNumberOfVertices());
    return 0;
  }

  return 1;
}
