// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <algorithm>

#include "vtkAMRDataObject.h"
#include "vtkAMRMetaData.h"
#include "vtkCartesianGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkRectilinearGrid.h"
#include "vtkType.h"
#include "vtkUniformGridAMRIterator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAMRDataObject);
vtkCxxSetSmartPointerMacro(vtkAMRDataObject, AMRMetaData, vtkAMRMetaData);

//------------------------------------------------------------------------------
vtkAMRDataObject::vtkAMRDataObject() = default;

//------------------------------------------------------------------------------
vtkAMRDataObject::~vtkAMRDataObject() = default;

//------------------------------------------------------------------------------
vtkAMRMetaData* vtkAMRDataObject::GetAMRMetaData()
{
  return this->AMRMetaData;
}

//------------------------------------------------------------------------------
vtkCartesianGrid* vtkAMRDataObject::GetDataSetAsCartesianGrid(unsigned int level, unsigned int idx)
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

  return vtkCartesianGrid::SafeDownCast(this->Superclass::GetPartition(level, idx));
}

//------------------------------------------------------------------------------
vtkImageData* vtkAMRDataObject::GetDataSetAsImageData(unsigned int level, unsigned int idx)
{
  return vtkImageData::SafeDownCast(this->GetDataSetAsCartesianGrid(level, idx));
};

//------------------------------------------------------------------------------
vtkDataSet* vtkAMRDataObject::GetPartition(unsigned int level, unsigned int idx)
{
  return this->GetDataSetAsCartesianGrid(level, idx);
}

//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkAMRDataObject::GetDataSetAsRectilinearGrid(
  unsigned int level, unsigned int idx)
{
  return vtkRectilinearGrid::SafeDownCast(this->GetDataSetAsCartesianGrid(level, idx));
};

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkAMRDataObject::NewIterator()
{
  vtkUniformGridAMRIterator* iter = vtkUniformGridAMRIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::InstantiateMetaData()
{
  this->SetAMRMetaData(vtkSmartPointer<vtkAMRMetaData>::New());
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::Initialize()
{
  this->Initialize(std::vector<unsigned int>{});
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::Initialize(const std::vector<unsigned int>& blocksPerLevel)
{
  this->InstantiateMetaData();
  this->AMRMetaData->Initialize(blocksPerLevel);
  this->InitializeInternal();
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::Initialize(vtkAMRMetaData* metadata)
{
  this->SetAMRMetaData(metadata);
  this->InitializeInternal();
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::InitializeInternal()
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
      this->Superclass::SetPartition(level, block, nullptr);
    }
  }
}

//------------------------------------------------------------------------------
unsigned int vtkAMRDataObject::GetNumberOfLevels() const
{
  unsigned int nlev = 0;
  if (this->AMRMetaData)
  {
    nlev = this->AMRMetaData->GetNumberOfLevels();
  }
  return nlev;
}

//------------------------------------------------------------------------------
unsigned int vtkAMRDataObject::GetNumberOfBlocks() const
{
  unsigned int nblocks = 0;
  if (this->AMRMetaData)
  {
    nblocks = this->AMRMetaData->GetNumberOfBlocks();
  }
  return nblocks;
}

//------------------------------------------------------------------------------
unsigned int vtkAMRDataObject::GetNumberOfBlocks(unsigned int level) const
{
  unsigned int ndata = 0;
  if (this->AMRMetaData)
  {
    ndata = this->AMRMetaData->GetNumberOfBlocks(level);
  }
  return ndata;
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::SetDataSet(unsigned int level, unsigned int idx, vtkDataSet* grid)
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

  vtkCartesianGrid* cg = vtkCartesianGrid::SafeDownCast(grid);
  if (!cg)
  {
    vtkErrorMacro("Unsupported grid type: " << grid->GetClassName());
    return;
  }

  int gridDescr = cg->GetDataDescription();
  if (this->AMRMetaData->GetGridDescription() < 0)
  {
    this->AMRMetaData->SetGridDescription(gridDescr);
  }
  else if (gridDescr != this->AMRMetaData->GetGridDescription())
  {
    vtkErrorMacro("Inconsistent types of vtkCartesianGrid");
    return;
  }

  // update bounds
  double bb[6];
  grid->GetBounds(bb);
  for (int i = 0; i < 3; ++i)
  {
    this->Bounds[i * 2] = std::min(bb[i * 2], this->Bounds[i * 2]);
    this->Bounds[i * 2 + 1] = std::max(bb[i * 2 + 1], this->Bounds[i * 2 + 1]);
  } // END for each dimension

  this->Superclass::SetPartition(level, idx, grid);
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::SetPartition(unsigned int idx, unsigned int partition, vtkDataObject* object)
{
  this->SetDataSet(idx, partition, vtkDataSet::SafeDownCast(object));
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::SetGridDescription(int gridDescription)
{
  if (this->AMRMetaData)
  {
    this->AMRMetaData->SetGridDescription(gridDescription);
  }
}

//------------------------------------------------------------------------------
int vtkAMRDataObject::GetGridDescription()
{
  int desc = 0;
  if (this->AMRMetaData)
  {
    desc = this->AMRMetaData->GetGridDescription();
  }
  return desc;
}

//------------------------------------------------------------------------------
int vtkAMRDataObject::GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const
{
  if (!this->AMRMetaData || level >= this->GetNumberOfLevels() ||
    index >= this->GetNumberOfBlocks(level))
  {
    return -1;
  }
  return this->AMRMetaData->GetAbsoluteBlockIndex(level, index);
}
//------------------------------------------------------------------------------
void vtkAMRDataObject::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkAMRDataObject::ComputeIndexPair(
  unsigned int compositeIdx, unsigned int& level, unsigned int& idx)
{
  if (this->AMRMetaData)
  {
    this->AMRMetaData->ComputeIndexPair(compositeIdx, level, idx);
  }
}

//------------------------------------------------------------------------------
vtkAMRDataObject* vtkAMRDataObject::GetData(vtkInformation* info)
{
  return info ? vtkAMRDataObject::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkAMRDataObject* vtkAMRDataObject::GetData(vtkInformationVector* v, int i)
{
  return vtkAMRDataObject::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::CompositeShallowCopy(vtkCompositeDataSet* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::CompositeShallowCopy(src);

  if (vtkAMRDataObject* hbds = vtkAMRDataObject::SafeDownCast(src))
  {
    this->SetAMRMetaData(hbds->GetAMRMetaData());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::DeepCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  this->Superclass::DeepCopy(src);

  if (vtkAMRDataObject* hbds = vtkAMRDataObject::SafeDownCast(src))
  {
    vtkAMRMetaData* hbdsMetaData = hbds->GetAMRMetaData();
    this->AMRMetaData = vtkSmartPointer<vtkAMRMetaData>::Take(hbdsMetaData->NewInstance());
    this->AMRMetaData->DeepCopy(hbdsMetaData);
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::CopyStructure(vtkCompositeDataSet* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::CopyStructure(src);

  if (vtkAMRDataObject* amr = vtkAMRDataObject::SafeDownCast(src))
  {
    this->SetAMRMetaData(amr->GetAMRMetaData());
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  this->Superclass::ShallowCopy(src);

  if (vtkAMRDataObject* hbds = vtkAMRDataObject::SafeDownCast(src))
  {
    this->SetAMRMetaData(hbds->GetAMRMetaData());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double) * 6);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
const double* vtkAMRDataObject::GetBounds()
{
  return this->Bounds;
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::GetBounds(double bounds[6])
{
  const double* bb = this->GetBounds();
  for (int i = 0; i < 6; ++i)
  {
    bounds[i] = bb[i];
  }
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::GetMin(double min[3])
{
  const double* bb = this->GetBounds();
  min[0] = bb[0];
  min[1] = bb[2];
  min[2] = bb[4];
}

//------------------------------------------------------------------------------
void vtkAMRDataObject::GetMax(double max[3])
{
  const double* bb = this->GetBounds();
  max[0] = bb[1];
  max[1] = bb[3];
  max[2] = bb[5];
}

VTK_ABI_NAMESPACE_END
