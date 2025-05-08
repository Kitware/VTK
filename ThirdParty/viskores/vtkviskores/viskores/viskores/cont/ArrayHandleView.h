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
#ifndef viskores_cont_ArrayHandleView_h
#define viskores_cont_ArrayHandleView_h

#include <viskores/Assert.h>

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayPortal.h>

namespace viskores
{

namespace internal
{

struct ViewIndices
{
  viskores::Id StartIndex = 0;
  viskores::Id NumberOfValues = 0;

  ViewIndices() = default;

  ViewIndices(viskores::Id start, viskores::Id numValues)
    : StartIndex(start)
    , NumberOfValues(numValues)
  {
  }
};

template <typename TargetPortalType>
class ArrayPortalView
{
  using Writable = viskores::internal::PortalSupportsSets<TargetPortalType>;

public:
  using ValueType = typename TargetPortalType::ValueType;

  VISKORES_EXEC_CONT
  ArrayPortalView() {}

  VISKORES_EXEC_CONT
  ArrayPortalView(const TargetPortalType& targetPortal, ViewIndices indices)
    : TargetPortal(targetPortal)
    , Indices(indices)
  {
  }

  template <typename OtherPortalType>
  VISKORES_EXEC_CONT ArrayPortalView(const ArrayPortalView<OtherPortalType>& otherPortal)
    : TargetPortal(otherPortal.GetTargetPortal())
    , Indices(otherPortal.GetStartIndex(), otherPortal.GetNumberOfValues())
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->Indices.NumberOfValues; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    return this->TargetPortal.Get(index + this->GetStartIndex());
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    this->TargetPortal.Set(index + this->GetStartIndex(), value);
  }

  VISKORES_EXEC_CONT
  const TargetPortalType& GetTargetPortal() const { return this->TargetPortal; }
  VISKORES_EXEC_CONT
  viskores::Id GetStartIndex() const { return this->Indices.StartIndex; }

private:
  TargetPortalType TargetPortal;
  ViewIndices Indices;
};

} // namespace internal

namespace cont
{

template <typename StorageTag>
struct VISKORES_ALWAYS_EXPORT StorageTagView
{
};

namespace internal
{

template <typename T, typename ST>
class Storage<T, StorageTagView<ST>>
{
  using ArrayHandleType = viskores::cont::ArrayHandle<T, ST>;
  using SourceStorage = Storage<T, ST>;

  static std::vector<viskores::cont::internal::Buffer> SourceBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  VISKORES_STORAGE_NO_RESIZE;

  using ReadPortalType =
    viskores::internal::ArrayPortalView<typename ArrayHandleType::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalView<typename ArrayHandleType::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfComponentsFlat(SourceBuffers(buffers));
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<viskores::internal::ViewIndices>().NumberOfValues;
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::internal::ViewIndices indices =
      buffers[0].GetMetaData<viskores::internal::ViewIndices>();
    return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                          indices);
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const T& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    viskores::internal::ViewIndices indices =
      buffers[0].GetMetaData<viskores::internal::ViewIndices>();
    viskores::Id adjustedStartIndex = startIndex + indices.StartIndex;
    viskores::Id adjustedEndIndex = (endIndex < indices.NumberOfValues)
      ? endIndex + indices.StartIndex
      : indices.NumberOfValues + indices.StartIndex;
    SourceStorage::Fill(
      SourceBuffers(buffers), fillValue, adjustedStartIndex, adjustedEndIndex, token);
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::internal::ViewIndices indices =
      buffers[0].GetMetaData<viskores::internal::ViewIndices>();
    return WritePortalType(SourceStorage::CreateWritePortal(SourceBuffers(buffers), device, token),
                           indices);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    viskores::Id startIndex = 0,
    viskores::Id numValues = 0,
    const ArrayHandleType& array = ArrayHandleType{})
  {
    return viskores::cont::internal::CreateBuffers(
      viskores::internal::ViewIndices(startIndex, numValues), array);
  }

  VISKORES_CONT static ArrayHandleType GetSourceArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ArrayHandleType(SourceBuffers(buffers));
  }

  VISKORES_CONT static viskores::Id GetStartIndex(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<viskores::internal::ViewIndices>().StartIndex;
  }
};

} // namespace internal

/// @brief Provided a windowed view into a `viskores::cont::ArrayHandle`.
///
/// `ArrayHandleView` is a fancy array that wraps around another `ArrayHandle`
/// and reindexes it to provide access to a specified region of values in the
/// array. This view is specified using the offset to the first index and the
/// length of the entries to view.
template <typename ArrayHandleType>
class ArrayHandleView
  : public viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                       StorageTagView<typename ArrayHandleType::StorageTag>>
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleView,
    (ArrayHandleView<ArrayHandleType>),
    (viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                 StorageTagView<typename ArrayHandleType::StorageTag>>));

  /// Create an `ArrayHandleView` over a provided source array.
  ///
  /// @param array The source array to create a view from.
  /// @param startIndex The offset in `array` to start the view.
  /// @param numValues The number of values in the view.
  VISKORES_CONT
  ArrayHandleView(const ArrayHandleType& array, viskores::Id startIndex, viskores::Id numValues)
    : Superclass(StorageType::CreateBuffers(startIndex, numValues, array))
  {
  }

  /// Retrieve the full array being viewed.
  VISKORES_CONT ArrayHandleType GetSourceArray() const
  {
    return this->GetStorage().GetSourceArray(this->GetBuffers());
  }

  /// Retrieve the start index from the array being viewed.
  /// (Note, to get the number of values, simply call the `GetNumberOfValues`
  /// method from the superclass.)
  VISKORES_CONT viskores::Id GetStartIndex() const
  {
    return this->GetStorage().GetStartIndex(this->GetBuffers());
  }
};

/// @brief Construct a `viskores::cont::ArrayHandleView` from a source array.
template <typename ArrayHandleType>
ArrayHandleView<ArrayHandleType> make_ArrayHandleView(const ArrayHandleType& array,
                                                      viskores::Id startIndex,
                                                      viskores::Id numValues)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  return ArrayHandleView<ArrayHandleType>(array, startIndex, numValues);
}

namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if
// the sub-storage is inefficient (thus making everything inefficient).
template <typename StorageTag>
struct ArrayExtractComponentImpl<StorageTagView<StorageTag>>
  : viskores::cont::internal::ArrayExtractComponentImpl<StorageTag>
{
  template <typename T>
  using StrideArrayType =
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>;

  template <typename T>
  StrideArrayType<T> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagView<StorageTag>>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    viskores::cont::ArrayHandleView<viskores::cont::ArrayHandle<T, StorageTag>> srcArray(src);
    StrideArrayType<T> subArray =
      ArrayExtractComponentImpl<StorageTag>{}(srcArray.GetSourceArray(), componentIndex, allowCopy);
    // Narrow the array by adjusting the size and offset.
    return StrideArrayType<T>(subArray.GetBasicArray(),
                              srcArray.GetNumberOfValues(),
                              subArray.GetStride(),
                              subArray.GetOffset() +
                                (subArray.GetStride() * srcArray.GetStartIndex()),
                              subArray.GetModulo(),
                              subArray.GetDivisor());
  }
};

} // namespace internal

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayHandleView_h
