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
#include <viskores/cont/BoundsGlobalCompute.h>

#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/FieldRangeGlobalCompute.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <numeric> // for std::accumulate

namespace viskores
{
namespace cont
{

namespace detail
{
VISKORES_CONT
viskores::Bounds MergeBoundsGlobal(const viskores::Bounds& local)
{
  viskores::cont::ArrayHandle<viskores::Range> ranges;
  ranges.Allocate(3);
  ranges.WritePortal().Set(0, local.X);
  ranges.WritePortal().Set(1, local.Y);
  ranges.WritePortal().Set(2, local.Z);

  ranges = viskores::cont::detail::MergeRangesGlobal(ranges);
  auto portal = ranges.ReadPortal();
  return viskores::Bounds(portal.Get(0), portal.Get(1), portal.Get(2));
}
}


//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::DataSet& dataset,
                                     viskores::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(viskores::cont::BoundsCompute(dataset, coordinate_system_index));
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::PartitionedDataSet& pds,
                                     viskores::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(viskores::cont::BoundsCompute(pds, coordinate_system_index));
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::DataSet& dataset,
                                     const std::string& name)
{
  return detail::MergeBoundsGlobal(viskores::cont::BoundsCompute(dataset, name));
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::PartitionedDataSet& pds,
                                     const std::string& name)
{
  return detail::MergeBoundsGlobal(viskores::cont::BoundsCompute(pds, name));
}
}
}
