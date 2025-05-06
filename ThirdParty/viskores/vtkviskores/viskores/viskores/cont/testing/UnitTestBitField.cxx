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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleBitField.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/testing/Testing.h>

#include <cstdio>

#if defined(KOKKOS_ENABLE_SYCL)
#define DEVICE_ASSERT_MSG(cond, message) \
  do                                     \
  {                                      \
    if (!(cond))                         \
    {                                    \
      return false;                      \
    }                                    \
  } while (false)

#define DEVICE_ASSERT(cond) \
  do                        \
  {                         \
    if (!(cond))            \
    {                       \
      return false;         \
    }                       \
  } while (false)
#else
#define DEVICE_ASSERT_MSG(cond, message)                                             \
  do                                                                                 \
  {                                                                                  \
    if (!(cond))                                                                     \
    {                                                                                \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n\t- Subtest: %s\n", \
             __FILE__,                                                               \
             __LINE__,                                                               \
             #cond,                                                                  \
             message);                                                               \
      return false;                                                                  \
    }                                                                                \
  } while (false)

#define DEVICE_ASSERT(cond)                                                                     \
  do                                                                                            \
  {                                                                                             \
    if (!(cond))                                                                                \
    {                                                                                           \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n", __FILE__, __LINE__, #cond); \
      return false;                                                                             \
    }                                                                                           \
  } while (false)
#endif

// Test with some trailing bits in partial last word:
#define NUM_BITS \
  viskores::Id   \
  {              \
    7681         \
  }

namespace
{

// Takes an ArrayHandleBitField as the boolean condition field
class ConditionalMergeWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn cond, FieldIn trueVals, FieldIn falseVals, FieldOut result);
  using ExecutionSignature = _4(_1, _2, _3);

  template <typename T>
  VISKORES_EXEC T operator()(bool cond, const T& trueVal, const T& falseVal) const
  {
    return cond ? trueVal : falseVal;
  }
};

// Takes a BitFieldInOut as the condition information, and reverses
// the bits in place after performing the merge.
class ConditionalMergeWorklet2 : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(BitFieldInOut bits,
                                FieldIn trueVals,
                                FieldIn falseVal,
                                FieldOut result);
  using ExecutionSignature = _4(InputIndex, _1, _2, _3);
  using InputDomain = _2;

  template <typename BitPortal, typename T>
  VISKORES_EXEC T
  operator()(const viskores::Id i, BitPortal& bits, const T& trueVal, const T& falseVal) const
  {
    return bits.XorBitAtomic(i, true) ? trueVal : falseVal;
  }
};

using Traits = viskores::cont::detail::BitFieldTraits;
using WordTypes = viskores::AtomicTypesSupported;

VISKORES_EXEC_CONT
bool RandomBitFromIndex(viskores::Id idx) noexcept
{
  // Some random operations that will give a pseudorandom stream of bits:
  auto m = idx + (idx * 2) - (idx / 3) + (idx * 5 / 7) - (idx * 11 / 13);
  return (m % 2) == 1;
}

template <typename WordType>
VISKORES_EXEC_CONT WordType RandomWordFromIndex(viskores::Id idx) noexcept
{
  viskores::UInt64 m = static_cast<viskores::UInt64>(idx * (NUM_BITS - 1) + (idx + 1) * NUM_BITS);
  m ^= m << 3;
  m ^= m << 7;
  m ^= m << 15;
  m ^= m << 31;
  m = (m << 32) | (m >> 32);

  const size_t mBits = 64;
  const size_t wordBits = sizeof(WordType) * CHAR_BIT;

  const WordType highWord = static_cast<WordType>(m >> (mBits - wordBits));
  return highWord;
}

VISKORES_CONT
static viskores::cont::BitField RandomBitField(viskores::Id numBits = NUM_BITS)
{
  viskores::cont::BitField field;
  field.Allocate(numBits);
  auto portal = field.WritePortal();
  for (viskores::Id i = 0; i < numBits; ++i)
  {
    portal.SetBit(i, RandomBitFromIndex(i));
  }

  return field;
}

VISKORES_CONT
void TestBlockAllocation()
{
  viskores::cont::BitField field;
  field.Allocate(NUM_BITS);

  // NumBits should be rounded up to the nearest block of bytes, as defined in
  // the traits:
  const viskores::BufferSizeType bytesInFieldData = field.GetBuffer().GetNumberOfBytes();

  const viskores::BufferSizeType blockSize = viskores::cont::detail::BitFieldTraits::BlockSize;
  const viskores::BufferSizeType numBytes = (NUM_BITS + CHAR_BIT - 1) / CHAR_BIT;
  const viskores::BufferSizeType numBlocks = (numBytes + blockSize - 1) / blockSize;
  const viskores::BufferSizeType expectedBytes = numBlocks * blockSize;

  VISKORES_TEST_ASSERT(bytesInFieldData == expectedBytes,
                       "The BitField allocation does not round up to the nearest "
                       "block. This can cause access-by-word to read/write invalid "
                       "memory.");
}

template <typename PortalType>
VISKORES_EXEC_CONT bool TestBitValue(const char* operation,
                                     viskores::Id i,
                                     PortalType portal,
                                     bool& bit,
                                     bool originalBit)
{
  auto expected = bit;
  auto result = portal.GetBitAtomic(i);
  DEVICE_ASSERT_MSG(result == expected, operation);

  // Reset
  bit = originalBit;
  portal.SetBitAtomic(i, bit);
  return true;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool HelpTestBit(viskores::Id i, PortalType portal)
{
  const auto origBit = RandomBitFromIndex(i);
  auto bit = origBit;

  const auto mod = RandomBitFromIndex(i + NUM_BITS);

  bit = mod;
  portal.SetBitAtomic(i, mod);
  DEVICE_ASSERT(TestBitValue("SetBitAtomic", i, portal, bit, origBit));

  bit = !bit;
  portal.NotBitAtomic(i);
  DEVICE_ASSERT(TestBitValue("NotBitAtomic", i, portal, bit, origBit));

  bit = bit && mod;
  portal.AndBitAtomic(i, mod);
  DEVICE_ASSERT(TestBitValue("AndBitAtomic", i, portal, bit, origBit));

  bit = bit || mod;
  portal.OrBitAtomic(i, mod);
  DEVICE_ASSERT(TestBitValue("OrBitAtomic", i, portal, bit, origBit));

  bit = bit != mod;
  portal.XorBitAtomic(i, mod);
  DEVICE_ASSERT(TestBitValue("XorBitAtomic", i, portal, bit, origBit));

  const auto notBit = !bit;
  // A compare-exchange that should fail
  auto expectedBit = notBit;
  bool cxResult = portal.CompareExchangeBitAtomic(i, &expectedBit, bit);
  DEVICE_ASSERT(!cxResult);
  DEVICE_ASSERT(expectedBit != notBit);
  DEVICE_ASSERT(portal.GetBit(i) == expectedBit);
  DEVICE_ASSERT(portal.GetBit(i) == bit);

  // A compare-exchange that should succeed.
  expectedBit = bit;
  cxResult = portal.CompareExchangeBitAtomic(i, &expectedBit, notBit);
  DEVICE_ASSERT(cxResult);
  DEVICE_ASSERT(expectedBit == bit);
  DEVICE_ASSERT(portal.GetBit(i) == notBit);

  return true;
}

template <typename WordType, typename PortalType>
VISKORES_EXEC_CONT bool TestWordValue(const char* operation,
                                      viskores::Id i,
                                      const PortalType& portal,
                                      WordType& word,
                                      WordType originalWord)
{
  auto expected = word;
  auto result = portal.template GetWordAtomic<WordType>(i);
  DEVICE_ASSERT_MSG(result == expected, operation);

  // Reset
  word = originalWord;
  portal.SetWordAtomic(i, word);
  return true;
}

template <typename WordType, typename PortalType>
VISKORES_EXEC_CONT bool HelpTestWord(viskores::Id i, PortalType portal)
{
  const auto origWord = RandomWordFromIndex<WordType>(i);
  auto word = origWord;

  const auto mod = RandomWordFromIndex<WordType>(i + NUM_BITS);

  portal.SetWord(i, word);
  DEVICE_ASSERT(TestWordValue("SetWord", i, portal, word, origWord));

  word = mod;
  portal.SetWordAtomic(i, mod);
  DEVICE_ASSERT(TestWordValue("SetWordAtomic", i, portal, word, origWord));

  // C++ promotes e.g. uint8 to int32 when performing bitwise not. Silence
  // conversion warning and mask unimportant bits:
  word = static_cast<WordType>(~word);
  portal.template NotWordAtomic<WordType>(i);
  DEVICE_ASSERT(TestWordValue("NotWordAtomic", i, portal, word, origWord));

  word = word & mod;
  portal.AndWordAtomic(i, mod);
  DEVICE_ASSERT(TestWordValue("AndWordAtomic", i, portal, word, origWord));

  word = word | mod;
  portal.OrWordAtomic(i, mod);
  DEVICE_ASSERT(TestWordValue("OrWordAtomic", i, portal, word, origWord));

  word = word ^ mod;
  portal.XorWordAtomic(i, mod);
  DEVICE_ASSERT(TestWordValue("XorWordAtomic", i, portal, word, origWord));

  // Compare-exchange that should fail
  const WordType notWord = static_cast<WordType>(~word);
  WordType expectedWord = notWord;
  bool cxResult = portal.CompareExchangeWordAtomic(i, &expectedWord, word);
  DEVICE_ASSERT(!cxResult);
  DEVICE_ASSERT(expectedWord != notWord);
  DEVICE_ASSERT(portal.template GetWord<WordType>(i) == expectedWord);
  DEVICE_ASSERT(portal.template GetWord<WordType>(i) == word);

  // Compare-exchange that should succeed
  expectedWord = word;
  cxResult = portal.CompareExchangeWordAtomic(i, &expectedWord, notWord);
  DEVICE_ASSERT(cxResult);
  DEVICE_ASSERT(expectedWord == word);
  DEVICE_ASSERT(portal.template GetWord<WordType>(i) == notWord);

  return true;
}

template <typename PortalType>
struct HelpTestWordOpsControl
{
  PortalType Portal;

  VISKORES_CONT
  HelpTestWordOpsControl(PortalType portal)
    : Portal(portal)
  {
  }

  template <typename WordType>
  VISKORES_CONT void operator()(WordType)
  {
    const auto numWords = this->Portal.template GetNumberOfWords<WordType>();
    for (viskores::Id i = 0; i < numWords; ++i)
    {
      VISKORES_TEST_ASSERT(HelpTestWord<WordType>(i, this->Portal));
    }
  }
};

template <typename Portal>
VISKORES_CONT void HelpTestPortalsControl(Portal portal)
{
  const auto numWords8 = (NUM_BITS + 7) / 8;
  const auto numWords16 = (NUM_BITS + 15) / 16;
  const auto numWords32 = (NUM_BITS + 31) / 32;
  const auto numWords64 = (NUM_BITS + 63) / 64;

  VISKORES_TEST_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
  VISKORES_TEST_ASSERT(portal.template GetNumberOfWords<viskores::UInt8>() == numWords8);
  VISKORES_TEST_ASSERT(portal.template GetNumberOfWords<viskores::UInt16>() == numWords16);
  VISKORES_TEST_ASSERT(portal.template GetNumberOfWords<viskores::UInt32>() == numWords32);
  VISKORES_TEST_ASSERT(portal.template GetNumberOfWords<viskores::UInt64>() == numWords64);

  for (viskores::Id i = 0; i < NUM_BITS; ++i)
  {
    HelpTestBit(i, portal);
  }

  HelpTestWordOpsControl<Portal> test(portal);
  viskores::ListForEach(test, viskores::AtomicTypesSupported{});
}

VISKORES_CONT
void TestControlPortals()
{
  auto field = RandomBitField();

  HelpTestPortalsControl(field.WritePortal());
}

template <typename Portal>
VISKORES_EXEC_CONT bool HelpTestPortalSanityExecution(Portal portal)
{
  const auto numWords8 = (NUM_BITS + 7) / 8;
  const auto numWords16 = (NUM_BITS + 15) / 16;
  const auto numWords32 = (NUM_BITS + 31) / 32;
  const auto numWords64 = (NUM_BITS + 63) / 64;

  DEVICE_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
  DEVICE_ASSERT(portal.template GetNumberOfWords<viskores::UInt8>() == numWords8);
  DEVICE_ASSERT(portal.template GetNumberOfWords<viskores::UInt16>() == numWords16);
  DEVICE_ASSERT(portal.template GetNumberOfWords<viskores::UInt32>() == numWords32);
  DEVICE_ASSERT(portal.template GetNumberOfWords<viskores::UInt64>() == numWords64);

  return true;
}

template <typename WordType, typename PortalType>
struct HelpTestPortalsExecutionWordsFunctor : viskores::exec::FunctorBase
{
  PortalType Portal;

  HelpTestPortalsExecutionWordsFunctor(PortalType portal)
    : Portal(portal)
  {
  }

  VISKORES_EXEC_CONT
  void operator()(viskores::Id i) const
  {
    if (i == 0)
    {
      if (!HelpTestPortalSanityExecution(this->Portal))
      {
        this->RaiseError("Testing Portal sanity failed.");
        return;
      }
    }

    if (!HelpTestWord<WordType>(i, this->Portal))
    {
      this->RaiseError("Testing word operations failed.");
      return;
    }
  }
};

template <typename PortalType>
struct HelpTestPortalsExecutionBitsFunctor : viskores::exec::FunctorBase
{
  PortalType Portal;

  HelpTestPortalsExecutionBitsFunctor(PortalType portal)
    : Portal(portal)
  {
  }

  VISKORES_EXEC_CONT
  void operator()(viskores::Id i) const
  {
    if (!HelpTestBit(i, this->Portal))
    {
      this->RaiseError("Testing bit operations failed.");
      return;
    }
  }
};

template <typename PortalType, typename Device>
struct HelpTestWordOpsExecution
{
  PortalType Portal;

  VISKORES_CONT
  HelpTestWordOpsExecution(PortalType portal)
    : Portal(portal)
  {
  }

  template <typename WordType>
  VISKORES_CONT void operator()(WordType)
  {
    const auto numWords = this->Portal.template GetNumberOfWords<WordType>();

    using WordFunctor = HelpTestPortalsExecutionWordsFunctor<WordType, PortalType>;
    WordFunctor test{ this->Portal };
    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(test, numWords);
  }
};

template <typename Portal, typename Device>
VISKORES_CONT void HelpTestPortalsExecution(Portal portal, Device)
{
  HelpTestPortalsExecutionBitsFunctor<Portal> bitTest{ portal };
  viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(bitTest, portal.GetNumberOfBits());


  HelpTestWordOpsExecution<Portal, Device> test(portal);
  viskores::ListForEach(test, viskores::AtomicTypesSupported{});
}

VISKORES_CONT
void TestExecutionPortals()
{
  viskores::cont::BitField field = RandomBitField();

  viskores::cont::TryExecute(
    [&](auto device)
    {
      viskores::cont::Token token;
      HelpTestPortalsExecution(field.PrepareForInPlace(device, token), device);
      return true;
    });
}

VISKORES_CONT
void TestFinalWordMask()
{
  auto testMask32 = [](viskores::Id numBits, viskores::UInt32 expectedMask)
  {
    viskores::cont::BitField field;
    field.Allocate(numBits);
    auto mask = field.ReadPortal().GetFinalWordMask<viskores::UInt32>();

    VISKORES_TEST_ASSERT(expectedMask == mask,
                         "Unexpected mask for BitField size ",
                         numBits,
                         ": Expected 0x",
                         std::hex,
                         expectedMask,
                         " got 0x",
                         mask);
  };

  auto testMask64 = [](viskores::Id numBits, viskores::UInt64 expectedMask)
  {
    viskores::cont::BitField field;
    field.Allocate(numBits);
    auto mask = field.ReadPortal().GetFinalWordMask<viskores::UInt64>();

    VISKORES_TEST_ASSERT(expectedMask == mask,
                         "Unexpected mask for BitField size ",
                         numBits,
                         ": Expected 0x",
                         std::hex,
                         expectedMask,
                         " got 0x",
                         mask);
  };

  testMask32(0, 0x00000000);
  testMask32(1, 0x00000001);
  testMask32(2, 0x00000003);
  testMask32(3, 0x00000007);
  testMask32(4, 0x0000000f);
  testMask32(5, 0x0000001f);
  testMask32(8, 0x000000ff);
  testMask32(16, 0x0000ffff);
  testMask32(24, 0x00ffffff);
  testMask32(25, 0x01ffffff);
  testMask32(31, 0x7fffffff);
  testMask32(32, 0xffffffff);
  testMask32(64, 0xffffffff);
  testMask32(128, 0xffffffff);
  testMask32(129, 0x00000001);

  testMask64(0, 0x0000000000000000);
  testMask64(1, 0x0000000000000001);
  testMask64(2, 0x0000000000000003);
  testMask64(3, 0x0000000000000007);
  testMask64(4, 0x000000000000000f);
  testMask64(5, 0x000000000000001f);
  testMask64(8, 0x00000000000000ff);
  testMask64(16, 0x000000000000ffff);
  testMask64(24, 0x0000000000ffffff);
  testMask64(25, 0x0000000001ffffff);
  testMask64(31, 0x000000007fffffff);
  testMask64(32, 0x00000000ffffffff);
  testMask64(40, 0x000000ffffffffff);
  testMask64(48, 0x0000ffffffffffff);
  testMask64(56, 0x00ffffffffffffff);
  testMask64(64, 0xffffffffffffffff);
  testMask64(128, 0xffffffffffffffff);
  testMask64(129, 0x0000000000000001);
}

VISKORES_CONT void TestFill()
{
  viskores::cont::BitField bitField;
  bitField.Allocate(NUM_BITS);

  bitField.Fill(true);
  {
    auto portal = bitField.ReadPortal();
    for (viskores::Id index = 0; index < NUM_BITS; ++index)
    {
      VISKORES_TEST_ASSERT(portal.GetBit(index));
    }
  }

  constexpr viskores::UInt8 word8 = 0xA6;
  bitField.Fill(word8);
  {
    auto portal = bitField.ReadPortal();
    for (viskores::Id index = 0; index < NUM_BITS; ++index)
    {
      VISKORES_TEST_ASSERT(portal.GetBit(index) == ((word8 >> (index % 8)) & 0x01));
    }
  }
}

struct ArrayHandleBitFieldChecker : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  bool InvertReference;

  VISKORES_CONT
  ArrayHandleBitFieldChecker(bool invert)
    : InvertReference(invert)
  {
  }

  VISKORES_EXEC
  void operator()(bool& bit, viskores::Id i) const
  {
    const bool ref = this->InvertReference ? !RandomBitFromIndex(i) : RandomBitFromIndex(i);
    if (bit != ref)
    {
      this->RaiseError("Unexpected value from ArrayHandleBitField portal.");
      return;
    }

    // Flip the bit for the next kernel launch, which tests that the bitfield
    // is inverted.
    bit = !ref;
  }
};

VISKORES_CONT
void TestArrayHandleBitField()
{
  viskores::cont::Invoker invoke;

  auto handle = viskores::cont::make_ArrayHandleBitField(RandomBitField());
  const viskores::Id numBits = handle.GetNumberOfValues();

  VISKORES_TEST_ASSERT(numBits == NUM_BITS,
                       "ArrayHandleBitField returned the wrong number of values. "
                       "Expected: ",
                       NUM_BITS,
                       " got: ",
                       numBits);

  invoke(ArrayHandleBitFieldChecker{ false }, handle);
  invoke(ArrayHandleBitFieldChecker{ true }, handle);

  handle.Fill(true);
  {
    auto portal = handle.ReadPortal();
    for (viskores::Id index = 0; index < NUM_BITS; ++index)
    {
      VISKORES_TEST_ASSERT(portal.Get(index));
    }
  }

  handle.Fill(false, 24);
  handle.Fill(true, 64);
  {
    auto portal = handle.ReadPortal();
    for (viskores::Id index = 0; index < NUM_BITS; ++index)
    {
      VISKORES_TEST_ASSERT(portal.Get(index) == ((index < 24) || (index >= 64)));
    }
  }
}

VISKORES_CONT
void TestArrayInvokeWorklet()
{
  auto condArray = viskores::cont::make_ArrayHandleBitField(RandomBitField());
  auto trueArray = viskores::cont::make_ArrayHandleCounting<viskores::Id>(20, 2, NUM_BITS);
  auto falseArray = viskores::cont::make_ArrayHandleCounting<viskores::Id>(13, 2, NUM_BITS);
  viskores::cont::ArrayHandle<viskores::Id> output;

  viskores::cont::Invoker invoke;
  invoke(ConditionalMergeWorklet{}, condArray, trueArray, falseArray, output);

  auto condVals = condArray.ReadPortal();
  auto trueVals = trueArray.ReadPortal();
  auto falseVals = falseArray.ReadPortal();
  auto outVals = output.ReadPortal();

  VISKORES_TEST_ASSERT(condVals.GetNumberOfValues() == trueVals.GetNumberOfValues());
  VISKORES_TEST_ASSERT(condVals.GetNumberOfValues() == falseVals.GetNumberOfValues());
  VISKORES_TEST_ASSERT(condVals.GetNumberOfValues() == outVals.GetNumberOfValues());

  for (viskores::Id i = 0; i < condVals.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(outVals.Get(i) == (condVals.Get(i) ? trueVals.Get(i) : falseVals.Get(i)));
  }
}

VISKORES_CONT
static void TestArrayInvokeWorklet2()
{
  auto condBits = RandomBitField();
  auto trueArray = viskores::cont::make_ArrayHandleCounting<viskores::Id>(20, 2, NUM_BITS);
  auto falseArray = viskores::cont::make_ArrayHandleCounting<viskores::Id>(13, 2, NUM_BITS);
  viskores::cont::ArrayHandle<viskores::Id> output;

  viskores::cont::Invoker invoke;
  invoke(ConditionalMergeWorklet2{}, condBits, trueArray, falseArray, output);

  auto condVals = condBits.ReadPortal();
  auto trueVals = trueArray.ReadPortal();
  auto falseVals = falseArray.ReadPortal();
  auto outVals = output.ReadPortal();

  VISKORES_TEST_ASSERT(condVals.GetNumberOfBits() == trueVals.GetNumberOfValues());
  VISKORES_TEST_ASSERT(condVals.GetNumberOfBits() == falseVals.GetNumberOfValues());
  VISKORES_TEST_ASSERT(condVals.GetNumberOfBits() == outVals.GetNumberOfValues());

  for (viskores::Id i = 0; i < condVals.GetNumberOfBits(); ++i)
  {
    // The worklet flips the bitfield in place after choosing true/false paths
    VISKORES_TEST_ASSERT(condVals.GetBit(i) == !RandomBitFromIndex(i));
    VISKORES_TEST_ASSERT(outVals.Get(i) ==
                         (!condVals.GetBit(i) ? trueVals.Get(i) : falseVals.Get(i)));
  }
}

VISKORES_CONT void Run()
{
  TestBlockAllocation();
  TestControlPortals();
  TestExecutionPortals();
  TestFinalWordMask();
  TestFill();
  TestArrayHandleBitField();
  TestArrayInvokeWorklet();
  TestArrayInvokeWorklet2();
}

} // anonymous namespace

int UnitTestBitField(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
