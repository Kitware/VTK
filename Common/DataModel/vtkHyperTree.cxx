// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTree.h"
#include "vtkBitArray.h"
#include "vtkHyperTreeGridScales.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkTypeInt64Array.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkHyperTree::vtkHyperTree()
{
  this->InitializeBase(2, 3, 8);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkHyperTree::InitializeBase(
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
}

//------------------------------------------------------------------------------
void vtkHyperTree::Initialize(
  unsigned char branchFactor, unsigned char dimension, unsigned char numberOfChildren)
{
  this->InitializeBase(branchFactor, dimension, numberOfChildren);
  this->InitializePrivate();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
std::shared_ptr<vtkHyperTreeGridScales> vtkHyperTree::InitializeScales(
  const double* scales, bool reinitialize) const
{
  if (this->Scales == nullptr || reinitialize)
  {
    this->Scales = std::make_shared<vtkHyperTreeGridScales>(this->BranchFactor, scales);
  }
  return this->Scales;
}

//------------------------------------------------------------------------------
void vtkHyperTree::GetScale(double s[3]) const
{
  assert("pre: scales_exists" && this->Scales != nullptr);
  const double* scale = this->Scales->GetScale(0);
  memcpy(s, scale, 3 * sizeof(double));
}

//------------------------------------------------------------------------------
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
  void ComputeBreadthFirstOrderDescriptor(vtkBitArray* inputMask,
    vtkTypeInt64Array* numberOfVerticesPerDepth, vtkBitArray* descriptor,
    vtkIdList* breadthFirstIdMap) override
  {
    int maxDepth = this->GetNumberOfLevels();
    std::vector<std::vector<bool>> descriptorPerDepth(maxDepth);
    std::vector<std::vector<vtkIdType>> breadthFirstOrderIdMapPerDepth(maxDepth);

    this->ComputeBreadthFirstOrderDescriptorImpl(
      inputMask, 0, 0, descriptorPerDepth, breadthFirstOrderIdMapPerDepth);

    // Reducing maxDepth to squeeze out depths in which all subtrees are
    // entirely masked.
    while (maxDepth && breadthFirstOrderIdMapPerDepth[--maxDepth].empty())
      ;

    ++maxDepth;

    for (int idepth = 0; idepth < maxDepth; ++idepth)
    {
      numberOfVerticesPerDepth->InsertNextValue(
        static_cast<vtkTypeInt64>(breadthFirstOrderIdMapPerDepth[idepth].size()));
      for (auto idg : breadthFirstOrderIdMapPerDepth[idepth])
      {
        breadthFirstIdMap->InsertNextId(idg);
      }
    }

    // We ignore last depth for the descriptor, as we already know that no
    // vertices have children.
    // However, we are careful not treating trees with only one depth. There
    // is no need to describe such trivial trees.
    for (int idepth = 0; idepth < maxDepth - 1; ++idepth)
    {
      for (auto state : descriptorPerDepth[idepth])
      {
        descriptor->InsertNextValue(state);
      }
    }
  }

  //---------------------------------------------------------------------------
  void BuildFromBreadthFirstOrderDescriptor(
    vtkBitArray* descriptor, vtkIdType numberOfBits, vtkIdType startIndex) override
  {
    this->CompactDatas->ParentToElderChild_stl.clear();
    int numberOfDepths = 1;
    vtkIdType numberOfCoarseVertices = 0;
    vtkIdType numberOfVertices = 1;
    if (!numberOfBits)
    {
      this->CompactDatas->ParentToElderChild_stl.emplace_back(
        std::numeric_limits<unsigned int>::max());
    }
    else
    {
      vtkIdType currentDepthSize = 1;
      vtkIdType nextDepthSize = 0;
      vtkIdType currentPositionAtDepth = 0;
      for (vtkIdType id = startIndex; id < startIndex + numberOfBits; ++id)
      {
        if (descriptor->GetValue(id))
        {
          this->CompactDatas->ParentToElderChild_stl.emplace_back(numberOfVertices);
          numberOfVertices += this->NumberOfChildren;
          ++numberOfCoarseVertices;
          nextDepthSize += this->NumberOfChildren;
        }
        else
        {
          this->CompactDatas->ParentToElderChild_stl.emplace_back(
            std::numeric_limits<unsigned int>::max());
        }
        if (++currentPositionAtDepth == currentDepthSize)
        {
          ++numberOfDepths;
          currentDepthSize = nextDepthSize;
          nextDepthSize = 0;
          currentPositionAtDepth = 0;
        }
      }
    }
    this->Datas->NumberOfLevels = numberOfDepths;
    this->Datas->NumberOfNodes = numberOfCoarseVertices;
    this->Datas->NumberOfVertices = numberOfVertices;
  }

  //---------------------------------------------------------------------------
  void InitializeForReader(vtkIdType numberOfLevels, vtkIdType nbVertices,
    vtkIdType nbVerticesOfLastdepth, vtkBitArray* isParent, vtkBitArray* isMasked,
    vtkBitArray* outIsMasked) override
  {
    if (isParent == nullptr)
    {
      this->CompactDatas->ParentToElderChild_stl.resize(1);
      this->CompactDatas->ParentToElderChild_stl[0] = std::numeric_limits<unsigned int>::max();
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

    vtkIdType firstOffsetLastdepth = nbVertices - nbVerticesOfLastdepth;
    if (nbIsParent < firstOffsetLastdepth)
    {
      firstOffsetLastdepth = nbIsParent;
    }
    this->CompactDatas->ParentToElderChild_stl.resize(firstOffsetLastdepth);

    vtkIdType nbCoarses = isParent->GetValue(0);
    if (nbCoarses)
    {
      vtkIdType off = 1;
      this->CompactDatas->ParentToElderChild_stl.resize(
        std::max(static_cast<vtkIdType>(1), firstOffsetLastdepth));
      this->CompactDatas->ParentToElderChild_stl[0] = off;
      for (vtkIdType i = 1; i < firstOffsetLastdepth; ++i)
      {
        if (isParent->GetValue(i))
        {
          off += this->NumberOfChildren;
          this->CompactDatas->ParentToElderChild_stl[i] = off;
          ++nbCoarses;
        }
        else
        {
          this->CompactDatas->ParentToElderChild_stl[i] = std::numeric_limits<unsigned int>::max();
        }
      }
    }
    else
    {
      this->CompactDatas->ParentToElderChild_stl.resize(1);
      this->CompactDatas->ParentToElderChild_stl[0] = std::numeric_limits<unsigned int>::max();
    }

    if (isMasked)
    {
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
    }

    this->Datas->NumberOfLevels = numberOfLevels;
    this->Datas->NumberOfNodes = nbCoarses;
    this->Datas->NumberOfVertices = nbVertices;
  }

  //---------------------------------------------------------------------------
  vtkHyperTree* Freeze(const char* vtkNotUsed(mode)) override
  {
    // Option not used
    return this;
  }

  //---------------------------------------------------------------------------
  ~vtkCompactHyperTree() override = default;

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
    if (!this->CompactDatas->GlobalIndexTable_stl.empty())
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
    if (static_cast<vtkIdType>(!this->CompactDatas->GlobalIndexTable_stl.empty()))
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
  // vtkHyperTreeGridGeometryDepthEntry
  vtkIdType GetElderChildIndex(unsigned int index_parent) const override
  {
    assert("pre: valid_range" &&
      index_parent < static_cast<unsigned int>(this->Datas->NumberOfVertices));
    return this->CompactDatas->ParentToElderChild_stl[index_parent];
  }

  //---------------------------------------------------------------------------
  // Description:
  // Access to the internals of the tree. Should be used for consulting,
  // not modification.
  const unsigned int* GetElderChildIndexArray(size_t& nbElements) const override
  {
    nbElements = this->CompactDatas->ParentToElderChild_stl.size();
    return this->CompactDatas->ParentToElderChild_stl.data();
  }

  //---------------------------------------------------------------------------
  void SubdivideLeaf(vtkIdType index, unsigned int depth) override
  {
    assert("pre: not_valid_index" && index < static_cast<vtkIdType>(this->Datas->NumberOfVertices));
    assert("pre: not_leaf" && this->IsLeaf(index));
    // The leaf becomes a node and is not anymore a leaf
    // Nodes get constructed with leaf flags set to 1.
    if (static_cast<vtkIdType>(this->CompactDatas->ParentToElderChild_stl.size()) <= index)
    {
      this->CompactDatas->ParentToElderChild_stl.resize(
        index + 1, std::numeric_limits<unsigned int>::max());
    }
    // The first new child

    this->CompactDatas->ParentToElderChild_stl[index] =
      static_cast<unsigned int>(this->Datas->NumberOfVertices);
    // Add the new leaves to the number of leaves at the next depth.
    if (depth + 1 == this->Datas->NumberOfLevels) // >=
    {
      // We have a new depth.
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
      return false;
    }

    for (unsigned int ichild = 0; ichild < this->NumberOfChildren; ++ichild)
    {
      if (!this->IsChildLeaf(index, ichild))
      {
        return false;
      }
    }
    return true;
  }

  //---------------------------------------------------------------------------
  bool IsLeaf(vtkIdType index) const override
  {
    assert("pre: valid_range" && index >= 0 && index < this->Datas->NumberOfVertices);
    return static_cast<unsigned long>(index) >= this->CompactDatas->ParentToElderChild_stl.size() ||
      this->CompactDatas->ParentToElderChild_stl[index] ==
      std::numeric_limits<unsigned int>::max() ||
      this->Datas->NumberOfVertices == 1;
  }

  //---------------------------------------------------------------------------
  bool IsChildLeaf(vtkIdType index_parent, unsigned int ichild) const
  {
    assert("pre: valid_range" && index_parent >= 0 && index_parent < this->Datas->NumberOfVertices);
    if (static_cast<unsigned long>(index_parent) >=
      this->CompactDatas->ParentToElderChild_stl.size())
    {
      return false;
    }
    assert("pre: valid_range" && ichild < this->NumberOfChildren);
    vtkIdType index_child = this->CompactDatas->ParentToElderChild_stl[index_parent] + ichild;
    return static_cast<unsigned long>(index_child) >=
      this->CompactDatas->ParentToElderChild_stl.size() ||
      this->CompactDatas->ParentToElderChild_stl[index_child] ==
      std::numeric_limits<unsigned int>::max();
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

  /**
   * Recursive implementation used by ComputeBreadthFirstOrderDescriptor to
   * compute per depth tree descriptor (`descriptorPerDepth`), its id mapping
   * (`breadthFirstOrderIdMapPerDepth`) with the current tree.
   *
   * The descriptor is a binary array associated to each depth such that leaf vertices
   * are mapped to zero, while non leaf vertices are mapped to one.
   */
  void ComputeBreadthFirstOrderDescriptorImpl(vtkBitArray* inputMask, int depth, vtkIdType index,
    std::vector<std::vector<bool>>& descriptorPerDepth,
    std::vector<std::vector<vtkIdType>>& breadthFirstOrderIdMapPerDepth)
  {
    vtkIdType idg = this->GetGlobalIndexFromLocal(index);
    bool mask = inputMask ? inputMask->GetValue(idg) : false;
    breadthFirstOrderIdMapPerDepth[depth].emplace_back(idg);
    if (!this->IsLeaf(index) && !mask)
    {
      descriptorPerDepth[depth].push_back(true);
      for (int iChild = 0; iChild < this->NumberOfChildren; ++iChild)
      {
        this->ComputeBreadthFirstOrderDescriptorImpl(inputMask, depth + 1,
          this->GetElderChildIndex(index) + iChild, descriptorPerDepth,
          breadthFirstOrderIdMapPerDepth);
      }
    }
    else
    {
      descriptorPerDepth[depth].push_back(false);
    }
  }

  //---------------------------------------------------------------------------
  std::shared_ptr<vtkCompactHyperTreeData> CompactDatas;

private:
  vtkCompactHyperTree(const vtkCompactHyperTree&) = delete;
  void operator=(const vtkCompactHyperTree&) = delete;
};

//------------------------------------------------------------------------------
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
VTK_ABI_NAMESPACE_END
