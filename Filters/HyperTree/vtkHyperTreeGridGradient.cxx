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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <set>

// ---- Gradient computation tools ---

namespace
{

// inherit from tuple to give automatic comparison operator
struct Neigh : public std::tuple<vtkIdType, vtkIdType>
{
  vtkIdType& source;
  vtkIdType& target;
  vtkIdType nId; // ignored in the comparison

  //----------------------------------------------------------------------------
  Neigh(vtkIdType s, vtkIdType t, vtkIdType n)
    : source(std::get<0>(*this))
    , target(std::get<1>(*this))
    , nId(n)
  {
    this->source = s;
    this->target = t;
  }
};

// main computation
struct GradientWorker
{
  using NeighList = std::set<Neigh>;

  // intput fields
  vtkDataArray* InArray;
  // output fields
  vtkDoubleArray* OutArray;
  // internal storage
  // WARN: not thread safe
  vtkNew<vtkIdList> Leaves;

  //----------------------------------------------------------------------------
  GradientWorker(vtkDataArray* in, vtkDoubleArray* out)
    : InArray{ in }
    , OutArray{ out }
  {
    auto outRange = vtk::DataArrayValueRange<3>(out);
    vtkSMPTools::Fill(outRange.begin(), outRange.end(), 0);
  }

  //----------------------------------------------------------------------------
  void AccumulateGradienAt(vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
  {
    NeighList neighEdges = this->FindNeighborsAt(supercursor);
    this->ComputeGradientForEdges(supercursor, neighEdges);
  }

  //----------------------------------------------------------------------------
  NeighList FindNeighborsAt(vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
  {
    // Retrieve cursor info
    vtkIdType id = supercursor->GetGlobalNodeIndex();
    vtkIdType dim = supercursor->GetDimension();
    vtkIdType lvl = supercursor->GetLevel();

    assert(supercursor->IsLeaf()); // cannot compute gradient on coarse

    // output
    NeighList neighEdges;

    // Cell is not masked, iterate over its corners
    vtkIdType numLeavesCorners = 1ULL << dim;
    for (vtkIdType cornerIdx = 0; cornerIdx < numLeavesCorners; ++cornerIdx)
    {
      this->Leaves->SetNumberOfIds(numLeavesCorners);

      // Iterate over every leaf touching the corner and check ownership
      for (vtkIdType leafIdx = 0; leafIdx < numLeavesCorners; ++leafIdx)
      {
        supercursor->GetCornerCursors(cornerIdx, leafIdx, this->Leaves);
      }

      // If cell owns dual cell, compute contours thereof
      // Iterate over cell corners
      double x[3];
      supercursor->GetPoint(x);
      for (vtkIdType _cornerIdx = 0; _cornerIdx < numLeavesCorners; ++_cornerIdx)
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
          neighEdges.emplace(id, idN, cursorId);
        }
      }
    }
    return neighEdges;
  }

  //----------------------------------------------------------------------------
  void ComputeGradientForEdges(
    vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor, const NeighList& neighEdges)
  {
    vtkIdType id = supercursor->GetGlobalNodeIndex();
    double center[3];
    supercursor->GetPoint(center);
    const double scal = this->InArray->GetTuple1(id);

    for (const auto& e : neighEdges)
    {
      assert(e.source == id);

      const double nScal = this->InArray->GetTuple1(e.target);
      const double scalDiff = scal - nScal;

      double centerN[3];
      supercursor->GetPoint(e.nId, centerN);

      // differential computation
      double grad[3] = { scalDiff, scalDiff, scalDiff };

      const double dx = center[0] - centerN[0];
      grad[0] *= dx;
      const double dy = center[1] - centerN[1];
      grad[1] *= dy;
      const double dz = center[2] - centerN[2];
      grad[2] *= dz;

      // This part is not THREAD SAFE

      double gradArrTuple[3];
      // source contribution
      this->OutArray->GetTypedTuple(e.source, gradArrTuple);
      gradArrTuple[0] += grad[0];
      gradArrTuple[1] += grad[1];
      gradArrTuple[2] += grad[2];
      this->OutArray->SetTypedTuple(e.source, gradArrTuple);
      // target contribution
      this->OutArray->GetTypedTuple(e.target, gradArrTuple);
      gradArrTuple[0] += grad[0];
      gradArrTuple[1] += grad[1];
      gradArrTuple[2] += grad[2];
      this->OutArray->SetTuple(e.target, gradArrTuple);
    }
  }
};

} // end of anonymous namespace

// ---- vtkHyperTreeGridGradient ---

vtkStandardNewMacro(vtkHyperTreeGridGradient);

//------------------------------------------------------------------------------
vtkHyperTreeGridGradient::vtkHyperTreeGridGradient()
{
  // Process active cells scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS);

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
  os << indent << "Result array name: " << this->ResultArrayName << "\n";
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
    vtkErrorMacro(<< "No scalar data to use for the gradient computation");
    return 1;
  }
  if (this->InScalars->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(<< "The given input array is not a scalar array");
    return 1;
  }

  this->InMask = input->HasMask() ? input->GetMask() : nullptr;
  this->InGhostArray = input->GetGhostCells();

  // Initialize output
  this->OutGradient->SetName(this->ResultArrayName.c_str());
  this->OutGradient->SetNumberOfComponents(3);
  this->OutGradient->SetNumberOfTuples(this->InScalars->GetNumberOfTuples());

  GradientWorker worker(this->InScalars, this->OutGradient);

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
    this->RecursivelyProcessTree(supercursor, worker);
  } // it

  output->ShallowCopy(input);
  output->GetCellData()->AddArray(this->OutGradient);
  output->GetCellData()->SetActiveVectors(this->ResultArrayName.c_str());

  return 1;
}

//------------------------------------------------------------------------------
template <class Worker>
void vtkHyperTreeGridGradient::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor, Worker& worker)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }

  // Descend further into input trees only if cursor is not a leaf
  if (!supercursor->IsLeaf())
  {
    unsigned int numChildren = supercursor->GetNumberOfChildren();
    for (unsigned int child = 0; child < numChildren; ++child)
    {
      // Create child cursor from parent in input grid
      supercursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessTree(supercursor, worker);
      supercursor->ToParent();
    }
  }
  else if (!this->InMask || !this->InMask->GetTuple1(id))
  {
    worker.AccumulateGradienAt(supercursor);
  }
}
