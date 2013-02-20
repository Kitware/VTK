/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveTreeLeafFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRemoveTreeLeafFilter.h"

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
#include "vtkNew.h"

#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

vtkStandardNewMacro(vtkRemoveTreeLeafFilter);


vtkRemoveTreeLeafFilter::vtkRemoveTreeLeafFilter()
{
  this->SetNumberOfInputPorts(2);
  this->ShouldRemoveParentVertex = true;
}

vtkRemoveTreeLeafFilter::~vtkRemoveTreeLeafFilter()
{
}


//----------------------------------------------------------------------------
int vtkRemoveTreeLeafFilter::FillInputPortInformation(int port, vtkInformation *info)
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

void vtkRemoveTreeLeafFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkRemoveTreeLeafFilter::BuildTree(vtkMutableDirectedGraph * builder, vtkIdType  parentId, vtkTree * inputTree, vtkIdType inputTreeVertexId, vtkIdTypeArray * list )
{
  vtkDataSetAttributes *inputVertexData = inputTree->GetVertexData();
  vtkDataSetAttributes *inputEdgeData = inputTree->GetEdgeData();
  vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();

  if (inputTree->IsLeaf(inputTreeVertexId))
    {
    return 0;
    }

  if (parentId == -1)
    {//root
    parentId = builder->AddVertex();
    builderVertexData->CopyData(inputVertexData, inputTree->GetRoot(),parentId);
    }
  int numChildrenAdded = 0;
  vtkIdType numChildren = inputTree->GetNumberOfChildren(inputTreeVertexId);
  for (vtkIdType i = 0; i < numChildren; i++)
    {
    vtkIdType childId = inputTree->GetChild(inputTreeVertexId, i);
    if (inputTree->IsLeaf(childId))
      {//Child is a ieaf
      bool TO_BE_REMOVED = false;
      for (unsigned int j = 0;j < list->GetNumberOfTuples();j++)
        {
        if (list->GetValue(j) == childId)
          {
          TO_BE_REMOVED = true;
          list->RemoveTuple(j);
          break;
          }
        }

      if (! TO_BE_REMOVED)
        {//add the child to the new tree
        vtkIdType newNodeId = builder->AddChild(parentId);
        builderVertexData->CopyData( inputVertexData,  childId, newNodeId);

        vtkIdType e = builder->GetEdgeId(parentId,newNodeId);
        vtkIdType input_e = inputTree->GetEdgeId(inputTreeVertexId,childId);
        builderEdgeData->CopyData( inputEdgeData, input_e, e);
        numChildrenAdded++;
        }
      }
    else
      {//Child is an internal node
      vtkIdType newNodeId = builder->AddChild(parentId);

      int subTreeHasChildren = this->BuildTree(builder,newNodeId,inputTree, childId,list);

      //recursively build the sub tree
      if (!subTreeHasChildren && this->ShouldRemoveParentVertex)
        {//the internal node had no children
        builder->RemoveVertex(newNodeId);
        vtkIdType eId = builder->GetEdgeId(parentId,newNodeId);
        builder->RemoveEdge(eId);
        }
      else
        {
        builderVertexData->CopyData( inputVertexData,  childId, newNodeId);

        vtkIdType e = builder->GetEdgeId(parentId,newNodeId);
        vtkIdType input_e = inputTree->GetEdgeId(inputTreeVertexId,childId);
        builderEdgeData->CopyData( inputEdgeData, input_e, e);

        numChildrenAdded++;
        }

      }
    }

  return numChildrenAdded;
}


int vtkRemoveTreeLeafFilter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkTree* inputTree = vtkTree::GetData(inputVector[0]);
  vtkSelection * leafSelection = vtkSelection::GetData(inputVector[1]);
  vtkTree* outputTree = vtkTree::GetData(outputVector);

  vtkSmartPointer<vtkIdTypeArray>  leafVertices = vtkSmartPointer<vtkIdTypeArray>::New();
  vtkConvertSelection::GetSelectedVertices(leafSelection, inputTree, leafVertices);

  if (leafVertices->GetSize()> 0 )
    {
    for (vtkIdType i = 0; i < leafVertices->GetNumberOfTuples(); i++)
      {
      vtkIdType id = leafVertices->GetValue(i);
      if (!inputTree->IsLeaf(id))
        {
        vtkErrorMacro("Vertex "<< id <<" is not a leaf.");
        return 0;
        }
      }
    }
  else
    {
    vtkErrorMacro("No leaf vertices are provided." );
    return 0;
    }

  vtkNew<vtkMutableDirectedGraph> builder;
  vtkNew<vtkIdTypeArray> list;
  list->DeepCopy(leafVertices);

  // Get the input and builder vertex and edge data.
  vtkDataSetAttributes * inputVertexData = inputTree->GetVertexData();
  vtkDataSetAttributes * inputEdgeData = inputTree->GetEdgeData();
  vtkDataSetAttributes * builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes * builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // build the tree recursively
  this->BuildTree(builder.GetPointer(), -1, inputTree, inputTree->GetRoot(), list.GetPointer());

  // Copy the structure into the output.
  if (!outputTree->CheckedShallowCopy(builder.GetPointer()))
    {
    vtkErrorMacro( <<"Invalid tree structure." << outputTree->GetNumberOfVertices());
    return 0;
    }

  return 1;
}
