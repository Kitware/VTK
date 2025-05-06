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

#ifndef viskores_cont_BitField_h
#define viskores_cont_BitField_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Atomic.h>
#include <viskores/List.h>
#include <viskores/Types.h>

#include <cassert>
#include <climits>
#include <memory>
#include <type_traits>

namespace viskores
{
namespace cont
{

class BitField;

namespace internal
{

struct StorageTagBitField;

struct VISKORES_ALWAYS_EXPORT BitFieldMetaData
{
  viskores::Id NumberOfBits = 0;
};

}

namespace detail
{

struct BitFieldTraits
{
  // Allocations will occur in blocks of BlockSize bytes. This ensures that
  // power-of-two word sizes up to BlockSize will not access invalid data
  // during word-based access, and that atomic values will be properly aligned.
  // We use the default StorageBasic alignment for this.
  constexpr static viskores::Id BlockSize = VISKORES_ALLOCATION_ALIGNMENT;

  // Make sure the blocksize is at least 64. Eventually we may implement SIMD
  // bit operations, and the current largest vector width is 512 bits.
  VISKORES_STATIC_ASSERT(BlockSize >= 64);

  /// Require an unsigned integral type that is <= BlockSize bytes.
  template <typename WordType>
  using IsValidWordType =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0>;

  /// Require an unsigned integral type that is <= BlockSize bytes, and is
  /// is supported by the specified AtomicInterface.
  template <typename WordType>
  using IsValidWordTypeAtomic =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0 &&
                             /* Supported by atomic interface */
                             viskores::ListHas<viskores::AtomicTypesSupported, WordType>::value>;
};

/// Identifies a bit in a BitField by Word and BitOffset. Note that these
/// values are dependent on the type of word used to generate the coordinate.
struct BitCoordinate
{
  /// The word containing the specified bit.
  viskores::Id WordIndex;

  /// The zero-indexed bit in the word.
  viskores::Int32 BitOffset; // [0, bitsInWord)
};

/// Portal for performing bit or word operations on a BitField.
///
/// This is the implementation used by BitPortal and BitPortalConst.
template <bool IsConst>
class BitPortalBase
{
  // Checks if PortalType has a GetIteratorBegin() method that returns a
  // pointer.
  template <typename PortalType,
            typename PointerType = decltype(std::declval<PortalType>().GetIteratorBegin())>
  struct HasPointerAccess : public std::is_pointer<PointerType>
  {
  };

  // Determine whether we should store a const vs. mutable pointer:
  template <typename T>
  using MaybeConstPointer = typename std::conditional<IsConst, T const*, T*>::type;
  using BufferType = MaybeConstPointer<void>; // void* or void const*, as appropriate

public:
  /// The fastest word type for performing bitwise operations through AtomicInterface.
  using WordTypePreferred = viskores::AtomicTypePreferred;

  /// MPL check for whether a WordType may be used for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = BitFieldTraits::IsValidWordType<WordType>;

  /// MPL check for whether a WordType may be used for atomic operations.
  template <typename WordType>
  using IsValidWordTypeAtomic = BitFieldTraits::IsValidWordTypeAtomic<WordType>;

  VISKORES_STATIC_ASSERT_MSG(IsValidWordType<WordTypeDefault>::value,
                             "Internal error: Default word type is invalid.");
  VISKORES_STATIC_ASSERT_MSG(IsValidWordType<WordTypePreferred>::value,
                             "Device-specific fast word type is invalid.");

  VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypeDefault>::value,
                             "Internal error: Default word type is invalid.");
  VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypePreferred>::value,
                             "Device-specific fast word type is invalid for atomic operations.");

protected:
  friend class viskores::cont::BitField;
  friend class viskores::cont::internal::Storage<bool,
                                                 viskores::cont::internal::StorageTagBitField>;

  /// Construct a BitPortal from a raw array.
  VISKORES_CONT BitPortalBase(BufferType rawArray, viskores::Id numberOfBits)
    : Data{ rawArray }
    , NumberOfBits{ numberOfBits }
  {
  }

public:
  BitPortalBase() noexcept = default;
  BitPortalBase(const BitPortalBase&) noexcept = default;
  BitPortalBase(BitPortalBase&&) noexcept = default;
  BitPortalBase& operator=(const BitPortalBase&) noexcept = default;
  BitPortalBase& operator=(BitPortalBase&&) noexcept = default;

  /// Returns the number of bits in the BitField.
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfBits() const noexcept { return this->NumberOfBits; }

  /// Returns how many words of type @a WordTypePreferred exist in the dataset.
  /// Note that this is rounded up and may contain partial words. See
  /// also GetFinalWordMask to handle the trailing partial word.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT viskores::Id GetNumberOfWords() const noexcept
  {
    VISKORES_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr viskores::Id WordSize = static_cast<viskores::Id>(sizeof(WordType));
    static constexpr viskores::Id WordBits = WordSize * CHAR_BIT;
    return (this->NumberOfBits + WordBits - 1) / WordBits;
  }

  /// Return a mask in which the valid bits in the final word (of type @a
  /// WordType) are set to 1.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType GetFinalWordMask() const noexcept
  {
    if (this->NumberOfBits == 0)
    {
      return WordType{ 0 };
    }

    static constexpr viskores::Int32 BitsPerWord =
      static_cast<viskores::Int32>(sizeof(WordType) * CHAR_BIT);

    const auto maxBit = this->NumberOfBits - 1;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(maxBit);
    const viskores::Int32 shift = BitsPerWord - coord.BitOffset - 1;
    return (~WordType{ 0 }) >> shift;
  }

  /// Given a bit index, compute a @a BitCoordinate that identifies the
  /// corresponding word index and bit offset.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT static BitCoordinate GetBitCoordinateFromIndex(viskores::Id bitIdx) noexcept
  {
    VISKORES_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr viskores::Id BitsPerWord =
      static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);
    return { static_cast<viskores::Id>(bitIdx / BitsPerWord),
             static_cast<viskores::Int32>(bitIdx % BitsPerWord) };
  }

  /// Set the bit at @a bitIdx to @a val. This method is not thread-safe --
  /// threads modifying bits nearby may interfere with this operation.
  /// Additionally, this should not be used for synchronization, as there are
  /// no memory ordering requirements. See SetBitAtomic for those usecases.
  VISKORES_EXEC_CONT
  void SetBit(viskores::Id bitIdx, bool val) const noexcept
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    WordType* wordAddr = this->GetWordAddress<WordType>(coord.WordIndex);
    if (val)
    {
      *wordAddr |= mask;
    }
    else
    {
      *wordAddr &= ~mask;
    }
  }

  /// Set the bit at @a bitIdx to @a val using atomic operations. This method
  /// is thread-safe and guarantees, at minimum, "release" memory ordering.
  VISKORES_EXEC_CONT
  void SetBitAtomic(viskores::Id bitIdx, bool val) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    if (val)
    {
      this->OrWordAtomic(coord.WordIndex, mask);
    }
    else
    {
      this->AndWordAtomic(coord.WordIndex, ~mask);
    }
  }

  /// Return whether or not the bit at @a bitIdx is set. Note that this uses
  /// non-atomic loads and thus should not be used for synchronization.
  VISKORES_EXEC_CONT
  bool GetBit(viskores::Id bitIdx) const noexcept
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWord<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Return whether or not the bit at @a bitIdx is set using atomic loads.
  /// This method is thread safe and guarantees, at minimum, "acquire" memory
  /// ordering.
  VISKORES_EXEC_CONT
  bool GetBitAtomic(viskores::Id bitIdx) const
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWordAtomic<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using
  /// non-atomic operations.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT void SetWord(viskores::Id wordIdx, WordType word) const noexcept
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    *this->GetWordAddress<WordType>(wordIdx) = word;
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using atomic
  /// operations. The store guarantees, at minimum, "release" memory ordering.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT void SetWordAtomic(viskores::Id wordIdx, WordType word) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    viskores::AtomicStore(this->GetWordAddress<WordType>(wordIdx), word);
  }

  /// Get the word (of type @a WordType) at @a wordIdx using non-atomic
  /// operations.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType GetWord(viskores::Id wordIdx) const noexcept
  {
    return *this->GetWordAddress<WordType>(wordIdx);
  }

  /// Get the word (of type @a WordType) at @ wordIdx using an atomic read with,
  /// at minimum, "acquire" memory ordering.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType GetWordAtomic(viskores::Id wordIdx) const
  {
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    return viskores::AtomicLoad(this->GetWordAddress<WordType>(wordIdx));
  }

  /// Toggle the bit at @a bitIdx, returning the original value. This method
  /// uses atomic operations and a full memory barrier.
  VISKORES_EXEC_CONT
  bool NotBitAtomic(viskores::Id bitIdx) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, mask);
    return (oldWord & mask) != WordType(0);
  }

  /// Perform a bitwise "not" operation on the word at @a wordIdx, returning the
  /// original word. This uses atomic operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType NotWordAtomic(viskores::Id wordIdx) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return viskores::AtomicNot(addr);
  }

  /// Perform an "and" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VISKORES_EXEC_CONT
  bool AndBitAtomic(viskores::Id bitIdx, bool val) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 1's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? ~WordType(0) : ~bitmask;
    const auto oldWord = this->AndWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "and" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType AndWordAtomic(viskores::Id wordIdx, WordType wordmask) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return viskores::AtomicAnd(addr, wordmask);
  }

  /// Perform an "of" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VISKORES_EXEC_CONT
  bool OrBitAtomic(viskores::Id bitIdx, bool val) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->OrWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "or" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType OrWordAtomic(viskores::Id wordIdx, WordType wordmask) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return viskores::AtomicOr(addr, wordmask);
  }

  /// Perform an "xor" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VISKORES_EXEC_CONT
  bool XorBitAtomic(viskores::Id bitIdx, bool val) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "xor" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT WordType XorWordAtomic(viskores::Id wordIdx, WordType wordmask) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return viskores::AtomicXor(addr, wordmask);
  }

  /// Perform an atomic compare-and-swap operation on the bit at @a bitIdx.
  /// If the value in memory is equal to @a oldBit, it is replaced with
  /// the value of @a newBit and true is returned. If the value in memory is
  /// not equal to @oldBit, @oldBit is changed to that value and false is
  /// returned. This method implements a full memory barrier around the atomic
  /// operation.
  VISKORES_EXEC_CONT
  bool CompareExchangeBitAtomic(viskores::Id bitIdx, bool* oldBit, bool newBit) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;

    WordType oldWord = this->GetWord<WordType>(coord.WordIndex);
    do
    {
      bool actualBit = (oldWord & bitmask) != WordType(0);
      if (actualBit != *oldBit)
      { // The bit-of-interest does not match what we expected.
        *oldBit = actualBit;
        return false;
      }
      else if (actualBit == newBit)
      { // The bit hasn't changed, but also already matches newVal. We're done.
        return true;
      }

      // Attempt to update the word with a compare-exchange in the loop condition.
      // If the old word changed since last queried, oldWord will get updated and
      // the loop will continue until it succeeds.
    } while (!this->CompareExchangeWordAtomic(coord.WordIndex, &oldWord, oldWord ^ bitmask));

    return true;
  }

  /// Perform an atomic compare-exchange operation on the word at @a wordIdx.
  /// If the word in memory is equal to @a oldWord, it is replaced with
  /// the value of @a newWord and true returned. If the word in memory is not
  /// equal to @oldWord, @oldWord is set to the word in memory and false is
  /// returned. This method implements a full memory barrier around the atomic
  /// operation.
  template <typename WordType = WordTypePreferred>
  VISKORES_EXEC_CONT bool CompareExchangeWordAtomic(viskores::Id wordIdx,
                                                    WordType* oldWord,
                                                    WordType newWord) const
  {
    VISKORES_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VISKORES_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                               "Requested WordType does not support atomic"
                               " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return viskores::AtomicCompareExchange(addr, oldWord, newWord);
  }

private:
  template <typename WordType>
  VISKORES_EXEC_CONT MaybeConstPointer<WordType> GetWordAddress(viskores::Id wordId) const noexcept
  {
    VISKORES_STATIC_ASSERT(IsValidWordType<WordType>::value);
    return reinterpret_cast<MaybeConstPointer<WordType>>(this->Data) + wordId;
  }

  BufferType Data{ nullptr };
  viskores::Id NumberOfBits{ 0 };
};

using BitPortal = BitPortalBase<false>;

using BitPortalConst = BitPortalBase<true>;

} // end namespace detail

class VISKORES_CONT_EXPORT BitField
{
  static constexpr viskores::Id BlockSize = detail::BitFieldTraits::BlockSize;

public:
  /// The BitPortal used in the control environment.
  using WritePortalType = detail::BitPortal;

  /// A read-only BitPortal used in the control environment.
  using ReadPortalType = detail::BitPortalConst;

  using WordTypePreferred = viskores::AtomicTypePreferred;

  template <typename Device>
  struct ExecutionTypes
  {
    /// The preferred word type used by the specified device.
    using WordTypePreferred = viskores::AtomicTypePreferred;

    /// A BitPortal that is usable on the specified device.
    using Portal = detail::BitPortal;

    /// A read-only BitPortal that is usable on the specified device.
    using PortalConst = detail::BitPortalConst;
  };

  /// Check whether a word type is valid for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = detail::BitFieldTraits::IsValidWordType<WordType>;

  /// Check whether a word type is valid for atomic operations.
  template <typename WordType, typename Device = void>
  using IsValidWordTypeAtomic = detail::BitFieldTraits::IsValidWordTypeAtomic<WordType>;

  VISKORES_CONT BitField();
  VISKORES_CONT BitField(const BitField&) = default;
  VISKORES_CONT BitField(BitField&&) noexcept = default;
  VISKORES_CONT ~BitField() = default;
  VISKORES_CONT BitField& operator=(const BitField&) = default;
  VISKORES_CONT BitField& operator=(BitField&&) noexcept = default;

  VISKORES_CONT
  bool operator==(const BitField& rhs) const { return this->Buffer == rhs.Buffer; }

  VISKORES_CONT
  bool operator!=(const BitField& rhs) const { return this->Buffer != rhs.Buffer; }

  /// Return the internal `Buffer` used to store the `BitField`.
  VISKORES_CONT viskores::cont::internal::Buffer GetBuffer() const { return this->Buffer; }

  /// Return the number of bits stored by this BitField.
  VISKORES_CONT viskores::Id GetNumberOfBits() const;

  /// Return the number of words (of @a WordType) stored in this bit fields.
  ///
  template <typename WordType>
  VISKORES_CONT viskores::Id GetNumberOfWords() const
  {
    VISKORES_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr viskores::Id WordBits = static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);
    return (this->GetNumberOfBits() + WordBits - 1) / WordBits;
  }

  /// Allocate the requested number of bits.
  VISKORES_CONT void Allocate(viskores::Id numberOfBits,
                              viskores::CopyFlag preserve,
                              viskores::cont::Token& token) const;

  /// Allocate the requested number of bits.
  VISKORES_CONT void Allocate(viskores::Id numberOfBits,
                              viskores::CopyFlag preserve = viskores::CopyFlag::Off) const
  {
    viskores::cont::Token token;
    this->Allocate(numberOfBits, preserve, token);
  }

  /// Allocate the requested number of bits and fill with the requested bit or word.
  template <typename ValueType>
  VISKORES_CONT void AllocateAndFill(viskores::Id numberOfBits,
                                     ValueType value,
                                     viskores::cont::Token& token) const
  {
    this->Allocate(numberOfBits, viskores::CopyFlag::Off, token);
    this->Fill(value, token);
  }
  template <typename ValueType>
  VISKORES_CONT void AllocateAndFill(viskores::Id numberOfBits, ValueType value) const
  {
    viskores::cont::Token token;
    this->AllocateAndFill(numberOfBits, value, token);
  }

private:
  VISKORES_CONT void FillImpl(const void* word,
                              viskores::BufferSizeType wordSize,
                              viskores::cont::Token& token) const;

public:
  /// Set subsequent words to the given word of bits.
  template <typename WordType>
  VISKORES_CONT void Fill(WordType word, viskores::cont::Token& token) const
  {
    this->FillImpl(&word, static_cast<viskores::BufferSizeType>(sizeof(WordType)), token);
  }
  template <typename WordType>
  VISKORES_CONT void Fill(WordType word) const
  {
    viskores::cont::Token token;
    this->Fill(word, token);
  }

  /// Set all the bits to the given value
  VISKORES_CONT void Fill(bool value, viskores::cont::Token& token) const
  {
    using WordType = WordTypePreferred;
    this->Fill(value ? ~WordType{ 0 } : WordType{ 0 }, token);
  }
  VISKORES_CONT void Fill(bool value) const
  {
    viskores::cont::Token token;
    this->Fill(value, token);
  }

  /// Release all execution-side resources held by this BitField.
  VISKORES_CONT void ReleaseResourcesExecution();

  /// Release all resources held by this BitField and reset to empty.
  VISKORES_CONT void ReleaseResources();

  /// Force the control array to sync with the last-used device.
  VISKORES_CONT void SyncControlArray() const;

  /// Returns true if the `BitField`'s data is on the given device. If the data are on the given
  /// device, then preparing for that device should not require any data movement.
  ///
  VISKORES_CONT bool IsOnDevice(viskores::cont::DeviceAdapterId device) const;

  /// Returns true if the `BitField`'s data is on the host. If the data are on the given
  /// device, then calling `ReadPortal` or `WritePortal` should not require any data movement.
  ///
  VISKORES_CONT bool IsOnHost() const
  {
    return this->IsOnDevice(viskores::cont::DeviceAdapterTagUndefined{});
  }

  /// \brief Get a portal to the data that is usable from the control environment.
  ///
  /// As long as this portal is in scope, no one else will be able to read or write the BitField.
  VISKORES_CONT WritePortalType WritePortal() const;

  /// \brief Get a read-only portal to the data that is usable from the control environment.
  ///
  /// As long as this portal is in scope, no one else will be able to write in the BitField.
  VISKORES_CONT ReadPortalType ReadPortal() const;

  /// Prepares this BitField to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this BitField does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  VISKORES_CONT ReadPortalType PrepareForInput(viskores::cont::DeviceAdapterId device,
                                               viskores::cont::Token& token) const;

  /// Prepares (allocates) this BitField to be used as an output from an
  /// operation in the execution environment. The internal state of this class
  /// is set to have valid data in the execution BitField with the assumption
  /// that the array will be filled soon (i.e. before any other methods of this
  /// object are called). Returns a portal that can be used in code running in
  /// the execution environment.
  VISKORES_CONT WritePortalType PrepareForOutput(viskores::Id numBits,
                                                 viskores::cont::DeviceAdapterId device,
                                                 viskores::cont::Token& token) const;

  /// Prepares this BitField to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this BitField does
  /// not yet contain any data. Returns a portal that can be used in code
  /// running in the execution environment.
  VISKORES_CONT WritePortalType PrepareForInPlace(viskores::cont::DeviceAdapterId device,
                                                  viskores::cont::Token& token) const;

private:
  mutable viskores::cont::internal::Buffer Buffer;
};
}
} // end namespace viskores::cont

#endif // viskores_cont_BitField_h
