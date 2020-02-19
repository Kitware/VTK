/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGrid.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTree.h"

#include "vtkObjectFactory.h"

#include "vtkHyperTreeGridScales.h"

#include "limits.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>
#include <vector>

#include "vtkBitArray.h"
#include "vtkUnsignedLongArray.h"

#include "vtkIdList.h"

//-----------------------------------------------------------------------------

void vtkHyperTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimension: " << this->Dimension << "\n";
  os << indent << "BranchFactor: " << this->BranchFactor << "\n";
  os << indent << "NumberOfChildren: " << this->NumberOfChildren << "\n";

  os << indent << "NumberOfLevels: " << this->Datas->NumberOfLevels << "\n";
  os << indent << "NumberOfVertices (coarse and leaves): " << this->Datas->NumberOfVertices << "\n";
  os << indent << "NumberOfNodes (coarse): " << this->Datas->NumberOfNodes << "\n";

  if (this->IsGlobalIndexImplicit())
  {
    os << indent << "Implicit global index mapping\n";
    os << indent << "GlobalIndexStart: " << this->Datas->GlobalIndexStart << "\n";
  }
  else
  {
    os << indent << "Explicit global index mapping\n";
  }

  this->PrintSelfPrivate(os, indent);
}

//-----------------------------------------------------------------------------

void vtkHyperTree::Initialize(
  unsigned char branchFactor, unsigned char dimension, unsigned char numberOfChildren)
{
  this->BranchFactor = branchFactor;
  this->Dimension = dimension;
  this->NumberOfChildren = numberOfChildren;

  this->Datas = std::make_shared<vtkHyperTreeData>();
  this->Datas->TreeIndex = -1;
  this->Datas->NumberOfLevels = 1;
  this->Datas->NumberOfVertices = 1;
  this->Datas->NumberOfNodes = 0;
  // By default, nothing is used
  // No GlobalIndexStart, no GlobalIndexFromLocal
  this->Datas->GlobalIndexStart = -1;

  this->Scales = nullptr;

  this->InitializePrivate();
}

//-----------------------------------------------------------------------------

void vtkHyperTree::CopyStructure(vtkHyperTree* ht)
{
  assert("pre: ht_exists" && ht != nullptr);

  // Copy or shared
  this->Datas = ht->Datas;
  this->BranchFactor = ht->BranchFactor;
  this->Dimension = ht->Dimension;
  this->NumberOfChildren = ht->NumberOfChildren;
  this->Scales = ht->Scales;
  this->CopyStructurePrivate(ht);
}

//-----------------------------------------------------------------------------

std::shared_ptr<vtkHyperTreeGridScales> vtkHyperTree::InitializeScales(
  const double* scales, bool reinitialize) const
{
  if (this->Scales == nullptr || reinitialize)
  {
    this->Scales = std::make_shared<vtkHyperTreeGridScales>(this->BranchFactor, scales);
  }
  return this->Scales;
}

//-----------------------------------------------------------------------------

void vtkHyperTree::GetScale(double s[3]) const
{
  assert("pre: scales_exists" && this->Scales != nullptr);
  const double* scale = this->Scales->GetScale(0);
  memcpy(s, scale, 3 * sizeof(double));
}

//-----------------------------------------------------------------------------

double vtkHyperTree::GetScale(unsigned int d) const
{
  assert("pre: scales_exists" && this->Scales != nullptr);
  const double* scale = this->Scales->GetScale(0);
  return scale[d];
}

//=============================================================================
struct vtkCompactHyperTreeData
{
  // Storage to record the parent of each tree vertex
  std::vector<unsigned int> ParentToElderChild_stl;

  // Storage to record the local to global id mapping
  std::vector<vtkIdType> GlobalIndexTable_stl;
};

//=============================================================================
class vtkCompactHyperTree : public vtkHyperTree
{
public:
  vtkTemplateTypeMacro(vtkCompactHyperTree, vtkHyperTree);

  //---------------------------------------------------------------------------
  static vtkCompactHyperTree* New();

  //---------------------------------------------------------------------------
  void RecursiveGetByLevelForWriter(vtkBitArray* inIsMasked, int level, vtkIdType index,
    std::vector<std::vector<bool> >& descByLevel, std::vector<std::vector<bool> >& maskByLevel,
    std::vector<std::vector<uint64_t> >& globalIdByLevel)
  {
    vtkIdType idg = this->GetGlobalIndexFromLocal(index);
    bool mask = (inIsMasked->GetValue(idg) != 0);
    maskByLevel[level].push_back(mask);
    globalIdByLevel[level].emplace_back(idg);
    if (!this->IsLeaf(index) && !mask)
    {
      descByLevel[level].push_back(true);
      for (int iChild = 0; iChild < this->NumberOfChildren; ++iChild)
      {
        RecursiveGetByLevelForWriter(inIsMasked, level + 1,
          this->GetElderChildIndex(index) + iChild, descByLevel, maskByLevel, globalIdByLevel);
      }
    }
    else
    {
      descByLevel[level].push_back(false);
    }
  }

  //---------------------------------------------------------------------------
  void GetByLevelForWriter(vtkBitArray* inIsMasked, vtkUnsignedLongArray* nbVerticesbyLevel,
    vtkBitArray* isParent, vtkBitArray* isMasked, vtkIdList* ids) override
  {
    int maxLevels = this->GetNumberOfLevels();
    std::vector<std::vector<bool> > descByLevel(maxLevels);
    std::vector<std::vector<bool> > maskByLevel(maxLevels);
    std::vector<std::vector<uint64_t> > globalIdByLevel(maxLevels);
    // Build information by levels
    RecursiveGetByLevelForWriter(inIsMasked, 0, 0, descByLevel, maskByLevel, globalIdByLevel);
    // nbVerticesbyLevel
    vtkIdType nb = 0;
    nbVerticesbyLevel->Resize(0);
    assert(globalIdByLevel.size() == static_cast<std::size_t>(maxLevels));
    for (int iLevel = 0; iLevel < maxLevels; ++iLevel)
    {
      nb += static_cast<vtkIdType>(globalIdByLevel[iLevel].size());
      nbVerticesbyLevel->InsertNextValue(static_cast<vtkIdType>(globalIdByLevel[iLevel].size()));
    }
    nbVerticesbyLevel->Squeeze();
    // Ids
    ids->SetNumberOfIds(nb);
    std::size_t i = 0;
    for (std::size_t iLevel = 0; iLevel < globalIdByLevel.size(); ++iLevel)
    {
      for (auto idg : globalIdByLevel[iLevel])
      {
        ids->SetId(static_cast<vtkIdType>(i), idg);
        ++i;
      }
      globalIdByLevel[iLevel].clear();
    }
    assert(static_cast<vtkIdType>(i) == nb);
    globalIdByLevel.clear();

    // isParent compressed
    {
      // Find last level with cells
      int reduceLevel = maxLevels - 1;
      for (; descByLevel[reduceLevel].size() == 0; --reduceLevel)
        ;
      // By definition, all values is false
      for (auto it = descByLevel[reduceLevel].begin(); it != descByLevel[reduceLevel].end(); ++it)
      {
        assert(!(*it));
      }
      // Move before last level with cells
      --reduceLevel;
      // We're looking for the latest true value
      if (reduceLevel > 0)
      {
        std::vector<bool>& desc = descByLevel[reduceLevel];
        for (std::vector<bool>::reverse_iterator it = desc.rbegin(); it != desc.rend(); ++it)
        {
          if (*it)
          {
            // Resize to ignore the latest false values
            // There is by definition at least one value true
            desc.resize(std::distance(it, desc.rend()));
            break;
          }
        }
      }

      isParent->Resize(0);
      for (int iLevel = 0; iLevel <= reduceLevel; ++iLevel)
      {
        for (auto state : descByLevel[iLevel])
        {
          isParent->InsertNextValue(state);
        }
      }
      isParent->Squeeze();
    }

    // isMasked compressed
    if (inIsMasked)
    {
      int reduceLevel = maxLevels - 1;
      bool isFinding = false;
      for (; reduceLevel > 0; --reduceLevel)
      {
        std::vector<bool>& mask = maskByLevel[reduceLevel];
        for (std::vector<bool>::reverse_iterator it = mask.rbegin(); it != mask.rend(); ++it)
        {
          if (*it)
          {
            // Resize to ignore the latest false values
            // There is by definition at least one value true
            mask.resize(std::distance(it, mask.rend()));
            isFinding = true;
            break;
          }
        }
        if (isFinding)
        {
          break;
        }
      }
      isMasked->Resize(0);
      for (int iLevel = 0; iLevel <= reduceLevel; ++iLevel)
      {
        for (auto etat : maskByLevel[iLevel])
        {
          isMasked->InsertNextValue(etat);
        }
      }
    }
    isMasked->Squeeze();
  }

  //---------------------------------------------------------------------------
  void InitializeForReader(vtkIdType numberOfLevels, vtkIdType nbVertices,
    vtkIdType nbVerticesOfLastLevel, vtkBitArray* isParent, vtkBitArray* isMasked,
    vtkBitArray* outIsMasked) override
  {
    if (isParent == nullptr)
    {
      this->CompactDatas->ParentToElderChild_stl.resize(1);
      this->CompactDatas->ParentToElderChild_stl[0] = UINT_MAX;
      if (isMasked)
      {
        vtkIdType nbIsMasked = isMasked->GetNumberOfTuples();
        if (nbIsMasked)
        {
          assert(isMasked->GetNumberOfComponents() == 1);
          outIsMasked->InsertValue(this->GetGlobalIndexFromLocal(0), isMasked->GetValue(0));
        }
      }
      return;
    }

    vtkIdType nbIsParent = isParent->GetNumberOfTuples();
    assert(isParent->GetNumberOfComponents() == 1);

    vtkIdType firstOffsetLastLevel = nbVertices - nbVerticesOfLastLevel;
    if (nbIsParent < firstOffsetLastLevel)
    {
      firstOffsetLastLevel = nbIsParent;
    }
    this->CompactDatas->ParentToElderChild_stl.resize(firstOffsetLastLevel);

    vtkIdType nbCoarses = 0;
    if (isParent->GetValue(0))
    {
      vtkIdType off = 1;
      this->CompactDatas->ParentToElderChild_stl[0] = off;
      for (vtkIdType i = 1; i < firstOffsetLastLevel; ++i)
      {
        if (isParent->GetValue(i))
        {
          off += this->NumberOfChildren;
          this->CompactDatas->ParentToElderChild_stl[i] = off;
          ++nbCoarses;
        }
        else
        {
          this->CompactDatas->ParentToElderChild_stl[i] = UINT_MAX;
        }
      }
    }
    else
    {
      this->CompactDatas->ParentToElderChild_stl[0] = UINT_MAX;
    }

    vtkIdType nbIsMasked = isMasked->GetNumberOfTuples();
    assert(isMasked->GetNumberOfComponents() == 1);

    vtkIdType i = 0;
    for (; i < nbIsMasked && i < nbVertices; ++i)
    {
      outIsMasked->InsertValue(this->GetGlobalIndexFromLocal(i), isMasked->GetValue(i));
    }
    // By convention, the final values not explicitly described
    // by the isMasked parameter are False.
    for (; i < nbVertices; ++i)
    {
      outIsMasked->InsertValue(this->GetGlobalIndexFromLocal(i), false);
    }

    this->Datas->NumberOfLevels = numberOfLevels;
    this->Datas->NumberOfNodes += nbCoarses;
    this->Datas->NumberOfVertices = nbVertices;
  }

  //---------------------------------------------------------------------------
  vtkHyperTree* Freeze(const char* vtkNotUsed(mode)) override
  {
    // Option not used
    return this;
  }

  //---------------------------------------------------------------------------
  ~vtkCompactHyperTree() override {}

  //---------------------------------------------------------------------------
  bool IsGlobalIndexImplicit() override { return this->Datas->GlobalIndexStart == -1; }

  //---------------------------------------------------------------------------
  void SetGlobalIndexStart(vtkIdType start) override
  {
    assert("pre: not_global_index_start_if_use_global_index_from_local" &&
      this->CompactDatas->GlobalIndexTable_stl.size() == 0);

    this->Datas->GlobalIndexStart = start;
  }

  //---------------------------------------------------------------------------
  void SetGlobalIndexFromLocal(vtkIdType index, vtkIdType global) override
  {
    assert("pre: not_global_index_from_local_if_use_global_index_start" &&
      this->Datas->GlobalIndexStart < 0);

    // If local index outside map range, resize the latter
    if (static_cast<vtkIdType>(this->CompactDatas->GlobalIndexTable_stl.size()) <= index)
    {
      this->CompactDatas->GlobalIndexTable_stl.resize(index + 1, -1);
    }
    // This service allows the value of the global index to be positioned
    // several times in order to take into account a first description,
    // a priori, incomplete followed by a more detailed description.
    // The last call overwrites, replaces what was written previously.
    this->CompactDatas->GlobalIndexTable_stl[index] = global;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalIndexFromLocal(vtkIdType index) const override
  {
    if (this->CompactDatas->GlobalIndexTable_stl.size() != 0)
    {
      // Case explicit global node index
      assert("pre: not_valid_index" && index >= 0 &&
        index < (vtkIdType)this->CompactDatas->GlobalIndexTable_stl.size());
      assert(
        "pre: not_positive_global_index" && this->CompactDatas->GlobalIndexTable_stl[index] >= 0);
      return this->CompactDatas->GlobalIndexTable_stl[index];
    }
    // Case implicit global node index
    assert("pre: not_positive_start_index" && this->Datas->GlobalIndexStart >= 0);
    assert("pre: not_valid_index" && index >= 0);
    return this->Datas->GlobalIndexStart + index;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalNodeIndexMax() const override
  {
    if (static_cast<vtkIdType>(this->CompactDatas->GlobalIndexTable_stl.size() != 0))
    {
      // Case explicit global node index
      const auto it_end = this->CompactDatas->GlobalIndexTable_stl.end();
      const auto elt_found =
        std::max_element(this->CompactDatas->GlobalIndexTable_stl.begin(), it_end);
      assert("pre: not_positive_global_index" &&
        (*std::max_element(this->CompactDatas->GlobalIndexTable_stl.begin(), it_end)) >= 0);
      return *elt_found;
    }
    // Case implicit global node index
    assert("pre: not_positive_start_index" && this->Datas->GlobalIndexStart >= 0);
    return this->Datas->GlobalIndexStart + this->Datas->NumberOfVertices - 1;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for entry: vtkHyperTreeGridEntry, vtkHyperTreeGridGeometryEntry,
  // vtkHyperTreeGridGeometryLevelEntry
  vtkIdType GetElderChildIndex(unsigned int index_parent) const override
  {
    assert("pre: valid_range" &&
      index_parent < static_cast<unsigned int>(this->Datas->NumberOfVertices));
    return this->CompactDatas->ParentToElderChild_stl[index_parent];
  }

  //---------------------------------------------------------------------------
  void SubdivideLeaf(vtkIdType index, unsigned int level) override
  {
    assert("pre: not_valid_index" && index < static_cast<vtkIdType>(this->Datas->NumberOfVertices));
    assert("pre: not_leaf" && this->IsLeaf(index));
    // The leaf becomes a node and is not anymore a leaf
    // Nodes get constructed with leaf flags set to 1.
    if (static_cast<vtkIdType>(this->CompactDatas->ParentToElderChild_stl.size()) <= index)
    {
      this->CompactDatas->ParentToElderChild_stl.resize(index + 1, UINT_MAX);
    }
    // The first new child
    unsigned int nextLeaf = static_cast<unsigned int>(this->Datas->NumberOfVertices);
    this->CompactDatas->ParentToElderChild_stl[index] = nextLeaf;
    // Add the new leaves to the number of leaves at the next level.
    if (level + 1 == this->Datas->NumberOfLevels) // >=
    {
      // We have a new level.
      ++this->Datas->NumberOfLevels;
    }
    // Update the number of non-leaf and all vertices
    this->Datas->NumberOfNodes += 1;
    this->Datas->NumberOfVertices += this->NumberOfChildren;
  }

  //---------------------------------------------------------------------------
  unsigned long GetActualMemorySizeBytes() override
  {
    // in bytes
    return static_cast<unsigned long>(
      sizeof(unsigned int) * this->CompactDatas->ParentToElderChild_stl.size() +
      sizeof(vtkIdType) * this->CompactDatas->GlobalIndexTable_stl.size() +
      3 * sizeof(unsigned char) + 6 * sizeof(vtkIdType));
  }

  //---------------------------------------------------------------------------
  bool IsTerminalNode(vtkIdType index) const override
  {
    assert("pre: valid_range" && index >= 0 && index < this->Datas->NumberOfVertices);
    if (static_cast<unsigned long>(index) >= this->CompactDatas->ParentToElderChild_stl.size())
    {
      return 0;
    }

    for (unsigned int ichild = 0; ichild < this->NumberOfChildren; ++ichild)
    {
      if (!this->IsChildLeaf(index, ichild))
      {
        return 0;
      }
    }
    return 1;
  }

  //---------------------------------------------------------------------------
  bool IsLeaf(vtkIdType index) const override
  {
    assert("pre: valid_range" && index >= 0 && index < this->Datas->NumberOfVertices);
    return static_cast<unsigned long>(index) >= this->CompactDatas->ParentToElderChild_stl.size() ||
      this->CompactDatas->ParentToElderChild_stl[index] == UINT_MAX ||
      this->Datas->NumberOfVertices == 1;
  }

  //---------------------------------------------------------------------------
  bool IsChildLeaf(vtkIdType index_parent, unsigned int ichild) const
  {
    assert("pre: valid_range" && index_parent >= 0 && index_parent < this->Datas->NumberOfVertices);
    if (static_cast<unsigned long>(index_parent) >=
      this->CompactDatas->ParentToElderChild_stl.size())
    {
      return 0;
    }
    assert("pre: valid_range" && ichild < this->NumberOfChildren);
    vtkIdType index_child = this->CompactDatas->ParentToElderChild_stl[index_parent] + ichild;
    return static_cast<unsigned long>(index_child) >=
      this->CompactDatas->ParentToElderChild_stl.size() ||
      this->CompactDatas->ParentToElderChild_stl[index_child] == UINT_MAX;
  }

  //---------------------------------------------------------------------------
  const std::vector<unsigned int>& GetParentElderChild() const
  {
    return this->CompactDatas->ParentToElderChild_stl;
  }

  //---------------------------------------------------------------------------
  const std::vector<vtkIdType>& GetGlobalIndexTable() const
  {
    return this->CompactDatas->GlobalIndexTable_stl;
  }

protected:
  //---------------------------------------------------------------------------
  vtkCompactHyperTree() { this->CompactDatas = std::make_shared<vtkCompactHyperTreeData>(); }

  //---------------------------------------------------------------------------
  void InitializePrivate() override
  {
    // Set default tree structure with a single node at the root
    this->CompactDatas->ParentToElderChild_stl.resize(1);
    this->CompactDatas->ParentToElderChild_stl[0] = 0;
    // By default, the root don't have parent
    this->CompactDatas->GlobalIndexTable_stl.clear();
  }

  //---------------------------------------------------------------------------
  void PrintSelfPrivate(ostream& os, vtkIndent indent) override
  {
    os << indent << "ParentToElderChild: " << this->CompactDatas->ParentToElderChild_stl.size()
       << endl;
    for (unsigned int i = 0; i < this->CompactDatas->ParentToElderChild_stl.size(); ++i)
    {
      os << this->CompactDatas->ParentToElderChild_stl[i] << " ";
    }
    os << endl;

    os << indent << "GlobalIndexTable: ";
    for (unsigned int i = 0; i < this->CompactDatas->GlobalIndexTable_stl.size(); ++i)
    {
      os << " " << this->CompactDatas->GlobalIndexTable_stl[i];
    }
    os << endl;
  }

  //---------------------------------------------------------------------------
  void CopyStructurePrivate(vtkHyperTree* ht) override
  {
    assert("pre: ht_exists" && ht != nullptr);
    vtkCompactHyperTree* htp = vtkCompactHyperTree::SafeDownCast(ht);
    assert("pre: same_type" && htp != nullptr);
    this->CompactDatas = htp->CompactDatas;
  }

  //---------------------------------------------------------------------------
  std::shared_ptr<vtkCompactHyperTreeData> CompactDatas;

private:
  vtkCompactHyperTree(const vtkCompactHyperTree&) = delete;
  void operator=(const vtkCompactHyperTree&) = delete;
};
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompactHyperTree);
//=============================================================================

vtkHyperTree* vtkHyperTree::CreateInstance(unsigned char factor, unsigned char dimension)
{
  if (factor < 2 || 3 < factor)
  {
    vtkGenericWarningMacro("Bad branching factor " << factor);
    return nullptr;
  }
  if (dimension < 1 || 3 < dimension)
  {
    vtkGenericWarningMacro("Bad dimension " << (int)dimension);
    return nullptr;
  }
  vtkHyperTree* ht = vtkCompactHyperTree::New();
  ht->Initialize(factor, dimension, pow(factor, dimension));
  return ht;
}
