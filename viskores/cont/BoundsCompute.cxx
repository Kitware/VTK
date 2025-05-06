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
#include <viskores/cont/BoundsCompute.h>

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <numeric> // for std::accumulate

namespace viskores
{
namespace cont
{

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsCompute(const viskores::cont::DataSet& dataset,
                               viskores::Id coordinate_system_index)
{
  return dataset.GetNumberOfCoordinateSystems() > coordinate_system_index
    ? dataset.GetCoordinateSystem(coordinate_system_index).GetBounds()
    : viskores::Bounds();
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsCompute(const viskores::cont::PartitionedDataSet& pds,
                               viskores::Id coordinate_system_index)
{
  return std::accumulate(
    pds.begin(),
    pds.end(),
    viskores::Bounds(),
    [=](const viskores::Bounds& val, const viskores::cont::DataSet& partition)
    { return val + viskores::cont::BoundsCompute(partition, coordinate_system_index); });
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsCompute(const viskores::cont::DataSet& dataset, const std::string& name)
{
  try
  {
    return dataset.GetCoordinateSystem(name).GetBounds();
  }
  catch (viskores::cont::ErrorBadValue&)
  {
    // missing coordinate_system_index, return empty bounds.
    return viskores::Bounds();
  }
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::Bounds BoundsCompute(const viskores::cont::PartitionedDataSet& pds,
                               const std::string& name)
{
  return std::accumulate(pds.begin(),
                         pds.end(),
                         viskores::Bounds(),
                         [=](const viskores::Bounds& val, const viskores::cont::DataSet& partition)
                         { return val + viskores::cont::BoundsCompute(partition, name); });
}
}
}
