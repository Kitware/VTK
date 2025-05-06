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

#include <viskores/BinaryOperators.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>

#include <viskores/TypeTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{
// The goal of this unit test is not to verify the correctness
// of the various algorithms. Since Algorithm is a header, we
// need to ensure we instantiate each algorithm in a source
// file to verify compilation.
//
static constexpr viskores::Id ARRAY_SIZE = 10;

TestEqualResult checkBitField(const viskores::cont::BitField& bitfield,
                              std::initializer_list<bool>&& expected)
{
  TestEqualResult result;
  if (bitfield.GetNumberOfBits() != static_cast<viskores::Id>(expected.size()))
  {
    result.PushMessage("Unexpected number of bits (" + std::to_string(bitfield.GetNumberOfBits()) +
                       ")");
    return result;
  }

  auto expectedBit = expected.begin();
  auto bitPortal = bitfield.ReadPortal();
  for (viskores::Id index = 0; index < bitPortal.GetNumberOfBits(); ++index)
  {
    if (bitPortal.GetBit(index) != *expectedBit)
    {
      result.PushMessage("Bad bit at index " + std::to_string(index));
    }
    ++expectedBit;
  }

  return result;
}

template <typename T>
TestEqualResult checkArrayHandle(const viskores::cont::UnknownArrayHandle& array,
                                 std::initializer_list<T>&& expected)
{
  return test_equal_ArrayHandles(array, viskores::cont::make_ArrayHandle(std::move(expected)));
}

void FillTest()
{
  viskores::cont::BitField bits;
  viskores::cont::ArrayHandle<viskores::Id> array;

  bits.Allocate(ARRAY_SIZE);
  array.Allocate(ARRAY_SIZE);

  viskores::cont::Algorithm::Fill(bits, true);
  VISKORES_TEST_ASSERT(
    checkBitField(bits, { true, true, true, true, true, true, true, true, true, true }));
  viskores::cont::Algorithm::Fill(bits, false, 5);
  VISKORES_TEST_ASSERT(checkBitField(bits, { false, false, false, false, false }));
  viskores::cont::Algorithm::Fill(bits, viskores::UInt8(0xab));
  bits.Allocate(8);
  VISKORES_TEST_ASSERT(checkBitField(bits, { true, true, false, true, false, true, false, true }));
  viskores::cont::Algorithm::Fill(bits, viskores::UInt8(0xab), 5);
  VISKORES_TEST_ASSERT(checkBitField(bits, { true, true, false, true, false }));
  viskores::cont::Algorithm::Fill(array, viskores::Id(5));
  VISKORES_TEST_ASSERT(checkArrayHandle(array, { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 }));
  viskores::cont::Algorithm::Fill(array, viskores::Id(6), 5);
  VISKORES_TEST_ASSERT(checkArrayHandle(array, { 6, 6, 6, 6, 6 }));
}

void CopyTest()
{
  viskores::cont::ArrayHandleIndex input(ARRAY_SIZE);
  viskores::cont::ArrayHandle<viskores::Id> output;
  viskores::cont::ArrayHandle<viskores::Id> stencil =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 0, 0, 1, 8, 9, 2 });

  viskores::cont::Algorithm::Copy(input, output);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(input, output));
  viskores::cont::Algorithm::CopyIf(input, stencil, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 1, 2, 3, 6, 7, 8, 9 }));
  viskores::cont::Algorithm::CopyIf(input, stencil, output, viskores::LogicalNot());
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 4, 5 }));
  viskores::cont::Algorithm::CopySubRange(input, 2, 1, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 2, 4, 5 }));
}

struct CustomCompare
{
  template <typename T>
  VISKORES_EXEC bool operator()(T a, T b) const
  {
    return (2 * a) < b;
  }
};

void BoundsTest()
{

  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 1, 2, 3, 5, 8, 13, 21, 34 });
  viskores::cont::ArrayHandle<viskores::Id> values =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 4, 9, 16, 25, 36, 49 });
  viskores::cont::ArrayHandle<viskores::Id> output;

  viskores::cont::Algorithm::LowerBounds(input, values, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 5, 7, 8, 9, 10, 10 }));
  viskores::cont::Algorithm::LowerBounds(input, values, output, CustomCompare{});
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 3, 5, 6, 7, 8, 9 }));
  viskores::cont::ArrayCopy(values, output);
  viskores::cont::Algorithm::LowerBounds(input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 5, 7, 8, 9, 10, 10 }));

  viskores::cont::Algorithm::UpperBounds(input, values, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 1, 3, 5, 7, 8, 9, 10, 10 }));
  viskores::cont::Algorithm::UpperBounds(input, values, output, CustomCompare{});
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 1, 4, 7, 8, 9, 10, 10, 10 }));
  viskores::cont::ArrayCopy(values, output);
  viskores::cont::Algorithm::UpperBounds(input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 1, 3, 5, 7, 8, 9, 10, 10 }));
}

void ReduceTest()
{

  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  viskores::cont::ArrayHandle<viskores::Id> keys =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });
  viskores::cont::ArrayHandle<viskores::Id> keysOut;
  viskores::cont::ArrayHandle<viskores::Id> valsOut;

  viskores::Id result;
  result = viskores::cont::Algorithm::Reduce(input, viskores::Id(0));
  VISKORES_TEST_ASSERT(test_equal(result, 51));
  result = viskores::cont::Algorithm::Reduce(input, viskores::Id(0), viskores::Maximum());
  VISKORES_TEST_ASSERT(test_equal(result, 9));
  viskores::cont::Algorithm::ReduceByKey(keys, input, keysOut, valsOut, viskores::Maximum());
  VISKORES_TEST_ASSERT(checkArrayHandle(keysOut, { 0, 1, 2, 5 }));
  VISKORES_TEST_ASSERT(checkArrayHandle(valsOut, { 6, 1, 9, 8 }));
}

void ScanTest()
{

  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  viskores::cont::ArrayHandle<viskores::Id> keys =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });
  viskores::cont::ArrayHandle<viskores::Id> output;

  viskores::Id out;
  out = viskores::cont::Algorithm::ScanInclusive(input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 6, 8, 13, 14, 23, 29, 30, 35, 43, 51 }));
  VISKORES_TEST_ASSERT(test_equal(out, 51));
  out = viskores::cont::Algorithm::ScanInclusive(input, output, viskores::Maximum());
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 6, 6, 6, 6, 9, 9, 9, 9, 9, 9 }));
  VISKORES_TEST_ASSERT(test_equal(out, 9));
  viskores::cont::Algorithm::ScanInclusiveByKey(keys, input, output, viskores::Maximum());
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 6, 6, 6, 1, 9, 9, 1, 5, 8, 8 }));
  viskores::cont::Algorithm::ScanInclusiveByKey(keys, input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 6, 8, 13, 1, 9, 15, 1, 6, 14, 22 }));
  out =
    viskores::cont::Algorithm::ScanExclusive(input, output, viskores::Maximum(), viskores::Id(0));
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 6, 6, 9, 9, 9, 9, 9 }));
  VISKORES_TEST_ASSERT(test_equal(out, 9));
  viskores::cont::Algorithm::ScanExclusiveByKey(
    keys, input, output, viskores::Id(0), viskores::Maximum());
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 0, 0, 9, 0, 1, 5, 8 }));
  viskores::cont::Algorithm::ScanExclusiveByKey(keys, input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 8, 0, 0, 9, 0, 1, 6, 14 }));
  viskores::cont::Algorithm::ScanExtended(input, output);
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 8, 13, 14, 23, 29, 30, 35, 43, 51 }));
  viskores::cont::Algorithm::ScanExtended(input, output, viskores::Maximum(), viskores::Id(0));
  VISKORES_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 6, 6, 9, 9, 9, 9, 9, 9 }));
}

struct DummyFunctor : public viskores::exec::FunctorBase
{
  template <typename IdType>
  VISKORES_EXEC void operator()(IdType) const
  {
  }
};

void ScheduleTest()
{
  viskores::cont::Algorithm::Schedule(DummyFunctor(), viskores::Id(1));
  viskores::Id3 id3(1, 1, 1);
  viskores::cont::Algorithm::Schedule(DummyFunctor(), id3);
}

struct CompFunctor
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x > y;
  }
};

struct CompExecObject : viskores::cont::ExecutionObjectBase
{
  VISKORES_CONT CompFunctor PrepareForExecution(viskores::cont::DeviceAdapterId,
                                                viskores::cont::Token&)
  {
    return CompFunctor();
  }
};

void SortTest()
{
  viskores::cont::ArrayHandle<viskores::Id> input;
  viskores::cont::ArrayHandle<viskores::Id> keys =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  viskores::cont::Algorithm::Sort(input);
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 }));

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  viskores::cont::Algorithm::Sort(input, CompFunctor());
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  viskores::cont::Algorithm::Sort(input, CompExecObject());
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));

  keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 4, 0, 3, 2, 5, 5 });
  viskores::cont::Algorithm::SortByKey(keys, input);
  VISKORES_TEST_ASSERT(checkArrayHandle(keys, { 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 }));
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 3, 3, 1, 2, 2, 0, 0, 5, 5, 4 }));

  keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 4, 0, 3, 2, 5, 5 });
  viskores::cont::Algorithm::SortByKey(keys, input, CompFunctor());
  VISKORES_TEST_ASSERT(checkArrayHandle(keys, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 4, 5, 5, 0, 0, 2, 2, 1, 3, 3 }));
  viskores::cont::Algorithm::SortByKey(keys, input, CompExecObject());
}

void SynchronizeTest()
{
  viskores::cont::Algorithm::Synchronize();
}

void TransformTest()
{
  auto transformInput =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 3, 5, 7, 9, 11, 13, 15 });
  auto transformInputOutput =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 2, 4, 8, 10, 12, 14, 16 });
  auto transformExpectedResult =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 5, 9, 15, 19, 23, 27, 31 });

  // Test simple call on two different arrays
  std::cout << "Testing Transform for summing arrays" << std::endl;
  viskores::cont::ArrayHandle<viskores::Id> transformOutput;
  viskores::cont::Algorithm::Transform(
    transformInput, transformInputOutput, transformOutput, viskores::Sum{});
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(transformOutput, transformExpectedResult));

  // Test using an array as both input and output
  std::cout << "Testing Transform with array for both input and output" << std::endl;
  viskores::cont::Algorithm::Transform(
    transformInputOutput, transformInput, transformInputOutput, viskores::Sum{});
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(transformInputOutput, transformExpectedResult));
}

struct Within3Functor
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return (x / 3) == (y / 3);
  }
};

struct Within3ExecObject : viskores::cont::ExecutionObjectBase
{
  VISKORES_CONT Within3Functor PrepareForExecution(viskores::cont::DeviceAdapterId,
                                                   viskores::cont::Token&)
  {
    return Within3Functor();
  }
};

void UniqueTest()
{
  viskores::cont::ArrayHandle<viskores::Id> input;

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  viskores::cont::Algorithm::Unique(input);
  VISKORES_TEST_ASSERT(checkArrayHandle(input, { 1, 2, 5, 6, 8, 9 }));

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  viskores::cont::Algorithm::Unique(input, Within3Functor());
  viskores::cont::printSummary_ArrayHandle(input, std::cout, true);
  // The result should be an array of size 4 with the first entry 1 or 2, the second 5,
  // the third 6 or 8, and the fourth 9.
  VISKORES_TEST_ASSERT(input.GetNumberOfValues() == 4);
  VISKORES_TEST_ASSERT(input.ReadPortal().Get(1) == 5);

  input = viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  viskores::cont::Algorithm::Unique(input, Within3ExecObject());
  // The result should be an array of size 4 with the first entry 1 or 2, the second 5,
  // the third 6 or 8, and the fourth 9.
  VISKORES_TEST_ASSERT(input.GetNumberOfValues() == 4);
  VISKORES_TEST_ASSERT(input.ReadPortal().Get(1) == 5);
}

void TestAll()
{
  FillTest();
  CopyTest();
  BoundsTest();
  ReduceTest();
  ScanTest();
  ScheduleTest();
  SortTest();
  SynchronizeTest();
  TransformTest();
  UniqueTest();
}

} // anonymous namespace

int UnitTestAlgorithm(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAll, argc, argv);
}
