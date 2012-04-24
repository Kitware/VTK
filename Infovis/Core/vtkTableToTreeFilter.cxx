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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTableToTreeFilter.h"

#include "vtkIdTypeArray.h"
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

#include <algorithm>
#include <vector>
#include <string>

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

  vtkSmartPointer<vtkTable> new_table = vtkSmartPointer<vtkTable>::New();
  new_table->DeepCopy( table );

  // Create a mutable graph for building the tree
  vtkSmartPointer<vtkMutableDirectedGraph> builder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();

  // Check for a corner case where we have a table with 0 rows
  if (new_table->GetNumberOfRows() != 0)
    {

    // The tree will have one more vertex than the number of rows
    // in the table (the extra vertex is the new root).
    for (vtkIdType v = 0; v <= new_table->GetNumberOfRows(); ++v)
      {
      builder->AddVertex();
      }

    // Make a star, originating at the new root (the last vertex).
    vtkIdType root = new_table->GetNumberOfRows();
    for (vtkIdType v = 0; v < new_table->GetNumberOfRows(); ++v)
      {
      builder->AddEdge(root, v);
      }

    // Insert a row in the table for the new root.
    new_table->InsertNextBlankRow(-1);
    }

  // Move the structure of the mutable graph into the tree.
  if (!tree->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Built graph is not a valid tree!");
    return 0;
    }

  // Copy the table data into the tree vertex data
  tree->GetVertexData()->PassData(new_table->GetRowData());

  // The edge data should at least have a pedigree id array.
  vtkSmartPointer<vtkIdTypeArray> edgeIds =
    vtkSmartPointer<vtkIdTypeArray>::New();
  edgeIds->SetName("TableToTree edge");
  vtkIdType numEdges = tree->GetNumberOfEdges();
  edgeIds->SetNumberOfTuples(numEdges);
  for (vtkIdType i = 0; i < numEdges; ++i)
    {
    edgeIds->SetValue(i, i);
    }
  tree->GetEdgeData()->SetPedigreeIds(edgeIds);

  return 1;
}
