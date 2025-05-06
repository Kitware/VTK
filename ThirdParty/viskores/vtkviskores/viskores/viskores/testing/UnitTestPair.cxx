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

#include <viskores/Matrix.h>
#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

#include <viskoresstd/is_trivial.h>

#include <viskores/testing/Testing.h>

namespace
{

template <typename T, typename U>
void PairTestConstructors()
{
  std::cout << "test that all the constructors work properly" << std::endl;
  viskores::Pair<T, U> no_params_pair;
  no_params_pair.first = TestValue(12, T());
  no_params_pair.second = TestValue(34, U());
  viskores::Pair<T, U> copy_constructor_pair(no_params_pair);
  viskores::Pair<T, U> assignment_pair = no_params_pair;

  VISKORES_TEST_ASSERT((no_params_pair == copy_constructor_pair),
                       "copy constructor doesn't match default constructor");
  VISKORES_TEST_ASSERT(!(no_params_pair != copy_constructor_pair),
                       "operator != is working properly");

  VISKORES_TEST_ASSERT((no_params_pair == assignment_pair),
                       "assignment constructor doesn't match default constructor");
  VISKORES_TEST_ASSERT(!(no_params_pair != assignment_pair), "operator != is working properly");
}

template <typename T, typename U>
void PairTestValues()
{
  std::cout << "Check assignment of values" << std::endl;
  T a = TestValue(56, T());
  U b = TestValue(78, U());

  viskores::Pair<T, U> pair_ab(a, b);
  viskores::Pair<T, U> copy_constructor_pair(pair_ab);
  viskores::Pair<T, U> assignment_pair = pair_ab;
  viskores::Pair<T, U> make_p = viskores::make_Pair(a, b);

  VISKORES_TEST_ASSERT(!(pair_ab != pair_ab),
                       "operator != isn't working properly for viskores::Pair");
  VISKORES_TEST_ASSERT((pair_ab == pair_ab),
                       "operator == isn't working properly for viskores::Pair");

  VISKORES_TEST_ASSERT((pair_ab == copy_constructor_pair),
                       "copy constructor doesn't match pair constructor");
  VISKORES_TEST_ASSERT((pair_ab == assignment_pair),
                       "assignment constructor doesn't match pair constructor");

  VISKORES_TEST_ASSERT(copy_constructor_pair.first == a, "first field not set right");
  VISKORES_TEST_ASSERT(assignment_pair.second == b, "second field not set right");

  VISKORES_TEST_ASSERT((pair_ab == make_p), "make_pair function doesn't match pair constructor");
}

template <typename T>
T NextValue(T x)
{
  return x + T(1);
}

template <typename T, typename U>
viskores::Pair<T, U> NextValue(viskores::Pair<T, U> x)
{
  return viskores::make_Pair(NextValue(x.first), NextValue(x.second));
}

template <typename T, typename U>
void PairTestOrdering()
{
  std::cout << "Check that ordering operations work" << std::endl;
  //in all cases pair_ab2 is > pair_ab. these verify that if the second
  //argument of the pair is different we respond properly
  T a = TestValue(67, T());
  U b = TestValue(89, U());

  U b2(b);
  viskores::VecTraits<U>::SetComponent(
    b2, 0, NextValue(viskores::VecTraits<U>::GetComponent(b2, 0)));

  viskores::Pair<T, U> pair_ab2(a, b2);
  viskores::Pair<T, U> pair_ab(a, b);

  VISKORES_TEST_ASSERT((pair_ab2 >= pair_ab), "operator >= failed");
  VISKORES_TEST_ASSERT((pair_ab2 >= pair_ab2), "operator >= failed");

  VISKORES_TEST_ASSERT((pair_ab2 > pair_ab), "operator > failed");
  VISKORES_TEST_ASSERT(!(pair_ab2 > pair_ab2), "operator > failed");

  VISKORES_TEST_ASSERT(!(pair_ab2 < pair_ab), "operator < failed");
  VISKORES_TEST_ASSERT(!(pair_ab2 < pair_ab2), "operator < failed");

  VISKORES_TEST_ASSERT(!(pair_ab2 <= pair_ab), "operator <= failed");
  VISKORES_TEST_ASSERT((pair_ab2 <= pair_ab2), "operator <= failed");

  VISKORES_TEST_ASSERT(!(pair_ab2 == pair_ab), "operator == failed");
  VISKORES_TEST_ASSERT((pair_ab2 != pair_ab), "operator != failed");

  T a2(a);
  viskores::VecTraits<T>::SetComponent(
    a2, 0, NextValue(viskores::VecTraits<T>::GetComponent(a2, 0)));
  viskores::Pair<T, U> pair_a2b(a2, b);
  //this way can verify that if the first argument of the pair is different
  //we respond properly
  VISKORES_TEST_ASSERT((pair_a2b >= pair_ab), "operator >= failed");
  VISKORES_TEST_ASSERT((pair_a2b >= pair_a2b), "operator >= failed");

  VISKORES_TEST_ASSERT((pair_a2b > pair_ab), "operator > failed");
  VISKORES_TEST_ASSERT(!(pair_a2b > pair_a2b), "operator > failed");

  VISKORES_TEST_ASSERT(!(pair_a2b < pair_ab), "operator < failed");
  VISKORES_TEST_ASSERT(!(pair_a2b < pair_a2b), "operator < failed");

  VISKORES_TEST_ASSERT(!(pair_a2b <= pair_ab), "operator <= failed");
  VISKORES_TEST_ASSERT((pair_a2b <= pair_a2b), "operator <= failed");

  VISKORES_TEST_ASSERT(!(pair_a2b == pair_ab), "operator == failed");
  VISKORES_TEST_ASSERT((pair_a2b != pair_ab), "operator != failed");
}

//general pair test
template <typename T, typename U>
void PairTest()
{
  {
    using P = viskores::Pair<T, U>;

    // Pair types should preserve the trivial properties of their components.
    // This insures that algorithms like std::copy will optimize fully.
    // (Note, if std::is_trivial is not supported by the compiler, then
    // viskoresstd::is_trivial will always report false, but VISKORES_IS_TRIVIAL will
    // always succeed.)
    VISKORES_IS_TRIVIAL(T);
    VISKORES_TEST_ASSERT(viskoresstd::is_trivial<U>::value == viskoresstd::is_trivial<P>::value,
                         "PairType's triviality differs from ComponentTypes.");
  }

  PairTestConstructors<T, U>();

  PairTestValues<T, U>();

  PairTestOrdering<T, U>();
}

using PairTypesToTry =
  viskores::List<viskores::Int8,                                     // Integer types
                 viskores::FloatDefault,                             // Float types
                 viskores::Id3,                                      // Vec types
                 viskores::Pair<viskores::Vec3f_32, viskores::Int64> // Recursive Pairs
                 >;

template <typename FirstType>
struct DecideSecondType
{
  template <typename SecondType>
  void operator()(const SecondType&) const
  {
    PairTest<FirstType, SecondType>();
  }
};

struct DecideFirstType
{
  template <typename T>
  void operator()(const T&) const
  {
    //T is our first type for viskores::Pair, now to figure out the second type
    viskores::testing::Testing::TryTypes(DecideSecondType<T>(), PairTypesToTry());
  }
};

void TestPair()
{
  //we want to test each combination of standard viskores types in a
  //viskores::Pair, so to do that we dispatch twice on TryTypes. We could
  //dispatch on all types, but that gets excessively large and takes a
  //long time to compile (although it runs fast). Instead, just select
  //a subset of non-trivial combinations.
  viskores::testing::Testing::TryTypes(DecideFirstType(), PairTypesToTry());
}

} // anonymous namespace

int UnitTestPair(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestPair, argc, argv);
}
