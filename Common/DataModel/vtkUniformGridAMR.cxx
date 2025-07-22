// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUniformGridAMR.h"
#include "vtkAMRMetaData.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkLegacy.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkType.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRIterator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUniformGridAMR);
vtkCxxSetSmartPointerMacro(vtkUniformGridAMR, AMRMetaData, vtkAMRMetaData);

//------------------------------------------------------------------------------
vtkUniformGridAMR::vtkUniformGridAMR() = default;

//------------------------------------------------------------------------------
vtkUniformGridAMR::~vtkUniformGridAMR() = default;

//------------------------------------------------------------------------------
vtkAMRMetaData* vtkUniformGridAMR::GetAMRMetaData()
{
  return this->AMRMetaData;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGridAMR::GetDataSet(unsigned int level, unsigned int idx)
{
  if (!this->AMRMetaData)
  {
    vtkErrorMacro("AMR is not initialized");
    return nullptr;
  }

  if (level >= this->GetNumberOfLevels() || idx >= this->GetNumberOfBlocks(level))
  {
    vtkErrorMacro("Invalid data set index: " << level << " " << idx);
    return nullptr;
  }

  return vtkUniformGrid::SafeDownCast(this->GetPartition(level, idx));
}

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkUniformGridAMR::NewIterator()
{
  vtkUniformGridAMRIterator* iter = vtkUniformGridAMRIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::InstantiateMetaData()
{
  this->SetAMRMetaData(vtkSmartPointer<vtkAMRMetaData>::New());
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize()
{
  this->Initialize(std::vector<unsigned int>{});
}

// VTK_DEPRECATED_IN_9_6_0("Use Initialize(const std::vector<unsigned int>&) instead")
//------------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize(int numLevels, const int* blocksPerLevel)
{
  std::vector<unsigned int> vec;
  vec.assign(blocksPerLevel, blocksPerLevel + numLevels);
  this->Initialize(vec);
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize(const std::vector<unsigned int>& blocksPerLevel)
{
  this->InstantiateMetaData();
  this->AMRMetaData->Initialize(blocksPerLevel);
  this->InitializeInternal();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize(vtkAMRMetaData* metadata)
{
  std::vector<unsigned int> blocksPerLevel(metadata->GetNumberOfLevels());
  for (unsigned int i = 0; i < metadata->GetNumberOfLevels(); i++)
  {
    blocksPerLevel.emplace_back(metadata->GetNumberOfBlocks(i));
  }
  this->SetAMRMetaData(metadata);
  this->InitializeInternal();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::InitializeInternal()
{
  this->Superclass::Initialize();

  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;

  unsigned int nLevels = this->AMRMetaData->GetNumberOfLevels();
  this->SetNumberOfPartitionedDataSets(nLevels);
  for (unsigned int level = 0; level < nLevels; level++)
  {
    unsigned int nBlocks = this->AMRMetaData->GetNumberOfBlocks(level);
    this->SetNumberOfPartitions(level, nBlocks);
    for (unsigned int block = 0; block < nBlocks; block++)
    {
      this->SetPartition(level, block, nullptr);
    }
  }
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfLevels() const
{
  unsigned int nlev = 0;
  if (this->AMRMetaData)
  {
    nlev = this->AMRMetaData->GetNumberOfLevels();
  }
  return nlev;
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfBlocks() const
{
  unsigned int nblocks = 0;
  if (this->AMRMetaData)
  {
    nblocks = this->AMRMetaData->GetNumberOfBlocks();
  }
  return nblocks;
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfBlocks(unsigned int level) const
{
  unsigned int ndata = 0;
  if (this->AMRMetaData)
  {
    ndata = this->AMRMetaData->GetNumberOfBlocks(level);
  }
  return ndata;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetDataSet(unsigned int level, unsigned int idx, vtkUniformGrid* grid)
{
  if (!grid || !this->AMRMetaData)
  {
    return; // nullptr grid, nothing to do
  }
  if (level >= this->GetNumberOfLevels() || idx >= this->GetNumberOfBlocks(level))
  {
    vtkErrorMacro("Invalid data set index: " << level << " " << idx);
    return;
  }

  if (this->AMRMetaData->GetGridDescription() < 0)
  {
    this->AMRMetaData->SetGridDescription(grid->GetDataDescription());
  }
  else if (grid->GetDataDescription() != this->AMRMetaData->GetGridDescription())
  {
    vtkErrorMacro("Inconsistent types of vtkUniformGrid");
    return;
  }

  // update bounds
  double bb[6];
  grid->GetBounds(bb);
  for (int i = 0; i < 3; ++i)
  {
    if (bb[i * 2] < this->Bounds[i * 2])
    {
      this->Bounds[i * 2] = bb[i * 2];
    }
    if (bb[i * 2 + 1] > this->Bounds[i * 2 + 1])
    {
      this->Bounds[i * 2 + 1] = bb[i * 2 + 1];
    }
  } // END for each dimension

  this->SetPartition(level, idx, grid);
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetGridDescription(int gridDescription)
{
  if (this->AMRMetaData)
  {
    this->AMRMetaData->SetGridDescription(gridDescription);
  }
}

//------------------------------------------------------------------------------
int vtkUniformGridAMR::GetGridDescription()
{
  int desc = 0;
  if (this->AMRMetaData)
  {
    desc = this->AMRMetaData->GetGridDescription();
  }
  return desc;
}

//------------------------------------------------------------------------------
int vtkUniformGridAMR::GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const
{
  if (!this->AMRMetaData || level >= this->GetNumberOfLevels() ||
    index >= this->GetNumberOfBlocks(level))
  {
    return -1;
  }
  return this->AMRMetaData->GetAbsoluteBlockIndex(level, index);
}
//------------------------------------------------------------------------------
void vtkUniformGridAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Bounds: ";
  for (unsigned int i = 0; i < 3; i++)
  {
    os << this->Bounds[i * 2] << " " << this->Bounds[i * 2 + 1] << " ";
  }
  os << "\n";

  if (this->AMRMetaData)
  {
    this->AMRMetaData->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ComputeIndexPair(
  unsigned int compositeIdx, unsigned int& level, unsigned int& idx)
{
  if (this->AMRMetaData)
  {
    this->AMRMetaData->ComputeIndexPair(compositeIdx, level, idx);
  }
}

//------------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMR::GetData(vtkInformation* info)
{
  return info ? vtkUniformGridAMR::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMR::GetData(vtkInformationVector* v, int i)
{
  return vtkUniformGridAMR::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::CompositeShallowCopy(vtkCompositeDataSet* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::CompositeShallowCopy(src);

  if (vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
  {
    this->SetAMRMetaData(hbds->GetAMRMetaData());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::DeepCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  this->Superclass::DeepCopy(src);

  if (vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
  {
    vtkAMRMetaData* hbdsMetaData = hbds->GetAMRMetaData();
    this->AMRMetaData = vtkSmartPointer<vtkAMRMetaData>::Take(hbdsMetaData->NewInstance());
    this->AMRMetaData->DeepCopy(hbdsMetaData);
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::CopyStructure(vtkCompositeDataSet* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::CopyStructure(src);

  if (vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
  {
    this->SetAMRMetaData(hbds->GetAMRMetaData());
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::ShallowCopy(src);

  if (vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
  {
    this->SetAMRMetaData(hbds->GetAMRMetaData());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
const double* vtkUniformGridAMR::GetBounds()
{
  return this->Bounds;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::GetBounds(double bounds[6])
{
  const double* bb = this->GetBounds();
  for (int i = 0; i < 6; ++i)
  {
    bounds[i] = bb[i];
  }
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::GetMin(double min[3])
{
  const double* bb = this->GetBounds();
  min[0] = bb[0];
  min[1] = bb[2];
  min[2] = bb[4];
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::GetMax(double max[3])
{
  const double* bb = this->GetBounds();
  max[0] = bb[1];
  max[1] = bb[3];
  max[2] = bb[5];
}
VTK_ABI_NAMESPACE_END
