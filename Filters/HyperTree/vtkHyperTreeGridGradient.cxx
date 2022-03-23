/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHyperTreeGridGradient.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridGradient);

//------------------------------------------------------------------------------
vtkHyperTreeGridGradient::vtkHyperTreeGridGradient()
{
  // Process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  // scalars point to null by default
  this->InScalars = nullptr;
  this->AvgChildScalars = nullptr;

  this->AppropriateOutput = true; // output is HTG
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGradient::~vtkHyperTreeGridGradient() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->InScalars)
  {
    os << indent << "InScalars:\n";
    this->InScalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InScalars: ( none )\n";
  }

  // This one is empty outside the RequestData
  if (this->AvgChildScalars)
  {
    os << indent << "AvgChildScalars:\n";
    this->AvgChildScalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "AvgChildScalars: ( none )\n";
  }
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGradient::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve scalar quantity of interest
  this->InScalars = this->GetInputArrayToProcess(0, input);
  if (!this->InScalars)
  {
    vtkWarningMacro(<< "No scalar data to use for the gradient computation");
    return 1;
  }

  // Avg temporary array
  this->AvgChildScalars = vtkDoubleArray::New();
  this->AvgChildScalars->SetNumberOfComponents(1);
  this->AvgChildScalars->SetNumberOfTuples(this->InScalars->GetNumberOfTuples());

  // Initialize output
  this->OutGradient->SetName("Gradient");
  this->OutGradient->SetNumberOfComponents(1);
  this->OutGradient->SetNumberOfTuples(this->InScalars->GetNumberOfTuples());

  // First pass across tree roots to fill the AvgChildScalars
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(cursor, index);
    // Pre-process tree recursively
    this->RecursivelyPreProcessTree(cursor);
  } // it

  // Second pass across tree roots: now compute the gradient recursively

  input->InitializeTreeIterator(it);
  // vtkNew<vtkHyperTreeGridNonOrientedCursor> supercursor;
  while (it.GetNextTree(index))
  {
    // Initialize new Moore cursor at root of current tree
    input->InitializeNonOrientedCursor(cursor, index);
    // Compute contours recursively
    this->RecursivelyProcessTree(cursor);
  } // it

  output->ShallowCopy(input);
  output->GetCellData()->AddArray(this->OutGradient);

  this->AvgChildScalars->Delete();

  return 1;
}

//------------------------------------------------------------------------------
// avg on coarse cells
// TODO: avoid recompute sum at each level
std::forward_list<vtkIdType> vtkHyperTreeGridGradient::RecursivelyPreProcessTree(
  vtkHyperTreeGridNonOrientedCursor* cursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  std::forward_list<vtkIdType> childs;
  childs.push_front(id);

  if (cursor->IsLeaf())
  {
    this->AvgChildScalars->SetTuple1(id, this->InScalars->GetTuple1(id));
  }
  else
  {
    // Cursor is not at leaf, recurse to all all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);

      this->OutGradient->SetTuple1(id, 0.);

      // Recurse and keep track of whether this branch is selected
      childs.merge(this->RecursivelyPreProcessTree(cursor));

      // Compute AVG
      // TODO: stream computation to avoid overflow
      vtkIdType nbChilds = 0;
      double sumScals = 0;
      for (const auto cid : childs)
      {
        ++nbChilds;
        sumScals += this->InScalars->GetTuple1(cid);
      }
      this->AvgChildScalars->SetTuple1(id, sumScals / nbChilds);

      cursor->ToParent();
    } // for child
  }

  return childs;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGradient::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedCursor* supercursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  // Retrieve dimensionality
  unsigned int dim = supercursor->GetDimension();

  // Descend further into input trees only if cursor is not a leaf
  if (!supercursor->IsLeaf())
  {
    // Cell is not selected until proven otherwise
    bool selected = false;

    // Process

    if (selected)
    {
      // Node has at least one neighbor containing one contour, recurse to all children
      unsigned int numChildren = supercursor->GetNumberOfChildren();
      for (unsigned int child = 0; child < numChildren; ++child)
      {
        // Create child cursor from parent in input grid
        supercursor->ToChild(child);
        // Recurse
        this->RecursivelyProcessTree(supercursor);
        supercursor->ToParent();
      }
    }
  }
}
