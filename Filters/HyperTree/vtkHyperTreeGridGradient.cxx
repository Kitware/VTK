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
#include "vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <set>

// ---- Gradient computation tools ---

namespace
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Utility
template <class Cursor>
bool IsLeaf(Cursor* cursor, vtkIdType sid = vtkHyperTreeGrid::InvalidIndex)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return cursor->IsLeaf();
  return cursor->IsLeaf(sid);
}
bool IsLeaf(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor,
  vtkIdType sid = vtkHyperTreeGrid::InvalidIndex)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return cursor->IsRealLeaf();
  return cursor->IsRealLeaf(sid);
}

template <class Cursor>
bool IsCoarse(Cursor* cursor, vtkIdType sid = vtkHyperTreeGrid::InvalidIndex)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return !cursor->IsLeaf();
  return !cursor->IsLeaf(sid);
}
bool IsCoarse(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor,
  vtkIdType sid = vtkHyperTreeGrid::InvalidIndex)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return !cursor->IsRealLeaf() && !cursor->IsVirtualLeaf();
  return !cursor->IsRealLeaf(sid) && !cursor->IsVirtualLeaf(sid);
}

template <class Cursor>
float GetExtensiveRatio(Cursor* cursor, vtkIdType sid)
{
  return 1;
}
float GetExtensiveRatio(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor, vtkIdType sid)
{
  return cursor->GetExtensivePropertyRatio(sid);
}

//------------------------------------------------------------------------------
// inherit from tuple to give automatic comparison operator
struct Neigh : public std::tuple<vtkIdType, vtkIdType>
{
  vtkIdType& source;
  vtkIdType& target;
  vtkIdType nId; // ignored in the comparison

  Neigh(vtkIdType s, vtkIdType t, vtkIdType n)
    : source(std::get<0>(*this))
    , target(std::get<1>(*this))
    , nId(n)
  {
    this->source = s;
    this->target = t;
  }
};

//------------------------------------------------------------------------------
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
  // apply extensive ratio ?
  bool ExtensiveComputation = false;

  //----------------------------------------------------------------------------
  GradientWorker(vtkDataArray* in, vtkDoubleArray* out, bool extensive)
    : InArray{ in }
    , OutArray{ out }
    , ExtensiveComputation{ extensive }
  {
    auto outRange = vtk::DataArrayValueRange<3>(out);
    vtkSMPTools::Fill(outRange.begin(), outRange.end(), 0);
  }

  //----------------------------------------------------------------------------
  void AccumulateGradienAt(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* supercursor)
  {
    // Method used to keep the same strcture
    this->ComputeGradientUnlimited(supercursor);
  }

  //----------------------------------------------------------------------------
  template <class Cursor>
  void ComputeGradientAt(Cursor* supercursor, vtkIdType subCursorId)
  {
    vtkIdType id = supercursor->GetGlobalNodeIndex();
    vtkIdType idN = supercursor->GetGlobalNodeIndex(subCursorId);

    if (idN < 0 || idN == vtkHyperTreeGrid::InvalidIndex)
    {
      std::cout << "Invalid: " << id << " || " << idN << std::endl;
      // invalid neigh, no computation
      return;
    }

    if (id == idN || IsCoarse(supercursor, subCursorId))
    {
      std::cout << "Avoid: " << id << " || " << idN << " lvl " << supercursor->GetLevel(subCursorId)
                << std::endl;
      // opposite cell has nothing to bring here
      return;
    }

    vtkIdType vid = supercursor->GetVertexId(subCursorId); // TODO use for scalars ?
    const double scal = this->InArray->GetTuple1(id);
    const double nScal = this->InArray->GetTuple1(idN);
    const double extensiveRatio =
      this->ExtensiveComputation ? GetExtensiveRatio(supercursor, subCursorId) : 1;
    const double scalDiff = extensiveRatio * (scal - nScal);

    double center[3];
    supercursor->GetPoint(center);
    double centerN[3];
    supercursor->GetPoint(subCursorId, centerN);

    std::cout << "Gradient: " << id << " || " << idN << std::endl;
    std::cout << "   " << center[0] << " " << center[1] << " " << center[2] << " " << std::endl;
    std::cout << "   " << centerN[0] << " " << centerN[1] << " " << centerN[2] << " " << std::endl;
    std::cout << "   ratio:" << extensiveRatio << std::endl;
    std::cout << "_____" << std::endl;

    // differential computation
    // TODO: add the ratio here
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
    this->OutArray->GetTypedTuple(id, gradArrTuple);
    gradArrTuple[0] += grad[0];
    gradArrTuple[1] += grad[1];
    gradArrTuple[2] += grad[2];
    this->OutArray->SetTypedTuple(id, gradArrTuple);
    // target contribution
    this->OutArray->GetTypedTuple(idN, gradArrTuple);
    gradArrTuple[0] += grad[0];
    gradArrTuple[1] += grad[1];
    gradArrTuple[2] += grad[2];
    this->OutArray->SetTuple(idN, gradArrTuple);
  }

  //----------------------------------------------------------------------------
  void ComputeGradientUnlimited(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* supercursor)
  {
    // Retrieve cursor info
    assert(supercursor->IsRealLeaf()); // cannot compute gradient on coarse

    auto nbc = supercursor->GetNumberOfCursors();
    for (unsigned i = 0; i < nbc; i++)
    {
      ComputeGradientAt(supercursor, i);
    }
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
    for (const auto& e : neighEdges)
    {
      assert(e.source == id);
      ComputeGradientAt(supercursor, e.nId);
    }
  }
};

VTK_ABI_NAMESPACE_END
} // end of anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
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

  GradientWorker worker(this->InScalars, this->OutGradient, this->ExtensiveComputation);

  // Computation
  if (this->Mode == ComputeMode::UNLIMITED)
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> supercursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      input->InitializeNonOrientedUnlimitedMooreSuperCursor(supercursor, index);
      // Compute contours recursively
      this->RecursivelyProcessTree(supercursor.Get(), worker);
    } // it
  }
  else // UNSTRUCTURED
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      input->InitializeNonOrientedMooreSuperCursor(supercursor, index);
      // Compute contours recursively
      this->RecursivelyProcessTree(supercursor.Get(), worker);
    } // it
  }

  std::cout << "Computation done ----------------------- " << std::endl;

  output->ShallowCopy(input);
  output->GetCellData()->AddArray(this->OutGradient);
  output->GetCellData()->SetActiveVectors(this->ResultArrayName.c_str());

  return 1;
}

//------------------------------------------------------------------------------
template <class Cursor, class Worker>
void vtkHyperTreeGridGradient::RecursivelyProcessTree(Cursor* supercursor, Worker& worker)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }

  // Descend further into input trees only if cursor is not a leaf
  // TODO specialize depending on the cursor type here
  if (!IsLeaf(supercursor))
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
VTK_ABI_NAMESPACE_END
