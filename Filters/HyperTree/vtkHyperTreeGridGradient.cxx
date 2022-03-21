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

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourHelper.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVoxel.h"

vtkStandardNewMacro(vtkHyperTreeGridGradient);

//------------------------------------------------------------------------------
vtkHyperTreeGridGradient::vtkHyperTreeGridGradient()
{
  // Process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  // Input scalars point to null by default
  this->InScalars = nullptr;

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

  // First pass across tree roots to evince cells intersected by contours
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

  // Second pass across tree roots: now compute isocontours recursively
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
  while (it.GetNextTree(index))
  {
    // Initialize new Moore cursor at root of current tree
    input->InitializeNonOrientedMooreSuperCursor(supercursor, index);
    // Compute contours recursively
    this->RecursivelyProcessTree(supercursor);
  } // it

  output->ShallowCopy(input);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGradient::RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedCursor* cursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Descend further into input trees only if cursor is not a leaf
  bool selected = false;
  if (cursor->IsLeaf())
  {
    selected = true;
    std::cout << "Leaf : " << id << " val: " << this->InScalars->GetTuple1(id) << std::endl;
  }
  else
  {
    // Cursor is not at leaf, recurse to all all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);

      std::cout << "Node : " << id << std::endl;

      // Recurse and keep track of whether this branch is selected
      selected |= this->RecursivelyPreProcessTree(cursor);

      // Check if branch not completely selected
      if (!selected)
      {
      }

      cursor->ToParent();
    } // for child
  }

  // Return whether current node was fully selected
  return selected;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGradient::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
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
