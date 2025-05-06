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
#ifndef viskores_cont_ArrayHandleBitField_h
#define viskores_cont_ArrayHandleBitField_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/Storage.h>

namespace viskores
{
namespace cont
{

namespace internal
{

template <typename BitPortalType>
class ArrayPortalBitField
{
public:
  using ValueType = bool;

  VISKORES_EXEC_CONT
  explicit ArrayPortalBitField(const BitPortalType& portal) noexcept
    : BitPortal{ portal }
  {
  }

  VISKORES_EXEC_CONT
  explicit ArrayPortalBitField(BitPortalType&& portal) noexcept
    : BitPortal{ std::move(portal) }
  {
  }

  ArrayPortalBitField() noexcept = default;
  ArrayPortalBitField(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField(ArrayPortalBitField&&) noexcept = default;
  ArrayPortalBitField& operator=(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField& operator=(ArrayPortalBitField&&) noexcept = default;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const noexcept { return this->BitPortal.GetNumberOfBits(); }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const noexcept { return this->BitPortal.GetBit(index); }

  VISKORES_EXEC_CONT
  void Set(viskores::Id index, ValueType value) const
  {
    // Use an atomic set so we don't clash with other threads writing nearby
    // bits.
    this->BitPortal.SetBitAtomic(index, value);
  }

private:
  BitPortalType BitPortal;
};

struct VISKORES_ALWAYS_EXPORT StorageTagBitField
{
};

template <>
class Storage<bool, StorageTagBitField>
{
  using BitPortalType = viskores::cont::detail::BitPortal;
  using BitPortalConstType = viskores::cont::detail::BitPortalConst;

  using WordType = viskores::WordTypeDefault;
  static constexpr viskores::Id BlockSize = viskores::cont::detail::BitFieldTraits::BlockSize;
  VISKORES_STATIC_ASSERT(BlockSize >= static_cast<viskores::Id>(sizeof(WordType)));

public:
  using ReadPortalType = viskores::cont::internal::ArrayPortalBitField<BitPortalConstType>;
  using WritePortalType = viskores::cont::internal::ArrayPortalBitField<BitPortalType>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<viskores::cont::internal::Buffer>(1);
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numberOfBits,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    const viskores::Id bytesNeeded = (numberOfBits + CHAR_BIT - 1) / CHAR_BIT;
    const viskores::Id blocksNeeded = (bytesNeeded + BlockSize - 1) / BlockSize;
    const viskores::Id numBytes = blocksNeeded * BlockSize;

    VISKORES_LOG_F(viskores::cont::LogLevel::MemCont,
                   "BitField Allocation: %llu bits, blocked up to %s bytes.",
                   static_cast<unsigned long long>(numberOfBits),
                   viskores::cont::GetSizeString(static_cast<viskores::UInt64>(numBytes)).c_str());

    buffers[0].SetNumberOfBytes(numBytes, preserve, token);
    buffers[0].GetMetaData<viskores::cont::internal::BitFieldMetaData>().NumberOfBits =
      numberOfBits;
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return 1;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    viskores::Id numberOfBits =
      buffers[0].GetMetaData<viskores::cont::internal::BitFieldMetaData>().NumberOfBits;
    VISKORES_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);
    return numberOfBits;
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 bool fillValue,
                                 viskores::Id startBit,
                                 viskores::Id endBit,
                                 viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    constexpr viskores::BufferSizeType wordTypeSize =
      static_cast<viskores::BufferSizeType>(sizeof(WordType));
    constexpr viskores::BufferSizeType wordNumBits = wordTypeSize * CHAR_BIT;
    // Special case where filling to end of array.
    viskores::Id totalBitsInArray = GetNumberOfValues(buffers);
    if (endBit >= totalBitsInArray)
    {
      endBit = ((totalBitsInArray + (wordNumBits - 1)) / wordNumBits) * wordNumBits;
    }
    if (((startBit % wordNumBits) == 0) && ((endBit % wordNumBits) == 0))
    {
      WordType fillWord = (fillValue ? ~WordType{ 0 } : WordType{ 0 });
      buffers[0].Fill(&fillWord, wordTypeSize, startBit / CHAR_BIT, endBit / CHAR_BIT, token);
    }
    else if (((startBit % CHAR_BIT) == 0) && ((endBit % CHAR_BIT) == 0))
    {
      viskores::UInt8 fillWord = (fillValue ? ~viskores::UInt8{ 0 } : viskores::UInt8{ 0 });
      buffers[0].Fill(&fillWord, 1, startBit / CHAR_BIT, endBit / CHAR_BIT, token);
    }
    else
    {
      throw viskores::cont::ErrorBadValue("Can only fill ArrayHandleBitField on 8-bit boundaries.");
    }
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    viskores::Id numberOfBits = GetNumberOfValues(buffers);
    VISKORES_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return ReadPortalType(
      BitPortalConstType(buffers[0].ReadPointerDevice(device, token), numberOfBits));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    viskores::Id numberOfBits = GetNumberOfValues(buffers);
    VISKORES_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return WritePortalType(
      BitPortalType(buffers[0].WritePointerDevice(device, token), numberOfBits));
  }
};

} // end namespace internal


/// The ArrayHandleBitField class is a boolean-valued ArrayHandle that is backed
/// by a BitField.
///
class ArrayHandleBitField : public ArrayHandle<bool, internal::StorageTagBitField>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleBitField,
                                    (ArrayHandle<bool, internal::StorageTagBitField>));

  VISKORES_CONT
  explicit ArrayHandleBitField(const viskores::cont::BitField& bitField)
    : Superclass(std::vector<viskores::cont::internal::Buffer>(1, bitField.GetBuffer()))
  {
  }
};

VISKORES_CONT inline viskores::cont::ArrayHandleBitField make_ArrayHandleBitField(
  const viskores::cont::BitField& bitField)
{
  return ArrayHandleBitField{ bitField };
}

VISKORES_CONT inline viskores::cont::ArrayHandleBitField make_ArrayHandleBitField(
  viskores::cont::BitField&& bitField) noexcept
{
  return ArrayHandleBitField{ std::move(bitField) };
}
}
} // end namespace viskores::cont

#endif // viskores_cont_ArrayHandleBitField_h
