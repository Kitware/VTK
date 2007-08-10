/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToTreeFilter.cxx

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

#include "vtkTableToTreeFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTree.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkTableToTreeFilter, "1.2");
vtkStandardNewMacro(vtkTableToTreeFilter);


vtkTableToTreeFilter::vtkTableToTreeFilter()
{
}

vtkTableToTreeFilter::~vtkTableToTreeFilter()
{
}

void vtkTableToTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


int vtkTableToTreeFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTree");
  return 1;
}

int vtkTableToTreeFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

int vtkTableToTreeFilter::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // Storing the inputTable and outputTree handles
  vtkTable* table = vtkTable::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree* tree = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Start by making a simple chain
  tree->AddRoot();
  for (vtkIdType vertex = 1; vertex < table->GetNumberOfRows(); vertex++)
    {
    tree->AddChild(vertex - 1);
    }

  // Add a new vertex to be the root
  vtkIdType root = tree->AddChild(table->GetNumberOfRows() - 1);
  tree->SetRoot(root);

  // Insert a row in the table for the new root.
  // This modifies the input, but it might be ok because we are 
  // just extending the arrays.
  table->InsertNextBlankRow();

  // Convert the tree from a path to a star
  for (vtkIdType vertex = 0; vertex < table->GetNumberOfRows() - 1; vertex++)
    {
    tree->SetParent(vertex, root);
    }

  // Copy the table data into the tree vertex data
  tree->GetVertexData()->PassData(table->GetFieldData());
 
  return 1;
}
