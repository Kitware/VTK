/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGroupLeafVertices.cxx

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

#include "vtkGroupLeafVertices.h"

#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"

#include <vtksys/stl/set>
#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

using vtksys_stl::pair;
using vtksys_stl::make_pair;
using vtksys_stl::set;
using vtksys_stl::map;
using vtksys_stl::vector;

vtkCxxRevisionMacro(vtkGroupLeafVertices, "1.3");
vtkStandardNewMacro(vtkGroupLeafVertices);


vtkGroupLeafVertices::vtkGroupLeafVertices()
{
}

vtkGroupLeafVertices::~vtkGroupLeafVertices()
{
}

void vtkGroupLeafVertices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkGroupLeafVertices::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // Storing the inputTable and outputTree handles
  vtkTree *input = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the field to filter on
  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == NULL)
    {
    vtkErrorMacro(<< "An input array must be specified");
    return 0;
    }

  // For now, the field must be a string array
  vtkStringArray* stringArr = vtkStringArray::SafeDownCast(arr);
  if (stringArr == NULL)
    {
    vtkErrorMacro(<< "The input array must be a string array");
    return 0;
    }

  // Get the (optional) name field.  Right now this will cause a warning
  // if the array is not set.
  vtkAbstractArray* nameArr = this->GetInputAbstractArrayToProcess(1, inputVector);
  vtkStringArray* nameStringArr = vtkStringArray::SafeDownCast(nameArr);
  if (nameArr != NULL && nameStringArr == NULL)
    {
    vtkErrorMacro(<< "The name array, if specified, must be a string array");
    return 0;
    }

  // Create builder to extend the tree
  vtkSmartPointer<vtkMutableDirectedGraph> builder = 
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  builder->DeepCopy(input);

  // Get the input and builder vertex and edge data.
  vtkDataSetAttributes *inputVertexData = input->GetVertexData();
  vtkDataSetAttributes *inputEdgeData = input->GetEdgeData();
  vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Make the builder's field data a table
  // so we can call InsertNextBlankRow.
  vtkSmartPointer<vtkTable> treeTable = 
    vtkSmartPointer<vtkTable>::New();
  treeTable->SetFieldData(builder->GetVertexData());

  // Copy everything into the new tree, adding group nodes.
  // Make a map of (parent id, group-by string) -> group vertex id.
  map<pair<vtkIdType, vtkStdString>, vtkIdType> group_vertices;
  vector< pair<vtkIdType, vtkIdType> > vertStack;
  vertStack.push_back(make_pair(input->GetRoot(), builder->AddVertex()));
  vtkSmartPointer<vtkOutEdgeIterator> it =
    vtkSmartPointer<vtkOutEdgeIterator>::New();
  while (!vertStack.empty())
    {
    vtkIdType tree_v = vertStack.back().first;
    vtkIdType v = vertStack.back().second;
    builderVertexData->CopyData(inputVertexData, tree_v, v);
    vertStack.pop_back();
    input->GetOutEdges(tree_v, it);
    while (it->HasNext())
      {
      vtkOutEdgeType tree_e = it->Next();
      vtkIdType tree_child = tree_e.Target;
      vtkIdType child = builder->AddVertex();
      if (!input->IsLeaf(tree_child))
        {
        // If it isn't a leaf, just add the child to the new tree
        // and recurse.
        vtkEdgeType e = builder->AddEdge(v, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(make_pair(tree_child, child));
        }
      else
        {
        // If it is a leaf, it should be grouped.
        // Look for a group vertex.  If there isn't one already, make one.
        vtkIdType group_vertex = -1;
        vtkStdString str = stringArr->GetValue(tree_child);
        if (group_vertices.count(make_pair(v, str)) > 0)
          {
          group_vertex = group_vertices[make_pair(v, str)];
          }
        else
          {
          group_vertex = builder->AddVertex();
          treeTable->InsertNextBlankRow();
          vtkEdgeType group_e = builder->AddEdge(v, group_vertex);
          builderEdgeData->CopyData(inputEdgeData, tree_e.Id, group_e.Id);
          group_vertices[make_pair(v, str)] = group_vertex;
          if (nameStringArr)
            {
            nameStringArr->InsertValue(group_vertex, str);
            }
          }
        vtkEdgeType e = builder->AddEdge(group_vertex, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(make_pair(tree_child, child));
        }
      }
    }

  // Move the structure to the output
  vtkTree *output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid tree strucutre!");
    return 0;
    }

  return 1;
}
