// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPartitionedDataSetCollection.h"

#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"

#include <algorithm>
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPartitionedDataSetCollection);
vtkCxxSetObjectMacro(vtkPartitionedDataSetCollection, DataAssembly, vtkDataAssembly);
//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection::vtkPartitionedDataSetCollection()
  : DataAssembly(nullptr)
{
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection::~vtkPartitionedDataSetCollection()
{
  this->SetDataAssembly(nullptr);
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(vtkInformation* info)
{
  return info ? vtkPartitionedDataSetCollection::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(
  vtkInformationVector* v, int i)
{
  return vtkPartitionedDataSetCollection::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetNumberOfPartitionedDataSets(unsigned int numDataSets)
{
  auto old_size = this->GetNumberOfPartitionedDataSets();
  this->Superclass::SetNumberOfChildren(numDataSets);

  // add non-null vtkPartitionedDataSet's to the collection.
  for (unsigned int cc = old_size; cc < numDataSets; ++cc)
  {
    auto ptd = vtkPartitionedDataSet::New();
    this->SetPartitionedDataSet(cc, ptd);
    ptd->FastDelete();
  }
}

//------------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetNumberOfPartitionedDataSets() const
{
  return const_cast<vtkPartitionedDataSetCollection*>(this)->Superclass::GetNumberOfChildren();
}

//------------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSetCollection::GetPartitionedDataSet(
  unsigned int idx) const
{
  return vtkPartitionedDataSet::SafeDownCast(
    const_cast<vtkPartitionedDataSetCollection*>(this)->Superclass::GetChild(idx));
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetPartitionedDataSet(
  unsigned int idx, vtkPartitionedDataSet* dataset)
{
  if (dataset == nullptr)
  {
    vtkErrorMacro("A partitioned dataset cannot be nullptr.");
    return;
  }
  this->Superclass::SetChild(idx, dataset);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::RemovePartitionedDataSet(unsigned int idx)
{
  this->Superclass::RemoveChild(idx);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetPartition(
  unsigned int idx, unsigned int partition, vtkDataObject* object)
{
  if (this->GetNumberOfPartitionedDataSets() <= idx)
  {
    this->SetNumberOfPartitionedDataSets(idx + 1);
  }
  auto ptd = this->GetPartitionedDataSet(idx);
  assert(ptd != nullptr);
  ptd->SetPartition(partition, object);
}

//------------------------------------------------------------------------------
vtkDataSet* vtkPartitionedDataSetCollection::GetPartition(unsigned int idx, unsigned int partition)
{
  auto ptd = this->GetPartitionedDataSet(idx);
  return ptd ? ptd->GetPartition(partition) : nullptr;
}

//------------------------------------------------------------------------------
vtkDataObject* vtkPartitionedDataSetCollection::GetPartitionAsDataObject(
  unsigned int idx, unsigned int partition)
{
  auto ptd = this->GetPartitionedDataSet(idx);
  return ptd ? ptd->GetPartitionAsDataObject(partition) : nullptr;
}

//------------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetNumberOfPartitions(unsigned int idx) const
{
  auto ptd = this->GetPartitionedDataSet(idx);
  return ptd ? ptd->GetNumberOfPartitions() : 0;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetNumberOfPartitions(
  unsigned int idx, unsigned int numPartitions)
{
  if (this->GetNumberOfPartitionedDataSets() <= idx)
  {
    this->SetNumberOfPartitionedDataSets(idx + 1);
  }
  auto ptd = this->GetPartitionedDataSet(idx);
  assert(ptd != nullptr);
  ptd->SetNumberOfPartitions(numPartitions);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkPartitionedDataSetCollection::GetMTime()
{
  return this->DataAssembly ? std::max(this->Superclass::GetMTime(), this->DataAssembly->GetMTime())
                            : this->Superclass::GetMTime();
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::Initialize()
{
  this->Superclass::Initialize();
  this->SetDataAssembly(nullptr);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::CopyStructure(vtkCompositeDataSet* input)
{
  this->Superclass::CopyStructure(input);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(input))
  {
    this->SetDataAssembly(pdc->GetDataAssembly());
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::CompositeShallowCopy(vtkCompositeDataSet* src)
{
  this->Superclass::CompositeShallowCopy(src);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(src))
  {
    this->SetDataAssembly(pdc->GetDataAssembly());
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::ShallowCopy(vtkDataObject* src)
{
  this->Superclass::ShallowCopy(src);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(src))
  {
    this->SetDataAssembly(pdc->GetDataAssembly());
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::DeepCopy(vtkDataObject* src)
{
  this->Superclass::DeepCopy(src);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(src))
  {
    if (auto srcDA = pdc->GetDataAssembly())
    {
      vtkNew<vtkDataAssembly> destDA;
      destDA->DeepCopy(srcDA);
      this->SetDataAssembly(destDA);
    }
    else
    {
      this->SetDataAssembly(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkPartitionedDataSetCollection::CreateForCopyStructure(vtkDataObjectTree* other)
{
  return vtkMultiPieceDataSet::SafeDownCast(other)
    ? vtkPartitionedDataSet::New()
    : this->Superclass::CreateForCopyStructure(other);
}

//------------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetCompositeIndex(unsigned int idx) const
{
  if (idx >= this->GetNumberOfPartitionedDataSets())
  {
    vtkLogF(ERROR, "invalid partition index '%u'", idx);
    return 0;
  }

  unsigned int cid = 1;
  for (unsigned int cc = 0; cc < idx; ++cc)
  {
    cid += 1 + this->GetNumberOfPartitions(cc);
  }
  return cid;
}

//------------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetCompositeIndex(
  unsigned int idx, unsigned int partition) const
{
  if (idx >= this->GetNumberOfPartitionedDataSets() ||
    partition >= this->GetNumberOfPartitions(idx))
  {
    vtkLogF(ERROR, "invalid partition index ('%u', '%u')", idx, partition);
    return 0;
  }

  unsigned int cid = this->GetCompositeIndex(idx);
  // cid is the vtkPartitionedDataSet's index. So add 1.
  return cid + partition + 1;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataAssembly: " << this->DataAssembly << endl;
}
VTK_ABI_NAMESPACE_END
