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
#ifndef viskores_cont_CellLocatorChooser_h
#define viskores_cont_CellLocatorChooser_h

#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename CoordinateSystemArrayType>
struct CellLocatorChooserImpl
{
  using type = viskores::cont::CellLocatorTwoLevel;
};

using UniformArray = viskores::cont::ArrayHandleUniformPointCoordinates;

template <>
struct CellLocatorChooserImpl<viskores::cont::CellSetStructured<3>, UniformArray>
{
  using type = viskores::cont::CellLocatorUniformGrid;
};

using RectilinearArray =
  viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>>;

template <>
struct CellLocatorChooserImpl<viskores::cont::CellSetStructured<3>, RectilinearArray>
{
  using type = viskores::cont::CellLocatorRectilinearGrid;
};

} // namespace detail

/// \brief A template to select an appropriate CellLocator based on CellSet type.
///
/// Given a concrete type for a `CellSet` subclass and a type of `ArrayHandle` for the
/// coordinate system, `CellLocatorChooser` picks an appropriate `CellLocator` for that
/// type of grid. It is a convenient class to use when you can resolve your templates
/// to discover the type of data set being used for location.
///
template <typename CellSetType, typename CoordinateSystemArrayType>
using CellLocatorChooser =
  typename detail::CellLocatorChooserImpl<CellSetType, CoordinateSystemArrayType>::type;

namespace detail
{

struct CastAndCallCellLocatorChooserFunctor
{
  template <typename CellLocatorType, typename Functor, typename... Args>
  void CallFunctorWithLocator(const viskores::cont::UnknownCellSet& cellSet,
                              const viskores::cont::CoordinateSystem& coordinateSystem,
                              Functor&& functor,
                              Args&&... args) const
  {
    CellLocatorType locator;
    locator.SetCellSet(cellSet);
    locator.SetCoordinates(coordinateSystem);

    functor(locator, std::forward<Args>(args)...);
  }

  template <typename CellSetType, typename Functor, typename... Args>
  void operator()(const CellSetType& cellSet,
                  const viskores::cont::CoordinateSystem& coordinateSystem,
                  Functor&& functor,
                  Args&&... args) const
  {
    this->CallFunctorWithLocator<viskores::cont::CellLocatorTwoLevel>(
      cellSet, coordinateSystem, std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  void operator()(const viskores::cont::CellSetStructured<3>& cellSet,
                  const viskores::cont::CoordinateSystem& coordinateSystem,
                  Functor&& functor,
                  Args&&... args) const
  {
    auto coordArray = coordinateSystem.GetData();
    if (coordArray.IsType<detail::UniformArray>())
    {
      this->CallFunctorWithLocator<viskores::cont::CellLocatorUniformGrid>(
        cellSet, coordinateSystem, std::forward<Functor>(functor), std::forward<Args>(args)...);
    }
    else if (coordArray.IsType<detail::RectilinearArray>())
    {
      this->CallFunctorWithLocator<viskores::cont::CellLocatorRectilinearGrid>(
        cellSet, coordinateSystem, std::forward<Functor>(functor), std::forward<Args>(args)...);
    }
    else
    {
      this->CallFunctorWithLocator<viskores::cont::CellLocatorTwoLevel>(
        cellSet, coordinateSystem, std::forward<Functor>(functor), std::forward<Args>(args)...);
    }
  }
};

} // namespace detail

/// \brief Calls a functor with the appropriate type of `CellLocator`.
///
/// Given a cell set and a coordinate system of unknown types, calls a functor with an appropriate
/// CellLocator of the given type. The CellLocator is populated with the provided cell set and
/// coordinate system.
///
/// Any additional args are passed to the functor.
///
template <typename CellSetType, typename Functor, typename... Args>
VISKORES_CONT void CastAndCallCellLocatorChooser(
  const CellSetType& cellSet,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  Functor&& functor,
  Args&&... args)
{
  viskores::cont::CastAndCall(cellSet,
                              detail::CastAndCallCellLocatorChooserFunctor{},
                              coordinateSystem,
                              std::forward<Functor>(functor),
                              std::forward<Args>(args)...);
}

/// \brief Calls a functor with the appropriate type of `CellLocator`.
///
/// Given a `DataSet`, calls a functor with an appropriate CellLocator of the given type.
/// The CellLocator is populated with the provided cell set and coordinate system.
///
/// Any additional args are passed to the functor.
///
template <typename Functor, typename... Args>
VISKORES_CONT void CastAndCallCellLocatorChooser(const viskores::cont::DataSet& dataSet,
                                                 Functor&& functor,
                                                 Args&&... args)
{
  CastAndCallCellLocatorChooser(dataSet.GetCellSet(),
                                dataSet.GetCoordinateSystem(),
                                std::forward<Functor>(functor),
                                std::forward<Args>(args)...);
}

}
} // namespace viskores::cont

#endif //viskores_cont_CellLocatorChooser_h
