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
#ifndef viskores_cont_ArrayHandleUniformPointCoordinates_h
#define viskores_cont_ArrayHandleUniformPointCoordinates_h

#include <viskores/Range.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/internal/ArrayPortalUniformPointCoordinates.h>

namespace viskores
{
namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagUniformPoints
{
};

namespace internal
{

using StorageTagUniformPointsSuperclass =
  viskores::cont::StorageTagImplicit<viskores::internal::ArrayPortalUniformPointCoordinates>;

template <>
struct Storage<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>
  : Storage<viskores::Vec3f, StorageTagUniformPointsSuperclass>
{
};

} // namespace internal

/// ArrayHandleUniformPointCoordinates is a specialization of ArrayHandle. It
/// contains the information necessary to compute the point coordinates in a
/// uniform orthogonal grid (extent, origin, and spacing) and implicitly
/// computes these coordinates in its array portal.
///
class VISKORES_CONT_EXPORT ArrayHandleUniformPointCoordinates
  : public viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS_NT(
    ArrayHandleUniformPointCoordinates,
    (viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>));

  /// Create an `ArrayHandleUniformPointCoordinates` with the given specifications.
  VISKORES_CONT
  ArrayHandleUniformPointCoordinates(viskores::Id3 dimensions,
                                     ValueType origin = ValueType(0.0f, 0.0f, 0.0f),
                                     ValueType spacing = ValueType(1.0f, 1.0f, 1.0f));

  // Implemented so that it is defined exclusively in the control environment.
  // If there is a separate device for the execution environment (for example,
  // with CUDA), then the automatically generated destructor could be
  // created for all devices, and it would not be valid for all devices.
  ~ArrayHandleUniformPointCoordinates();

  /// Get the number of points of the uniform grid in the x, y, and z directions.
  VISKORES_CONT viskores::Id3 GetDimensions() const;
  /// Get the coordinates of the "lower-left" cornder of the mesh.
  VISKORES_CONT viskores::Vec3f GetOrigin() const;
  /// Get the spacing between points of the grid in the x, y, and z directions.
  VISKORES_CONT viskores::Vec3f GetSpacing() const;
};

template <typename T>
class ArrayHandleStride;

namespace internal
{

template <typename S>
struct ArrayExtractComponentImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayExtractComponentImpl<viskores::cont::StorageTagUniformPoints>
{
  viskores::cont::ArrayHandleStride<viskores::FloatDefault> operator()(
    const viskores::cont::ArrayHandleUniformPointCoordinates& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const;
};

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::StorageTagUniformPoints>
{
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandleUniformPointCoordinates& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const;
};

} // namespace internal

}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <>
struct SerializableTypeString<viskores::cont::ArrayHandleUniformPointCoordinates>
{
  static VISKORES_CONT std::string Get() { return "AH_UniformPointCoordinates"; }
};

template <>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>>
  : SerializableTypeString<viskores::cont::ArrayHandleUniformPointCoordinates>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<viskores::cont::ArrayHandleUniformPointCoordinates>
{
private:
  using Type = viskores::cont::ArrayHandleUniformPointCoordinates;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.ReadPortal();
    viskoresdiy::save(bb, portal.GetDimensions());
    viskoresdiy::save(bb, portal.GetOrigin());
    viskoresdiy::save(bb, portal.GetSpacing());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Id3 dims;
    typename BaseType::ValueType origin, spacing;

    viskoresdiy::load(bb, dims);
    viskoresdiy::load(bb, origin);
    viskoresdiy::load(bb, spacing);

    obj = viskores::cont::ArrayHandleUniformPointCoordinates(dims, origin, spacing);
  }
};

template <>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>>
  : Serialization<viskores::cont::ArrayHandleUniformPointCoordinates>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_+m_cont_ArrayHandleUniformPointCoordinates_h
