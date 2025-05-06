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
#ifndef viskores_cont_ArrayHandleStride_h
#define viskores_cont_ArrayHandleStride_h

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/internal/ArrayCopyUnknown.h>

#include <viskores/internal/ArrayPortalBasic.h>

namespace viskores
{
namespace internal
{

struct ArrayStrideInfo
{
  viskores::Id NumberOfValues = 0;
  viskores::Id Stride = 1;
  viskores::Id Offset = 0;
  viskores::Id Modulo = 0;
  viskores::Id Divisor = 0;

  ArrayStrideInfo() = default;

  ArrayStrideInfo(viskores::Id numValues,
                  viskores::Id stride,
                  viskores::Id offset,
                  viskores::Id modulo,
                  viskores::Id divisor)
    : NumberOfValues(numValues)
    , Stride(stride)
    , Offset(offset)
    , Modulo(modulo)
    , Divisor(divisor)
  {
  }

  VISKORES_EXEC_CONT viskores::Id ArrayIndex(viskores::Id index) const
  {
    viskores::Id arrayIndex = index;
    if (this->Divisor > 1)
    {
      arrayIndex = arrayIndex / this->Divisor;
    }
    if (this->Modulo > 0)
    {
      arrayIndex = arrayIndex % this->Modulo;
    }
    arrayIndex = (arrayIndex * this->Stride) + this->Offset;
    return arrayIndex;
  }
};

template <typename T>
class ArrayPortalStrideRead
{
  const T* Array = nullptr;
  ArrayStrideInfo Info;

public:
  ArrayPortalStrideRead() = default;
  ArrayPortalStrideRead(ArrayPortalStrideRead&&) = default;
  ArrayPortalStrideRead(const ArrayPortalStrideRead&) = default;
  ArrayPortalStrideRead& operator=(ArrayPortalStrideRead&&) = default;
  ArrayPortalStrideRead& operator=(const ArrayPortalStrideRead&) = default;

  ArrayPortalStrideRead(const T* array, const ArrayStrideInfo& info)
    : Array(array)
    , Info(info)
  {
  }

  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->Info.NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    return detail::ArrayPortalBasicReadGet(this->Array + this->Info.ArrayIndex(index));
  }

  VISKORES_EXEC_CONT const ValueType* GetArray() const { return this->Array; }
  VISKORES_EXEC_CONT const ArrayStrideInfo& GetInfo() const { return this->Info; }
};

template <typename T>
class ArrayPortalStrideWrite
{
  T* Array = nullptr;
  ArrayStrideInfo Info;

public:
  ArrayPortalStrideWrite() = default;
  ArrayPortalStrideWrite(ArrayPortalStrideWrite&&) = default;
  ArrayPortalStrideWrite(const ArrayPortalStrideWrite&) = default;
  ArrayPortalStrideWrite& operator=(ArrayPortalStrideWrite&&) = default;
  ArrayPortalStrideWrite& operator=(const ArrayPortalStrideWrite&) = default;

  ArrayPortalStrideWrite(T* array, const ArrayStrideInfo& info)
    : Array(array)
    , Info(info)
  {
  }

  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->Info.NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    return detail::ArrayPortalBasicWriteGet(this->Array + this->Info.ArrayIndex(index));
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    detail::ArrayPortalBasicWriteSet(this->Array + this->Info.ArrayIndex(index), value);
  }

  VISKORES_EXEC_CONT ValueType* GetArray() const { return this->Array; }
  VISKORES_EXEC_CONT const ArrayStrideInfo& GetInfo() const { return this->Info; }
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagStride
{
};

namespace internal
{

template <typename T>
class VISKORES_ALWAYS_EXPORT Storage<T, viskores::cont::StorageTagStride>
{
  using StrideInfo = viskores::internal::ArrayStrideInfo;

public:
  using ReadPortalType = viskores::internal::ArrayPortalStrideRead<T>;
  using WritePortalType = viskores::internal::ArrayPortalStrideWrite<T>;

  VISKORES_CONT static StrideInfo& GetInfo(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<StrideInfo>();
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<T>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetInfo(buffers).NumberOfValues;
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    StrideInfo& info = GetInfo(buffers);

    if (info.NumberOfValues == numValues)
    {
      // Array resized to current size. Don't need to do anything.
      return;
    }

    // Find the end index after dealing with the divsor and modulo.
    auto lengthDivMod = [info](viskores::Id length) -> viskores::Id
    {
      viskores::Id resultLength = ((length - 1) / info.Divisor) + 1;
      if ((info.Modulo > 0) && (info.Modulo < resultLength))
      {
        resultLength = info.Modulo;
      }
      return resultLength;
    };
    viskores::Id lastStridedIndex = lengthDivMod(numValues);

    viskores::Id originalStride;
    viskores::Id originalOffset;
    if (info.Stride > 0)
    {
      originalStride = info.Stride;
      originalOffset = info.Offset;
    }
    else
    {
      // The stride is negative, which means we are counting backward. Here we have to be careful
      // about the offset, which should move to push to the end of the array. We also need to
      // be careful about multiplying by the stride.
      originalStride = -info.Stride;

      viskores::Id originalSize = lengthDivMod(info.NumberOfValues);

      // Because the stride is negative, we expect the offset to be at the end of the array.
      // We will call the "real" offset the distance from that end.
      originalOffset = originalSize - info.Offset - 1;
    }

    // If the offset is more than the stride, that means there are values skipped at the
    // beginning of the array, and it is impossible to know exactly how many. In this case,
    // we cannot know how to resize. (If this is an issue, we will have to change
    // `ArrayHandleStride` to take resizing parameters.)
    if (originalOffset >= originalStride)
    {
      if (numValues == 0)
      {
        // Array resized to zero. This can happen when releasing resources.
        // Should we try to clear out the buffers, or avoid that for messing up shared buffers?
        return;
      }
      throw viskores::cont::ErrorBadAllocation(
        "Cannot resize stride array with offset greater than stride (start of stride unknown).");
    }

    // lastIndex should be the index in the source array after each stride block. Assuming the
    // offset is inside the first stride, this should be the end of the array regardless of
    // offset.
    viskores::Id lastIndex = lastStridedIndex * originalStride;

    buffers[1].SetNumberOfBytes(
      viskores::internal::NumberOfValuesToNumberOfBytes<T>(lastIndex), preserve, token);
    info.NumberOfValues = numValues;

    if (info.Stride < 0)
    {
      // As described above, when the stride is negative, we are counting backward. This means
      // that the offset is actually relative to the end, so we need to adjust it to the new
      // end of the array.
      info.Offset = lastIndex - originalOffset - 1;
    }
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const T& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token);

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(reinterpret_cast<const T*>(buffers[1].ReadPointerDevice(device, token)),
                          GetInfo(buffers));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(reinterpret_cast<T*>(buffers[1].WritePointerDevice(device, token)),
                           GetInfo(buffers));
  }

  static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const viskores::cont::internal::Buffer& sourceBuffer = viskores::cont::internal::Buffer{},
    viskores::internal::ArrayStrideInfo&& info = viskores::internal::ArrayStrideInfo{})
  {
    return viskores::cont::internal::CreateBuffers(info, sourceBuffer);
  }

  static viskores::cont::ArrayHandleBasic<T> GetBasicArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>({ buffers[1] });
  }
};

} // namespace internal

/// \brief An `ArrayHandle` that accesses a basic array with strides and offsets.
///
/// `ArrayHandleStride` is a simple `ArrayHandle` that accesses data with a prescribed
/// stride and offset. You specify the stride and offset at construction. So when a portal
/// for this `ArrayHandle` `Get`s or `Set`s a value at a specific index, the value accessed
/// in the underlying C array is:
///
/// (index * stride) + offset
///
/// Optionally, you can also specify a modulo and divisor. If they are specified, the index
/// mangling becomes:
///
/// (((index / divisor) % modulo) * stride) + offset
///
/// You can "disable" any of the aforementioned operations by setting them to the following
/// values (most of which are arithmetic identities):
///
///   * stride: 1
///   * offset: 0
///   * modulo: 0
///   * divisor: 1
///
/// Note that all of these indices are referenced by the `ValueType` of the array. So, an
/// `ArrayHandleStride<viskores::Float32>` with an offset of 1 will actually offset by 4 bytes
/// (the size of a `viskores::Float32`).
///
/// `ArrayHandleStride` is used to provide a unified type for pulling a component out of
/// an `ArrayHandle`. This way, you can iterate over multiple components in an array without
/// having to implement a template instance for each vector size or representation.
///
template <typename T>
class VISKORES_ALWAYS_EXPORT ArrayHandleStride
  : public viskores::cont::ArrayHandle<T, viskores::cont::StorageTagStride>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleStride,
                                 (ArrayHandleStride<T>),
                                 (ArrayHandle<T, viskores::cont::StorageTagStride>));

  ArrayHandleStride(viskores::Id stride,
                    viskores::Id offset,
                    viskores::Id modulo = 0,
                    viskores::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        viskores::cont::internal::Buffer{},
        viskores::internal::ArrayStrideInfo(0, stride, offset, modulo, divisor)))
  {
  }

  /// @brief Construct an `ArrayHandleStride` from a basic array with specified access patterns.
  ArrayHandleStride(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& array,
                    viskores::Id numValues,
                    viskores::Id stride,
                    viskores::Id offset,
                    viskores::Id modulo = 0,
                    viskores::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        array.GetBuffers()[0],
        viskores::internal::ArrayStrideInfo(numValues, stride, offset, modulo, divisor)))
  {
  }

  ArrayHandleStride(const viskores::cont::internal::Buffer& buffer,
                    viskores::Id numValues,
                    viskores::Id stride,
                    viskores::Id offset,
                    viskores::Id modulo = 0,
                    viskores::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        buffer,
        viskores::internal::ArrayStrideInfo(numValues, stride, offset, modulo, divisor)))
  {
  }

  /// @brief Get the stride that values are accessed.
  ///
  /// The stride is the spacing between consecutive values. The stride is measured
  /// in terms of the number of values. A stride of 1 means a fully packed array.
  /// A stride of 2 means selecting every other values.
  viskores::Id GetStride() const { return StorageType::GetInfo(this->GetBuffers()).Stride; }

  /// @brief Get the offset to start reading values.
  ///
  /// The offset is the number of values to skip before the first value. The offset
  /// is measured in terms of the number of values. An offset of 0 means the first value
  /// at the beginning of the array.
  ///
  /// The offset is unaffected by the stride and dictates where the strides starts
  /// counting. For example, given an array with size 3 vectors packed into an array,
  /// a strided array referencing the middle component will have offset 1 and stride 3.
  viskores::Id GetOffset() const { return StorageType::GetInfo(this->GetBuffers()).Offset; }

  /// @brief Get the modulus of the array index.
  ///
  /// When the index is modulo a value, it becomes the remainder after dividing by that
  /// value. The effect of the modulus is to cause the index to repeat over the values
  /// in the array.
  ///
  /// If the modulo is set to 0, then it is ignored.
  viskores::Id GetModulo() const { return StorageType::GetInfo(this->GetBuffers()).Modulo; }

  /// @brief Get the divisor of the array index.
  ///
  /// The index is divided by the divisor before the other effects. The default divisor of
  /// 1 will have no effect on the indexing. Setting the divisor to a value greater than 1
  /// has the effect of repeating each value that many times.
  viskores::Id GetDivisor() const { return StorageType::GetInfo(this->GetBuffers()).Divisor; }

  /// @brief Return the underlying data as a basic array handle.
  ///
  /// It is common for the same basic array to be shared among multiple
  /// `viskores::cont::ArrayHandleStride` objects.
  viskores::cont::ArrayHandleBasic<T> GetBasicArray() const
  {
    return StorageType::GetBasicArray(this->GetBuffers());
  }
};

/// @brief Create an array by adding a stride to a basic array.
///
template <typename T>
viskores::cont::ArrayHandleStride<T> make_ArrayHandleStride(
  const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& array,
  viskores::Id numValues,
  viskores::Id stride,
  viskores::Id offset,
  viskores::Id modulo = 0,
  viskores::Id divisor = 1)
{
  return { array, numValues, stride, offset, modulo, divisor };
}

}
} // namespace viskores::cont

namespace viskores
{
namespace cont
{
namespace internal
{

template <typename T>
VISKORES_CONT inline void Storage<T, viskores::cont::StorageTagStride>::Fill(
  const std::vector<viskores::cont::internal::Buffer>& buffers,
  const T& fillValue,
  viskores::Id startIndex,
  viskores::Id endIndex,
  viskores::cont::Token& token)
{
  const StrideInfo& info = GetInfo(buffers);
  viskores::cont::ArrayHandleBasic<T> basicArray = GetBasicArray(buffers);
  if ((info.Stride == 1) && (info.Modulo == 0) && (info.Divisor <= 1))
  {
    // Standard stride in array allows directly calling fill on the basic array.
    basicArray.Fill(fillValue, startIndex + info.Offset, endIndex + info.Offset, token);
  }
  else
  {
    // The fill does not necessarily cover a contiguous region. We have to have a loop
    // to set it. But we are not allowed to write device code here. Instead, create
    // a stride array containing the fill value with a modulo of 1 so that this fill
    // value repeates. Then feed this into a precompiled array copy that supports
    // stride arrays.
    const viskores::Id numFill = endIndex - startIndex;
    auto fillValueArray = viskores::cont::make_ArrayHandle({ fillValue });
    viskores::cont::ArrayHandleStride<T> constantArray(fillValueArray, numFill, 1, 0, 1, 1);
    viskores::cont::ArrayHandleStride<T> outputView(GetBasicArray(buffers),
                                                    numFill,
                                                    info.Stride,
                                                    info.ArrayIndex(startIndex),
                                                    info.Modulo,
                                                    info.Divisor);
    // To prevent circular dependencies, this header file does not actually include
    // UnknownArrayHandle.h. Thus, it is possible to get a compile error on the following
    // line for using a declared but not defined `UnknownArrayHandle`. In the unlikely
    // event this occurs, simply include `viskores/cont/UnknownArrayHandle.h` somewhere up the
    // include chain.
    viskores::cont::internal::ArrayCopyUnknown(constantArray, outputView);
  }
}

}
}
} // namespace viskores::cont::internal

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleStride<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AHStride<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagStride>>
  : SerializableTypeString<viskores::cont::ArrayHandleStride<T>>
{
};

}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleStride<T>>
{
private:
  using BaseType = viskores::cont::ArrayHandle<T, viskores::cont::StorageTagStride>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj_)
  {
    viskores::cont::ArrayHandleStride<T> obj = obj_;
    viskoresdiy::save(bb, obj.GetNumberOfValues());
    viskoresdiy::save(bb, obj.GetStride());
    viskoresdiy::save(bb, obj.GetOffset());
    viskoresdiy::save(bb, obj.GetModulo());
    viskoresdiy::save(bb, obj.GetDivisor());
    viskoresdiy::save(bb, obj.GetBuffers()[1]);
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Id numValues;
    viskores::Id stride;
    viskores::Id offset;
    viskores::Id modulo;
    viskores::Id divisor;
    viskores::cont::internal::Buffer buffer;

    viskoresdiy::load(bb, numValues);
    viskoresdiy::load(bb, stride);
    viskoresdiy::load(bb, offset);
    viskoresdiy::load(bb, modulo);
    viskoresdiy::load(bb, divisor);
    viskoresdiy::load(bb, buffer);

    obj = viskores::cont::ArrayHandleStride<T>(buffer, stride, offset, modulo, divisor);
  }
};

} // namespace diy
/// @endcond SERIALIZATION

/// \cond
/// Make doxygen ignore this section
#ifndef viskores_cont_ArrayHandleStride_cxx

namespace viskores
{
namespace cont
{

namespace internal
{

extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<char, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Int8, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::UInt8, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Int16, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::UInt16, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Int32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::UInt32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Int64, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::UInt64, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Float32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<viskores::Float64, StorageTagStride>;

} // namespace internal

extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<char, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::Int8, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::UInt8, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::Int16, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::UInt16, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::Int32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::UInt32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::Int64, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<viskores::UInt64, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT
  ArrayHandle<viskores::Float32, StorageTagStride>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT
  ArrayHandle<viskores::Float64, StorageTagStride>;

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayHandleStride_cxx
/// \endcond

#endif //viskores_cont_ArrayHandleStride_h
