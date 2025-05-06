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
#ifndef viskores_cont_ArrayHandleDiscard_h
#define viskores_cont_ArrayHandleDiscard_h

#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/internal/Unreachable.h>

#include <type_traits>

namespace viskores
{
namespace exec
{
namespace internal
{

/// \brief An output-only array portal with no storage. All written values are
/// discarded.
template <typename ValueType_>
class ArrayPortalDiscard
{
public:
  using ValueType = ValueType_;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalDiscard()
    : NumberOfValues(0)
  {
  } // needs to be host and device so that cuda can create lvalue of these

  VISKORES_CONT
  explicit ArrayPortalDiscard(viskores::Id numValues)
    : NumberOfValues(numValues)
  {
  }

  /// Copy constructor for any other ArrayPortalDiscard with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV>
  VISKORES_CONT ArrayPortalDiscard(const ArrayPortalDiscard<OtherV>& src)
    : NumberOfValues(src.NumberOfValues)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  ValueType Get(viskores::Id) const
  {
    VISKORES_UNREACHABLE("Cannot read from ArrayHandleDiscard.");
    return viskores::TypeTraits<ValueType>::ZeroInitialization();
  }

  VISKORES_EXEC
  void Set(viskores::Id index, const ValueType&) const
  {
    VISKORES_ASSERT(index < this->GetNumberOfValues());
    (void)index;
    // no-op
  }

private:
  viskores::Id NumberOfValues;
};

} // end namespace internal
} // end namespace exec

namespace cont
{

namespace internal
{

struct VISKORES_ALWAYS_EXPORT StorageTagDiscard
{
};

struct VISKORES_ALWAYS_EXPORT DiscardMetaData
{
  viskores::Id NumberOfValues = 0;
};

template <typename ValueType>
class Storage<ValueType, StorageTagDiscard>
{
public:
  using WritePortalType = viskores::exec::internal::ArrayPortalDiscard<ValueType>;

  // Note that this portal is write-only, so you will probably run into problems if
  // you actually try to use this read portal.
  using ReadPortalType = viskores::exec::internal::ArrayPortalDiscard<ValueType>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    DiscardMetaData metaData;
    metaData.NumberOfValues = 0;
    return viskores::cont::internal::CreateBuffers(metaData);
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag,
    viskores::cont::Token&)
  {
    VISKORES_ASSERT(numValues >= 0);
    buffers[0].GetMetaData<DiscardMetaData>().NumberOfValues = numValues;
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<ValueType>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<DiscardMetaData>().NumberOfValues;
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const ValueType&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    // Fill is a NO-OP.
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>&,
    viskores::cont::DeviceAdapterId,
    viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadValue("Cannot read from ArrayHandleDiscard.");
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId,
    viskores::cont::Token&)
  {
    return WritePortalType(GetNumberOfValues(buffers));
  }
};

template <typename ValueType_>
struct ArrayHandleDiscardTraits
{
  using ValueType = ValueType_;
  using StorageTag = StorageTagDiscard;
  using Superclass = viskores::cont::ArrayHandle<ValueType, StorageTag>;
};

} // end namespace internal

/// ArrayHandleDiscard is a write-only array that discards all data written to
/// it. This can be used to save memory when a filter provides optional outputs
/// that are not needed.
template <typename ValueType_>
class ArrayHandleDiscard : public internal::ArrayHandleDiscardTraits<ValueType_>::Superclass
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleDiscard,
    (ArrayHandleDiscard<ValueType_>),
    (typename internal::ArrayHandleDiscardTraits<ValueType_>::Superclass));
};

/// Helper to determine if an ArrayHandle type is an ArrayHandleDiscard.
template <typename T>
struct IsArrayHandleDiscard : std::false_type
{
};

template <typename T>
struct IsArrayHandleDiscard<ArrayHandle<T, internal::StorageTagDiscard>> : std::true_type
{
};

} // end namespace cont
} // end namespace viskores

#endif // viskores_cont_ArrayHandleDiscard_h
