// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridNonOrientedUnlimitedSuperCursor.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "vtkHyperTreeGridTools.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedUnlimitedSuperCursor*
vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::Clone()
{
  vtkHyperTreeGridNonOrientedUnlimitedSuperCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->CentralCursor->Initialize(this->CentralCursor.Get());
  clone->CurrentFirstNonValidEntryByLevel = this->CurrentFirstNonValidEntryByLevel;
  {
    clone->FirstNonValidEntryByLevel.resize(this->FirstNonValidEntryByLevel.size());
    std::vector<unsigned int>::iterator in = this->FirstNonValidEntryByLevel.begin();
    std::vector<unsigned int>::iterator out = clone->FirstNonValidEntryByLevel.begin();
    for (; in != this->FirstNonValidEntryByLevel.end(); ++in, ++out)
    {
      (*out) = (*in);
    }
  }
  {
    clone->Entries.resize(this->Entries.size());
    std::vector<vtkHyperTreeGridGeometryUnlimitedLevelEntry>::iterator in = this->Entries.begin();
    std::vector<vtkHyperTreeGridGeometryUnlimitedLevelEntry>::iterator out = clone->Entries.begin();
    for (; in != this->Entries.end(); ++in, ++out)
    {
      (*out).Copy(&(*in));
    }
  }
  clone->FirstCurrentNeighboorReferenceEntry = this->FirstCurrentNeighboorReferenceEntry;
  {
    clone->ReferenceEntries.resize(this->ReferenceEntries.size());
    std::vector<unsigned int>::iterator in = this->ReferenceEntries.begin();
    std::vector<unsigned int>::iterator out = clone->ReferenceEntries.begin();
    for (; in != this->ReferenceEntries.end(); ++in, ++out)
    {
      (*out) = (*in);
    }
  }
  clone->IndiceCentralCursor = this->IndiceCentralCursor;
  clone->NumberOfCursors = this->NumberOfCursors;
  clone->ChildCursorToParentCursorTable = this->ChildCursorToParentCursorTable;
  clone->ChildCursorToChildTable = this->ChildCursorToChildTable;
  return clone;
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetGrid()
{
  return this->Grid;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::HasTree()
{
  return this->CentralCursor->HasTree();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::HasTree(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->HasTree();
  }
  return vtk::hypertreegrid::HasTree(this->Entries[this->GetIndiceEntry(icursor)]);
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetTree()
{
  return this->CentralCursor->GetTree();
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetTree(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetTree();
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetTree();
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetVertexId()
{
  return this->CentralCursor->GetVertexId();
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetVertexId(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetVertexId();
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetVertexId();
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetGlobalNodeIndex()
{
  return this->CentralCursor->GetGlobalNodeIndex();
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetGlobalNodeIndex(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetGlobalNodeIndex();
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetGlobalNodeIndex();
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetInformation(
  unsigned int icursor, unsigned int& level, bool& leaf, vtkIdType& id)
{
  if (icursor == this->IndiceCentralCursor)
  {
    level = this->CentralCursor->GetLevel();
    leaf = this->CentralCursor->IsLeaf();
    id = this->CentralCursor->GetGlobalNodeIndex();
    return this->CentralCursor->GetTree();
  }
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry = this->Entries[this->GetIndiceEntry(icursor)];
  vtkHyperTree* tree = entry.GetTree();
  if (tree)
  {
    level = entry.GetLevel();
    leaf = entry.IsLeaf(this->Grid);
    id = entry.GetGlobalNodeIndex();
  }
  else
  {
    level = std::numeric_limits<unsigned int>::max();
  }
  return tree;
}

//------------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//------------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetNumberOfChildren()
{
  return this->CentralCursor->GetTree()->GetNumberOfChildren();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->CentralCursor->SetGlobalIndexStart(index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->CentralCursor->SetGlobalIndexFromLocal(index);
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetOrigin()
{
  return this->CentralCursor->GetOrigin();
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetSize()
{
  return this->CentralCursor->GetSize();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetBounds(double bounds[6])
{
  this->CentralCursor->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetBounds(
  unsigned int icursor, double bounds[6])
{
  if (icursor == this->IndiceCentralCursor)
  {
    this->CentralCursor->GetBounds(bounds);
  }
  else
  {
    this->Entries[this->GetIndiceEntry(icursor)].GetBounds(bounds);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetPoint(double point[3])
{
  this->CentralCursor->GetPoint(point);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetPoint(
  unsigned int icursor, double point[3])
{
  if (icursor == this->IndiceCentralCursor)
  {
    this->CentralCursor->GetPoint(point);
  }
  else
  {
    this->Entries[this->GetIndiceEntry(icursor)].GetPoint(point);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::SetMask(bool state)
{
  assert("pre: not_tree" && this->CentralCursor->GetTree());
  this->CentralCursor->SetMask(state);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::SetMask(unsigned int icursor, bool state)
{
  if (icursor == this->IndiceCentralCursor)
  {
    this->SetMask(state);
  }
  else
  {
    vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry =
      this->Entries[this->GetIndiceEntry(icursor)];
    assert("pre: not_tree" && entry.GetTree());
    entry.SetMask(this->Grid, state);
  }
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsMasked()
{
  return this->CentralCursor->IsMasked();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsMasked(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->IsMasked();
  }
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry = this->Entries[this->GetIndiceEntry(icursor)];
  return entry.IsMasked(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsLeaf()
{
  return this->CentralCursor->IsLeaf();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsLeaf(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->IsLeaf();
  }
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry = this->Entries[this->GetIndiceEntry(icursor)];
  return entry.IsLeaf(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsRealLeaf()
{
  return this->CentralCursor->IsRealLeaf();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsRealLeaf(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->IsRealLeaf();
  }
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry = this->Entries[this->GetIndiceEntry(icursor)];
  return entry.IsRealLeaf(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsVirtualLeaf()
{
  return this->CentralCursor->IsVirtualLeaf();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsVirtualLeaf(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->IsVirtualLeaf();
  }
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry = this->Entries[this->GetIndiceEntry(icursor)];
  return entry.IsVirtualLeaf(this->Grid);
}

//------------------------------------------------------------------------------
double vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetExtensivePropertyRatio()
{
  return this->GetExtensivePropertyRatio(this->IndiceCentralCursor);
}

//------------------------------------------------------------------------------
double vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetExtensivePropertyRatio(vtkIdType index)
{
  const unsigned int nbVirtual = this->GetLevel(index) - this->GetLastRealLevel(index);
  const double nbDiv =
    std::pow(this->GetTree()->GetBranchFactor(), nbVirtual * this->GetDimension());
  return 1.0 / nbDiv;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::IsRoot()
{
  return this->CentralCursor->IsRoot();
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetLevel()
{
  return this->CentralCursor->GetLevel();
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetLevel(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetLevel();
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetLevel();
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetLastRealLevel()
{
  return this->CentralCursor->GetLastRealLevel();
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetLastRealLevel(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetLastRealLevel();
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetLastRealLevel();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::ToChild(unsigned char ichild)
{
  ++this->CurrentFirstNonValidEntryByLevel;
  if (this->FirstNonValidEntryByLevel.size() == this->CurrentFirstNonValidEntryByLevel)
  {
    this->FirstNonValidEntryByLevel.emplace_back();
  }
  this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel] =
    this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel - 1];

  this->FirstCurrentNeighboorReferenceEntry += (this->NumberOfCursors - 1);
  if (this->ReferenceEntries.size() == this->FirstCurrentNeighboorReferenceEntry)
  {
    this->ReferenceEntries.resize(
      this->FirstCurrentNeighboorReferenceEntry + (this->NumberOfCursors - 1));
  }

  // Point into traversal tables at child location
  int offset = ichild * this->NumberOfCursors;
  const unsigned int* pTab = this->ChildCursorToParentCursorTable + offset;
  const unsigned int* cTab = this->ChildCursorToChildTable + offset;

  // Move each cursor in the supercursor down to a child
  for (unsigned int i = 0; i < this->NumberOfCursors; ++i)
  {
    if (i != this->IndiceCentralCursor)
    {
      // Make relevant cursor in parent cell point towards current child cursor
      unsigned int j = pTab[i];

      // If neighboring cell is further subdivided, then descend into it
      unsigned int reference = std::numeric_limits<unsigned int>::max();
      if (j == this->IndiceCentralCursor)
      {
        reference = this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel];
        ++this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel];
        if (this->Entries.size() <= reference)
        {
          this->Entries.resize(reference + 1);
        }
        //
        if (i > this->IndiceCentralCursor)
        {
          this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i - 1] = reference;
        }
        else
        {
          this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i] = reference;
        }
        //
        vtkHyperTreeGridGeometryUnlimitedLevelEntry& current = this->Entries[reference];
        current.Initialize(this->CentralCursor->GetTree(), this->CentralCursor->GetLevel(),
          this->CentralCursor->GetVertexId(), this->CentralCursor->GetOrigin());

        if (!this->IsMasked())
        {
          if (current.GetTree() && !current.IsLeaf(this->Grid))
          {
            // Move to child
            current.ToChild(this->Grid, cTab[i]);
          }
        }
      }
      else
      {
        unsigned int previous = this->GetIndicePreviousEntry(j);
        //
        if (this->Entries[previous].GetTree() && !this->Entries[previous].IsLeaf(this->Grid) &&
          !(this->GetGrid()->HasMask()
              ? this->GetGrid()->GetMask()->GetValue(this->Entries[previous].GetGlobalNodeIndex())
              : 0))
        {
          reference = this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel];
          ++this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel];
          if (this->Entries.size() <= reference)
          {
            this->Entries.resize(reference + 1);
          }
          if (i > this->IndiceCentralCursor)
          {
            this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i - 1] = reference;
          }
          else
          {
            this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i] = reference;
          }
          //
          vtkHyperTreeGridGeometryUnlimitedLevelEntry& current = this->Entries[reference];
          current.Copy(&(this->Entries[previous]));
          current.ToChild(this->Grid, cTab[i]);
        }
        else
        {
          if (j > this->IndiceCentralCursor)
          {
            reference = this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
              (this->NumberOfCursors - 1) + j - 1];
          }
          else
          {
            reference = this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
              (this->NumberOfCursors - 1) + j];
          }
          if (i > this->IndiceCentralCursor)
          {
            this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i - 1] = reference;
          }
          else
          {
            this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + i] = reference;
          }
        }
      }
    }
  } // i
  this->CentralCursor->ToChild(cTab[this->IndiceCentralCursor]);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::ToRoot()
{
  assert("pre: hypertree_exist" && !this->Entries.empty());
  this->CentralCursor->ToRoot();
  this->CurrentFirstNonValidEntryByLevel = 0;
  this->FirstCurrentNeighboorReferenceEntry = 0;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::ToParent()
{
  assert("pre: Non_root" && !this->IsRoot());
  assert("has: Valid entry" && this->CurrentFirstNonValidEntryByLevel > 0);
  this->CentralCursor->ToParent();
  this->CurrentFirstNonValidEntryByLevel--;
  this->FirstCurrentNeighboorReferenceEntry -= (this->NumberOfCursors - 1);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedUnlimitedSuperCursor--" << endl;
  this->CentralCursor->PrintSelf(os, indent);
  os << indent << "IndiceCentralCursor: " << this->IndiceCentralCursor << endl;
  os << indent << "NumberOfCursors: " << this->NumberOfCursors << endl;
}

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::vtkHyperTreeGridNonOrientedUnlimitedSuperCursor()
{
  this->CentralCursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor>::New();
}

//------------------------------------------------------------------------------

vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::
  ~vtkHyperTreeGridNonOrientedUnlimitedSuperCursor() = default;

//------------------------------------------------------------------------------
vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>
vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetOrientedGeometryCursor(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetHyperTreeGridOrientedGeometryCursor(this->Grid);
  }
  return this->Entries[this->GetIndiceEntry(icursor)].GetHyperTreeGridOrientedGeometryCursor(
    this->Grid);
}

//------------------------------------------------------------------------------

vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>
vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetNonOrientedGeometryCursor(unsigned int icursor)
{
  if (icursor == this->IndiceCentralCursor)
  {
    return this->CentralCursor->GetHyperTreeGridNonOrientedGeometryCursor(this->Grid);
  }
  assert(false);
  return this->Entries[this->GetIndiceEntry(icursor)].GetHyperTreeGridNonOrientedGeometryCursor(
    this->Grid);
}

//------------------------------------------------------------------------------

unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetIndiceEntry(unsigned int icursor)
{
  assert("pre: icursor != IndiceCentralCursor" && icursor != this->IndiceCentralCursor);
  assert("pre: valid_icursor" && icursor < this->NumberOfCursors);

  const long refId = icursor > this->IndiceCentralCursor
    ? this->FirstCurrentNeighboorReferenceEntry + icursor - 1
    : this->FirstCurrentNeighboorReferenceEntry + icursor;

  assert("pre: valid_icursor" && 0 <= refId && refId < long(this->ReferenceEntries.size()));
  assert("pre: valid_icursor" && this->ReferenceEntries[refId] < this->Entries.size());

  return this->ReferenceEntries[refId];
}

//------------------------------------------------------------------------------

unsigned int vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::GetIndicePreviousEntry(
  unsigned int icursor)
{
  assert("pre: icursor != IndiceCentralCursor" && icursor != this->IndiceCentralCursor);
  assert("pre: valid_icursor" && icursor < this->NumberOfCursors);

  const long refId = icursor > this->IndiceCentralCursor
    ? this->FirstCurrentNeighboorReferenceEntry - (this->NumberOfCursors - 1) + icursor - 1
    : this->FirstCurrentNeighboorReferenceEntry - (this->NumberOfCursors - 1) + icursor;

  assert("pre: valid_icursor" && 0 <= refId && refId < long(this->ReferenceEntries.size()));
  assert("pre: valid_icursor" && this->ReferenceEntries[refId] < this->Entries.size());

  return this->ReferenceEntries[refId];
}
VTK_ABI_NAMESPACE_END
