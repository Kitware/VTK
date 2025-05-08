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

#include <viskores/List.h>

#include <viskores/testing/Testing.h>

#include <vector>

namespace
{

template <int N>
struct TestClass : std::integral_constant<int, N>
{
};

} // anonymous namespace

namespace viskores
{
namespace testing
{

template <int N>
struct TypeName<TestClass<N>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "TestClass<" << N << ">";
    return stream.str();
  }
};
}
} // namespace viskores::testing

namespace
{

template <typename T>
struct DoubleTransformLazy;
template <int N>
struct DoubleTransformLazy<TestClass<N>>
{
  using type = TestClass<2 * N>;
};

template <typename T>
using DoubleTransform = typename DoubleTransformLazy<T>::type;

template <typename T>
struct EvenPredicate;
template <int N>
struct EvenPredicate<TestClass<N>> : std::integral_constant<bool, (N % 2) == 0>
{
};

template <typename T>
using OddPredicate = viskores::internal::meta::Not<EvenPredicate<T>>;

template <typename T1, typename T2>
struct AddOperator : TestClass<T1::value + T2::value>
{
};

template <typename T1, typename T2>
void CheckSame(T1, T2)
{
  VISKORES_STATIC_ASSERT((std::is_same<T1, T2>::value));

  std::cout << "     Got expected type: " << viskores::testing::TypeName<T1>::Name() << std::endl;
}

template <typename ExpectedList, typename List>
void CheckList(ExpectedList, List)
{
  VISKORES_IS_LIST(List);
  CheckSame(ExpectedList{}, List{});
}

template <int N>
int test_number(TestClass<N>)
{
  return N;
}

template <typename T>
struct MutableFunctor
{
  std::vector<T> FoundTypes;

  template <typename U>
  VISKORES_CONT void operator()(U u)
  {
    this->FoundTypes.push_back(test_number(u));
  }
};

template <typename T>
struct ConstantFunctor
{
  template <typename U, typename VectorType>
  VISKORES_CONT void operator()(U u, VectorType& vector) const
  {
    vector.push_back(test_number(u));
  }
};

void TryForEach()
{
  using TestList = viskores::
    List<TestClass<1>, TestClass<1>, TestClass<2>, TestClass<3>, TestClass<5>, TestClass<8>>;
  const std::vector<int> expectedList = { 1, 1, 2, 3, 5, 8 };

  std::cout << "Check mutable for each" << std::endl;
  MutableFunctor<int> functor;
  viskores::ListForEach(functor, TestList{});
  VISKORES_TEST_ASSERT(expectedList == functor.FoundTypes);

  std::cout << "Check constant for each" << std::endl;
  std::vector<int> foundTypes;
  viskores::ListForEach(ConstantFunctor<int>{}, TestList{}, foundTypes);
  VISKORES_TEST_ASSERT(expectedList == foundTypes);
}

void TestLists()
{
  using SimpleCount = viskores::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>>;
  using EvenList = viskores::List<TestClass<2>, TestClass<4>, TestClass<6>, TestClass<8>>;
  using LongList = viskores::List<TestClass<1>,
                                  TestClass<2>,
                                  TestClass<3>,
                                  TestClass<4>,
                                  TestClass<5>,
                                  TestClass<6>,
                                  TestClass<7>,
                                  TestClass<8>,
                                  TestClass<9>,
                                  TestClass<10>,
                                  TestClass<11>,
                                  TestClass<12>,
                                  TestClass<13>,
                                  TestClass<14>>;
  using RepeatList = viskores::List<TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<1>,
                                    TestClass<14>>;

  TryForEach();

  std::cout << "Valid List Tag Checks" << std::endl;
  VISKORES_TEST_ASSERT(viskores::internal::IsList<viskores::List<TestClass<11>>>::value);
  VISKORES_TEST_ASSERT(
    viskores::internal::IsList<viskores::List<TestClass<21>, TestClass<22>>>::value);
  VISKORES_TEST_ASSERT(viskores::internal::IsList<viskores::ListEmpty>::value);
  VISKORES_TEST_ASSERT(viskores::internal::IsList<viskores::ListUniversal>::value);

  std::cout << "ListEmpty" << std::endl;
  CheckList(viskores::List<>{}, viskores::ListEmpty{});

  std::cout << "ListAppend" << std::endl;
  CheckList(viskores::List<TestClass<31>,
                           TestClass<32>,
                           TestClass<33>,
                           TestClass<11>,
                           TestClass<21>,
                           TestClass<22>>{},
            viskores::ListAppend<viskores::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                                 viskores::List<TestClass<11>>,
                                 viskores::List<TestClass<21>, TestClass<22>>>{});

  std::cout << "ListFill" << std::endl;
  CheckList(viskores::List<int, int, int, int, int>{}, viskores::ListFill<int, 5>{});

  std::cout << "ListTransform" << std::endl;
  CheckList(EvenList{}, viskores::ListTransform<SimpleCount, DoubleTransform>{});

  std::cout << "ListRemoveIf" << std::endl;
  CheckList(viskores::List<TestClass<1>, TestClass<3>>{},
            viskores::ListRemoveIf<SimpleCount, EvenPredicate>{});

  std::cout << "ListIntersect" << std::endl;
  CheckList(viskores::List<TestClass<3>, TestClass<5>>{},
            viskores::ListIntersect<
              viskores::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>, TestClass<5>>,
              viskores::List<TestClass<3>, TestClass<5>, TestClass<6>>>{});
  CheckList(
    viskores::List<TestClass<1>, TestClass<2>>{},
    viskores::ListIntersect<viskores::List<TestClass<1>, TestClass<2>>, viskores::ListUniversal>{});
  CheckList(
    viskores::List<TestClass<1>, TestClass<2>>{},
    viskores::ListIntersect<viskores::ListUniversal, viskores::List<TestClass<1>, TestClass<2>>>{});

  std::cout << "ListSize" << std::endl;
  VISKORES_TEST_ASSERT(viskores::ListSize<viskores::ListEmpty>::value == 0);
  VISKORES_TEST_ASSERT(viskores::ListSize<viskores::List<TestClass<2>>>::value == 1);
  VISKORES_TEST_ASSERT(viskores::ListSize<viskores::List<TestClass<2>, TestClass<4>>>::value == 2);

  std::cout << "ListCross" << std::endl;
  CheckList(viskores::List<viskores::List<TestClass<31>, TestClass<11>>,
                           viskores::List<TestClass<31>, TestClass<12>>,
                           viskores::List<TestClass<32>, TestClass<11>>,
                           viskores::List<TestClass<32>, TestClass<12>>,
                           viskores::List<TestClass<33>, TestClass<11>>,
                           viskores::List<TestClass<33>, TestClass<12>>>{},
            viskores::ListCross<viskores::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                                viskores::List<TestClass<11>, TestClass<12>>>{});

  std::cout << "ListAt" << std::endl;
  CheckSame(TestClass<2>{}, viskores::ListAt<EvenList, 0>{});
  CheckSame(TestClass<4>{}, viskores::ListAt<EvenList, 1>{});
  CheckSame(TestClass<6>{}, viskores::ListAt<EvenList, 2>{});
  CheckSame(TestClass<8>{}, viskores::ListAt<EvenList, 3>{});

  std::cout << "ListIndexOf" << std::endl;
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<EvenList, TestClass<2>>::value == 0);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<EvenList, TestClass<4>>::value == 1);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<EvenList, TestClass<6>>::value == 2);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<EvenList, TestClass<8>>::value == 3);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<EvenList, TestClass<1>>::value == -1);

  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<1>>::value == 0);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<2>>::value == 1);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<3>>::value == 2);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<4>>::value == 3);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<5>>::value == 4);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<6>>::value == 5);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<7>>::value == 6);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<8>>::value == 7);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<9>>::value == 8);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<10>>::value == 9);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<11>>::value == 10);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<12>>::value == 11);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<13>>::value == 12);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<14>>::value == 13);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<15>>::value == -1);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<LongList, TestClass<0>>::value == -1);

  VISKORES_TEST_ASSERT(viskores::ListIndexOf<RepeatList, TestClass<0>>::value == -1);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<RepeatList, TestClass<1>>::value == 0);
  VISKORES_TEST_ASSERT(viskores::ListIndexOf<RepeatList, TestClass<14>>::value == 13);

  std::cout << "ListHas" << std::endl;
  VISKORES_TEST_ASSERT(viskores::ListHas<EvenList, TestClass<2>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<EvenList, TestClass<4>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<EvenList, TestClass<6>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<EvenList, TestClass<8>>::value);
  VISKORES_TEST_ASSERT(!viskores::ListHas<EvenList, TestClass<1>>::value);

  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<1>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<2>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<3>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<4>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<5>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<6>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<7>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<7>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<8>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<9>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<10>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<11>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<12>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<13>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<LongList, TestClass<14>>::value);
  VISKORES_TEST_ASSERT(!viskores::ListHas<LongList, TestClass<15>>::value);
  VISKORES_TEST_ASSERT(!viskores::ListHas<LongList, TestClass<0>>::value);

  VISKORES_TEST_ASSERT(!viskores::ListHas<RepeatList, TestClass<0>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<RepeatList, TestClass<1>>::value);
  VISKORES_TEST_ASSERT(viskores::ListHas<RepeatList, TestClass<14>>::value);

  std::cout << "ListReduce" << std::endl;
  using Zero = std::integral_constant<int, 0>;
  VISKORES_TEST_ASSERT((viskores::ListReduce<SimpleCount, AddOperator, Zero>::value == 10));
  VISKORES_TEST_ASSERT((viskores::ListReduce<EvenList, AddOperator, Zero>::value == 20));
  VISKORES_TEST_ASSERT((viskores::ListReduce<LongList, AddOperator, Zero>::value == 105));
  VISKORES_TEST_ASSERT((viskores::ListReduce<RepeatList, AddOperator, Zero>::value == 27));

  std::cout << "ListAll" << std::endl;
  VISKORES_TEST_ASSERT(
    (viskores::ListAll<viskores::ListTransform<SimpleCount, EvenPredicate>>::value == false));
  VISKORES_TEST_ASSERT(
    (viskores::ListAll<viskores::ListTransform<EvenList, EvenPredicate>>::value == true));
  VISKORES_TEST_ASSERT(
    (viskores::ListAll<viskores::ListTransform<LongList, EvenPredicate>>::value == false));
  VISKORES_TEST_ASSERT((viskores::ListAll<viskores::List<>>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAll<SimpleCount, EvenPredicate>::value == false));
  VISKORES_TEST_ASSERT((viskores::ListAll<EvenList, EvenPredicate>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAll<LongList, EvenPredicate>::value == false));
  VISKORES_TEST_ASSERT((viskores::ListAll<viskores::List<>, EvenPredicate>::value == true));

  std::cout << "ListAny" << std::endl;
  VISKORES_TEST_ASSERT(
    (viskores::ListAny<viskores::ListTransform<SimpleCount, EvenPredicate>>::value == true));
  VISKORES_TEST_ASSERT(
    (viskores::ListAny<viskores::ListTransform<EvenList, EvenPredicate>>::value == true));
  VISKORES_TEST_ASSERT(
    (viskores::ListAny<viskores::ListTransform<EvenList, OddPredicate>>::value == false));
  VISKORES_TEST_ASSERT(
    (viskores::ListAny<viskores::ListTransform<LongList, EvenPredicate>>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAny<viskores::List<>>::value == false));
  VISKORES_TEST_ASSERT((viskores::ListAny<SimpleCount, EvenPredicate>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAny<EvenList, EvenPredicate>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAny<EvenList, OddPredicate>::value == false));
  VISKORES_TEST_ASSERT((viskores::ListAny<LongList, EvenPredicate>::value == true));
  VISKORES_TEST_ASSERT((viskores::ListAny<viskores::List<>, EvenPredicate>::value == false));
}

} // anonymous namespace

int UnitTestList(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestLists, argc, argv);
}
