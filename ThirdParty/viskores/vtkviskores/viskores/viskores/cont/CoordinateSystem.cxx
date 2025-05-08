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

#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ErrorBadValue.h>

namespace viskores
{
namespace cont
{

VISKORES_CONT CoordinateSystem::CoordinateSystem()
  : Superclass()
{
}

VISKORES_CONT CoordinateSystem::CoordinateSystem(const viskores::cont::Field& src)
  : Superclass(src)
{
  if (src.GetAssociation() != viskores::cont::Field::Association::Points)
  {
    throw viskores::cont::ErrorBadValue("CoordinateSystems can only be point field.");
  }
}

VISKORES_CONT CoordinateSystem::CoordinateSystem(std::string name,
                                                 const viskores::cont::UnknownArrayHandle& data)
  : Superclass(name, Association::Points, data)
{
}

/// This constructor of coordinate system sets up a regular grid of points.
///
VISKORES_CONT
CoordinateSystem::CoordinateSystem(std::string name,
                                   viskores::Id3 dimensions,
                                   viskores::Vec3f origin,
                                   viskores::Vec3f spacing)
  : Superclass(name,
               Association::Points,
               viskores::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing))
{
}

VISKORES_CONT
viskores::cont::UncertainArrayHandle<viskores::TypeListFieldVec3, VISKORES_DEFAULT_STORAGE_LIST>
CoordinateSystem::GetData() const
{
  return viskores::cont::UncertainArrayHandle<viskores::TypeListFieldVec3,
                                              VISKORES_DEFAULT_STORAGE_LIST>(
    this->Superclass::GetData());
}


VISKORES_CONT viskores::cont::CoordinateSystem::MultiplexerArrayType
CoordinateSystem::GetDataAsMultiplexer() const
{
  return this->GetData().AsArrayHandle<MultiplexerArrayType>();
}

VISKORES_CONT
void CoordinateSystem::PrintSummary(std::ostream& out, bool full) const
{
  out << "    Coordinate System ";
  this->Superclass::PrintSummary(out, full);
}

template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<viskores::Vec<float, 3>>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<viskores::Vec<double, 3>>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f,
    viskores::cont::StorageTagImplicit<viskores::internal::ArrayPortalUniformPointCoordinates>>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_32,
    viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic>>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_64,
    viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic>>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_32,
    typename viskores::cont::ArrayHandleCompositeVector<
      viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Float32,
                                  viskores::cont::StorageTagBasic>>::StorageTag>&);
template VISKORES_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_64,
    typename viskores::cont::ArrayHandleCompositeVector<
      viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Float64,
                                  viskores::cont::StorageTagBasic>>::StorageTag>&);
}
} // namespace viskores::cont
