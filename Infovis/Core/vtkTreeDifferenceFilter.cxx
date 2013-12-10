/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeDifferenceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeDifferenceFilter.h"

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkTreeDifferenceFilter);

//---------------------------------------------------------------------------
vtkTreeDifferenceFilter::vtkTreeDifferenceFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->IdArrayName = 0;
  this->ComparisonArrayName = 0;
  this->OutputArrayName = 0;
  this->ComparisonArrayIsVertexData = false;
}

//---------------------------------------------------------------------------
vtkTreeDifferenceFilter::~vtkTreeDifferenceFilter()
{
  // release memory
  this->SetIdArrayName(0);
  this->SetComparisonArrayName(0);
  this->SetOutputArrayName(0);
}

//---------------------------------------------------------------------------
int vtkTreeDifferenceFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  if(port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    }
  else if(port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkTreeDifferenceFilter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* tree1_info = inputVector[0]->GetInformationObject(0);
  vtkTree* tree1 = vtkTree::SafeDownCast(
    tree1_info->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the structure into the output.
  vtkTree* outputTree = vtkTree::GetData(outputVector);

  vtkInformation* tree2_info = inputVector[1]->GetInformationObject(0);
  if(!tree2_info)
    {
    // If no second tree provided, we're done
    outputTree->CheckedShallowCopy(tree1);
    return 0;
    }

  vtkTree* tree2 = vtkTree::SafeDownCast(
    tree2_info->Get(vtkDataObject::DATA_OBJECT()));

  if (this->IdArrayName != 0)
    {
    if (!this->GenerateMapping(tree1, tree2))
      {
      return 0;
      }
    }
  else
    {
    this->VertexMap.clear();
    for (vtkIdType vertex = 0; vertex < tree1->GetNumberOfVertices(); ++vertex)
      {
      this->VertexMap[vertex] = vertex;
      }

    this->EdgeMap.clear();
    for (vtkIdType edge = 0; edge < tree1->GetNumberOfEdges(); ++edge)
      {
      this->EdgeMap[edge] = edge;
      }
    }

  vtkSmartPointer<vtkDoubleArray> resultArray =
    this->ComputeDifference(tree1, tree2);

  if (!outputTree->CheckedShallowCopy(tree1))
    {
    vtkErrorMacro(<<"Invalid tree structure.");
    return 0;
    }

  if (this->ComparisonArrayIsVertexData)
    {
    outputTree->GetVertexData()->AddArray(resultArray);
    }
  else
    {
    outputTree->GetEdgeData()->AddArray(resultArray);
    }

  return 1;
}

//---------------------------------------------------------------------------
bool vtkTreeDifferenceFilter::GenerateMapping(vtkTree *tree1, vtkTree *tree2)
{
  this->VertexMap.clear();
  this->VertexMap.assign(tree1->GetNumberOfVertices(), -1);

  this->EdgeMap.clear();
  this->EdgeMap.assign(tree1->GetNumberOfEdges(), -1);

  vtkStringArray *nodeNames1 = vtkStringArray::SafeDownCast(
    tree1->GetVertexData()->GetAbstractArray(this->IdArrayName));
  if (nodeNames1 == NULL)
    {
    vtkErrorMacro("tree #1's VertexData does not have a vtkStringArray named "
      << this->IdArrayName);
    return false;
    }

  vtkStringArray *nodeNames2 = vtkStringArray::SafeDownCast(
    tree2->GetVertexData()->GetAbstractArray(this->IdArrayName));
  if (nodeNames2 == NULL)
    {
    vtkErrorMacro("tree #2's VertexData does not have a vtkStringArray named "
      << this->IdArrayName);
    return false;
    }

  vtkIdType root1 = tree1->GetRoot();
  vtkIdType root2 = tree2->GetRoot();
  this->VertexMap[root1] = root2;

  vtkIdType edgeId1 = -1;
  vtkIdType edgeId2 = -1;

  // iterate over the vertex names for tree #1, finding the corresponding
  // vertex in tree #2.
  for (vtkIdType vertexItr = 0; vertexItr < nodeNames1->GetNumberOfTuples();
       ++vertexItr)
    {
    vtkIdType vertexId1 = vertexItr;
    std::string nodeName = nodeNames1->GetValue(vertexId1);
    if (nodeName.compare("") == 0)
      {
      continue;
      }

    // record this correspondence in the maps
    vtkIdType vertexId2 = nodeNames2->LookupValue(nodeName);
    if (vertexId2 == -1)
      {
      vtkWarningMacro("tree #2 does not contain a vertex named " << nodeName);
      continue;
      }
    this->VertexMap[vertexId1] = vertexId2;

    if (vertexId1 == root1 || vertexId2 == root2)
      {
      continue;
      }

    edgeId1 = tree1->GetEdgeId(tree1->GetParent(vertexId1), vertexId1);
    edgeId2 = tree2->GetEdgeId(tree2->GetParent(vertexId2), vertexId2);
    this->EdgeMap[edgeId1] = edgeId2;

    // ascend the tree until we reach the root, mapping parent vertices to
    // each other along the way.
    while (tree1->GetParent(vertexId1) != root1 &&
           tree2->GetParent(vertexId2) != root2)
      {
      vertexId1 = tree1->GetParent(vertexId1);
      vertexId2 = tree2->GetParent(vertexId2);
      if (this->VertexMap[vertexId1] == -1)
        {
        this->VertexMap[vertexId1] = vertexId2;
        edgeId1 = tree1->GetEdgeId(tree1->GetParent(vertexId1), vertexId1);
        edgeId2 = tree2->GetEdgeId(tree2->GetParent(vertexId2), vertexId2);
        this->EdgeMap[edgeId1] = edgeId2;
        }
      }
    }

  return true;
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray>
vtkTreeDifferenceFilter::ComputeDifference(vtkTree *tree1, vtkTree *tree2)
{
  if (this->ComparisonArrayName == 0)
    {
    vtkErrorMacro("ComparisonArrayName has not been set.");
    return NULL;
    }

  vtkDataSetAttributes *treeData1, *treeData2;
  const char *dataName;
  if (this->ComparisonArrayIsVertexData)
    {
    treeData1 = tree1->GetVertexData();
    treeData2 = tree2->GetVertexData();
    dataName = "VertexData";
    }
  else
    {
    treeData1 = tree1->GetEdgeData();
    treeData2 = tree2->GetEdgeData();
    dataName = "EdgeData";
    }

  vtkDataArray *arrayToCompare1 =
    treeData1->GetArray(this->ComparisonArrayName);
  if (arrayToCompare1 == NULL)
    {
    vtkErrorMacro("tree #1's " << dataName <<
      " does not have a vtkDoubleArray named " << this->ComparisonArrayName);
    return NULL;
    }

  vtkDataArray *arrayToCompare2 =
    treeData2->GetArray(this->ComparisonArrayName);
  if (arrayToCompare2 == NULL)
    {
    vtkErrorMacro("tree #2's " << dataName <<
      " does not have a vtkDoubleArray named " << this->ComparisonArrayName);
    return NULL;
    }

  vtkSmartPointer<vtkDoubleArray> resultArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  resultArray->SetNumberOfValues(arrayToCompare1->GetNumberOfTuples());
  resultArray->FillComponent(0, vtkMath::Nan());

  if (this->OutputArrayName == 0)
    {
    resultArray->SetName("difference");
    }
  else
    {
    resultArray->SetName(this->OutputArrayName);
    }

  vtkIdType treeId2;
  for (vtkIdType treeId1 = 0; treeId1 < arrayToCompare1->GetNumberOfTuples();
       ++treeId1)
    {
    if (this->ComparisonArrayIsVertexData)
      {
      treeId2 = this->VertexMap[treeId1];
      }
    else
      {
      treeId2 = this->EdgeMap[treeId1];
      }
    double result =
      arrayToCompare1->GetTuple1(treeId1) - arrayToCompare2->GetTuple1(treeId2);
    resultArray->SetValue(treeId1, result);
    }

  return resultArray;
}

//---------------------------------------------------------------------------
void vtkTreeDifferenceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IdArrayName)
    {
    os << indent << "IdArrayName: "
       << this->IdArrayName << std::endl;
    }
  else
    {
    os << indent << "IdArrayName: "
       << "(None)" << std::endl;
    }
  if (this->ComparisonArrayName)
    {
    os << indent << "ComparisonArrayName: "
       << this->ComparisonArrayName << std::endl;
    }
  else
    {
    os << indent << "ComparisonArrayName: "
       << "(None)" << std::endl;
    }
  if (this->OutputArrayName)
    {
    os << indent << "OutputArrayName: "
       << this->OutputArrayName << std::endl;
    }
  else
    {
    os << indent << "OutputArrayName: "
       << "(None)" << std::endl;
    }
  os << indent << "ComparisonArrayIsVertexData: "
     << this->ComparisonArrayIsVertexData << std::endl;
}
