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
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

#include <set>

// Gradient computation tools (cf. vtkGradientFilter)
namespace
{

template <typename T>
int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}

template <class DataType>
struct GradientHTG
{

  // intput fields
  DataType* InArray;
  int NumComp;

  vtkDataArray* CellsCenters;
  const unsigned int* Dims;

  // output fields
  vtkDoubleArray* Gradients;

  GradientHTG(
    DataType* in, int nc, vtkDataArray* centers, const unsigned int* dims, vtkDoubleArray* out)
    : InArray{ in }
    , NumComp{ nc }
    , CellsCenters{ centers }
    , Dims{ dims }
    , Gradients{ out }
  {
  }

  // central diff
  void ComputeGradienAt(vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
  {
    // Values at point
    vtkIdType id = supercursor->GetGlobalNodeIndex();
    double center[3]; // used to discriminate sides
    this->CellsCenters->GetTuple(id, center);
    double scal = this->InArray->GetTuple1(id);

    // scalar diff on each axis.
    double scalDx = 0;
    double scalDy = 0;
    double scalDz = 0;

    // Neighbors
    vtkIdType numberOfCursors = supercursor->GetNumberOfCursors();
    for (vtkIdType c = 0; c < numberOfCursors; c++)
    {
      vtkIdType nid = supercursor->GetGlobalNodeIndex(c);
      if (nid >= 0 && nid != id)
      { // valid cell
        double ncenter[3];
        this->CellsCenters->GetTuple(nid, ncenter);
        double nscal = this->InArray->GetTuple1(nid);

        // X derivative
        double dx = ncenter[0] - center[0];
        if (dx) // Dangerous div
        {
          scalDx += (nscal - scal) / dx;
        }

        // Y derivative
        double dy = ncenter[1] - center[1];
        scalDy += dy * (nscal - scal);

        // Z derivative
        double dz = ncenter[2] - center[2];
        scalDz += dz * (nscal - scal);
      }
    }

    this->Gradients->SetTuple3(id, scalDx, scalDy, scalDz);
  }
};

}

vtkStandardNewMacro(vtkHyperTreeGridGradient);

//------------------------------------------------------------------------------
vtkHyperTreeGridGradient::vtkHyperTreeGridGradient()
{
  // Process active cells scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS);

  // internal field
  this->InScalars = nullptr;
  this->Leaves = nullptr;

  // output is HTG
  this->AppropriateOutput = true;
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

  // Create storage for output scalar values
  this->CellScalars = this->InScalars->NewInstance();
  this->CellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
  this->CellScalars->Allocate(this->CellScalars->GetNumberOfComponents() * 8);

  this->InMask = input->HasMask() ? input->GetMask() : nullptr;
  this->InGhostArray = input->GetGhostCells();
  this->Leaves = vtkIdList::New(); // FIXME

  // Initialize output
  this->OutGradient->SetName("Gradient");
  this->OutGradient->SetNumberOfComponents(3);
  this->OutGradient->SetNumberOfTuples(this->InScalars->GetNumberOfTuples());

  this->Dims = const_cast<unsigned int*>(input->GetDimensions());

  // Computation
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
  while (it.GetNextTree(index))
  {
    // Initialize new cursor at root of current tree
    input->InitializeNonOrientedMooreSuperCursor(supercursor, index);
    // Compute contours recursively
    this->RecursivelyProcessTree(supercursor);
  } // it

  output->ShallowCopy(input);
  output->GetCellData()->AddArray(this->OutGradient);

  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGradient::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }
  // Retrieve dimensionality
  vtkIdType dim = supercursor->GetDimension();
  vtkIdType lvl = supercursor->GetLevel();

  std::set<std::pair<vtkIdType, vtkIdType>> neighEdges;

  // Descend further into input trees only if cursor is not a leaf
  if (!supercursor->IsLeaf())
  {
    // Iterate over contours
    // TODO
    std::cout << "coarse: " << id << " : " << dim << std::endl;

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
  else if (!this->InMask || !this->InMask->GetTuple1(id))
  {

    std::cout << "leaf: " << id << " : " << dim << std::endl;

    // Cell is not masked, iterate over its corners
    unsigned int numLeavesCorners = 1 << dim;
    for (unsigned int cornerIdx = 0; cornerIdx < numLeavesCorners; ++cornerIdx)
    {
      this->Leaves->SetNumberOfIds(numLeavesCorners);

      // Iterate over every leaf touching the corner and check ownership
      for (unsigned int leafIdx = 0; leafIdx < numLeavesCorners; ++leafIdx)
      {
        supercursor->GetCornerCursors(cornerIdx, leafIdx, this->Leaves);
      }

      // If cell owns dual cell, compute contours thereof
      // Iterate over cell corners
      double x[3];
      supercursor->GetPoint(x);
      for (unsigned int _cornerIdx = 0; _cornerIdx < numLeavesCorners; ++_cornerIdx)
      {
        // Get cursor corresponding to this corner
        vtkIdType cursorId = this->Leaves->GetId(_cornerIdx);

        // Retrieve neighbor index and add to list of cell vertices
        vtkIdType idN = supercursor->GetGlobalNodeIndex(cursorId);
        vtkIdType lvlN = supercursor->GetLevel(cursorId);

        if (idN < 0 || !supercursor->IsLeaf(cursorId))
        {
          // invalid neigh (inexistant or coarse)
          continue;
        }

        if (lvl > lvlN || idN > id)
        {
          // process edges if neigh is higher in the tree of if current cell has lowest ID
          neighEdges.emplace(id, idN);
          // process for BOTH
        }
      }
    }
    for (auto e : neighEdges)
    {
      std::cout << "     " << e.first << " - " << e.second << std::endl;
    }
    neighEdges.clear();
  }
}
