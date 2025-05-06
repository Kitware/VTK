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
#ifndef viskores_cont_ArrayHandleRecombineVec_h
#define viskores_cont_ArrayHandleRecombineVec_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/internal/ArrayPortalValueReference.h>

namespace viskores
{
namespace internal
{

// Forward declaration
template <typename SourcePortalType>
class ArrayPortalRecombineVec;

template <typename PortalType>
class RecombineVec
{
  viskores::VecCConst<PortalType> Portals;
  viskores::Id Index;

  /// @cond NOPE
  friend viskores::internal::ArrayPortalRecombineVec<PortalType>;
  /// @endcond

public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  RecombineVec(const RecombineVec&) = default;

  VISKORES_EXEC_CONT RecombineVec(const viskores::VecCConst<PortalType>& portals,
                                  viskores::Id index)
    : Portals(portals)
    , Index(index)
  {
  }

  VISKORES_EXEC_CONT viskores::IdComponent GetNumberOfComponents() const
  {
    return this->Portals.GetNumberOfComponents();
  }

  VISKORES_EXEC_CONT
  viskores::internal::ArrayPortalValueReference<PortalType> operator[](
    viskores::IdComponent cIndex) const
  {
    return viskores::internal::ArrayPortalValueReference<PortalType>(this->Portals[cIndex],
                                                                     this->Index);
  }

  template <typename T, viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<T, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->GetNumberOfComponents());
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      dest[cIndex] = this->Portals[cIndex].Get(this->Index);
    }
    // Clear out any components not held by this dynamic Vec-like
    for (viskores::IdComponent cIndex = numComponents; cIndex < DestSize; ++cIndex)
    {
      dest[cIndex] = viskores::TypeTraits<T>::ZeroInitialization();
    }
  }

  VISKORES_EXEC_CONT viskores::Id GetIndex() const { return this->Index; }

  VISKORES_EXEC_CONT RecombineVec& operator=(const RecombineVec& src)
  {
    if ((&this->Portals[0] != &src.Portals[0]) || (this->Index != src.Index))
    {
      this->DoCopy(src);
    }
    else
    {
      // Copying to myself. Do not need to do anything.
    }
    return *this;
  }

  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator=(const T& src)
  {
    this->DoCopy(src);
    return *this;
  }

  VISKORES_EXEC_CONT operator ComponentType() const { return this->Portals[0].Get(this->Index); }

  template <viskores::IdComponent N>
  VISKORES_EXEC_CONT operator viskores::Vec<ComponentType, N>() const
  {
    viskores::Vec<ComponentType, N> result;
    this->CopyInto(result);
    return result;
  }

  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator+=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] += VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator-=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] -= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator*=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] *= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator/=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] /= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator%=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] %= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator&=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] &= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator|=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] |= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator^=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] ^= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator>>=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] >>= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T>
  VISKORES_EXEC_CONT RecombineVec& operator<<=(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    VISKORES_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] <<= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }

private:
  template <typename T>
  VISKORES_EXEC_CONT void DoCopy(const T& src)
  {
    using VTraits = viskores::VecTraits<T>;
    viskores::IdComponent numComponents = VTraits::GetNumberOfComponents(src);
    if (numComponents > 1)
    {
      if (numComponents > this->GetNumberOfComponents())
      {
        numComponents = this->GetNumberOfComponents();
      }
      for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
      {
        this->Portals[cIndex].Set(this->Index,
                                  static_cast<ComponentType>(VTraits::GetComponent(src, cIndex)));
      }
    }
    else
    {
      // Special case when copying from a scalar
      for (viskores::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
      {
        this->Portals[cIndex].Set(this->Index,
                                  static_cast<ComponentType>(VTraits::GetComponent(src, 0)));
      }
    }
  }
};

} // namespace internal

template <typename PortalType>
struct TypeTraits<viskores::internal::RecombineVec<PortalType>>
{
private:
  using VecType = viskores::internal::RecombineVec<PortalType>;
  using ComponentType = typename VecType::ComponentType;

public:
  using NumericTag = typename viskores::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = viskores::TypeTraitsVectorTag;

  VISKORES_EXEC_CONT static viskores::internal::RecombineVec<PortalType> ZeroInitialization()
  {
    // Return a vec-like of size 0.
    return viskores::internal::RecombineVec<PortalType>{};
  }
};

template <typename PortalType>
struct VecTraits<viskores::internal::RecombineVec<PortalType>>
{
  using VecType = viskores::internal::RecombineVec<PortalType>;
  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeVariable;

  VISKORES_EXEC_CONT static viskores::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  VISKORES_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, viskores::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VISKORES_EXEC_CONT static void SetComponent(const VecType& vector,
                                              viskores::IdComponent componentIndex,
                                              const ComponentType& component)
  {
    vector[componentIndex] = component;
  }

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

namespace internal
{

template <typename SourcePortalType>
class ArrayPortalRecombineVec
{
  // Note that this ArrayPortal has a pointer to a C array of other portals. We need to
  // make sure that the pointer is valid on the device we are using it on. See the
  // CreateReadPortal and CreateWritePortal in the Storage below to see how that is
  // managed.
  const SourcePortalType* Portals;
  viskores::IdComponent NumberOfComponents;

public:
  using ValueType = viskores::internal::RecombineVec<SourcePortalType>;

  ArrayPortalRecombineVec() = default;
  ArrayPortalRecombineVec(const SourcePortalType* portals, viskores::IdComponent numComponents)
    : Portals(portals)
    , NumberOfComponents(numComponents)
  {
  }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const
  {
    return this->Portals[0].GetNumberOfValues();
  }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    return ValueType({ this->Portals, this->NumberOfComponents }, index);
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    if ((value.GetIndex() == index) && (value.Portals.GetPointer() == this->Portals))
    {
      // The ValueType is actually a reference back to the portals. If this reference is
      // actually pointing back to the same index, we don't need to do anything.
    }
    else
    {
      this->DoCopy(index, value);
    }
  }

  template <typename T>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const T& value) const
  {
    this->DoCopy(index, value);
  }

private:
  template <typename T>
  VISKORES_EXEC_CONT void DoCopy(viskores::Id index, const T& value) const
  {
    using Traits = viskores::VecTraits<T>;
    VISKORES_ASSERT(Traits::GetNumberOfComponents(value) == this->NumberOfComponents);
    for (viskores::IdComponent cIndex = 0; cIndex < this->NumberOfComponents; ++cIndex)
    {
      this->Portals[cIndex].Set(index, Traits::GetComponent(value, cIndex));
    }
  }
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

namespace internal
{

struct StorageTagRecombineVec
{
};

namespace detail
{

struct RecombineVecMetaData
{
  mutable std::vector<viskores::cont::internal::Buffer> PortalBuffers;
  std::vector<std::size_t> ArrayBufferOffsets;

  RecombineVecMetaData() = default;

  RecombineVecMetaData(const RecombineVecMetaData& src) { *this = src; }

  RecombineVecMetaData& operator=(const RecombineVecMetaData& src)
  {
    this->ArrayBufferOffsets = src.ArrayBufferOffsets;

    this->PortalBuffers.clear();
    // Intentionally not copying portals. Portals will be recreated from proper array when requsted.

    return *this;
  }
};

template <typename T>
using RecombinedPortalType = viskores::internal::ArrayPortalMultiplexer<
  typename viskores::cont::internal::Storage<T, viskores::cont::StorageTagStride>::ReadPortalType,
  typename viskores::cont::internal::Storage<T, viskores::cont::StorageTagStride>::WritePortalType>;

template <typename T>
using RecombinedValueType = viskores::internal::RecombineVec<RecombinedPortalType<T>>;

} // namespace detail

template <typename ReadWritePortal>
class Storage<viskores::internal::RecombineVec<ReadWritePortal>,
              viskores::cont::internal::StorageTagRecombineVec>
{
  using ComponentType = typename ReadWritePortal::ValueType;
  using SourceStorage =
    viskores::cont::internal::Storage<ComponentType, viskores::cont::StorageTagStride>;
  using ArrayType = viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagStride>;

  VISKORES_STATIC_ASSERT(
    (std::is_same<ReadWritePortal, detail::RecombinedPortalType<ComponentType>>::value));

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> BuffersForComponent(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::IdComponent componentIndex)
  {
    auto& metaData = buffers[0].GetMetaData<detail::RecombineVecMetaData>();
    std::size_t index = static_cast<std::size_t>(componentIndex);
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + metaData.ArrayBufferOffsets[index],
      buffers.begin() + metaData.ArrayBufferOffsets[index + 1]);
  }

public:
  using ReadPortalType = viskores::internal::ArrayPortalRecombineVec<ReadWritePortal>;
  using WritePortalType = viskores::internal::ArrayPortalRecombineVec<ReadWritePortal>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponents(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return static_cast<viskores::IdComponent>(
      buffers[0].GetMetaData<detail::RecombineVecMetaData>().ArrayBufferOffsets.size() - 1);
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::IdComponent numComponents = GetNumberOfComponents(buffers);
    viskores::IdComponent numSubComponents =
      SourceStorage::GetNumberOfComponentsFlat(BuffersForComponent(buffers, 0));
    return numComponents * numSubComponents;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(BuffersForComponent(buffers, 0));
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    viskores::IdComponent numComponents = GetNumberOfComponents(buffers);
    for (viskores::IdComponent component = 0; component < numComponents; ++component)
    {
      SourceStorage::ResizeBuffers(
        numValues, BuffersForComponent(buffers, component), preserve, token);
    }
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const viskores::internal::RecombineVec<ReadWritePortal>&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandleRecombineVec.");
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::IdComponent numComponents = GetNumberOfComponents(buffers);

    // The array portal needs a runtime-allocated array of portals for each component.
    // We use the viskores::cont::internal::Buffer object to allow us to allocate memory on the
    // device and copy data there.
    viskores::cont::internal::Buffer portalBuffer;
    portalBuffer.SetNumberOfBytes(static_cast<viskores::BufferSizeType>(sizeof(ReadWritePortal)) *
                                    numComponents,
                                  viskores::CopyFlag::Off,
                                  token);

    // Save a reference of the portal in our metadata.
    // Note that the buffer we create is going to hang around until the ArrayHandle gets
    // destroyed. The buffers are small and should not be a problem unless you create a
    // lot of portals.
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().PortalBuffers.push_back(portalBuffer);

    // Get the control-side memory and fill it with the execution-side portals
    ReadWritePortal* portals =
      reinterpret_cast<ReadWritePortal*>(portalBuffer.WritePointerHost(token));
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      portals[cIndex] = ReadWritePortal(
        SourceStorage::CreateReadPortal(BuffersForComponent(buffers, cIndex), device, token));
    }

    // Now get the execution-side memory (portals will be copied as necessary) and create
    // the portal for the appropriate device
    return ReadPortalType(
      reinterpret_cast<const ReadWritePortal*>(portalBuffer.ReadPointerDevice(device, token)),
      numComponents);
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::IdComponent numComponents = GetNumberOfComponents(buffers);

    // The array portal needs a runtime-allocated array of portals for each component.
    // We use the viskores::cont::internal::Buffer object to allow us to allocate memory on the
    // device and copy data there.
    viskores::cont::internal::Buffer portalBuffer;
    portalBuffer.SetNumberOfBytes(static_cast<viskores::BufferSizeType>(sizeof(ReadWritePortal)) *
                                    numComponents,
                                  viskores::CopyFlag::Off,
                                  token);

    // Save a reference of the portal in our metadata.
    // Note that the buffer we create is going to hang around until the ArrayHandle gets
    // destroyed. The buffers are small and should not be a problem unless you create a
    // lot of portals.
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().PortalBuffers.push_back(portalBuffer);

    // Get the control-side memory and fill it with the execution-side portals
    ReadWritePortal* portals =
      reinterpret_cast<ReadWritePortal*>(portalBuffer.WritePointerHost(token));
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      portals[cIndex] = ReadWritePortal(
        SourceStorage::CreateWritePortal(BuffersForComponent(buffers, cIndex), device, token));
    }

    // Now get the execution-side memory (portals will be copied as necessary) and create
    // the portal for the appropriate device
    return WritePortalType(
      reinterpret_cast<const ReadWritePortal*>(portalBuffer.ReadPointerDevice(device, token)),
      numComponents);
  }

  VISKORES_CONT static ArrayType ArrayForComponent(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::IdComponent componentIndex)
  {
    return ArrayType(BuffersForComponent(buffers, componentIndex));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    detail::RecombineVecMetaData metaData;
    metaData.ArrayBufferOffsets.push_back(1);
    return viskores::cont::internal::CreateBuffers(metaData);
  }

  VISKORES_CONT static void AppendComponent(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const ArrayType& array)
  {
    // Add buffers of new array to our list of buffers.
    buffers.insert(buffers.end(), array.GetBuffers().begin(), array.GetBuffers().end());
    // Update metadata for new offset to end.
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().ArrayBufferOffsets.push_back(
      buffers.size());
  }
};

} // namespace internal

/// @brief A grouping of `ArrayHandleStride`s into an `ArrayHandle` of `viskores::Vec`s.
///
/// The main intention of `ArrayHandleStride` is to pull out a component of an
/// `ArrayHandle` without knowing there `ArrayHandle`'s storage or `viskores::Vec` shape.
/// However, usually you want to do an operation on all the components together.
/// `ArrayHandleRecombineVec` implements the functionality to easily take a
/// group of extracted components and treat them as a single `ArrayHandle` of
/// `viskores::Vec` values.
///
/// Note that caution should be used with `ArrayHandleRecombineVec` because the
/// size of the `viskores::Vec` values is not known at compile time. Thus, the value
/// type of this array is forced to a special `RecombineVec` class that can cause
/// surprises if treated as a `viskores::Vec`. In particular, the static `NUM_COMPONENTS`
/// expression does not exist. Furthermore, new variables of type `RecombineVec`
/// cannot be created. This means that simple operators like `+` will not work
/// because they require an intermediate object to be created. (Equal operators
/// like `+=` do work because they are given an existing variable to place the
/// output.)
///
template <typename ComponentType>
class ArrayHandleRecombineVec
  : public viskores::cont::ArrayHandle<internal::detail::RecombinedValueType<ComponentType>,
                                       viskores::cont::internal::StorageTagRecombineVec>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRecombineVec,
    (ArrayHandleRecombineVec<ComponentType>),
    (viskores::cont::ArrayHandle<internal::detail::RecombinedValueType<ComponentType>,
                                 viskores::cont::internal::StorageTagRecombineVec>));

  /// @brief Return the number of components in each value of the array.
  ///
  /// This is also equal to the number of component arrays referenced by this
  /// fancy array.
  ///
  /// `ArrayHandleRecombineVec` always stores flat Vec values. As such, this number
  /// of components is the same as the number of base components.
  viskores::IdComponent GetNumberOfComponents() const
  {
    return StorageType::GetNumberOfComponents(this->GetBuffers());
  }

  /// @brief Get the array storing the values for a particular component.
  ///
  /// The returned array is a `viskores::cont::ArrayHandleStride`. It is possible
  /// that the returned arrays from different components reference the same area
  /// of physical memory (usually referencing values interleaved with each other).
  viskores::cont::ArrayHandleStride<ComponentType> GetComponentArray(
    viskores::IdComponent componentIndex) const
  {
    return StorageType::ArrayForComponent(this->GetBuffers(), componentIndex);
  }

  /// @brief Add a component array.
  ///
  /// `AppendComponentArray()` provides an easy way to build an `ArrayHandleRecombineVec`
  /// by iteratively adding the component arrays.
  void AppendComponentArray(
    const viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagStride>& array)
  {
    std::vector<viskores::cont::internal::Buffer> buffers = this->GetBuffers();
    StorageType::AppendComponent(buffers, array);
    this->SetBuffers(std::move(buffers));
  }
};

namespace internal
{

template <>
struct ArrayExtractComponentImpl<viskores::cont::internal::StorageTagRecombineVec>
{
  template <typename RecombineVec>
  viskores::cont::ArrayHandleStride<
    typename viskores::VecFlat<typename RecombineVec::ComponentType>::ComponentType>
  operator()(
    const viskores::cont::ArrayHandle<RecombineVec,
                                      viskores::cont::internal::StorageTagRecombineVec>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    using ComponentType = typename RecombineVec::ComponentType;
    viskores::cont::ArrayHandleRecombineVec<ComponentType> array(src);
    constexpr viskores::IdComponent subComponents =
      viskores::VecFlat<ComponentType>::NUM_COMPONENTS;
    return viskores::cont::ArrayExtractComponent(
      array.GetComponentArray(componentIndex / subComponents),
      componentIndex % subComponents,
      allowCopy);
  }
};

//-------------------------------------------------------------------------------------------------
template <typename S>
struct ArrayRangeComputeImpl;

template <typename S>
struct ArrayRangeComputeMagnitudeImpl;

template <typename T, typename S>
inline viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeImplCaller(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  return viskores::cont::internal::ArrayRangeComputeImpl<S>{}(
    input, maskArray, computeFiniteRange, device);
}

template <typename T, typename S>
inline viskores::Range ArrayRangeComputeMagnitudeImplCaller(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  return viskores::cont::internal::ArrayRangeComputeMagnitudeImpl<S>{}(
    input, maskArray, computeFiniteRange, device);
}

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::internal::StorageTagRecombineVec>
{
  template <typename RecombineVecType>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<RecombineVecType,
                                      viskores::cont::internal::StorageTagRecombineVec>& input_,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const
  {
    auto input = static_cast<
      viskores::cont::ArrayHandleRecombineVec<typename RecombineVecType::ComponentType>>(input_);

    viskores::cont::ArrayHandle<viskores::Range> result;
    result.Allocate(input.GetNumberOfComponents());

    if (input.GetNumberOfValues() < 1)
    {
      result.Fill(viskores::Range{});
      return result;
    }

    auto resultPortal = result.WritePortal();
    for (viskores::IdComponent i = 0; i < input.GetNumberOfComponents(); ++i)
    {
      auto rangeAH = ArrayRangeComputeImplCaller(
        input.GetComponentArray(i), maskArray, computeFiniteRange, device);
      resultPortal.Set(i, rangeAH.ReadPortal().Get(0));
    }

    return result;
  }
};

template <typename ArrayHandleType>
struct ArrayValueIsNested;

template <typename RecombineVecType>
struct ArrayValueIsNested<
  viskores::cont::ArrayHandle<RecombineVecType, viskores::cont::internal::StorageTagRecombineVec>>
{
  static constexpr bool Value = false;
};

template <>
struct VISKORES_CONT_EXPORT
  ArrayRangeComputeMagnitudeImpl<viskores::cont::internal::StorageTagRecombineVec>
{
  template <typename RecombineVecType>
  VISKORES_CONT viskores::Range operator()(
    const viskores::cont::ArrayHandle<RecombineVecType,
                                      viskores::cont::internal::StorageTagRecombineVec>& input_,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const
  {
    auto input = static_cast<
      viskores::cont::ArrayHandleRecombineVec<typename RecombineVecType::ComponentType>>(input_);

    if (input.GetNumberOfValues() < 1)
    {
      return viskores::Range{};
    }
    if (input.GetNumberOfComponents() == 1)
    {
      return ArrayRangeComputeMagnitudeImplCaller(
        input.GetComponentArray(0), maskArray, computeFiniteRange, device);
    }

    return ArrayRangeComputeMagnitudeGeneric(input_, maskArray, computeFiniteRange, device);
  }
};

} // namespace internal

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayHandleRecombineVec_h
