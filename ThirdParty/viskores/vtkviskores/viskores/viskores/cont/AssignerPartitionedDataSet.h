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
#ifndef viskores_cont_AssignerPartitionedDataSet_h
#define viskores_cont_AssignerPartitionedDataSet_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>
#include <viskores/thirdparty/diy/Configure.h>

#include <vector>

#include <viskores/thirdparty/diy/diy.h>

#ifdef VISKORES_MSVC
#pragma warning(push)
// disable C4275: non-dll interface base class warnings
#pragma warning(disable : 4275)
#endif

namespace viskores
{
namespace cont
{

class PartitionedDataSet;

/// \brief Assigner for PartitionedDataSet partitions.
///
/// `AssignerPartitionedDataSet` is a `viskoresdiy::StaticAssigner` implementation
/// that uses `PartitionedDataSet`'s partition distribution to build
/// global-id/rank associations needed for several `diy` operations.
/// It uses a contiguous assignment strategy to map partitions to global ids,
/// i.e. partitions on rank 0 come first, then rank 1, etc. Any rank may have 0
/// partitions.
///
/// AssignerPartitionedDataSet uses collectives in the constructor hence it is
/// essential it gets created on all ranks irrespective of whether the rank has
/// any partitions.
///
class VISKORES_CONT_EXPORT AssignerPartitionedDataSet : public viskoresdiy::StaticAssigner
{
public:
  /// Initialize the assigner using a partitioned dataset.
  /// This may initialize collective operations to populate the assigner with
  /// information about partitions on all ranks.
  VISKORES_CONT
  AssignerPartitionedDataSet(const viskores::cont::PartitionedDataSet& pds);

  VISKORES_CONT
  AssignerPartitionedDataSet(viskores::Id num_partitions);

  VISKORES_CONT
  ~AssignerPartitionedDataSet() override;

  ///@{
  /// viskoresdiy::Assigner API implementation.
  VISKORES_CONT
  void local_gids(int rank, std::vector<int>& gids) const override;

  VISKORES_CONT
  int rank(int gid) const override;
  ///@}
private:
  std::vector<viskores::Id> IScanPartitionCounts;
};
}
}

#ifdef VISKORES_MSVC
#pragma warning(pop)
#endif

#endif
