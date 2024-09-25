// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRDataInternals.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

#include <algorithm>
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAMRDataInternals);

vtkAMRDataInternals::Block::Block(unsigned int i, vtkUniformGrid* g)
{
  this->Index = i;
  this->Grid = g;
}

//------------------------------------------------------------------------------

vtkAMRDataInternals::vtkAMRDataInternals()
  : InternalIndex(nullptr)
{
}

void vtkAMRDataInternals::Initialize()
{
  delete this->InternalIndex;
  this->InternalIndex = nullptr;
  this->Blocks.clear();
}

vtkAMRDataInternals::~vtkAMRDataInternals()
{
  this->Blocks.clear();
  delete this->InternalIndex;
}

void vtkAMRDataInternals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkAMRDataInternals::Insert(unsigned int index, vtkUniformGrid* grid)
{
  const auto it = std::lower_bound(this->Blocks.begin(), this->Blocks.end(), index,
    [](const Block& block, unsigned int idx) { return block.Index < idx; });
  this->Blocks.insert(it, Block(index, grid));
}

vtkUniformGrid* vtkAMRDataInternals::GetDataSet(unsigned int compositeIndex)
{
  unsigned int internalIndex(0);
  if (!this->GetInternalIndex(compositeIndex, internalIndex))
  {
    return nullptr;
  }
  return this->Blocks[internalIndex].Grid;
}

bool vtkAMRDataInternals::GetInternalIndex(unsigned int compositeIndex, unsigned int& internalIndex)
{
  this->GenerateIndex();
  if (compositeIndex >= this->InternalIndex->size())
  {
    return false;
  }
  int idx = (*this->InternalIndex)[compositeIndex];
  if (idx < 0)
  {
    return false;
  }
  internalIndex = static_cast<unsigned int>(idx);
  return true;
}

void vtkAMRDataInternals::GenerateIndex(bool force)
{
  if (!force && this->InternalIndex)
  {
    return;
  }
  delete this->InternalIndex;
  this->InternalIndex = new std::vector<int>();
  std::vector<int>& internalIndex(*this->InternalIndex);

  for (unsigned i = 0; i < this->Blocks.size(); i++)
  {
    unsigned int index = this->Blocks[i].Index;
    for (unsigned int j = static_cast<unsigned int>(internalIndex.size()); j <= index; j++)
    {
      internalIndex.push_back(-1);
    }
    internalIndex[index] = static_cast<int>(i);
  }
}

void vtkAMRDataInternals::CompositeShallowCopy(vtkObject* src)
{
  if (src == this)
  {
    return;
  }

  if (vtkAMRDataInternals* hbds = vtkAMRDataInternals::SafeDownCast(src))
  {
    this->Blocks = hbds->Blocks;
  }

  this->Modified();
}

void vtkAMRDataInternals::DeepCopy(vtkObject* src)
{
  if (src == this)
  {
    return;
  }

  if (vtkAMRDataInternals* hbds = vtkAMRDataInternals::SafeDownCast(src))
  {
    this->Blocks = hbds->Blocks;
    for (auto& item : this->Blocks)
    {
      if (item.Grid)
      {
        auto clone = item.Grid->NewInstance();
        clone->DeepCopy(item.Grid);
        item.Grid.TakeReference(clone);
      }
    }
  }

  this->Modified();
}

void vtkAMRDataInternals::ShallowCopy(vtkObject* src)
{
  if (src == this)
  {
    return;
  }

  if (vtkAMRDataInternals* hbds = vtkAMRDataInternals::SafeDownCast(src))
  {
    this->Blocks = hbds->Blocks;
    for (auto& item : this->Blocks)
    {
      if (item.Grid)
      {
        auto clone = item.Grid->NewInstance();
        clone->ShallowCopy(item.Grid);
        item.Grid.TakeReference(clone);
      }
    }
  }

  this->Modified();
}
VTK_ABI_NAMESPACE_END
