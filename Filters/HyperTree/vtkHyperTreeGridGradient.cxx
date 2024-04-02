// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkVoxel.h"

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
template <>
bool IsLeaf(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor, vtkIdType sid)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return cursor->IsRealLeaf();
  return cursor->IsRealLeaf(sid);
}

bool IsCoarse(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor,
  vtkIdType sid = vtkHyperTreeGrid::InvalidIndex)
{
  if (sid == vtkHyperTreeGrid::InvalidIndex)
    return !cursor->IsRealLeaf() && !cursor->IsVirtualLeaf();
  return !cursor->IsRealLeaf(sid) && !cursor->IsVirtualLeaf(sid);
}

template <class Cursor>
float GetExtensiveRatio(Cursor* vtkNotUsed(cursor), vtkIdType vtkNotUsed(sid))
{
  return 1;
}
template <>
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

  // input scalars
  vtkDataArray* InArray;
  // output gradient
  vtkDoubleArray* OutArray;
  // apply extensive ratio
  bool ExtensiveComputation = false;
  // internal storage
  // WARN: not thread safe
  vtkNew<vtkIdList> Leaves;
  vtkLine* Line;
  vtkPixel* Pixel;
  vtkVoxel* Voxel;

  //----------------------------------------------------------------------------
  GradientWorker(vtkDataArray* input, vtkDoubleArray* output, bool extensive)
    : InArray{ input }
    , OutArray{ output }
    , ExtensiveComputation{ extensive }
  {
    this->OutArray->Fill(0);
  }

  //----------------------------------------------------------------------------
  template <class Cursor>
  void ComputeRequestedArraysAt(Cursor* supercursor, vtkIdType subCursorId)
  {
    vtkIdType id = supercursor->GetGlobalNodeIndex();
    vtkIdType idN = supercursor->GetGlobalNodeIndex(subCursorId);

    const double extensiveRatio =
      this->ExtensiveComputation ? GetExtensiveRatio(supercursor, subCursorId) : 1;

    const int nbComp = this->InArray->GetNumberOfComponents();
    assert(nbComp <= 3); // already checked in main method

    std::vector<double> scals(nbComp);
    this->InArray->GetTuple(id, scals.data());
    std::vector<double> scalsN(nbComp);
    this->InArray->GetTuple(idN, scalsN.data());

    double center[3];
    supercursor->GetPoint(center);
    double centerN[3];
    supercursor->GetPoint(subCursorId, centerN);

    // base gradient

    std::vector<double> dist(3, 0);
    double norm = 0;
    for (int dim = 0; dim < 3; dim++)
    {
      const auto centerDist = center[dim] - centerN[dim];
      dist[dim] = centerDist;
      norm += centerDist * centerDist;
    }

    std::vector<double> grad(nbComp * 3, 0);
    if (norm != 0)
    {
      for (int comp = 0; comp < nbComp; comp++)
      {
        double scalDiff = extensiveRatio * (scals[comp] - scalsN[comp]);
        for (int dim = 0; dim < 3; dim++)
        {
          grad[comp * 3 + dim] = (scalDiff * dist[dim]) / norm;
        }
      }
    }

    // Output: impact both id and idN values
    // This part is not THREAD SAFE
    std::vector<double> gradArrTuple(nbComp * 3);
    // id contribution
    this->OutArray->GetTypedTuple(id, gradArrTuple.data());
    for (int elt = 0; elt < nbComp * 3; elt++)
    {
      gradArrTuple[elt] += grad[elt];
    }
    this->OutArray->SetTypedTuple(id, gradArrTuple.data());
    // idN contribution
    this->OutArray->GetTypedTuple(idN, gradArrTuple.data());
    for (int elt = 0; elt < nbComp * 3; elt++)
    {
      gradArrTuple[elt] += grad[elt];
    }
    this->OutArray->SetTuple(idN, gradArrTuple.data());
  }

  //----------------------------------------------------------------------------
  void AccumulateGradienAt(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* supercursor)
  {
    // Method used to keep the same structure
    this->ComputeGradientUnlimited(supercursor);
  }

  //----------------------------------------------------------------------------
  void ComputeGradientUnlimited(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* supercursor)
  {
    // Retrieve cursor info
    assert(supercursor->IsRealLeaf()); // cannot compute gradient on coarse

    auto nbc = supercursor->GetNumberOfCursors();
    for (unsigned i = 0; i < nbc; i++)
    {

      vtkIdType idN = supercursor->GetGlobalNodeIndex(i);
      if (idN < 0 || idN == vtkHyperTreeGrid::InvalidIndex)
      {
        // invalid neigh, no computation
        // can be a boundary for example
        continue;
      }

      if (IsCoarse(supercursor, i))
      {
        // avoid conting non leaf cells
        continue;
      }

      if (supercursor->IsRealLeaf(i) && supercursor->GetGlobalNodeIndex() <= idN)
      {
        // avoid double computation between siblings
        continue;
      }

      if (supercursor->IsMasked(i))
      {
        // do not consider masked cells
        continue;
      }

      ComputeRequestedArraysAt(supercursor, i);
    }
  }

  //----------------------------------------------------------------------------
  void AccumulateGradienAt(vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
  {
    NeighList neighEdges = this->FindNeighborsAt(supercursor);
    this->ComputeGradientUnstructured(supercursor, neighEdges);
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
          // invalid neigh (boundary or coarse)
          continue;
        }

        if (supercursor->IsMasked(cursorId))
        {
          // masked neigh are ignored
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
  void ComputeGradientUnstructured(
    vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor, const NeighList& neighEdges)
  {
    for (const auto& edge : neighEdges)
    {
      assert(edge.source == supercursor->GetGlobalNodeIndex());
      ComputeRequestedArraysAt(supercursor, edge.nId);
    }
  }
};

struct FieldsWorker
{
  // input fields
  vtkDoubleArray* InGradArray = nullptr;
  // output fields, filled if not nullptr
  vtkDoubleArray* OutDivArray = nullptr;
  vtkDoubleArray* OutVortArray = nullptr;
  vtkDoubleArray* OutQCritArray = nullptr;

  //----------------------------------------------------------------------------
  FieldsWorker(vtkDoubleArray* input)
    : InGradArray{ input }
  {
    if (!input || input->GetNumberOfComponents() != 9)
    {
      vtkErrorWithObjectMacro(nullptr, "Invalid input, should be an array with 9 components");
    }
  }

  //----------------------------------------------------------------------------
  void InitDivergenceArray(vtkDoubleArray* divergence)
  {
    divergence->Fill(0);
    this->OutDivArray = divergence;
  }

  //----------------------------------------------------------------------------
  void InitVorticityArray(vtkDoubleArray* vort)
  {
    vort->Fill(0);
    this->OutVortArray = vort;
  }

  //----------------------------------------------------------------------------
  void InitQCriterionArray(vtkDoubleArray* qcrit)
  {
    qcrit->Fill(0);
    this->OutQCritArray = qcrit;
  }

  //----------------------------------------------------------------------------
  void ComputeRequestedArraysAt(vtkIdType id)
  {
    std::vector<double> grad(9);
    this->InGradArray->GetTuple(id, grad.data());

    if (this->OutDivArray != nullptr)
    {
      // compute from gradient
      double div = grad[0] + grad[4] + grad[8];
      this->OutDivArray->SetTuple1(id, div);
    }

    if (this->OutVortArray != nullptr)
    {
      // compute from gradient
      double vort0 = grad[7] - grad[5];
      double vort1 = grad[2] - grad[6];
      double vort2 = grad[3] - grad[1];

      this->OutVortArray->SetTuple3(id, vort0, vort1, vort2);
    }

    if (this->OutQCritArray != nullptr)
    {
      // compute from gradient
      // see http://public.kitware.com/pipermail/paraview/2015-May/034233.html for
      // paper citation and formula on Q-criterion.
      double qCrit = -(grad[0] * grad[0] + grad[4] * grad[4] + grad[8] * grad[8]) / 2. -
        (grad[1] * grad[3] + grad[2] * grad[6] + grad[5] * grad[7]);

      this->OutQCritArray->SetTuple1(id, qCrit);
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

  if (this->InArray)
  {
    os << indent << "InArray:\n";
    this->InArray->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InArray: ( none )\n";
  }
  os << indent << "Result array name: " << this->GradientArrayName << "\n";
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
  this->InArray = this->GetInputArrayToProcess(0, input);
  if (!this->InArray)
  {
    vtkErrorMacro(<< "No input array to use for the gradient computation");
    return 1;
  }
  const int nbComp = this->InArray->GetNumberOfComponents();
  if (nbComp != 1 && nbComp != 3)
  {
    vtkErrorMacro(<< "Input array should contains scalars or 3d-vectors");
    return 1;
  }
  if ((this->ComputeQCriterion || this->ComputeVorticity || this->ComputeDivergence) && nbComp != 3)
  {
    vtkErrorMacro("Input array must have exactly three components with "
      << "ComputeDivergence, ComputeVorticity or ComputeQCriterion flag enabled.");
    return 1;
  }

  if (!this->ComputeGradient && !this->ComputeQCriterion && !this->ComputeVorticity &&
    !this->ComputeDivergence)
  {
    // nothing to do, early exit
    output->ShallowCopy(input);
    return 1;
  }

  this->InMask = nullptr; // Masks aren't supported in this filter for now.
  this->InGhostArray = input->GetGhostCells();

  // Gradient is always computed

  this->OutGradArray->SetName(this->GradientArrayName);
  this->OutGradArray->SetNumberOfComponents(this->InArray->GetNumberOfComponents() * 3);
  this->OutGradArray->SetNumberOfTuples(this->InArray->GetNumberOfTuples());
  GradientWorker gradientWorker(this->InArray, this->OutGradArray, this->ExtensiveComputation);

  // For now HTG Gradient doesn't support masks because the unlimited cursors don't either.
  // See https://gitlab.kitware.com/vtk/vtk/-/issues/19294
  // So we need to make a copy of the input and remove its mask to perform the
  // gradient processing.
  vtkNew<vtkHyperTreeGrid> inputCopy;
  inputCopy->ShallowCopy(input);
  inputCopy->SetMask(nullptr);

  // GradieGradientnt computation

  if (this->Mode == ComputeMode::UNLIMITED)
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    inputCopy->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor> supercursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      inputCopy->InitializeNonOrientedUnlimitedMooreSuperCursor(supercursor, index);
      // Compute gradient recursively
      this->RecursivelyProcessGradientTree(supercursor.Get(), gradientWorker);

      this->CheckAbort();
      if (this->GetAbortOutput())
      {
        break;
      }
    } // it
  }
  else // UNSTRUCTURED
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    inputCopy->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      inputCopy->InitializeNonOrientedMooreSuperCursor(supercursor, index);
      // Compute contours recursively
      this->RecursivelyProcessGradientTree(supercursor.Get(), gradientWorker);

      this->CheckAbort();
      if (this->GetAbortOutput())
      {
        break;
      }
    } // it
  }

  if (this->ComputeDivergence || this->ComputeVorticity || this->ComputeQCriterion)
  {
    FieldsWorker fieldsWorker(this->OutGradArray);
    if (this->ComputeDivergence)
    {
      this->OutDivArray->SetName(this->DivergenceArrayName);
      this->OutDivArray->SetNumberOfComponents(1);
      this->OutDivArray->SetNumberOfTuples(this->InArray->GetNumberOfTuples());
      fieldsWorker.InitDivergenceArray(this->OutDivArray);
    }
    if (this->ComputeVorticity)
    {
      this->OutVortArray->SetName(this->VorticityArrayName);
      this->OutVortArray->SetNumberOfComponents(3);
      this->OutVortArray->SetNumberOfTuples(this->InArray->GetNumberOfTuples());
      fieldsWorker.InitVorticityArray(this->OutVortArray);
    }
    if (this->ComputeQCriterion)
    {
      this->OutQCritArray->SetName(this->QCriterionArrayName);
      this->OutQCritArray->SetNumberOfComponents(1);
      this->OutQCritArray->SetNumberOfTuples(this->InArray->GetNumberOfTuples());
      fieldsWorker.InitQCriterionArray(this->OutQCritArray);
    }

    this->ProcessFields(fieldsWorker);
  }

  // Generate output

  output->ShallowCopy(input);
  if (this->ComputeGradient)
  {
    output->GetCellData()->AddArray(this->OutGradArray);
    if (nbComp == 1)
    {
      output->GetCellData()->SetVectors(this->OutGradArray);
    }
    else if (nbComp == 3)
    {
      output->GetCellData()->SetTensors(this->OutGradArray);
    }
  }
  if (this->ComputeVorticity)
  {
    output->GetCellData()->AddArray(this->OutVortArray);
  }
  if (this->ComputeDivergence)
  {
    output->GetCellData()->AddArray(this->OutDivArray);
  }
  if (this->ComputeQCriterion)
  {
    output->GetCellData()->AddArray(this->OutQCritArray);
  }

  return 1;
}

//------------------------------------------------------------------------------
template <class Cursor, class Worker>
void vtkHyperTreeGridGradient::RecursivelyProcessGradientTree(Cursor* supercursor, Worker& worker)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }

  // Descend further into input trees only if cursor is not a leaf
  if (!IsLeaf(supercursor))
  {
    unsigned int numChildren = supercursor->GetNumberOfChildren();
    for (unsigned int child = 0; child < numChildren; ++child)
    {
      // Create child cursor from parent in input grid
      supercursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessGradientTree(supercursor, worker);
      supercursor->ToParent();
    }
  }
  else if (!this->InMask || !this->InMask->GetTuple1(id))
  {
    worker.AccumulateGradienAt(supercursor);
  }
}

//------------------------------------------------------------------------------
template <class Worker>
void vtkHyperTreeGridGradient::ProcessFields(Worker& worker)
{
  // Retrieve global index of input cursor
  vtkIdType nbCells = this->OutGradArray->GetNumberOfTuples();
  for (vtkIdType id = 0; id < nbCells; id++)
  {
    if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
    {
      continue;
    }
    if (this->InMask && this->InMask->GetTuple1(id))
    {
      continue;
    }
    worker.ComputeRequestedArraysAt(id);
  }
}
VTK_ABI_NAMESPACE_END
