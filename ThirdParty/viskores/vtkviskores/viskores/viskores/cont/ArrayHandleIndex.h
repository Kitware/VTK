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
#ifndef viskores_cont_ArrayHandleIndex_h
#define viskores_cont_ArrayHandleIndex_h

#include <viskores/Range.h>
#include <viskores/cont/ArrayHandleImplicit.h>

namespace viskores
{

namespace internal
{

struct VISKORES_ALWAYS_EXPORT IndexFunctor
{
  VISKORES_EXEC_CONT viskores::Id operator()(viskores::Id index) const { return index; }
};

} // namespace internal

namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagIndex
{
};

namespace internal
{

using StorageTagIndexSuperclass =
  typename viskores::cont::ArrayHandleImplicit<viskores::internal::IndexFunctor>::StorageTag;

template <>
struct Storage<viskores::Id, viskores::cont::StorageTagIndex>
  : Storage<viskores::Id, StorageTagIndexSuperclass>
{
};

} // namespace internal

/// \brief An implicit array handle containing the its own indices.
///
/// \c ArrayHandleIndex is an implicit array handle containing the values
/// 0, 1, 2, 3,... to a specified size. Every value in the array is the same
/// as the index to that value.
///
class ArrayHandleIndex : public viskores::cont::ArrayHandle<viskores::Id, StorageTagIndex>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleIndex,
                                    (viskores::cont::ArrayHandle<viskores::Id, StorageTagIndex>));

  /// Construct an index array containing values from 0 to `length` - 1.
  VISKORES_CONT
  ArrayHandleIndex(viskores::Id length)
    : Superclass(
        internal::FunctorToArrayHandleImplicitBuffers(viskores::internal::IndexFunctor{}, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleIndex. It takes the
/// size of the array and generates an array holding viskores::Id from [0, size - 1]
VISKORES_CONT inline viskores::cont::ArrayHandleIndex make_ArrayHandleIndex(viskores::Id length)
{
  return viskores::cont::ArrayHandleIndex(length);
}

namespace internal
{

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::StorageTagIndex>
{
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const;
};

template <typename S>
struct ArrayRangeComputeMagnitudeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeMagnitudeImpl<viskores::cont::StorageTagIndex>
{
  VISKORES_CONT viskores::Range operator()(
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const
  {
    auto rangeAH = ArrayRangeComputeImpl<viskores::cont::StorageTagIndex>{}(
      input, maskArray, computeFiniteRange, device);
    return rangeAH.ReadPortal().Get(0);
  }
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
struct SerializableTypeString<viskores::cont::ArrayHandleIndex>
{
  static VISKORES_CONT std::string Get() { return "AH_Index"; }
};

template <>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>>
  : SerializableTypeString<viskores::cont::ArrayHandleIndex>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<viskores::cont::ArrayHandleIndex>
{
private:
  using BaseType = viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, obj.GetNumberOfValues());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Id length = 0;
    viskoresdiy::load(bb, length);

    obj = viskores::cont::ArrayHandleIndex(length);
  }
};

template <>
struct Serialization<viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>>
  : Serialization<viskores::cont::ArrayHandleIndex>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleIndex_h
