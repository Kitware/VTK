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
#ifndef viskores_worklet_DispatcherReduceByKey_h
#define viskores_worklet_DispatcherReduceByKey_h

#include <viskores/cont/DeviceAdapter.h>

#include <viskores/cont/arg/TypeCheckTagKeys.h>
#include <viskores/worklet/internal/DispatcherBase.h>

namespace viskores
{
namespace worklet
{
class WorkletReduceByKey;

/// \brief Dispatcher for worklets that inherit from \c WorkletReduceByKey.
///
template <typename WorkletType>
class DispatcherReduceByKey
  : public viskores::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                       WorkletType,
                                                       viskores::worklet::WorkletReduceByKey>
{
  using Superclass =
    viskores::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                WorkletType,
                                                viskores::worklet::WorkletReduceByKey>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VISKORES_CONT DispatcherReduceByKey(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    using namespace viskores::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something other than viskores::worklet::Keys as the input domain, which
    // is illegal.
    VISKORES_STATIC_ASSERT_MSG(
      (viskores::cont::arg::TypeCheck<viskores::cont::arg::TypeCheckTagKeys,
                                      InputDomainType>::value),
      "Invalid input domain for WorkletReduceByKey.");

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BasicInvoke.
    this->BasicInvoke(invocation, SchedulingRange(inputDomain));
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_DispatcherReduceByKey_h
