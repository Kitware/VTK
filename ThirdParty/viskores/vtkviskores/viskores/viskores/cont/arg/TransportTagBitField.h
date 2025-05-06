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
#ifndef viskores_cont_arg_TransportTagBitField_h
#define viskores_cont_arg_TransportTagBitField_h

#include <viskores/cont/arg/Transport.h>

#include <viskores/cont/BitField.h>

namespace viskores
{
namespace cont
{
namespace arg
{

struct TransportTagBitFieldIn
{
};
struct TransportTagBitFieldOut
{
};
struct TransportTagBitFieldInOut
{
};

template <typename Device>
struct Transport<viskores::cont::arg::TransportTagBitFieldIn, viskores::cont::BitField, Device>
{
  using ExecObjectType =
    typename viskores::cont::BitField::template ExecutionTypes<Device>::PortalConst;

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(viskores::cont::BitField& field,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    return field.PrepareForInput(Device{}, token);
  }
};

template <typename Device>
struct Transport<viskores::cont::arg::TransportTagBitFieldOut, viskores::cont::BitField, Device>
{
  using ExecObjectType = typename viskores::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(viskores::cont::BitField& field,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    // This behaves similarly to WholeArray tags, where "Out" maps to InPlace
    // since we don't want to reallocate or enforce size restrictions.
    return field.PrepareForInPlace(Device{}, token);
  }
};

template <typename Device>
struct Transport<viskores::cont::arg::TransportTagBitFieldInOut, viskores::cont::BitField, Device>
{
  using ExecObjectType = typename viskores::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(viskores::cont::BitField& field,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    return field.PrepareForInPlace(Device{}, token);
  }
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagBitField_h
