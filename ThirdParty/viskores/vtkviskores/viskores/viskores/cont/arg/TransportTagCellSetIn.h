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
#ifndef viskores_cont_arg_TransportTagCellSetIn_h
#define viskores_cont_arg_TransportTagCellSetIn_h

#include <viskores/Types.h>

#include <viskores/cont/CellSet.h>

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagCellSetIn is a tag used with the \c Transport class to
/// transport topology objects for input data.
///
template <typename VisitTopology, typename IncidentTopology>
struct TransportTagCellSetIn
{
};

template <typename VisitTopology,
          typename IncidentTopology,
          typename ContObjectType,
          typename Device>
struct Transport<viskores::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
                 ContObjectType,
                 Device>
{
  VISKORES_IS_CELL_SET(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(
    Device(),
    VisitTopology(),
    IncidentTopology(),
    std::declval<viskores::cont::Token&>()));

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    return object.PrepareForInput(Device(), VisitTopology(), IncidentTopology(), token);
  }
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagCellSetIn_h
