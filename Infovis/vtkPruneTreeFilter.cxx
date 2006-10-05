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

#include "vtkPruneTreeFilter.h"
#include "vtkTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"

vtkCxxRevisionMacro(vtkPruneTreeFilter, "1.1");
vtkStandardNewMacro(vtkPruneTreeFilter);


vtkPruneTreeFilter::vtkPruneTreeFilter()
{
  this->ParentNode = 0;
}

vtkPruneTreeFilter::~vtkPruneTreeFilter()
{
}

void vtkPruneTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Parent: " << this->ParentNode << endl;
}

int vtkPruneTreeFilter::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Store the XML hierarchy into a vtkTree
  vtkTree* inputTree = vtkTree::GetData(inputVector[0]);
  vtkTree* outputTree = vtkTree::GetData(outputVector);

  if (this->ParentNode < 0 || this->ParentNode >= inputTree->GetNumberOfNodes())
    {
    vtkErrorMacro("Parent node must be part of the tree " << this->ParentNode 
      << " >= " << inputTree->GetNumberOfNodes());
    return 0;
    }

  outputTree->DeepCopy(inputTree);

  // Now, prune the tree
  outputTree->RemoveNodeAndDescendants(this->ParentNode);

  return 1;
}
