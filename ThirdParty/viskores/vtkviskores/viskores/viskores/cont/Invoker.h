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
#ifndef viskores_cont_Invoker_h
#define viskores_cont_Invoker_h

#include <viskores/worklet/internal/MaskBase.h>
#include <viskores/worklet/internal/ScatterBase.h>

#include <viskores/cont/TryExecute.h>

namespace viskores
{
namespace cont
{

namespace detail
{
template <typename T>
using scatter_or_mask = std::integral_constant<bool,
                                               viskores::worklet::internal::is_mask<T>::value ||
                                                 viskores::worklet::internal::is_scatter<T>::value>;
}

/// \brief Allows launching any worklet without a dispatcher.
///
/// \c Invoker is a generalized \c Dispatcher that is able to automatically
/// determine how to properly launch/invoke any worklet that is passed to it.
/// When an \c Invoker is constructed it is provided the desired device adapter
/// that all worklets invoked by it should be launched on.
///
/// \c Invoker is designed to not only reduce the verbosity of constructing
/// multiple dispatchers inside a block of logic, but also makes it easier to
/// make sure all worklets execute on the same device.
struct Invoker
{

  /// Constructs an Invoker that will try to launch worklets on any device
  /// that is enabled.
  ///
  explicit Invoker()
    : DeviceId(viskores::cont::DeviceAdapterTagAny{})
  {
  }

  /// Constructs an Invoker that will try to launch worklets only on the
  /// provided device adapter.
  ///
  explicit Invoker(viskores::cont::DeviceAdapterId device)
    : DeviceId(device)
  {
  }

  /// Launch the worklet that is provided as the first parameter.
  /// Optional second parameter is either the scatter or mask type associated with the worklet.
  /// Any additional parameters are the ControlSignature arguments for the worklet.
  ///
  template <typename Worklet,
            typename T,
            typename... Args,
            typename std::enable_if<detail::scatter_or_mask<T>::value, int>::type* = nullptr>
  inline void operator()(Worklet&& worklet, T&& scatterOrMask, Args&&... args) const
  {
    using WorkletType = viskores::internal::remove_cvref<Worklet>;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet, scatterOrMask);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<Args>(args)...);
  }

  /// Launch the worklet that is provided as the first parameter.
  /// Optional second parameter is either the scatter or mask type associated with the worklet.
  /// Optional third parameter is either the scatter or mask type associated with the worklet.
  /// Any additional parameters are the ControlSignature arguments for the worklet.
  ///
  template <
    typename Worklet,
    typename T,
    typename U,
    typename... Args,
    typename std::enable_if<detail::scatter_or_mask<T>::value && detail::scatter_or_mask<U>::value,
                            int>::type* = nullptr>
  inline void operator()(Worklet&& worklet,
                         T&& scatterOrMaskA,
                         U&& scatterOrMaskB,
                         Args&&... args) const
  {
    using WorkletType = viskores::internal::remove_cvref<Worklet>;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet, scatterOrMaskA, scatterOrMaskB);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<Args>(args)...);
  }

  /// Launch the worklet that is provided as the first parameter.
  /// Optional second parameter is either the scatter or mask type associated with the worklet.
  /// Any additional parameters are the ControlSignature arguments for the worklet.
  ///
  template <typename Worklet,
            typename T,
            typename... Args,
            typename std::enable_if<!detail::scatter_or_mask<T>::value, int>::type* = nullptr>
  inline void operator()(Worklet&& worklet, T&& t, Args&&... args) const
  {
    using WorkletType = viskores::internal::remove_cvref<Worklet>;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<T>(t), std::forward<Args>(args)...);
  }

  /// Get the device adapter that this Invoker is bound too
  ///
  viskores::cont::DeviceAdapterId GetDevice() const { return DeviceId; }

private:
  viskores::cont::DeviceAdapterId DeviceId;
};
}
}

#endif
