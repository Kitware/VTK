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
                                               viskores::Vec3f& parametric) const
  {
    return locator.FindCell(point, cellId, parametric);
  }

  template <typename Locator, typename LastCell>
  VISKORES_EXEC viskores::ErrorCode operator()(Locator&& locator,
                                               const viskores::Vec3f& point,
                                               viskores::Id& cellId,
                                               viskores::Vec3f& parametric,
                                               LastCell& lastCell) const
  {
    using ConcreteLastCell = typename std::decay_t<Locator>::LastCell;
    if (!lastCell.template IsType<ConcreteLastCell>())
    {
      lastCell = ConcreteLastCell{};
    }
    return locator.FindCell(point, cellId, parametric, lastCell.template Get<ConcreteLastCell>());
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
                                             viskores::Vec3f& parametric) const
  {
    return this->Locators.CastAndCall(detail::FindCellFunctor{}, point, cellId, parametric);
  }

  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& parametric,
                                             LastCell& lastCell) const
  {
    return this->Locators.CastAndCall(
      detail::FindCellFunctor{}, point, cellId, parametric, lastCell);
  }
};

}
} // namespace viskores::exec

#endif //viskores_exec_CellLocatorMultiplexer_h
