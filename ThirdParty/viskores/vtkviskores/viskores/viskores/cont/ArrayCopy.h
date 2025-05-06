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
#ifndef viskores_cont_ArrayCopy_h
#define viskores_cont_ArrayCopy_h

#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/internal/MapArrayPermutation.h>

#include <viskores/StaticAssert.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename S>
struct ArrayCopyConcreteSrc;

template <typename SrcIsArrayHandle>
inline void ArrayCopyImpl(const viskores::cont::UnknownArrayHandle& source,
                          viskores::cont::UnknownArrayHandle& destination,
                          SrcIsArrayHandle,
                          std::false_type)
{
  destination.DeepCopyFrom(source);
}
template <typename SrcIsArrayHandle>
inline void ArrayCopyImpl(const viskores::cont::UnknownArrayHandle& source,
                          const viskores::cont::UnknownArrayHandle& destination,
                          SrcIsArrayHandle,
                          std::false_type)
{
  destination.DeepCopyFrom(source);
}

template <typename T, typename S>
void ArrayCopyImpl(const viskores::cont::UnknownArrayHandle& source,
                   viskores::cont::ArrayHandle<T, S>& destination,
                   std::false_type,
                   std::true_type)
{
  using DestType = viskores::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    destination.DeepCopyFrom(source.AsArrayHandle<DestType>());
  }
  else
  {
    viskores::cont::UnknownArrayHandle destWrapper(destination);
    viskores::cont::detail::ArrayCopyImpl(
      source, destWrapper, std::false_type{}, std::false_type{});
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

template <typename TS, typename SS, typename TD, typename SD>
void ArrayCopyImpl(const viskores::cont::ArrayHandle<TS, SS>& source,
                   viskores::cont::ArrayHandle<TD, SD>& destination,
                   std::true_type,
                   std::true_type)
{
  detail::ArrayCopyConcreteSrc<SS>{}(source, destination);
}

// Special case of copying data when type is the same.
template <typename T, typename S>
void ArrayCopyImpl(const viskores::cont::ArrayHandle<T, S>& source,
                   viskores::cont::ArrayHandle<T, S>& destination,
                   std::true_type,
                   std::true_type)
{
  destination.DeepCopyFrom(source);
}

}

/// \brief Does a deep copy from one array to another array.
///
/// Given a source `ArrayHandle` and a destination `ArrayHandle`, this
/// function allocates the destination `ArrayHandle` to the correct size and
/// deeply copies all the values from the source to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
/// This should work on some non-writable array handles as well, as long as
/// both \a source and \a destination are the same type.
///
/// This version of array copy uses a precompiled version of copy that is
/// efficient for most standard memory layouts. However, there are some
/// types of fancy `ArrayHandle` that cannot be handled directly, and
/// the fallback for these arrays can be slow. If you see a warning in
/// the log about an inefficient memory copy when extracting a component,
/// pay heed and look for a different way to copy the data (perhaps
/// using `ArrayCopyDevice`).
///
template <typename SourceArrayType, typename DestArrayType>
inline void ArrayCopy(const SourceArrayType& source, DestArrayType& destination)
{
  detail::ArrayCopyImpl(source,
                        destination,
                        typename internal::ArrayHandleCheck<SourceArrayType>::type{},
                        typename internal::ArrayHandleCheck<DestArrayType>::type{});
}

// Special case where we allow a const UnknownArrayHandle as output.
/// @copydoc ArrayCopy
template <typename SourceArrayType>
inline void ArrayCopy(const SourceArrayType& source,
                      viskores::cont::UnknownArrayHandle& destination)
{
  detail::ArrayCopyImpl(source,
                        destination,
                        typename internal::ArrayHandleCheck<SourceArrayType>::type{},
                        std::false_type{});
}

// Invalid const ArrayHandle in destination, which is not allowed because it will
// not work in all cases.
template <typename T, typename S>
void ArrayCopy(const viskores::cont::UnknownArrayHandle&, const viskores::cont::ArrayHandle<T, S>&)
{
  VISKORES_STATIC_ASSERT_MSG(sizeof(T) == 0, "Copying to a constant ArrayHandle is not allowed.");
}

/// \brief Copies from an unknown to a known array type.
///
/// Often times you have an array of an unknown type (likely from a data set),
/// and you need it to be of a particular type (or can make a reasonable but uncertain
/// assumption about it being a particular type). You really just want a shallow
/// copy (a reference in a concrete `ArrayHandle`) if that is possible.
///
/// `ArrayCopyShallowIfPossible()` pulls an array of a specific type from an
/// `UnknownArrayHandle`. If the type is compatible, it will perform a shallow copy.
/// If it is not possible, a deep copy is performed to get it to the correct type.
///
template <typename T, typename S>
VISKORES_CONT void ArrayCopyShallowIfPossible(const viskores::cont::UnknownArrayHandle source,
                                              viskores::cont::ArrayHandle<T, S>& destination)
{
  using DestType = viskores::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    source.AsArrayHandle(destination);
  }
  else
  {
    viskores::cont::UnknownArrayHandle destWrapper(destination);
    viskores::cont::ArrayCopy(source, destWrapper);
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

namespace detail
{

template <typename S>
struct ArrayCopyConcreteSrc
{
  template <typename T, typename DestArray>
  void operator()(const viskores::cont::ArrayHandle<T, S>& source, DestArray& destination) const
  {
    using ArrayType = viskores::cont::ArrayHandle<T, S>;
    this->DoIt(source,
               destination,
               viskores::cont::internal::ArrayExtractComponentIsInefficient<ArrayType>{});
  }

  template <typename T, typename DestArray>
  void DoIt(const viskores::cont::ArrayHandle<T, S>& source,
            DestArray& destination,
            std::false_type viskoresNotUsed(isInefficient)) const
  {
    viskores::cont::ArrayCopy(viskores::cont::UnknownArrayHandle{ source }, destination);
  }

  template <typename T, typename DestArray>
  void DoIt(const viskores::cont::ArrayHandle<T, S>& source,
            DestArray& destination,
            std::true_type viskoresNotUsed(isInefficient)) const
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Warn,
      "Attempting to copy from an array of type " +
        viskores::cont::TypeToString<viskores::cont::ArrayHandle<T, S>>() +
        " with ArrayCopy is inefficient. It is highly recommended you use another method "
        "such as viskores::cont::ArrayCopyDevice.");
    // Still call the precompiled `ArrayCopy`. You will get another warning after this,
    // but it will still technically work, albiet slowly.
    viskores::cont::ArrayCopy(viskores::cont::UnknownArrayHandle{ source }, destination);
  }
};

// Special case for constant arrays to be efficient.
template <>
struct ArrayCopyConcreteSrc<viskores::cont::StorageTagConstant>
{
  template <typename T1, typename T2, typename S2>
  void operator()(
    const viskores::cont::ArrayHandle<T1, viskores::cont::StorageTagConstant>& source_,
    viskores::cont::ArrayHandle<T2, S2>& destination) const
  {
    viskores::cont::ArrayHandleConstant<T1> source = source_;
    destination.AllocateAndFill(source.GetNumberOfValues(), static_cast<T2>(source.GetValue()));
  }
};

// Special case for ArrayHandleIndex to be efficient.
template <>
struct ArrayCopyConcreteSrc<viskores::cont::StorageTagIndex>
{
  template <typename T, typename S>
  void operator()(const viskores::cont::ArrayHandleIndex& source,
                  viskores::cont::ArrayHandle<T, S>& destination) const
  {
    // Skip warning about inefficient copy because there is a special case in ArrayCopyUnknown.cxx
    // to copy ArrayHandleIndex efficiently.
    viskores::cont::ArrayCopy(viskores::cont::UnknownArrayHandle{ source }, destination);
  }
};

// Special case for ArrayHandleCounting to be efficient.
template <>
struct VISKORES_CONT_EXPORT ArrayCopyConcreteSrc<viskores::cont::StorageTagCounting>
{
  template <typename T1, typename T2, typename S2>
  void operator()(const viskores::cont::ArrayHandle<T1, viskores::cont::StorageTagCounting>& source,
                  viskores::cont::ArrayHandle<T2, S2>& destination) const
  {
    viskores::cont::ArrayHandleCounting<T1> countingSource = source;
    T1 start = countingSource.GetStart();
    T1 step = countingSource.GetStep();
    viskores::Id size = countingSource.GetNumberOfValues();
    destination.Allocate(size);
    viskores::cont::UnknownArrayHandle unknownDest = destination;

    using VTraits1 = viskores::VecTraits<T1>;
    using VTraits2 = viskores::VecTraits<T2>;
    for (viskores::IdComponent comp = 0; comp < VTraits1::GetNumberOfComponents(start); ++comp)
    {
      this->CopyCountingFloat(
        static_cast<viskores::FloatDefault>(VTraits1::GetComponent(start, comp)),
        static_cast<viskores::FloatDefault>(VTraits1::GetComponent(step, comp)),
        size,
        unknownDest.ExtractComponent<typename VTraits2::BaseComponentType>(comp));
    }
  }

  void operator()(
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagCounting>& source,
    viskores::cont::ArrayHandle<viskores::Id>& destination) const
  {
    destination = this->CopyCountingId(source);
  }

private:
  void CopyCountingFloat(viskores::FloatDefault start,
                         viskores::FloatDefault step,
                         viskores::Id size,
                         const viskores::cont::UnknownArrayHandle& result) const;
  viskores::cont::ArrayHandle<Id> CopyCountingId(
    const viskores::cont::ArrayHandleCounting<viskores::Id>& source) const;
};

// Special case for ArrayHandleConcatenate to be efficient
template <typename ST1, typename ST2>
struct ArrayCopyConcreteSrc<viskores::cont::StorageTagConcatenate<ST1, ST2>>
{
  template <typename SourceArrayType, typename DestArrayType>
  void operator()(const SourceArrayType& source, DestArrayType& destination) const
  {
    auto source1 = source.GetStorage().GetArray1(source.GetBuffers());
    auto source2 = source.GetStorage().GetArray2(source.GetBuffers());

    // Need to preallocate because view decorator will not be able to resize.
    destination.Allocate(source.GetNumberOfValues());
    auto dest1 = viskores::cont::make_ArrayHandleView(destination, 0, source1.GetNumberOfValues());
    auto dest2 = viskores::cont::make_ArrayHandleView(
      destination, source1.GetNumberOfValues(), source2.GetNumberOfValues());

    viskores::cont::ArrayCopy(source1, dest1);
    viskores::cont::ArrayCopy(source2, dest2);
  }
};

// Special case for ArrayHandlePermutation to be efficient
template <typename SIndex, typename SValue>
struct ArrayCopyConcreteSrc<viskores::cont::StorageTagPermutation<SIndex, SValue>>
{
  using SourceStorageTag = viskores::cont::StorageTagPermutation<SIndex, SValue>;
  template <typename T1, typename T2, typename S2>
  void operator()(const viskores::cont::ArrayHandle<T1, SourceStorageTag>& source,
                  viskores::cont::ArrayHandle<T2, S2>& destination) const
  {
    auto indexArray = source.GetStorage().GetIndexArray(source.GetBuffers());
    auto valueArray = source.GetStorage().GetValueArray(source.GetBuffers());
    viskores::cont::UnknownArrayHandle copy =
      viskores::cont::internal::MapArrayPermutation(valueArray, indexArray);
    viskores::cont::ArrayCopyShallowIfPossible(copy, destination);
  }
};

} // namespace detail

} // namespace cont
} // namespace viskores

#endif //viskores_cont_ArrayCopy_h
