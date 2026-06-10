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

struct FindCellIdFunctor
{
  template <typename Locator>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               viskores::Id& cellId) const
  {
    return locator.FindCellId(point, cellId);
  }

  template <typename Locator, typename LastCell>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               viskores::Id& cellId,
                                               LastCell& lastCell) const
  {
    using ConcreteLastCell = typename std::decay_t<Locator>::LastCell;
    if (!lastCell.template IsType<ConcreteLastCell>())
    {
      lastCell = ConcreteLastCell{};
    }
    return locator.FindCellId(point, cellId, lastCell.template Get<ConcreteLastCell>());
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

struct FindAllCellIdsFunctor
{
  template <typename Locator, typename CellIdsType>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               CellIdsType& cellIds) const
  {
    return locator.FindAllCellIds(point, cellIds);
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

  /// @brief Locate the id of the cell containing the provided point.
  ///
  /// This method returns the same cell id as `FindCell()` without requiring
  /// the caller to provide parametric coordinates.
  VISKORES_EXEC viskores::ErrorCode FindCellId(const viskores::Vec3f& point,
                                               viskores::Id& cellId) const
  {
    return this->Locators.CastAndCall(detail::FindCellIdFunctor{}, point, cellId);
  }

  /// @brief Locate the id of the cell containing the provided point.
  ///
  /// This overload uses a `LastCell` cache that can accelerate successive
  /// searches for nearby points.
  VISKORES_EXEC viskores::ErrorCode FindCellId(const viskores::Vec3f& point,
                                               viskores::Id& cellId,
                                               LastCell& lastCell) const
  {
    return this->Locators.CastAndCall(detail::FindCellIdFunctor{}, point, cellId, lastCell);
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

  /// @brief Locate all cell ids containing the provided point.
  ///
  /// This method returns the same cell ids as `FindAllCells()` without
  /// requiring parametric coordinates.
  template <typename CellIdsType>
  VISKORES_EXEC viskores::ErrorCode FindAllCellIds(const viskores::Vec3f& point,
                                                   CellIdsType& cellIds) const
  {
    return this->Locators.CastAndCall(detail::FindAllCellIdsFunctor{}, point, cellIds);
  }
};

}
} // namespace viskores::exec

#endif //viskores_exec_CellLocatorMultiplexer_h
