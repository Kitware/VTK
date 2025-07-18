// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUniformGridAMR.h"
#include "vtkAMRDataInternals.h"
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
#include "vtkUniformGridAMRDataIterator.h"

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
  return this->AMRData->GetDataSet(this->GetAbsoluteBlockIndex(level, idx));
}

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkUniformGridAMR::NewIterator()
{
  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::New();
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
  this->Initialize({});
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
  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;

  this->InstantiateMetaData();
  this->AMRMetaData->Initialize(blocksPerLevel);
  this->AMRData->Initialize();
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
  if (!grid)
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
  int index = this->AMRMetaData->GetAbsoluteBlockIndex(level, idx);
  this->AMRData->Insert(index, grid);

  // update bounds
  double bb[6];
  grid->GetBounds(bb);
  // update bounds
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
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetDataSet(vtkCompositeDataIterator* compositeIter, vtkDataObject* dataObj)
{
  if (auto amrIter = vtkUniformGridAMRDataIterator::SafeDownCast(compositeIter))
  {
    this->SetDataSet(amrIter->GetCurrentLevel(), amrIter->GetCurrentIndex(),
      vtkUniformGrid::SafeDownCast(dataObj));
  }
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
vtkDataObject* vtkUniformGridAMR::GetDataSet(vtkCompositeDataIterator* compositeIter)
{
  if (auto amrIter = vtkUniformGridAMRDataIterator::SafeDownCast(compositeIter))
  {
    return this->GetDataSet(amrIter->GetCurrentLevel(), amrIter->GetCurrentIndex());
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkUniformGridAMR::GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const
{
  if (level >= this->GetNumberOfLevels() || index >= this->GetNumberOfBlocks(level))
  {
    return -1;
  }
  return this->AMRMetaData->GetAbsoluteBlockIndex(level, index);
}
//------------------------------------------------------------------------------
void vtkUniformGridAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->AMRMetaData)
  {
    this->AMRMetaData->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ComputeIndexPair(
  unsigned int compositeIdx, unsigned int& level, unsigned int& idx)
{
  this->AMRMetaData->ComputeIndexPair(compositeIdx, level, idx);
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
    this->AMRData->CompositeShallowCopy(hbds->AMRData);
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
    this->AMRData->DeepCopy(hbds->AMRData);
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
    this->AMRData->ShallowCopy(hbds->AMRData);
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
