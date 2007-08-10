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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"

#include <vtksys/stl/set>
#include <vtksys/stl/map>

vtkCxxRevisionMacro(vtkGroupLeafVertices, "1.2");
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
  vtkTree* input = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree* output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  output->ShallowCopy(input);

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

  vtkTable* treeTable = vtkTable::New();
  treeTable->SetFieldData(output->GetFieldData());

  // Add grouping vertices to all parents of leaf vertices
  vtksys_stl::set<vtkIdType> done;
  vtksys_stl::set<vtkIdType> newVertices;
  vtksys_stl::map<vtkStdString, vtkIdType> groupVertex;
  vtkIdType numVertices = output->GetNumberOfVertices();
  vtkIdTypeArray* children = vtkIdTypeArray::New();
  for (vtkIdType i = 0; i < numVertices; i++)
    {
    if (output->IsLeaf(i))
      {
      vtkIdType parent = output->GetParent(i);
      if (done.count(parent) == 0 && newVertices.count(parent) == 0)
        {
        groupVertex.clear();
        children->SetNumberOfValues(0);
        for (vtkIdType j = 0; j < output->GetNumberOfChildren(parent); j++)
          {
          children->InsertNextValue(output->GetChild(parent, j));
          }
        for (vtkIdType c = 0; c < children->GetNumberOfTuples(); c++)
          {
          if (children->GetValue(c) < numVertices)
            {
            vtkStdString value = stringArr->GetValue(children->GetValue(c));
            if (groupVertex.count(value) > 0)
              {
              vtkIdType newParent = groupVertex[value];
              output->SetParent(children->GetValue(c), newParent);
              }
            else
              {
              vtkIdType newParent = output->AddChild(parent);
              treeTable->InsertNextBlankRow();
              newVertices.insert(newParent);
              if (nameStringArr != NULL)
                {
                nameStringArr->InsertValue(newParent, value);
                }
              groupVertex[value] = newParent;
              output->SetParent(children->GetValue(c), newParent);
              }
            }
          }
        done.insert(parent);
        }
      }
    }

  children->Delete();
  treeTable->Delete();

  return 1;
}
