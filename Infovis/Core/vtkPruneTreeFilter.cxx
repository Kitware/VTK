/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPruneTreeFilter.cxx

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

#include "vtkPruneTreeFilter.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"

#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

vtkStandardNewMacro(vtkPruneTreeFilter);


vtkPruneTreeFilter::vtkPruneTreeFilter()
{
  this->ParentVertex = 0;
}

vtkPruneTreeFilter::~vtkPruneTreeFilter()
{
}

void vtkPruneTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Parent: " << this->ParentVertex << endl;
}

int vtkPruneTreeFilter::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkTree* inputTree = vtkTree::GetData(inputVector[0]);
  vtkTree* outputTree = vtkTree::GetData(outputVector);

  if (this->ParentVertex < 0 || this->ParentVertex >= inputTree->GetNumberOfVertices())
    {
    vtkErrorMacro("Parent vertex must be part of the tree " << this->ParentVertex 
      << " >= " << inputTree->GetNumberOfVertices());
    return 0;
    }

  // Structure for building the tree.
  vtkSmartPointer<vtkMutableDirectedGraph> builder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();

  // Child iterator.
  vtkSmartPointer<vtkOutEdgeIterator> it =
    vtkSmartPointer<vtkOutEdgeIterator>::New();

  // Get the input and builder vertex and edge data.
  vtkDataSetAttributes *inputVertexData = inputTree->GetVertexData();
  vtkDataSetAttributes *inputEdgeData = inputTree->GetEdgeData();
  vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Build a copy of the tree, skipping the parent vertex to remove.
  vtksys_stl::vector< vtksys_stl::pair<vtkIdType, vtkIdType> > vertStack;
  if (inputTree->GetRoot() != this->ParentVertex)
    {
    vertStack.push_back(vtksys_stl::make_pair(inputTree->GetRoot(), builder->AddVertex()));
    }
  while (!vertStack.empty())
    {
    vtkIdType tree_v = vertStack.back().first;
    vtkIdType v = vertStack.back().second;
    builderVertexData->CopyData(inputVertexData, tree_v, v);
    vertStack.pop_back();
    inputTree->GetOutEdges(tree_v, it);
    while (it->HasNext())
      {
      vtkOutEdgeType tree_e = it->Next();
      vtkIdType tree_child = tree_e.Target;
      if (tree_child != this->ParentVertex)
        {
        vtkIdType child = builder->AddVertex();
        vtkEdgeType e = builder->AddEdge(v, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(vtksys_stl::make_pair(tree_child, child));
        }
      }
    }

  // Copy the structure into the output.
  if (!outputTree->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid tree structure.");
    return 0;
    }

  return 1;
}
