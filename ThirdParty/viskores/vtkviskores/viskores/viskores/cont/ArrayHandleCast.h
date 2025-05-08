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
#ifndef viskores_cont_ArrayHandleCast_h
#define viskores_cont_ArrayHandleCast_h

#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/cont/Logging.h>

#include <viskores/Range.h>
#include <viskores/VecTraits.h>

#include <limits>

namespace viskores
{
namespace cont
{

template <typename SourceT, typename SourceStorage>
struct VISKORES_ALWAYS_EXPORT StorageTagCast
{
};

namespace internal
{

template <typename FromType, typename ToType>
struct VISKORES_ALWAYS_EXPORT Cast
{
// The following operator looks like it should never issue a cast warning because of
// the static_cast (and we don't want it to issue a warning). However, if ToType is
// an object that has a constructor that takes a value that FromType can be cast to,
// that cast can cause a warning. For example, if FromType is viskores::Float64 and ToType
// is viskores::Vec<viskores::Float32, 3>, the static_cast will first implicitly cast the
// Float64 to a Float32 (which causes a warning) before using the static_cast to
// construct the Vec with the Float64. The easiest way around the problem is to
// just disable all conversion warnings here. (The pragmas are taken from those
// used in Types.h for the VecBase class.)
#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif //not using cuda < 8
#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

  VISKORES_EXEC_CONT
  ToType operator()(const FromType& val) const { return static_cast<ToType>(val); }

#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
};

namespace detail
{

template <typename TargetT, typename SourceT, typename SourceStorage, bool... CastFlags>
struct ArrayHandleCastTraits;

template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>
  : ArrayHandleCastTraits<TargetT,
                          SourceT,
                          SourceStorage,
                          std::is_convertible<SourceT, TargetT>::value,
                          std::is_convertible<TargetT, SourceT>::value>
{
};

// Case where the forward cast is invalid, so this array is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage, bool CanCastBackward>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, false, CanCastBackward>
{
  struct StorageSuperclass : viskores::cont::internal::UndefinedStorage
  {
    using PortalType = viskores::cont::internal::detail::UndefinedArrayPortal<TargetT>;
    using PortalConstType = viskores::cont::internal::detail::UndefinedArrayPortal<TargetT>;
  };
};

// Case where the forward cast is valid but the backward cast is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, false>
{
  using StorageTagSuperclass =
    StorageTagTransform<viskores::cont::ArrayHandle<SourceT, SourceStorage>,
                        viskores::cont::internal::Cast<SourceT, TargetT>>;
  using StorageSuperclass = viskores::cont::internal::Storage<TargetT, StorageTagSuperclass>;
};

// Case where both forward and backward casts are valid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, true>
{
  using StorageTagSuperclass =
    StorageTagTransform<viskores::cont::ArrayHandle<SourceT, SourceStorage>,
                        viskores::cont::internal::Cast<SourceT, TargetT>,
                        viskores::cont::internal::Cast<TargetT, SourceT>>;
  using StorageSuperclass = viskores::cont::internal::Storage<TargetT, StorageTagSuperclass>;
};

} // namespace detail

template <typename TargetT, typename SourceT, typename SourceStorage_>
struct Storage<TargetT, viskores::cont::StorageTagCast<SourceT, SourceStorage_>>
  : detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage_>::StorageSuperclass
{
  using Superclass =
    typename detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage_>::StorageSuperclass;

  using Superclass::Superclass;
};

} // namespace internal

/// \brief Cast the values of an array to the specified type, on demand.
///
/// ArrayHandleCast is a specialization of ArrayHandleTransform. Given an ArrayHandle
/// and a type, it creates a new handle that returns the elements of the array cast
/// to the specified type.
///
template <typename T, typename ArrayHandleType>
class ArrayHandleCast
  : public viskores::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCast,
    (ArrayHandleCast<T, ArrayHandleType>),
    (viskores::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>));

  /// Construct an `ArrayHandleCast` from a source array handle.
  ArrayHandleCast(const viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                    typename ArrayHandleType::StorageTag>& handle)
    : Superclass(Superclass::StorageType::CreateBuffers(handle))
  {
    this->ValidateTypeCast<typename ArrayHandleType::ValueType>();
  }

  // Implemented so that it is defined exclusively in the control environment.
  // If there is a separate device for the execution environment (for example,
  // with CUDA), then the automatically generated destructor could be
  // created for all devices, and it would not be valid for all devices.
  ~ArrayHandleCast() {}

  /// \brief Returns the `ArrayHandle` that is being transformed.
  ArrayHandleType GetSourceArray() const
  {
    return Superclass::StorageType::GetArray(this->GetBuffers());
  }

private:
  // Log warnings if type cast is valid but lossy:
  template <typename SrcValueType>
  VISKORES_CONT static typename std::enable_if<!std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
#ifdef VISKORES_ENABLE_LOGGING
    using DstValueType = T;
    using SrcComp = typename viskores::VecTraits<SrcValueType>::BaseComponentType;
    using DstComp = typename viskores::VecTraits<DstValueType>::BaseComponentType;
    using SrcLimits = std::numeric_limits<SrcComp>;
    using DstLimits = std::numeric_limits<DstComp>;

    const viskores::Range SrcRange{ SrcLimits::lowest(), SrcLimits::max() };
    const viskores::Range DstRange{ DstLimits::lowest(), DstLimits::max() };

    const bool RangeLoss = (SrcRange.Max > DstRange.Max || SrcRange.Min < DstRange.Min);
    const bool PrecLoss = SrcLimits::digits > DstLimits::digits;

    if (RangeLoss && PrecLoss)
    {
      VISKORES_LOG_F(viskores::cont::LogLevel::Warn,
                     "ArrayHandleCast: Casting ComponentType of "
                     "%s to %s reduces range and precision.",
                     viskores::cont::TypeToString<SrcValueType>().c_str(),
                     viskores::cont::TypeToString<DstValueType>().c_str());
    }
    else if (RangeLoss)
    {
      VISKORES_LOG_F(viskores::cont::LogLevel::Warn,
                     "ArrayHandleCast: Casting ComponentType of "
                     "%s to %s reduces range.",
                     viskores::cont::TypeToString<SrcValueType>().c_str(),
                     viskores::cont::TypeToString<DstValueType>().c_str());
    }
    else if (PrecLoss)
    {
      VISKORES_LOG_F(viskores::cont::LogLevel::Warn,
                     "ArrayHandleCast: Casting ComponentType of "
                     "%s to %s reduces precision.",
                     viskores::cont::TypeToString<SrcValueType>().c_str(),
                     viskores::cont::TypeToString<DstValueType>().c_str());
    }
#endif // Logging
  }

  template <typename SrcValueType>
  VISKORES_CONT static typename std::enable_if<std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
    //no-op if types match
  }
};

namespace detail
{

template <typename CastType, typename OriginalType, typename ArrayType>
struct MakeArrayHandleCastImpl
{
  using ReturnType = viskores::cont::ArrayHandleCast<CastType, ArrayType>;

  VISKORES_CONT static ReturnType DoMake(const ArrayType& array) { return ReturnType(array); }
};

template <typename T, typename ArrayType>
struct MakeArrayHandleCastImpl<T, T, ArrayType>
{
  using ReturnType = ArrayType;

  VISKORES_CONT static ReturnType DoMake(const ArrayType& array) { return array; }
};

} // namespace detail

/// `make_ArrayHandleCast` is convenience function to generate an
/// ArrayHandleCast.
///
template <typename T, typename ArrayType>
VISKORES_CONT
  typename detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>::ReturnType
  make_ArrayHandleCast(const ArrayType& array, const T& = T())
{
  VISKORES_IS_ARRAY_HANDLE(ArrayType);
  using MakeImpl = detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>;
  return MakeImpl::DoMake(array);
}
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace viskores
{
namespace cont
{

template <typename T, typename AH>
struct SerializableTypeString<viskores::cont::ArrayHandleCast<T, AH>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name =
      "AH_Cast<" + SerializableTypeString<T>::Get() + "," + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename T1, typename T2, typename S>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<T1, viskores::cont::StorageTagCast<T2, S>>>
  : SerializableTypeString<viskores::cont::ArrayHandleCast<T1, viskores::cont::ArrayHandle<T2, S>>>
{
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <typename TargetT, typename SourceT, typename SourceStorage>
struct Serialization<
  viskores::cont::ArrayHandle<TargetT, viskores::cont::StorageTagCast<SourceT, SourceStorage>>>
{
private:
  using BaseType =
    viskores::cont::ArrayHandle<TargetT, viskores::cont::StorageTagCast<SourceT, SourceStorage>>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskores::cont::ArrayHandleCast<TargetT, viskores::cont::ArrayHandle<SourceT, SourceStorage>>
      castArray = obj;
    viskoresdiy::save(bb, castArray.GetSourceArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::cont::ArrayHandle<SourceT, SourceStorage> array;
    viskoresdiy::load(bb, array);
    obj = viskores::cont::make_ArrayHandleCast<TargetT>(array);
  }
};

template <typename TargetT, typename AH>
struct Serialization<viskores::cont::ArrayHandleCast<TargetT, AH>>
  : Serialization<viskores::cont::ArrayHandle<
      TargetT,
      viskores::cont::StorageTagCast<typename AH::ValueType, typename AH::StorageTag>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // viskores_cont_ArrayHandleCast_h
