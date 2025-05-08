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
#include <viskores/cont/FieldRangeCompute.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace viskores
{
namespace cont
{

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeCompute(
  const viskores::cont::DataSet& dataset,
  const std::string& name,
  viskores::cont::Field::Association assoc)
{
  viskores::cont::Field field;
  try
  {
    field = dataset.GetField(name, assoc);
  }
  catch (viskores::cont::ErrorBadValue&)
  {
    // field missing, return empty range.
    return viskores::cont::ArrayHandle<viskores::Range>();
  }

  return field.GetRange();
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeCompute(
  const viskores::cont::PartitionedDataSet& pds,
  const std::string& name,
  viskores::cont::Field::Association assoc)
{
  std::vector<viskores::Range> result_vector =
    std::accumulate(pds.begin(),
                    pds.end(),
                    std::vector<viskores::Range>(),
                    [&](const std::vector<viskores::Range>& accumulated_value,
                        const viskores::cont::DataSet& dataset)
                    {
                      viskores::cont::ArrayHandle<viskores::Range> partition_range =
                        viskores::cont::FieldRangeCompute(dataset, name, assoc);

                      std::vector<viskores::Range> result = accumulated_value;

                      // if the current partition has more components than we have seen so far,
                      // resize the result to fit all components.
                      result.resize(std::max(
                        result.size(), static_cast<size_t>(partition_range.GetNumberOfValues())));

                      auto portal = partition_range.ReadPortal();
                      std::transform(viskores::cont::ArrayPortalToIteratorBegin(portal),
                                     viskores::cont::ArrayPortalToIteratorEnd(portal),
                                     result.begin(),
                                     result.begin(),
                                     std::plus<viskores::Range>());
                      return result;
                    });

  return viskores::cont::make_ArrayHandleMove(std::move(result_vector));
}
}
} // namespace viskores::cont
