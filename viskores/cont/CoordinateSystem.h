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
#ifndef viskores_cont_CoordinateSystem_h
#define viskores_cont_CoordinateSystem_h

#include <viskores/Bounds.h>

#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UncertainArrayHandle.h>

namespace viskores
{
namespace cont
{

/// @brief Manages a coordinate system for a `DataSet`.
///
/// A coordinate system is really a field with a special meaning, so `CoordinateSystem`
/// class inherits from the `Field` class. `CoordinateSystem` constrains the field to
/// be associated with points and typically has 3D floating point vectors for values.
class VISKORES_CONT_EXPORT CoordinateSystem : public viskores::cont::Field
{
  using Superclass = viskores::cont::Field;

public:
  VISKORES_CONT
  CoordinateSystem();

  // It's OK for regular _point_ fields to become a CoordinateSystem object.
  VISKORES_CONT CoordinateSystem(const viskores::cont::Field& src);

  VISKORES_CONT CoordinateSystem(std::string name, const viskores::cont::UnknownArrayHandle& data);

  template <typename T, typename Storage>
  VISKORES_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data)
    : Superclass(name, Association::Points, data)
  {
  }

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  VISKORES_CONT
  CoordinateSystem(std::string name,
                   viskores::Id3 dimensions,
                   viskores::Vec3f origin = viskores::Vec3f(0.0f, 0.0f, 0.0f),
                   viskores::Vec3f spacing = viskores::Vec3f(1.0f, 1.0f, 1.0f));

  VISKORES_CONT
  viskores::Id GetNumberOfPoints() const { return this->GetNumberOfValues(); }

  VISKORES_CONT
  viskores::cont::UncertainArrayHandle<viskores::TypeListFieldVec3, VISKORES_DEFAULT_STORAGE_LIST>
  GetData() const;

private:
#ifdef VISKORES_USE_DOUBLE_PRECISION
  using FloatNonDefault = viskores::Float32;
#else
  using FloatNonDefault = viskores::Float64;
#endif
  using Vec3f_nd = viskores::Vec<FloatNonDefault, 3>;

  struct StorageToArrayDefault
  {
    template <typename S>
    using IsInvalid = viskores::cont::internal::IsInvalidArrayHandle<viskores::Vec3f, S>;

    template <typename S>
    using Transform = viskores::cont::ArrayHandle<viskores::Vec3f, S>;
  };

  struct StorageToArrayNonDefault
  {
    template <typename S>
    using IsInvalid = viskores::cont::internal::IsInvalidArrayHandle<Vec3f_nd, S>;

    template <typename S>
    using Transform =
      viskores::cont::ArrayHandleCast<viskores::Vec3f, viskores::cont::ArrayHandle<Vec3f_nd, S>>;
  };

  using ArraysFloatDefault = viskores::ListTransform<
    viskores::ListRemoveIf<VISKORES_DEFAULT_STORAGE_LIST, StorageToArrayDefault::IsInvalid>,
    StorageToArrayDefault::Transform>;
  using ArraysFloatNonDefault = viskores::ListTransform<
    viskores::ListRemoveIf<VISKORES_DEFAULT_STORAGE_LIST, StorageToArrayNonDefault::IsInvalid>,
    StorageToArrayNonDefault::Transform>;

public:
  using MultiplexerArrayType = //
    viskores::cont::ArrayHandleMultiplexerFromList<
      viskores::ListAppend<ArraysFloatDefault, ArraysFloatNonDefault>>;

  /// \brief Returns the data for the coordinate system as an `ArrayHandleMultiplexer`.
  ///
  /// This array will handle all potential types supported by CoordinateSystem, so all types can be
  /// handled with one compile pass. However, using this precludes specialization for special
  /// arrays such as `ArrayHandleUniformPointCoordinates` that could have optimized code paths
  ///
  VISKORES_CONT MultiplexerArrayType GetDataAsMultiplexer() const;

  VISKORES_CONT
  void GetRange(viskores::Range* range) const { this->Superclass::GetRange(range); }

  VISKORES_CONT
  viskores::Vec<viskores::Range, 3> GetRange() const
  {
    viskores::Vec<viskores::Range, 3> range;
    this->GetRange(&range[0]);
    return range;
  }

  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Range> GetRangeAsArrayHandle() const
  {
    return this->Superclass::GetRange();
  }

  VISKORES_CONT
  viskores::Bounds GetBounds() const
  {
    viskores::Range ranges[3];
    this->GetRange(ranges);
    return viskores::Bounds(ranges[0], ranges[1], ranges[2]);
  }

  void PrintSummary(std::ostream& out, bool full = false) const override;

  VISKORES_CONT void ReleaseResourcesExecution() override
  {
    this->Superclass::ReleaseResourcesExecution();
    this->GetData().ReleaseResourcesExecution();
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const viskores::cont::CoordinateSystem& coords, Functor&& f, Args&&... args)
{
  CastAndCall(coords.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}

template <typename T>
viskores::cont::CoordinateSystem make_CoordinateSystem(
  std::string name,
  const std::vector<T>& data,
  viskores::CopyFlag copy = viskores::CopyFlag::Off)
{
  return viskores::cont::CoordinateSystem(name, viskores::cont::make_ArrayHandle(data, copy));
}

template <typename T>
viskores::cont::CoordinateSystem make_CoordinateSystem(
  std::string name,
  const T* data,
  viskores::Id numberOfValues,
  viskores::CopyFlag copy = viskores::CopyFlag::Off)
{
  return viskores::cont::CoordinateSystem(
    name, viskores::cont::make_ArrayHandle(data, numberOfValues, copy));
}

namespace internal
{

template <>
struct DynamicTransformTraits<viskores::cont::CoordinateSystem>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};


} // namespace internal
} // namespace cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct Serialization<viskores::cont::CoordinateSystem> : Serialization<viskores::cont::Field>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_CoordinateSystem_h
