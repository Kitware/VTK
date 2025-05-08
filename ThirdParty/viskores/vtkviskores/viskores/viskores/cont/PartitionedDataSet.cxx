//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/StaticAssert.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/internal/Configure.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace viskores
{
namespace cont
{

VISKORES_CONT
PartitionedDataSet::PartitionedDataSet(const viskores::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

VISKORES_CONT
PartitionedDataSet::PartitionedDataSet(const std::vector<viskores::cont::DataSet>& partitions)
{
  this->Partitions = partitions;
}

VISKORES_CONT
PartitionedDataSet::PartitionedDataSet(viskores::Id size)
{
  this->Partitions.reserve(static_cast<std::size_t>(size));
}

VISKORES_CONT
viskores::cont::Field PartitionedDataSet::GetFieldFromPartition(const std::string& field_name,
                                                                int partition_index) const
{
  assert(partition_index >= 0);
  assert(static_cast<std::size_t>(partition_index) < this->Partitions.size());
  return this->Partitions[static_cast<std::size_t>(partition_index)].GetField(field_name);
}

VISKORES_CONT
viskores::Id PartitionedDataSet::GetNumberOfPartitions() const
{
  return static_cast<viskores::Id>(this->Partitions.size());
}

VISKORES_CONT
viskores::Id PartitionedDataSet::GetGlobalNumberOfPartitions() const
{
#ifdef VISKORES_ENABLE_MPI
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id globalSize = 0;
  viskoresdiy::mpi::all_reduce(
    comm, GetNumberOfPartitions(), globalSize, std::plus<viskores::Id>{});
  return globalSize;
#else
  return GetNumberOfPartitions();
#endif
}

VISKORES_CONT
const viskores::cont::DataSet& PartitionedDataSet::GetPartition(viskores::Id blockId) const
{
  return this->Partitions[static_cast<std::size_t>(blockId)];
}

VISKORES_CONT
const std::vector<viskores::cont::DataSet>& PartitionedDataSet::GetPartitions() const
{
  return this->Partitions;
}

VISKORES_CONT
void PartitionedDataSet::AppendPartition(const viskores::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

VISKORES_CONT
void PartitionedDataSet::AppendPartitions(const std::vector<viskores::cont::DataSet>& partitions)
{
  this->Partitions.insert(this->Partitions.end(), partitions.begin(), partitions.end());
}

VISKORES_CONT
void PartitionedDataSet::InsertPartition(viskores::Id index, const viskores::cont::DataSet& ds)
{
  if (index <= static_cast<viskores::Id>(this->Partitions.size()))
  {
    this->Partitions.insert(this->Partitions.begin() + index, ds);
  }
  else
  {
    std::string msg = "invalid insert position\n ";
    throw ErrorBadValue(msg);
  }
}

VISKORES_CONT
void PartitionedDataSet::ReplacePartition(viskores::Id index, const viskores::cont::DataSet& ds)
{
  if (index < static_cast<viskores::Id>(this->Partitions.size()))
    this->Partitions.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorBadValue(msg);
  }
}

VISKORES_CONT
void PartitionedDataSet::CopyPartitions(const viskores::cont::PartitionedDataSet& source)
{
  this->Partitions = source.Partitions;
}

VISKORES_CONT
void PartitionedDataSet::PrintSummary(std::ostream& stream) const
{
  stream << "PartitionedDataSet [" << this->Partitions.size() << " partitions]:\n";

  for (size_t part = 0; part < this->Partitions.size(); ++part)
  {
    stream << "Partition " << part << ":\n";
    this->Partitions[part].PrintSummary(stream);
  }

  stream << "  Fields[" << this->GetNumberOfFields() << "]\n";
  for (viskores::Id index = 0; index < this->GetNumberOfFields(); index++)
  {
    this->GetField(index).PrintSummary(stream);
  }
}
}
} // namespace viskores::cont
