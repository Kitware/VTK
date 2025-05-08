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

#include <viskores/testing/Testing.h>

#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/internal/ArrayPortalValueReference.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename ArrayPortalType>
void SetReference(viskores::Id index,
                  viskores::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  ref = TestValue(index, ValueType());
}

template <typename ArrayPortalType>
void CheckReference(viskores::Id index,
                    viskores::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  VISKORES_TEST_ASSERT(test_equal(ref, TestValue(index, ValueType())),
                       "Got bad value from reference.");
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(viskores::Id index,
                       viskores::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                       viskores::TypeTraitsScalarTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  ValueType expected = TestValue(index, ValueType());
  VISKORES_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  VISKORES_TEST_ASSERT(!(ref < ref));
  VISKORES_TEST_ASSERT(ref < ValueType(expected + ValueType(1)));
  VISKORES_TEST_ASSERT(ValueType(expected - ValueType(1)) < ref);

  VISKORES_TEST_ASSERT(!(ref > ref));
  VISKORES_TEST_ASSERT(ref > ValueType(expected - ValueType(1)));
  VISKORES_TEST_ASSERT(ValueType(expected + ValueType(1)) > ref);

  VISKORES_TEST_ASSERT(ref <= ref);
  VISKORES_TEST_ASSERT(ref <= ValueType(expected + ValueType(1)));
  VISKORES_TEST_ASSERT(ValueType(expected - ValueType(1)) <= ref);

  VISKORES_TEST_ASSERT(ref >= ref);
  VISKORES_TEST_ASSERT(ref >= ValueType(expected - ValueType(1)));
  VISKORES_TEST_ASSERT(ValueType(expected + ValueType(1)) >= ref);
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(viskores::Id,
                       viskores::internal::ArrayPortalValueReference<ArrayPortalType>,
                       viskores::TypeTraitsVectorTag)
{
}

template <typename ArrayPortalType>
void TryOperatorsInt(viskores::Id index,
                     viskores::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                     viskores::internal::ArrayPortalValueReference<ArrayPortalType> scratch,
                     viskores::TypeTraitsScalarTag,
                     viskores::TypeTraitsIntegerTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  ValueType expected = TestValue(index, ValueType());
  VISKORES_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  VISKORES_TEST_ASSERT((ref % ref) == (expected % expected));
  VISKORES_TEST_ASSERT((ref % expected) == (expected % expected));
  VISKORES_TEST_ASSERT((expected % ref) == (expected % expected));

  VISKORES_TEST_ASSERT((ref ^ ref) == (expected ^ expected));
  VISKORES_TEST_ASSERT((ref ^ expected) == (expected ^ expected));
  VISKORES_TEST_ASSERT((expected ^ ref) == (expected ^ expected));

  VISKORES_TEST_ASSERT((ref | ref) == (expected | expected));
  VISKORES_TEST_ASSERT((ref | expected) == (expected | expected));
  VISKORES_TEST_ASSERT((expected | ref) == (expected | expected));

  VISKORES_TEST_ASSERT((ref & ref) == (expected & expected));
  VISKORES_TEST_ASSERT((ref & expected) == (expected & expected));
  VISKORES_TEST_ASSERT((expected & ref) == (expected & expected));

  VISKORES_TEST_ASSERT((ref << ref) == (expected << expected));
  VISKORES_TEST_ASSERT((ref << expected) == (expected << expected));
  VISKORES_TEST_ASSERT((expected << ref) == (expected << expected));

  VISKORES_TEST_ASSERT((ref << ref) == (expected << expected));
  VISKORES_TEST_ASSERT((ref << expected) == (expected << expected));
  VISKORES_TEST_ASSERT((expected << ref) == (expected << expected));

  VISKORES_TEST_ASSERT(~ref == ~expected);

  VISKORES_TEST_ASSERT(!(!ref));

  VISKORES_TEST_ASSERT(ref && ref);
  VISKORES_TEST_ASSERT(ref && expected);
  VISKORES_TEST_ASSERT(expected && ref);

  VISKORES_TEST_ASSERT(ref || ref);
  VISKORES_TEST_ASSERT(ref || expected);
  VISKORES_TEST_ASSERT(expected || ref);

  scratch = ValueType(7);
  const ValueType operand = ValueType(7);
#define RESET                          \
  ref = TestValue(index, ValueType()); \
  expected = TestValue(index, ValueType());

  RESET;
  ref &= scratch;
  expected &= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref &= operand;
  expected &= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref |= scratch;
  expected |= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref |= operand;
  expected |= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref >>= scratch;
  expected >>= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref >>= operand;
  expected >>= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref <<= scratch;
  expected <<= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref <<= operand;
  expected <<= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref ^= scratch;
  expected ^= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref ^= operand;
  expected ^= operand;
  VISKORES_TEST_ASSERT(ref == expected);

#undef RESET
}

template <typename ArrayPortalType, typename DimTag, typename NumericTag>
void TryOperatorsInt(viskores::Id,
                     viskores::internal::ArrayPortalValueReference<ArrayPortalType>,
                     viskores::internal::ArrayPortalValueReference<ArrayPortalType>,
                     DimTag,
                     NumericTag)
{
}

template <typename ArrayPortalType>
void TryOperators(viskores::Id index,
                  viskores::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                  viskores::internal::ArrayPortalValueReference<ArrayPortalType> scratch)
{
  using ValueType = typename ArrayPortalType::ValueType;

  ValueType expected = TestValue(index, ValueType());
  VISKORES_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  // Test comparison operators.
  VISKORES_TEST_ASSERT(ref == ref);
  VISKORES_TEST_ASSERT(ref == expected);
  VISKORES_TEST_ASSERT(expected == ref);

  VISKORES_TEST_ASSERT(!(ref != ref));
  VISKORES_TEST_ASSERT(!(ref != expected));
  VISKORES_TEST_ASSERT(!(expected != ref));

  TryOperatorsNoVec(index, ref, typename viskores::TypeTraits<ValueType>::DimensionalityTag());

  VISKORES_TEST_ASSERT((ref + ref) == (expected + expected));
  VISKORES_TEST_ASSERT((ref + expected) == (expected + expected));
  VISKORES_TEST_ASSERT((expected + ref) == (expected + expected));

  VISKORES_TEST_ASSERT((ref - ref) == (expected - expected));
  VISKORES_TEST_ASSERT((ref - expected) == (expected - expected));
  VISKORES_TEST_ASSERT((expected - ref) == (expected - expected));

  VISKORES_TEST_ASSERT((ref * ref) == (expected * expected));
  VISKORES_TEST_ASSERT((ref * expected) == (expected * expected));
  VISKORES_TEST_ASSERT((expected * ref) == (expected * expected));

  VISKORES_TEST_ASSERT((ref / ref) == (expected / expected));
  VISKORES_TEST_ASSERT((ref / expected) == (expected / expected));
  VISKORES_TEST_ASSERT((expected / ref) == (expected / expected));


  scratch = ValueType(7);
  const ValueType operand = ValueType(7);
#define RESET                          \
  ref = TestValue(index, ValueType()); \
  expected = TestValue(index, ValueType());

  RESET;
  ref += scratch;
  expected += operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref += operand;
  expected += operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref -= scratch;
  expected -= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref -= operand;
  expected -= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref *= scratch;
  expected *= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref *= operand;
  expected *= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  ref /= scratch;
  expected /= operand;
  VISKORES_TEST_ASSERT(ref == expected);
  RESET;
  ref /= operand;
  expected /= operand;
  VISKORES_TEST_ASSERT(ref == expected);

  RESET;
  TryOperatorsInt(index,
                  ref,
                  scratch,
                  typename viskores::TypeTraits<ValueType>::DimensionalityTag(),
                  typename viskores::TypeTraits<ValueType>::NumericTag());

#undef RESET
}

struct DoTestForType
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType&) const
  {
    viskores::cont::ArrayHandle<ValueType> array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Set array using reference" << std::endl;
    using PortalType = typename viskores::cont::ArrayHandle<ValueType>::WritePortalType;
    PortalType portal = array.WritePortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      SetReference(index, viskores::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check values" << std::endl;
    CheckPortal(portal);

    std::cout << "Check references in set array." << std::endl;
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      CheckReference(index,
                     viskores::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Make a scratch buffer for ref-ref operations." << std::endl;
    viskores::cont::ArrayHandle<ValueType> scratchArray;
    scratchArray.Allocate(1);
    PortalType scratchPortal = scratchArray.WritePortal();

    std::cout << "Check that operators work." << std::endl;
    // Start at 1 to avoid issues with 0.
    for (viskores::Id index = 1; index < ARRAY_SIZE; ++index)
    {
      TryOperators(index,
                   viskores::internal::ArrayPortalValueReference<PortalType>(portal, index),
                   viskores::internal::ArrayPortalValueReference<PortalType>(scratchPortal, 0));
    }
  }
};

void DoTest()
{
  // We are not testing on the default (exemplar) types because we want to test operators, and
  // many basic C types could fail on basic operations. Small integer types (such as unsigned
  // bytes) get automatically promoted to larger types, so doing somthing like a += operation
  // causes annoying compiler warnings. Float types are also problematic because comparison
  // operations like == can fail even when you expect the values to be the same.
  viskores::testing::Testing::TryTypes(
    DoTestForType(), viskores::List<viskores::Int32, viskores::UInt64, viskores::Id3>());
}

} // anonymous namespace

int UnitTestArrayPortalValueReference(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(DoTest, argc, argv);
}
