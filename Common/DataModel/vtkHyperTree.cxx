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
#include <limits>
#include <memory>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkHyperTree);

vtkHyperTree::vtkHyperTree()
{
  this->Initialize(2, 3);
}

//------------------------------------------------------------------------------
void vtkHyperTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimension: " << static_cast<int>(this->Dimension) << "\n";
  os << indent << "BranchFactor: " << static_cast<int>(this->BranchFactor) << "\n";
  os << indent << "NumberOfChildren: " << static_cast<vtkIdType>(this->NumberOfChildren) << "\n";

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

  os << indent << "ParentToElderChild: " << this->Datas->ParentToElderChild.size() << endl;
  for (unsigned int i = 0; i < this->Datas->ParentToElderChild.size(); ++i)
  {
    os << this->Datas->ParentToElderChild[i] << " ";
  }
  os << endl;

  os << indent << "GlobalIndexTable: ";
  for (unsigned int i = 0; i < this->Datas->GlobalIndexTable.size(); ++i)
  {
    os << " " << this->Datas->GlobalIndexTable[i];
  }
  os << endl;
}

//------------------------------------------------------------------------------
void vtkHyperTree::Initialize(
  unsigned char branchFactor, unsigned char dimension, unsigned char vtkNotUsed(numberOfChildren))
{
  this->Initialize(branchFactor, dimension);
}

//------------------------------------------------------------------------------
bool vtkHyperTree::Initialize(const unsigned char branchFactor, const unsigned char dimension)
{
  if (branchFactor < 2 || 3 < branchFactor)
  {
    vtkGenericWarningMacro("Bad branching factor " << branchFactor);
    return false;
  }
  if (dimension < 1 || 3 < dimension)
  {
    vtkGenericWarningMacro("Bad dimension " << static_cast<int>(dimension));
    return false;
  }

  this->BranchFactor = branchFactor;
  this->Dimension = dimension;
  this->NumberOfChildren = pow(branchFactor, dimension);

  this->Datas = std::make_shared<vtkHyperTreeData>();
  this->Datas->TreeIndex = -1;
  this->Datas->NumberOfLevels = 1;
  this->Datas->NumberOfVertices = 1;
  this->Datas->NumberOfNodes = 0;
  // By default, nothing is used
  // No GlobalIndexStart, no GlobalIndexFromLocal
  this->Datas->GlobalIndexStart = -1;

  this->Datas->ParentToElderChild.resize(1);
  this->Datas->ParentToElderChild[0] = std::numeric_limits<unsigned int>::max();
  // By default, the root doesn't have a parent
  this->Datas->GlobalIndexTable.clear();

  this->Scales = nullptr;

  return true;
}

//------------------------------------------------------------------------------
void vtkHyperTree::CopyStructure(vtkHyperTree* ht)
{
  assert("pre: ht_exists" && ht != nullptr);

  // Copy ht data
  this->Datas = std::make_shared<vtkHyperTreeData>(*ht->Datas);
  this->SetScales(std::make_shared<vtkHyperTreeGridScales>(
    ht->Scales->GetBranchFactor(), ht->Scales->ComputeScale(0)));
  this->BranchFactor = ht->BranchFactor;
  this->Dimension = ht->Dimension;
  this->NumberOfChildren = ht->NumberOfChildren;
}

//------------------------------------------------------------------------------
std::shared_ptr<vtkHyperTreeGridScales> vtkHyperTree::InitializeScales(
  const double* scales, bool reinitialize)
{
  if (this->Scales == nullptr || reinitialize)
  {
    this->SetScales(std::make_shared<vtkHyperTreeGridScales>(this->BranchFactor, scales));
  }
  return this->Scales;
}

//------------------------------------------------------------------------------
void vtkHyperTree::GetScale(double s[3]) const
{
  assert("pre: scales_exists" && this->Scales != nullptr);
  const double* scale = this->Scales->ComputeScale(0);
  memcpy(s, scale, 3 * sizeof(double));
}

//------------------------------------------------------------------------------
double vtkHyperTree::GetScale(unsigned int d) const
{
  assert("pre: scales_exists" && this->Scales != nullptr);
  const double* scale = this->Scales->ComputeScale(0);
  return scale[d];
}

//---------------------------------------------------------------------------
void vtkHyperTree::ComputeBreadthFirstOrderDescriptor(const unsigned int depthLimiter,
  vtkBitArray* inputMask, vtkTypeInt64Array* numberOfVerticesPerDepth, vtkBitArray* descriptor,
  vtkIdList* breadthFirstIdMap)
{
  int maxDepth = this->GetNumberOfLevels();

  std::vector<std::vector<bool>> descriptorPerDepth(maxDepth);
  std::vector<std::vector<vtkIdType>> breadthFirstOrderIdMapPerDepth(maxDepth);

  this->ComputeBreadthFirstOrderDescriptorImpl(
    depthLimiter, inputMask, 0, 0, descriptorPerDepth, breadthFirstOrderIdMapPerDepth);

  // Reducing maxDepth to squeeze out depths in which all subtrees are
  // entirely masked.
  while (maxDepth && breadthFirstOrderIdMapPerDepth[--maxDepth].empty())
    ;
  ++maxDepth;

  for (int idepth = 0; idepth < maxDepth; ++idepth)
  {
    numberOfVerticesPerDepth->InsertNextValue(
      static_cast<vtkTypeInt64>(breadthFirstOrderIdMapPerDepth[idepth].size()));
    for (const vtkIdType& idg : breadthFirstOrderIdMapPerDepth[idepth])
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
    for (const bool state : descriptorPerDepth[idepth])
    {
      descriptor->InsertNextValue(state);
    }
  }
}

//---------------------------------------------------------------------------
void vtkHyperTree::BuildFromBreadthFirstOrderDescriptor(
  vtkBitArray* descriptor, vtkIdType numberOfBits, vtkIdType startIndex)
{
  this->Datas->ParentToElderChild.clear();
  int numberOfDepths = 1;
  vtkIdType numberOfCoarseVertices = 0;
  vtkIdType numberOfVertices = 1;
  if (!numberOfBits)
  {
    this->Datas->ParentToElderChild.emplace_back(std::numeric_limits<unsigned int>::max());
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
        this->Datas->ParentToElderChild.emplace_back(numberOfVertices);
        numberOfVertices += this->NumberOfChildren;
        ++numberOfCoarseVertices;
        nextDepthSize += this->NumberOfChildren;
      }
      else
      {
        this->Datas->ParentToElderChild.emplace_back(std::numeric_limits<unsigned int>::max());
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
void vtkHyperTree::InitializeForReader(vtkIdType numberOfLevels, vtkIdType nbVertices,
  vtkIdType nbVerticesOfLastdepth, vtkBitArray* isParent, vtkBitArray* isMasked,
  vtkBitArray* outIsMasked)
{
  if (isParent == nullptr)
  {
    this->Datas->ParentToElderChild.resize(1);
    this->Datas->ParentToElderChild[0] = std::numeric_limits<unsigned int>::max();
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
  this->Datas->ParentToElderChild.resize(firstOffsetLastdepth);

  vtkIdType nbCoarses = isParent->GetValue(0);
  if (nbCoarses)
  {
    vtkIdType off = 1;
    this->Datas->ParentToElderChild.resize(
      std::max(static_cast<vtkIdType>(1), firstOffsetLastdepth));
    this->Datas->ParentToElderChild[0] = off;
    for (vtkIdType i = 1; i < firstOffsetLastdepth; ++i)
    {
      if (isParent->GetValue(i))
      {
        off += this->NumberOfChildren;
        this->Datas->ParentToElderChild[i] = off;
        ++nbCoarses;
      }
      else
      {
        this->Datas->ParentToElderChild[i] = std::numeric_limits<unsigned int>::max();
      }
    }
  }
  else
  {
    this->Datas->ParentToElderChild.resize(1);
    this->Datas->ParentToElderChild[0] = std::numeric_limits<unsigned int>::max();
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
bool vtkHyperTree::IsGlobalIndexImplicit()
{
  return this->Datas->GlobalIndexStart == -1;
}

//---------------------------------------------------------------------------
void vtkHyperTree::SetGlobalIndexStart(vtkIdType start)
{
  this->Datas->GlobalIndexStart = start;
}

//---------------------------------------------------------------------------
void vtkHyperTree::SetGlobalIndexFromLocal(vtkIdType index, vtkIdType global)
{
  assert("pre: not_globalindex_from_local_if_use_globalindex_start" &&
    this->Datas->GlobalIndexStart < 0);

  // If local index outside map range, resize the latter
  if (static_cast<vtkIdType>(this->Datas->GlobalIndexTable.size()) <= index)
  {
    this->Datas->GlobalIndexTable.resize(index + 1, -1);
  }
  // This service allows the value of the global index to be positioned
  // several times in order to take into account a first description,
  // a priori, incomplete followed by a more detailed description.
  // The last call overwrites, replaces what was written previously.
  this->Datas->GlobalIndexTable[index] = global;
}

//---------------------------------------------------------------------------
vtkIdType vtkHyperTree::GetGlobalIndexFromLocal(vtkIdType index) const
{
  if (!this->Datas->GlobalIndexTable.empty())
  {
    // Case explicit global node index
    assert("pre: not_validindex" && index >= 0 &&
      index < (vtkIdType)this->Datas->GlobalIndexTable.size());
    assert("pre: not_positive_globalindex" && this->Datas->GlobalIndexTable[index] >= 0);
    return this->Datas->GlobalIndexTable[index];
  }
  // Case implicit global node index
  assert("pre: not_positive_startindex" && this->Datas->GlobalIndexStart >= 0);
  assert("pre: not_validindex" && index >= 0);
  return this->Datas->GlobalIndexStart + index;
}

//---------------------------------------------------------------------------
vtkIdType vtkHyperTree::GetGlobalNodeIndexMax() const
{
  if (static_cast<vtkIdType>(!this->Datas->GlobalIndexTable.empty()))
  {
    // Case explicit global node index
    const std::vector<vtkIdType>::iterator it_end = this->Datas->GlobalIndexTable.end();
    const std::vector<vtkIdType>::iterator elt_found =
      std::max_element(this->Datas->GlobalIndexTable.begin(), it_end);
    assert("pre: not_positive_globalindex" &&
      (*std::max_element(this->Datas->GlobalIndexTable.begin(), it_end)) >= 0);
    return *elt_found;
  }
  // Case implicit global node index
  assert("pre: not_positive_startindex" && this->Datas->GlobalIndexStart >= 0);
  return this->Datas->GlobalIndexStart + this->Datas->NumberOfVertices - 1;
}

//---------------------------------------------------------------------------
// Description:
// Public only for entry: vtkHyperTreeGridEntry, vtkHyperTreeGridGeometryEntry,
// vtkHyperTreeGridGeometryDepthEntry
vtkIdType vtkHyperTree::GetElderChildIndex(unsigned int index_parent) const
{
  assert(
    "pre: valid_range" && index_parent < static_cast<unsigned int>(this->Datas->NumberOfVertices));
  return this->Datas->ParentToElderChild[index_parent];
}

//---------------------------------------------------------------------------
// Description:
// Access to the internals of the tree. Should be used for consulting,
// not modification.
const unsigned int* vtkHyperTree::GetElderChildIndexArray(size_t& nbElements) const
{
  nbElements = this->Datas->ParentToElderChild.size();
  return this->Datas->ParentToElderChild.data();
}

//---------------------------------------------------------------------------
void vtkHyperTree::SubdivideLeaf(vtkIdType index, unsigned int depth)
{
  assert("pre: not_validindex" && index < static_cast<vtkIdType>(this->Datas->NumberOfVertices));
  assert("pre: not_leaf" && this->IsLeaf(index));
  // The leaf becomes a node and is not anymore a leaf
  // Nodes get constructed with leaf flags set to 1.
  if (static_cast<vtkIdType>(this->Datas->ParentToElderChild.size()) <= index)
  {
    this->Datas->ParentToElderChild.resize(index + 1, std::numeric_limits<unsigned int>::max());
  }
  // The first new child

  this->Datas->ParentToElderChild[index] = static_cast<unsigned int>(this->Datas->NumberOfVertices);
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
unsigned long vtkHyperTree::GetActualMemorySizeBytes()
{
  // in bytes
  // NOLINTNEXTLINE(readability-redundant-casting): needed on Windows
  return static_cast<unsigned long>(sizeof(unsigned int) * this->Datas->ParentToElderChild.size() +
    sizeof(vtkIdType) * this->Datas->GlobalIndexTable.size() + 3 * sizeof(unsigned char) +
    6 * sizeof(vtkIdType));
}

//---------------------------------------------------------------------------
bool vtkHyperTree::IsChildLeaf(vtkIdType index_parent, unsigned int ichild) const
{
  assert("pre: valid_range" && index_parent >= 0 && index_parent < this->Datas->NumberOfVertices);
  if (static_cast<unsigned long>(index_parent) >= this->Datas->ParentToElderChild.size())
  {
    return false;
  }
  assert("pre: valid_range" && ichild < this->NumberOfChildren);
  vtkIdType index_child = this->Datas->ParentToElderChild[index_parent] + ichild;
  return static_cast<unsigned long>(index_child) >= this->Datas->ParentToElderChild.size() ||
    this->Datas->ParentToElderChild[index_child] == std::numeric_limits<unsigned int>::max();
}

//---------------------------------------------------------------------------
bool vtkHyperTree::IsTerminalNode(vtkIdType index) const
{
  assert("pre: valid_range" && index >= 0 && index < this->Datas->NumberOfVertices);
  if (static_cast<unsigned long>(index) >= this->Datas->ParentToElderChild.size())
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
bool vtkHyperTree::IsLeaf(vtkIdType index) const
{
  assert("pre: valid_range" && index >= 0 && index < this->Datas->NumberOfVertices);
  return static_cast<unsigned long>(index) >= this->Datas->ParentToElderChild.size() ||
    this->Datas->ParentToElderChild[index] == std::numeric_limits<unsigned int>::max() ||
    this->Datas->NumberOfVertices == 1;
}

//---------------------------------------------------------------------------
void vtkHyperTree::ComputeBreadthFirstOrderDescriptorImpl(const unsigned int depthLimiter,
  vtkBitArray* inputMask, const unsigned int depth, vtkIdType index,
  std::vector<std::vector<bool>>& descriptorPerDepth,
  std::vector<std::vector<vtkIdType>>& breadthFirstOrderIdMapPerDepth)
{
  vtkIdType idg = this->GetGlobalIndexFromLocal(index);
  bool mask = inputMask ? inputMask->GetValue(idg) : false;
  breadthFirstOrderIdMapPerDepth[depth].emplace_back(idg);
  assert("pre: depth valid" && depth < std::numeric_limits<unsigned int>::max());
  if (!this->IsLeaf(index) && !mask && depth < depthLimiter)
  {
    descriptorPerDepth[depth].push_back(true);
    for (int iChild = 0; iChild < this->NumberOfChildren; ++iChild)
    {
      this->ComputeBreadthFirstOrderDescriptorImpl(depthLimiter, inputMask, depth + 1,
        this->GetElderChildIndex(index) + iChild, descriptorPerDepth,
        breadthFirstOrderIdMapPerDepth);
    }
  }
  else
  {
    descriptorPerDepth[depth].push_back(false);
  }
}

// VTK_DEPRECATED_IN_9_6_0
vtkHyperTree* vtkHyperTree::CreateInstance(unsigned char factor, unsigned char dimension)
{
  vtkHyperTree* ht = vtkHyperTree::New();
  return ht->Initialize(factor, dimension) ? ht : nullptr;
}
VTK_ABI_NAMESPACE_END
