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
#ifndef viskores_cont_arg_TransportTagWholeArrayInOut_h
#define viskores_cont_arg_TransportTagWholeArrayInOut_h

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

/// \brief \c Transport tag for in-place arrays with random access.
///
/// \c TransportTagWholeArrayInOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for data that is both input and output
/// (that is, in place modification of array data).
///
/// The worklet will have random access to the array through a portal
/// interface, but care should be taken to not write a value in one instance
/// that will be read by or overridden by another entry.
///
struct TransportTagWholeArrayInOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTagWholeArrayInOut, ContObjectType, Device>
{
  // MSVC will issue deprecation warnings here if this template is instantiated with
  // a deprecated class even if the template is used from a section of code where
  // deprecation warnings are suppressed. This is annoying behavior since this template
  // has no control over what class it is used with. To get around it, we have to
  // suppress all deprecation warnings here.
#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
#endif

  // If you get a compile error here, it means you tried to use an object that
  // is not an array handle as an argument that is expected to be one.
  VISKORES_IS_ARRAY_HANDLE(ContObjectType);

  using ValueType = typename ContObjectType::ValueType;
  using StorageTag = typename ContObjectType::StorageTag;

  using ExecObjectType = typename ContObjectType::WritePortalType;

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(ContObjectType& array,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.

    return array.PrepareForInPlace(Device{}, token);
  }

#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_END
#endif
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagWholeArrayInOut_h
