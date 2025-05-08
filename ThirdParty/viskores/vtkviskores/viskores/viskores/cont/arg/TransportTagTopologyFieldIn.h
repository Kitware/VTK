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
#ifndef viskores_cont_arg_TransportTagTopologyFieldIn_h
#define viskores_cont_arg_TransportTagTopologyFieldIn_h

#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSet.h>

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays in topology maps.
///
/// \c TransportTagTopologyFieldIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data. The transport is templated
/// on a topology element tag and expects a cell set input domain to check the
/// size of the input array.
///
template <typename TopologyElementTag>
struct TransportTagTopologyFieldIn
{
};

namespace detail
{

VISKORES_CONT
inline static viskores::Id TopologyDomainSize(const viskores::cont::CellSet& cellSet,
                                              viskores::TopologyElementTagPoint)
{
  return cellSet.GetNumberOfPoints();
}

VISKORES_CONT
inline static viskores::Id TopologyDomainSize(const viskores::cont::CellSet& cellSet,
                                              viskores::TopologyElementTagCell)
{
  return cellSet.GetNumberOfCells();
}

VISKORES_CONT
inline static viskores::Id TopologyDomainSize(const viskores::cont::CellSet& cellSet,
                                              viskores::TopologyElementTagFace)
{
  return cellSet.GetNumberOfFaces();
}

VISKORES_CONT
inline static viskores::Id TopologyDomainSize(const viskores::cont::CellSet& cellSet,
                                              viskores::TopologyElementTagEdge)
{
  return cellSet.GetNumberOfEdges();
}

} // namespace detail

template <typename TopologyElementTag, typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTagTopologyFieldIn<TopologyElementTag>,
                 ContObjectType,
                 Device>
{
  VISKORES_IS_ARRAY_HANDLE(ContObjectType);


  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(
    Device(),
    std::declval<viskores::cont::Token&>()));

  VISKORES_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const viskores::cont::CellSet& inputDomain,
                            viskores::Id,
                            viskores::Id,
                            viskores::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != detail::TopologyDomainSize(inputDomain, TopologyElementTag()))
    {
      throw viskores::cont::ErrorBadValue("Input array to worklet invocation the wrong size.");
    }

    return object.PrepareForInput(Device(), token);
  }
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagTopologyFieldIn_h
