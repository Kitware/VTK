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
#ifndef viskores_exec_CellLocatorMultiplexer_h
#define viskores_exec_CellLocatorMultiplexer_h

#include <viskores/ErrorCode.h>
#include <viskores/TypeList.h>

#include <viskores/exec/Variant.h>

namespace viskores
{
namespace exec
{

namespace detail
{

struct FindCellFunctor
{
  template <typename Locator>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               viskores::Id& cellId,
                                               viskores::Vec3f& pCoords) const
  {
    return locator.FindCell(point, cellId, pCoords);
  }

  template <typename Locator, typename LastCell>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               viskores::Id& cellId,
                                               viskores::Vec3f& pCoords,
                                               LastCell& lastCell) const
  {
    using ConcreteLastCell = typename std::decay_t<Locator>::LastCell;
    if (!lastCell.template IsType<ConcreteLastCell>())
    {
      lastCell = ConcreteLastCell{};
    }
    return locator.FindCell(point, cellId, pCoords, lastCell.template Get<ConcreteLastCell>());
  }
};

struct CountAllCellsFunctor
{
  template <typename Locator>
  VISKORES_EXEC viskores::IdComponent operator()(Locator&& locator,
                                                 const viskores::Vec3f& point) const
  {
    return locator.CountAllCells(point);
  }
};

struct FindAllCellsFunctor
{
  template <typename Locator, typename CellIdsType, typename ParametricVecType>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               CellIdsType& cellIds,
                                               ParametricVecType& pCoords) const
  {
    return locator.FindAllCells(point, cellIds, pCoords);
  }
};

} // namespace detail

template <typename... LocatorTypes>
class VISKORES_ALWAYS_EXPORT CellLocatorMultiplexer
{
  viskores::exec::Variant<LocatorTypes...> Locators;

public:
  CellLocatorMultiplexer() = default;

  using LastCell = viskores::exec::Variant<typename LocatorTypes::LastCell...>;

  template <typename Locator>
  VISKORES_CONT CellLocatorMultiplexer(const Locator& locator)
    : Locators(locator)
  {
  }

  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& pCoords) const
  {
    return this->Locators.CastAndCall(detail::FindCellFunctor{}, point, cellId, pCoords);
  }

  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& pCoords,
                                             LastCell& lastCell) const
  {
    return this->Locators.CastAndCall(detail::FindCellFunctor{}, point, cellId, pCoords, lastCell);
  }

  VISKORES_EXEC viskores::Id CountAllCells(const viskores::Vec3f& point) const
  {
    return this->Locators.CastAndCall(detail::CountAllCellsFunctor{}, point);
  }

  template <typename CellIdsType, typename ParametricVecType>
  VISKORES_EXEC viskores::ErrorCode FindAllCells(const viskores::Vec3f& point,
                                                 CellIdsType& cellIds,
                                                 ParametricVecType& pCoords) const
  {
    return this->Locators.CastAndCall(detail::FindAllCellsFunctor{}, point, cellIds, pCoords);
  }
};

}
} // namespace viskores::exec

#endif //viskores_exec_CellLocatorMultiplexer_h
