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

#include <viskores/Tuple.h>

#include <viskoresstd/integer_sequence.h>

#include <viskores/testing/Testing.h>

namespace
{

// Do some compile-time testing of viskoresstd::integer_sequence. This is only tangentially
// related to Tuple, but the two are often used together.
template <viskores::IdComponent... Ns>
using SequenceId = viskoresstd::integer_sequence<viskores::IdComponent, Ns...>;

template <viskores::IdComponent N>
using MakeSequenceId = viskoresstd::make_integer_sequence<viskores::IdComponent, N>;

VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<0>, SequenceId<>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<1>, SequenceId<0>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<2>, SequenceId<0, 1>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<3>, SequenceId<0, 1, 2>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<5>, SequenceId<0, 1, 2, 3, 4>>::value));
VISKORES_STATIC_ASSERT(
  (std::is_same<MakeSequenceId<8>, SequenceId<0, 1, 2, 3, 4, 5, 6, 7>>::value));
VISKORES_STATIC_ASSERT(
  (std::is_same<MakeSequenceId<13>, SequenceId<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12>>::value));
VISKORES_STATIC_ASSERT(
  (std::is_same<
    MakeSequenceId<21>,
    SequenceId<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<34>,
                                     SequenceId<0,
                                                1,
                                                2,
                                                3,
                                                4,
                                                5,
                                                6,
                                                7,
                                                8,
                                                9,
                                                10,
                                                11,
                                                12,
                                                13,
                                                14,
                                                15,
                                                16,
                                                17,
                                                18,
                                                19,
                                                20,
                                                21,
                                                22,
                                                23,
                                                24,
                                                25,
                                                26,
                                                27,
                                                28,
                                                29,
                                                30,
                                                31,
                                                32,
                                                33>>::value));
VISKORES_STATIC_ASSERT((std::is_same<MakeSequenceId<89>,
                                     SequenceId<0,
                                                1,
                                                2,
                                                3,
                                                4,
                                                5,
                                                6,
                                                7,
                                                8,
                                                9,
                                                10,
                                                11,
                                                12,
                                                13,
                                                14,
                                                15,
                                                16,
                                                17,
                                                18,
                                                19,
                                                20,
                                                21,
                                                22,
                                                23,
                                                24,
                                                25,
                                                26,
                                                27,
                                                28,
                                                29,
                                                30,
                                                31,
                                                32,
                                                33,
                                                34,
                                                35,
                                                36,
                                                37,
                                                38,
                                                39,
                                                40,
                                                41,
                                                42,
                                                43,
                                                44,
                                                45,
                                                46,
                                                47,
                                                48,
                                                49,
                                                50,
                                                51,
                                                52,
                                                53,
                                                54,
                                                55,
                                                56,
                                                57,
                                                58,
                                                59,
                                                60,
                                                61,
                                                62,
                                                63,
                                                64,
                                                65,
                                                66,
                                                67,
                                                68,
                                                69,
                                                70,
                                                71,
                                                72,
                                                73,
                                                74,
                                                75,
                                                76,
                                                77,
                                                78,
                                                79,
                                                80,
                                                81,
                                                82,
                                                83,
                                                84,
                                                85,
                                                86,
                                                87,
                                                88>>::value));

template <viskores::IdComponent Index>
struct TypePlaceholder
{
  viskores::Id X;
  TypePlaceholder(viskores::Id x)
    : X(x)
  {
  }
};

void Check2(TypePlaceholder<0> a0, TypePlaceholder<1> a1)
{
  VISKORES_TEST_ASSERT(a0.X == TestValue(0, viskores::Id{}));
  VISKORES_TEST_ASSERT(a1.X == TestValue(1, viskores::Id{}));
}

void Check22(TypePlaceholder<0> a0,
             TypePlaceholder<1> a1,
             TypePlaceholder<2> a2,
             TypePlaceholder<3> a3,
             TypePlaceholder<4> a4,
             TypePlaceholder<5> a5,
             TypePlaceholder<6> a6,
             TypePlaceholder<7> a7,
             TypePlaceholder<8> a8,
             TypePlaceholder<9> a9,
             TypePlaceholder<10> a10,
             TypePlaceholder<11> a11,
             TypePlaceholder<12> a12,
             TypePlaceholder<13> a13,
             TypePlaceholder<14> a14,
             TypePlaceholder<15> a15,
             TypePlaceholder<16> a16,
             TypePlaceholder<17> a17,
             TypePlaceholder<18> a18,
             TypePlaceholder<19> a19,
             TypePlaceholder<20> a20,
             TypePlaceholder<21> a21)
{
  VISKORES_TEST_ASSERT(a0.X == TestValue(0, viskores::Id{}));
  VISKORES_TEST_ASSERT(a1.X == TestValue(1, viskores::Id{}));
  VISKORES_TEST_ASSERT(a2.X == TestValue(2, viskores::Id{}));
  VISKORES_TEST_ASSERT(a3.X == TestValue(3, viskores::Id{}));
  VISKORES_TEST_ASSERT(a4.X == TestValue(4, viskores::Id{}));
  VISKORES_TEST_ASSERT(a5.X == TestValue(5, viskores::Id{}));
  VISKORES_TEST_ASSERT(a6.X == TestValue(6, viskores::Id{}));
  VISKORES_TEST_ASSERT(a7.X == TestValue(7, viskores::Id{}));
  VISKORES_TEST_ASSERT(a8.X == TestValue(8, viskores::Id{}));
  VISKORES_TEST_ASSERT(a9.X == TestValue(9, viskores::Id{}));
  VISKORES_TEST_ASSERT(a10.X == TestValue(10, viskores::Id{}));
  VISKORES_TEST_ASSERT(a11.X == TestValue(11, viskores::Id{}));
  VISKORES_TEST_ASSERT(a12.X == TestValue(12, viskores::Id{}));
  VISKORES_TEST_ASSERT(a13.X == TestValue(13, viskores::Id{}));
  VISKORES_TEST_ASSERT(a14.X == TestValue(14, viskores::Id{}));
  VISKORES_TEST_ASSERT(a15.X == TestValue(15, viskores::Id{}));
  VISKORES_TEST_ASSERT(a16.X == TestValue(16, viskores::Id{}));
  VISKORES_TEST_ASSERT(a17.X == TestValue(17, viskores::Id{}));
  VISKORES_TEST_ASSERT(a18.X == TestValue(18, viskores::Id{}));
  VISKORES_TEST_ASSERT(a19.X == TestValue(19, viskores::Id{}));
  VISKORES_TEST_ASSERT(a20.X == TestValue(20, viskores::Id{}));
  VISKORES_TEST_ASSERT(a21.X == TestValue(21, viskores::Id{}));
}

struct CheckReturn
{
  template <typename Function, typename... Ts>
  viskores::Id operator()(Function f, Ts... args)
  {
    f(args...);
    return viskores::Id(sizeof...(Ts));
  }
};

struct CheckValues
{
  viskores::IdComponent NumChecked = 0;

  template <viskores::IdComponent Index>
  void operator()(TypePlaceholder<Index> x)
  {
    VISKORES_TEST_ASSERT(x.X == TestValue(Index, viskores::Id{}));
    this->NumChecked++;
  }
};

struct TransformValues
{
  viskores::Id AddValue;
  TransformValues(viskores::Id addValue)
    : AddValue(addValue)
  {
  }

  template <viskores::IdComponent Index>
  viskores::Id operator()(TypePlaceholder<Index> x) const
  {
    return x.X + this->AddValue;
  }
};

void TestTuple2()
{
  using TupleType = viskores::Tuple<TypePlaceholder<0>, TypePlaceholder<1>>;

  VISKORES_STATIC_ASSERT(viskores::TupleSize<TupleType>::value == 2);
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<0>, viskores::TupleElement<0, TupleType>>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<1>, viskores::TupleElement<1, TupleType>>::value));

  TupleType tuple(TestValue(0, viskores::Id()), TestValue(1, viskores::Id()));

  tuple.Apply(Check2);

  viskores::Id result = tuple.Apply(CheckReturn{}, Check2);
  VISKORES_TEST_ASSERT(result == 2);

  CheckValues checkFunctor;
  VISKORES_TEST_ASSERT(checkFunctor.NumChecked == 0);
  tuple.ForEach(checkFunctor);
  VISKORES_TEST_ASSERT(checkFunctor.NumChecked == 2);

  auto transformedTuple = tuple.Transform(TransformValues{ 10 });
  using TransformedTupleType = decltype(transformedTuple);
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<0, TransformedTupleType>, viskores::Id>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<1, TransformedTupleType>, viskores::Id>::value));

  VISKORES_TEST_ASSERT(viskores::Get<0>(transformedTuple) == TestValue(0, viskores::Id{}) + 10);
  VISKORES_TEST_ASSERT(viskores::Get<1>(transformedTuple) == TestValue(1, viskores::Id{}) + 10);
}

void TestTuple22()
{
  using TupleType = viskores::Tuple<TypePlaceholder<0>,
                                    TypePlaceholder<1>,
                                    TypePlaceholder<2>,
                                    TypePlaceholder<3>,
                                    TypePlaceholder<4>,
                                    TypePlaceholder<5>,
                                    TypePlaceholder<6>,
                                    TypePlaceholder<7>,
                                    TypePlaceholder<8>,
                                    TypePlaceholder<9>,
                                    TypePlaceholder<10>,
                                    TypePlaceholder<11>,
                                    TypePlaceholder<12>,
                                    TypePlaceholder<13>,
                                    TypePlaceholder<14>,
                                    TypePlaceholder<15>,
                                    TypePlaceholder<16>,
                                    TypePlaceholder<17>,
                                    TypePlaceholder<18>,
                                    TypePlaceholder<19>,
                                    TypePlaceholder<20>,
                                    TypePlaceholder<21>>;

  VISKORES_STATIC_ASSERT(viskores::TupleSize<TupleType>::value == 22);
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<0>, viskores::TupleElement<0, TupleType>>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<1>, viskores::TupleElement<1, TupleType>>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<20>, viskores::TupleElement<20, TupleType>>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<TypePlaceholder<21>, viskores::TupleElement<21, TupleType>>::value));

  TupleType tuple(TestValue(0, viskores::Id()),
                  TestValue(1, viskores::Id()),
                  TestValue(2, viskores::Id()),
                  TestValue(3, viskores::Id()),
                  TestValue(4, viskores::Id()),
                  TestValue(5, viskores::Id()),
                  TestValue(6, viskores::Id()),
                  TestValue(7, viskores::Id()),
                  TestValue(8, viskores::Id()),
                  TestValue(9, viskores::Id()),
                  TestValue(10, viskores::Id()),
                  TestValue(11, viskores::Id()),
                  TestValue(12, viskores::Id()),
                  TestValue(13, viskores::Id()),
                  TestValue(14, viskores::Id()),
                  TestValue(15, viskores::Id()),
                  TestValue(16, viskores::Id()),
                  TestValue(17, viskores::Id()),
                  TestValue(18, viskores::Id()),
                  TestValue(19, viskores::Id()),
                  TestValue(20, viskores::Id()),
                  TestValue(21, viskores::Id()));

  tuple.Apply(Check22);

  viskores::Id result = tuple.Apply(CheckReturn{}, Check22);
  VISKORES_TEST_ASSERT(result == 22);

  CheckValues checkFunctor;
  VISKORES_TEST_ASSERT(checkFunctor.NumChecked == 0);
  tuple.ForEach(checkFunctor);
  VISKORES_TEST_ASSERT(checkFunctor.NumChecked == 22);

  auto transformedTuple = tuple.Transform(TransformValues{ 10 });
  using TransformedTupleType = decltype(transformedTuple);
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<0, TransformedTupleType>, viskores::Id>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<1, TransformedTupleType>, viskores::Id>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<20, TransformedTupleType>, viskores::Id>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<viskores::TupleElement<21, TransformedTupleType>, viskores::Id>::value));

  VISKORES_TEST_ASSERT(viskores::Get<0>(transformedTuple) == TestValue(0, viskores::Id{}) + 10);
  VISKORES_TEST_ASSERT(viskores::Get<1>(transformedTuple) == TestValue(1, viskores::Id{}) + 10);
  VISKORES_TEST_ASSERT(viskores::Get<20>(transformedTuple) == TestValue(20, viskores::Id{}) + 10);
  VISKORES_TEST_ASSERT(viskores::Get<21>(transformedTuple) == TestValue(21, viskores::Id{}) + 10);
}

void TestTuple()
{
  TestTuple2();
  TestTuple22();
}

} // anonymous namespace

int UnitTestTuple(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestTuple, argc, argv);
}
