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
#include <viskores/cont/CellLocatorGeneral.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/CellSetStructured.h>

namespace
{

template <typename LocatorImplType, typename LocatorVariantType>
void BuildForType(viskores::cont::CellLocatorGeneral& locator, LocatorVariantType& locatorVariant)
{
  constexpr viskores::IdComponent LOCATOR_INDEX =
    LocatorVariantType::template GetIndexOf<LocatorImplType>();
  if (locatorVariant.GetIndex() != LOCATOR_INDEX)
  {
    locatorVariant = LocatorImplType{};
  }
  LocatorImplType& locatorImpl = locatorVariant.template Get<LOCATOR_INDEX>();
  locatorImpl.SetCellSet(locator.GetCellSet());
  locatorImpl.SetCoordinates(locator.GetCoordinates());
  locatorImpl.Update();
}

} // anonymous namespace

namespace viskores
{
namespace cont
{

VISKORES_CONT void CellLocatorGeneral::Build()
{
  using StructuredCellSet = viskores::cont::CellSetStructured<3>;
  using UniformCoordinates = viskores::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordinates = viskores::cont::ArrayHandleCartesianProduct<
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>>;

  viskores::cont::UnknownCellSet cellSet = this->GetCellSet();
  viskores::cont::CoordinateSystem coords = this->GetCoordinates();

  if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<UniformCoordinates>())
  {
    BuildForType<viskores::cont::CellLocatorUniformGrid>(*this, this->LocatorImpl);
  }
  else if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<RectilinearCoordinates>())
  {
    BuildForType<viskores::cont::CellLocatorRectilinearGrid>(*this, this->LocatorImpl);
  }
  else
  {
    BuildForType<viskores::cont::CellLocatorTwoLevel>(*this, this->LocatorImpl);
  }
}

struct CellLocatorGeneral::PrepareFunctor
{
  template <typename LocatorType>
  ExecObjType operator()(LocatorType&& locator,
                         viskores::cont::DeviceAdapterId device,
                         viskores::cont::Token& token) const
  {
    return locator.PrepareForExecution(device, token);
  }
};

CellLocatorGeneral::ExecObjType CellLocatorGeneral::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  this->Update();
  return this->LocatorImpl.CastAndCall(PrepareFunctor{}, device, token);
}

}
} // viskores::cont
