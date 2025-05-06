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
#ifndef viskores_cont_arg_TransportTagArrayIn_h
#define viskores_cont_arg_TransportTagArrayIn_h

#include <viskores/Deprecated.h>
#include <viskores/Types.h>

#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
struct TransportTagArrayIn
{
};

template <typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTagArrayIn, ContObjectType, Device>
{
  // MSVC will issue deprecation warnings here if this template is instantiated with
  // a deprecated class even if the template is used from a section of code where
  // deprecation warnings are suppressed. This is annoying behavior since this template
  // has no control over what class it is used with. To get around it, we have to
  // suppress all deprecation warnings here.
#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
#endif

  VISKORES_IS_ARRAY_HANDLE(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(
    Device(),
    std::declval<viskores::cont::Token&>()));

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const InputDomainType& viskoresNotUsed(inputDomain),
                                          viskores::Id inputRange,
                                          viskores::Id viskoresNotUsed(outputRange),
                                          viskores::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != inputRange)
    {
      throw viskores::cont::ErrorBadValue("Input array to worklet invocation the wrong size.");
    }

    return object.PrepareForInput(Device(), token);
  }

#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_END
#endif
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagArrayIn_h
