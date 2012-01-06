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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGroupLeafVertices.h"

#include "vtkIdList.h"
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
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkUnicodeStringArray.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

vtkStandardNewMacro(vtkGroupLeafVertices);

// Forward function reference (definition at bottom :)
static int splitString(const vtkStdString& input, 
                       std::vector<vtkStdString>& results);

//---------------------------------------------------------------------------
class vtkGroupLeafVerticesCompare
{
public:
  bool operator()(
    const vtksys_stl::pair<vtkIdType, vtkVariant>& a,
    const vtksys_stl::pair<vtkIdType, vtkVariant>& b) const
  {
    if (a.first != b.first)
      {
      return a.first < b.first;
      }
    return vtkVariantLessThan()(a.second, b.second);
  }
};

//---------------------------------------------------------------------------
template <typename T>
vtkVariant vtkGroupLeafVerticesGetValue(T* arr, vtkIdType index)
{
  return vtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
vtkVariant vtkGroupLeafVerticesGetVariant(vtkAbstractArray* arr, vtkIdType i)
{
  vtkVariant val;
  switch(arr->GetDataType())
    {
    vtkSuperExtraExtendedTemplateMacro(val = vtkGroupLeafVerticesGetValue(
      static_cast<VTK_TT*>(arr->GetVoidPointer(0)), i));
    }
  return val;
}

vtkGroupLeafVertices::vtkGroupLeafVertices()
{
  this->GroupDomain = 0;
  this->SetGroupDomain("group_vertex");
}

vtkGroupLeafVertices::~vtkGroupLeafVertices()
{
  this->SetGroupDomain(0);
}

void vtkGroupLeafVertices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GroupDomain: " << (this->GroupDomain ? this->GroupDomain : "(null)") << endl;
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
  vtkTree *output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check for corner case of 'empty' tree
  if (input->GetNumberOfVertices() == 0)
    {
    output->ShallowCopy(input);
    return 1;
    }

  // Create builder to extend the tree
  vtkSmartPointer<vtkMutableDirectedGraph> builder = 
    vtkSmartPointer<vtkMutableDirectedGraph>::New();

  // Get the input and builder vertex and edge data.
  vtkDataSetAttributes *inputVertexData = input->GetVertexData();
  vtkDataSetAttributes *inputEdgeData = input->GetEdgeData();
  vtkDataSetAttributes *builderVertexData = builder->GetVertexData();
  vtkDataSetAttributes *builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Get the field to filter on
  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == NULL)
    {
    vtkErrorMacro(<< "An input array must be specified");
    return 0;
    }

  // Get the builder's group array.
  vtkAbstractArray *outputGroupArr = 0;
  char *groupname = arr->GetName();
  outputGroupArr = builderVertexData->GetAbstractArray(groupname);
  if (outputGroupArr == NULL)
    {
    vtkErrorMacro(<< "Could not find the group array in the builder.");
    return 0;
    }


  // Get the (optional) name field.  Right now this will cause a warning
  // if the array is not set.
  vtkAbstractArray* inputNameArr = this->GetInputAbstractArrayToProcess(1, inputVector);

  // Get the builder's name array.
  vtkAbstractArray *outputNameArr = 0;
  if (inputNameArr)
    {
    char *name = inputNameArr->GetName();
    outputNameArr = builderVertexData->GetAbstractArray(name);
    if (outputNameArr == NULL)
      {
      vtkErrorMacro(<< "Could not find the name array in the builder.");
      return 0;
      }
    }

  // Get the pedigree id array on the vertices
  vtkAbstractArray* pedigreeIdArr = builderVertexData->GetPedigreeIds();
  if(!pedigreeIdArr)
    {
    vtkErrorMacro(<< "Pedigree ids not assigned to vertices on input graph.");
    return 0;
    }

  // Get the domain array. If none exists, create one, and initialize
  bool addInputDomain = false;
  vtkStringArray* domainArr = vtkStringArray::SafeDownCast(builderVertexData->GetAbstractArray("domain"));
  int group_index = 0;
  if(!domainArr)
    {
    domainArr = vtkStringArray::New();
    domainArr->SetNumberOfTuples(builderVertexData->GetNumberOfTuples());
    domainArr->SetName("domain");
    builderVertexData->AddArray(domainArr);
    domainArr->Delete();
    addInputDomain = true;
    }
  else
    {
    // If a domain array already exists, look for indices that match the group 
    // domain name. Use to index in to pedigree id array and find max group value.

    vtkSmartPointer<vtkIdList> groupIds = vtkSmartPointer<vtkIdList>::New();
    domainArr->LookupValue(this->GroupDomain, groupIds);

    if(pedigreeIdArr->IsNumeric())
      {
      for(vtkIdType i=0; i<groupIds->GetNumberOfIds(); ++i)
        {
        vtkVariant v = pedigreeIdArr->GetVariantValue(i);
        bool ok;
        int num = v.ToInt(&ok);
        if(ok)
          {
          group_index = (num > group_index) ? num : group_index;
          }
        }
      }
    else if(vtkStringArray::SafeDownCast(pedigreeIdArr)||vtkVariantArray::SafeDownCast(pedigreeIdArr))
      {
      for(vtkIdType i=0; i<groupIds->GetNumberOfIds(); ++i)
        {
        std::vector<vtkStdString> tokens;
        vtkVariant v = pedigreeIdArr->GetVariantValue(i);
        splitString(v.ToString(), tokens);
        vtkVariant last = tokens[tokens.size()-1];
        bool ok;
        int num = last.ToInt(&ok);
        if(ok)
          {
          group_index = (num > group_index) ? num : group_index;
          }
        }
      }
    else
      {
      vtkErrorMacro(<< "PedigreeId array type not supported.");
      return 0;
      }
    }

  // Copy everything into the new tree, adding group nodes.
  // Make a map of (parent id, group-by string) -> group vertex id.
  vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkVariant>,
    vtkIdType, vtkGroupLeafVerticesCompare> group_vertices;
  vtksys_stl::vector< vtksys_stl::pair<vtkIdType, vtkIdType> > vertStack;
  vertStack.push_back(vtksys_stl::make_pair(input->GetRoot(), builder->AddVertex()));
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

      // If the input vertices do not have a "domain" attribute, 
      // we need to set one.
      if(addInputDomain)
        {
        domainArr->InsertValue(child, pedigreeIdArr->GetName());
        }

      if (!input->IsLeaf(tree_child))
        {
        // If it isn't a leaf, just add the child to the new tree
        // and recurse.
        vtkEdgeType e = builder->AddEdge(v, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(vtksys_stl::make_pair(tree_child, child));
        }
      else
        {
        // If it is a leaf, it should be grouped.
        // Look for a group vertex.  If there isn't one already, make one.
        vtkIdType group_vertex = -1;
        vtkVariant groupVal = vtkGroupLeafVerticesGetVariant(arr, tree_child);
        if (group_vertices.count(vtksys_stl::make_pair(v, groupVal)) > 0)
          {
          group_vertex = group_vertices[vtksys_stl::make_pair(v, groupVal)];
          }
        else
          {
          group_vertex = builder->AddVertex();

          // Set the domain for this non-leaf vertex
          domainArr->InsertValue(group_vertex, this->GroupDomain);

          // Initialize vertex attributes that aren't the pedigree ids 
          // to -1, empty string, etc.
          vtkIdType ncol = builderVertexData->GetNumberOfArrays();
          for (vtkIdType i = 0; i < ncol; i++)
            {
            vtkAbstractArray* arr2 = builderVertexData->GetAbstractArray(i);
            if(arr2 == pedigreeIdArr || arr2 == domainArr)
              {
              continue;
              }
            int comps = arr->GetNumberOfComponents();
            if (vtkDataArray::SafeDownCast(arr2))
              {
              vtkDataArray* data = vtkDataArray::SafeDownCast(arr2);
              double* tuple = new double[comps];
              for (int j = 0; j < comps; j++)
                {
                tuple[j] = -1;
                }
              data->InsertTuple(group_vertex, tuple);
              delete[] tuple;
              }
            else if (vtkStringArray::SafeDownCast(arr2))
              {
              vtkStringArray* data = vtkStringArray::SafeDownCast(arr2);
              for (int j = 0; j < comps; j++)
                {
                data->InsertValue(group_vertex + j - 1, vtkStdString(""));
                }
              }
            else if (vtkVariantArray::SafeDownCast(arr2))
              {
              vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr2);
              for (int j = 0; j < comps; j++)
                {
                data->InsertValue(group_vertex + j - 1, vtkVariant());
                }
              }
            else if (vtkUnicodeStringArray::SafeDownCast(arr2))
              {
              vtkUnicodeStringArray* data = vtkUnicodeStringArray::SafeDownCast(arr2);
              for (int j = 0; j < comps; j++)
                {
                data->InsertValue(group_vertex + j - 1, vtkUnicodeString::from_utf8(""));
                }
              }
            else
              {
              vtkErrorMacro(<< "Unsupported array type for InsertNextBlankRow");
              }
            }

          vtkEdgeType group_e = builder->AddEdge(v, group_vertex);
          builderEdgeData->CopyData(inputEdgeData, tree_e.Id, group_e.Id);
          group_vertices[vtksys_stl::make_pair(v, groupVal)] = group_vertex;

          if (outputNameArr)
            {
            outputNameArr->InsertVariantValue(group_vertex, groupVal);
            }
          if (outputGroupArr)
            {
            outputGroupArr->InsertVariantValue(group_vertex, groupVal);
            }
          if(pedigreeIdArr != outputNameArr && pedigreeIdArr != outputGroupArr)
            {
            if(pedigreeIdArr->IsNumeric())
              {
              pedigreeIdArr->InsertVariantValue(group_vertex, group_index);
              }
            else
              {
              vtkStdString groupPrefix = "group ";
              groupPrefix += vtkVariant(group_index).ToString();
              pedigreeIdArr->InsertVariantValue(group_vertex, groupPrefix);
              }
            group_index++;
            }
          }
        vtkEdgeType e = builder->AddEdge(group_vertex, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(vtksys_stl::make_pair(tree_child, child));
        }
      }
    }

  // Move the structure to the output
  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid tree structure!");
    return 0;
    }

  return 1;
}

// ----------------------------------------------------------------------

static int 
splitString(const vtkStdString& input, 
            std::vector<vtkStdString>& results)
{
  if (input.size() == 0)
    {
    return 0;
    }

  char thisCharacter = 0;
  char lastCharacter = 0;

  std::string currentField;

  for (unsigned int i = 0; i < input.size(); ++i)
    {
    thisCharacter = input[i];

    // Zeroth: are we in an escape sequence? If so, interpret this
    // character accordingly.
    if (lastCharacter == '\\')
      {
      char characterToAppend;
      switch (thisCharacter)
        {
        case '0': characterToAppend = '\0'; break;
        case 'a': characterToAppend = '\a'; break;
        case 'b': characterToAppend = '\b'; break;
        case 't': characterToAppend = '\t'; break;
        case 'n': characterToAppend = '\n'; break;
        case 'v': characterToAppend = '\v'; break;
        case 'f': characterToAppend = '\f'; break;
        case 'r': characterToAppend = '\r'; break;
        case '\\': characterToAppend = '\\'; break;
        default:  characterToAppend = thisCharacter; break;
        }

      currentField += characterToAppend;
      lastCharacter = thisCharacter;
      if (lastCharacter == '\\') lastCharacter = 0;
      }
    else 
      {
      // We're not in an escape sequence.

      // First, are we /starting/ an escape sequence?
      if (thisCharacter == '\\')
        {
        lastCharacter = thisCharacter;
        continue;
        }
      else if ((strchr(" ", thisCharacter) != NULL))
        {
        // A delimiter starts a new field unless we're in a string, in
        // which case it's normal text and we won't even get here.
        if (currentField.size() > 0)
          {
          results.push_back(currentField);
          }
        currentField = vtkStdString();
        }
      else
        {
        // The character is just plain text.  Accumulate it and move on.
        currentField += thisCharacter;
        }
      
      lastCharacter = thisCharacter;
      }
    }

  results.push_back(currentField);
  return static_cast<int>(results.size());
}
