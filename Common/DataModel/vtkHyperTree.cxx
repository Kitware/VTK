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
#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>
#include <vector>

//-----------------------------------------------------------------------------

void vtkHyperTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimension: " << this->Dimension << "\n";
  os << indent << "BranchFactor: " << this->BranchFactor << "\n";
  os << indent << "NumberOfChildren: " << this->NumberOfChildren << "\n";

  os << indent << "NumberOfLevels: " << this->Datas->NumberOfLevels << "\n";
  os << indent << "NumberOfVertices: " << this->Datas->NumberOfVertices << "\n";
  os << indent << "NumberOfNodes: " << this->Datas->NumberOfNodes << "\n";
  os << indent << "GlobalIndexStart: " << this->Datas->GlobalIndexStart << "\n";

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
  vtkHyperTree* Freeze(const char* vtkNotUsed(mode)) override
  {
    // Option not used
    return this;
  }

  //---------------------------------------------------------------------------
  ~vtkCompactHyperTree() override {}

  //---------------------------------------------------------------------------
  void SetGlobalIndexFromLocal(vtkIdType index, vtkIdType global) override
  {
    // If local index outside map range, resize the latter
    if (static_cast<vtkIdType>(this->CompactDatas->GlobalIndexTable_stl.size()) <= index)
    {
      this->CompactDatas->GlobalIndexTable_stl.resize(index + 1, -1);
    }
    // JB Cette version de ce service permet de (re)positionner plusieurs fois la valeur de l'indice
    // global
    // JB afin de prendre en compte une premiere description, a priori, incomplete suivant d'une
    // JB description plus fine. Le dernier appel ecrase ce qui a ete ecrit precedemment.
    this->CompactDatas->GlobalIndexTable_stl[index] = global;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalIndexFromLocal(vtkIdType index) const override
  {
    // JB If map range exit, return value map index
    if (this->CompactDatas->GlobalIndexTable_stl.size() != 0)
    {
      assert("pre: not_valid_index" && index >= 0 &&
        index < (vtkIdType)this->CompactDatas->GlobalIndexTable_stl.size());
      assert(
        "pre: not_positive_global_index" && this->CompactDatas->GlobalIndexTable_stl[index] >= 0);
      return this->CompactDatas->GlobalIndexTable_stl[index];
    } // JB else return global index start + local
    assert("pre: not_positive_start_index" && this->Datas->GlobalIndexStart >= 0);
    assert("pre: not_valid_index" && index >= 0);
    return this->Datas->GlobalIndexStart + index;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalNodeIndexMax() const override
  {
    // JB If map range
    if (static_cast<vtkIdType>(this->CompactDatas->GlobalIndexTable_stl.size() != 0))
    {
      const auto it_end = this->CompactDatas->GlobalIndexTable_stl.end();
      const auto elt_found =
        std::max_element(this->CompactDatas->GlobalIndexTable_stl.begin(), it_end);
      assert("pre: not_positive_global_index" &&
        (*std::max_element(this->CompactDatas->GlobalIndexTable_stl.begin(), it_end)) >= 0);
      return *elt_found;
    } // JB else not map range
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
  // Description
  // Are children all leaves?
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
      this->CompactDatas->ParentToElderChild_stl[index] == UINT_MAX;
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
  void FindChildParameters(unsigned char ichild, vtkIdType& index_parent, bool& isLeaf) override
  {
    assert("pre: valid_range" && index_parent >= 0 &&
      static_cast<unsigned long>(index_parent) < this->CompactDatas->ParentToElderChild_stl.size());
    assert("pre: valid_range" && ichild < this->NumberOfChildren);
    index_parent = this->CompactDatas->ParentToElderChild_stl[index_parent] + ichild;
    isLeaf = static_cast<unsigned long>(index_parent) >=
        this->CompactDatas->ParentToElderChild_stl.size() ||
      this->CompactDatas->ParentToElderChild_stl[index_parent] == UINT_MAX;
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
    // le root n'a pas de parent
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
    vtkGenericWarningMacro("Bad dimension " << dimension);
    return nullptr;
  }
  vtkHyperTree* ht = vtkCompactHyperTree::New();
  ht->Initialize(factor, dimension, pow(factor, dimension));
  return ht;
}
