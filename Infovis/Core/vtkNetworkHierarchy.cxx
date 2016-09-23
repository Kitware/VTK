/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetworkHierarchy.cxx

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

#include "vtkNetworkHierarchy.h"

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
#include "vtkGraph.h"
#include "vtkTree.h"
#include "vtkVariant.h"

#include <map>
#include <utility>
#include <vector>
#include <sstream>
#include <algorithm>

vtkStandardNewMacro(vtkNetworkHierarchy);

// This is just a macro wrapping for smart pointers
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


vtkNetworkHierarchy::vtkNetworkHierarchy()
{
  this->IPArrayName = 0;
  this->SetIPArrayName("ip");
}

vtkNetworkHierarchy::~vtkNetworkHierarchy()
{
  this->SetIPArrayName(0);
}

void vtkNetworkHierarchy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IPArrayName: " << (this->IPArrayName ? "" : "(null)") << endl;
}

int vtkNetworkHierarchy::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTree");
  return 1;
}

int vtkNetworkHierarchy::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}


void vtkNetworkHierarchy::GetSubnets(unsigned int packedIP, int *subnets)
{
 unsigned int num = packedIP;
 subnets[3] = num % 256;
 num = num >> 8;
 subnets[2] = num % 256;
 num = num >> 8;
 subnets[1] = num % 256;
 num = num >> 8;
 subnets[0] = num;
}

unsigned int vtkNetworkHierarchy::ITON(vtkStdString ip)
{
  unsigned int subnets[4];
  sscanf(ip.c_str(),"%u.%u.%u.%u",
      &(subnets[0]),&(subnets[1]),&(subnets[2]),&(subnets[3]));
  int num = subnets[0];
  num = num << 8;
  num += subnets[1];
  num = num << 8;
  num += subnets[2];
  num = num << 8;
  num += subnets[3];
  return num;
}

int vtkNetworkHierarchy::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTable and outputTree handles
  vtkGraph *inputGraph = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *outputTree = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the field to filter on
  vtkAbstractArray* arr =
    inputGraph->GetVertexData()->GetAbstractArray(this->IPArrayName);
  vtkStringArray* ipArray = vtkArrayDownCast<vtkStringArray>(arr);
  if (ipArray == NULL)
  {
    vtkErrorMacro(<< "An string based ip array must be specified");
    return 0;
  }


  // Build subnet map
  typedef std::vector<std::pair<unsigned int, vtkIdType> > subnet_map_type;
  subnet_map_type SubnetMap;
  for (vtkIdType i = 0; i < ipArray->GetNumberOfTuples(); ++i)
  {
    unsigned int packedID = this->ITON(ipArray->GetValue(i));
    SubnetMap.push_back(std::make_pair(packedID,i));
  }
  std::sort(SubnetMap.begin(), SubnetMap.end());

  // Create builder for the tree
  VTK_CREATE(vtkMutableDirectedGraph,builder);

  // Make a bunch of blank vertices
  for(int i=0; i<inputGraph->GetNumberOfVertices(); ++i)
  {
    builder->AddVertex();
  }

  // Get the input graph and copy the vertex data
  vtkDataSetAttributes *inputVertexData = inputGraph->GetVertexData();
  vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
  builderVertexData->DeepCopy(inputVertexData);

  // Get pedigree ids.
  vtkAbstractArray* pedIDArr = builderVertexData->GetPedigreeIds();

  // Get domain. If there isn't one, make one.
  vtkStringArray* domainArr = vtkArrayDownCast<vtkStringArray>(
    builderVertexData->GetAbstractArray("domain"));
  if (pedIDArr && !domainArr)
  {
    domainArr = vtkStringArray::New();
    domainArr->SetName("domain");
    for (vtkIdType r = 0; r < inputGraph->GetNumberOfVertices(); ++r)
    {
      domainArr->InsertNextValue(pedIDArr->GetName());
    }
    builderVertexData->AddArray(domainArr);
  }

  // All new vertices will be placed in this domain.
  vtkStdString newVertexDomain = "subnet";

  // Make the builder's field data a table
  // so we can call InsertNextBlankRow.
  vtkSmartPointer<vtkTable> treeTable =
    vtkSmartPointer<vtkTable>::New();
  treeTable->SetRowData(builder->GetVertexData());

  // Get the pedigree ID and domain columns
  int pedIDColumn = -1;
  int domainColumn = -1;
  if (pedIDArr)
  {
    treeTable->GetRowData()->GetAbstractArray(pedIDArr->GetName(), pedIDColumn);
    treeTable->GetRowData()->GetAbstractArray("domain", domainColumn);
  }

  // Add root
  vtkIdType rootID = builder->AddVertex();
  treeTable->InsertNextBlankRow();

  // Don't label the root node...
  // treeTable->SetValueByName(rootID, this->IPArrayName, vtkVariant("Internet"));
  treeTable->SetValueByName(rootID, this->IPArrayName, vtkVariant(""));
  if (pedIDArr)
  {
    treeTable->SetValue(rootID, pedIDColumn, rootID);
    treeTable->SetValue(rootID, domainColumn, newVertexDomain);
  }

  // Iterate through the different subnets
  subnet_map_type::iterator I;
  int currentSubnet0 = -1;
  int currentSubnet1 = -1;
  int currentSubnet2 = -1;
  int subnets[4];
  vtkIdType currentParent0 = 0;
  vtkIdType currentParent1 = 0;
  vtkIdType currentParent2 = 0;
  vtkIdType treeIndex;
  vtkIdType leafIndex;
  for (I = SubnetMap.begin(); I != SubnetMap.end(); ++I)
  {
    unsigned int packedID = (*I).first;
    leafIndex = (*I).second;
    this->GetSubnets(packedID,subnets);

    // Is this a new subnet 0
    if (subnets[0] != currentSubnet0)
    {
      // Add child
      treeIndex = builder->AddChild(rootID);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, vtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet0 = subnets[0];
      currentParent0 = treeIndex;

      // Invalidate subnets
      currentSubnet1 = currentSubnet2 = -1;
    }

    // Is this a new subnet 1
    if (subnets[1] != currentSubnet1)
    {
      // Add child
      treeIndex = builder->AddChild(currentParent0);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0] << "." << subnets[1];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, vtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet1 = subnets[1];
      currentParent1 = treeIndex;

      // Invalidate subnets
      currentSubnet2 = -1;
    }

    // Is this a new subnet 2
    if (subnets[2] != currentSubnet2)
    {
      // Add child
      treeIndex = builder->AddChild(currentParent1);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0] << "." << subnets[1] << "." << subnets[2];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, vtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet2 = subnets[2];
      currentParent2 = treeIndex;
    }

    builder->AddEdge(currentParent2, leafIndex);
  }


  // Move the structure to the output
  if (!outputTree->CheckedShallowCopy(builder))
  {
    vtkErrorMacro(<<"Invalid tree structure!");
    return 0;
  }

  return 1;
}
