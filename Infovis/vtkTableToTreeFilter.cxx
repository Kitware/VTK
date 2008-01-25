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
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTree.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkTableToTreeFilter, "1.3");
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

  // Create a mutable graph for building the tree
  vtkSmartPointer<vtkMutableDirectedGraph> builder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();

  // The tree will have one more vertex than the number of rows
  // in the table (the extra vertex is the new root.
  for (vtkIdType v = 0; v <= table->GetNumberOfRows(); ++v)
    {
    builder->AddVertex();
    }

  // Make a star, originating at the new root (the last vertex).
  vtkIdType root = table->GetNumberOfRows();
  for (vtkIdType v = 0; v < table->GetNumberOfRows(); ++v)
    {
    builder->AddEdge(root, v);
    }

  // Insert a row in the table for the new root.
  // This modifies the input, but it might be ok because we are 
  // just extending the arrays.
  table->InsertNextBlankRow();

  // Move the structure of the mutable graph into the tree.
  if (!tree->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Built graph is not a valid tree!");
    return 0;
    }

  // Copy the table data into the tree vertex data
  tree->GetVertexData()->PassData(table->GetFieldData());
 
  return 1;
}
