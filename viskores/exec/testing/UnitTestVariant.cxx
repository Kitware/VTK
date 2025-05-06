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

#include <viskores/exec/Variant.h>

#include <viskores/testing/Testing.h>

#include <cstdlib>
#include <vector>

namespace test_variant
{

template <viskores::IdComponent Index>
struct TypePlaceholder
{
  viskores::IdComponent Value = Index;
};

// A class that is trivially copiable but not totally trivial.
struct TrivialCopy
{
  viskores::Id Value = 0;
};

static viskores::Id g_NonTrivialCount;

// A class that must is not trivial to copy nor construct.
struct NonTrivial
{
  viskores::Id Value;
  NonTrivial* Self;

  void CheckState() const
  {
    VISKORES_TEST_ASSERT(this->Value == 12345);
    VISKORES_TEST_ASSERT(this->Self == this);
  }

  NonTrivial()
    : Value(12345)
    , Self(this)
  {
    this->CheckState();
    ++g_NonTrivialCount;
  }

  NonTrivial(const NonTrivial& src)
    : Value(src.Value)
  {
    src.CheckState();
    this->Self = this;
    this->CheckState();
    ++g_NonTrivialCount;
  }

  NonTrivial& operator=(const NonTrivial& src)
  {
    this->CheckState();
    src.CheckState();
    return *this;
  }

  ~NonTrivial()
  {
    if ((this->Value == 12345) && (this->Self == this))
    {
      // Normal destruction
      this->Value = -1;
      this->Self = nullptr;
      --g_NonTrivialCount;
    }
    else
    {
      // Normally we would use VISKORES_TEST_ASSERT or VISKORES_TEST_FAIL, but it's not good to throw
      // exceptions from destructors (especially since Variant marks these calls as noexcept).
      // Instead, just check and terminate the program.
      std::cout << "ERROR at " << __FILE__ << ":" << __LINE__ << ":\n";
      std::cout << "Destroying a class that was not properly constructed." << std::endl;
      std::exit(1);
    }
  }
};

void TestSize()
{
  std::cout << "Test size" << std::endl;

  using VariantType = viskores::exec::Variant<float, double, char, short, int, long>;

  constexpr size_t variantSize = sizeof(VariantType);

  VISKORES_TEST_ASSERT(variantSize <= 16,
                       "Size of variant should not be larger than biggest type plus and index. ",
                       variantSize);
}

void TestIndexing()
{
  std::cout << "Test indexing" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>,
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
                                              TypePlaceholder<21>,
                                              TypePlaceholder<22>,
                                              TypePlaceholder<23>,
                                              TypePlaceholder<24>,
                                              TypePlaceholder<25>,
                                              TypePlaceholder<26>,
                                              TypePlaceholder<27>,
                                              TypePlaceholder<28>,
                                              TypePlaceholder<29>>;

  VariantType variant;

  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<0>>::value == 0);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<1>>::value == 1);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<2>>::value == 2);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<3>>::value == 3);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<4>>::value == 4);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<5>>::value == 5);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<6>>::value == 6);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<7>>::value == 7);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<8>>::value == 8);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<9>>::value == 9);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<10>>::value == 10);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<11>>::value == 11);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<12>>::value == 12);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<13>>::value == 13);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<14>>::value == 14);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<15>>::value == 15);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<16>>::value == 16);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<17>>::value == 17);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<18>>::value == 18);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<19>>::value == 19);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<20>>::value == 20);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<21>>::value == 21);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<22>>::value == 22);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<23>>::value == 23);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<24>>::value == 24);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<25>>::value == 25);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<26>>::value == 26);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<27>>::value == 27);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<28>>::value == 28);
  VISKORES_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<29>>::value == 29);

  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<0>>() == 0);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<1>>() == 1);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<2>>() == 2);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<3>>() == 3);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<4>>() == 4);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<5>>() == 5);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<6>>() == 6);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<7>>() == 7);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<8>>() == 8);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<9>>() == 9);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<10>>() == 10);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<11>>() == 11);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<12>>() == 12);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<13>>() == 13);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<14>>() == 14);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<15>>() == 15);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<16>>() == 16);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<17>>() == 17);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<18>>() == 18);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<19>>() == 19);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<20>>() == 20);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<21>>() == 21);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<22>>() == 22);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<23>>() == 23);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<24>>() == 24);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<25>>() == 25);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<26>>() == 26);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<27>>() == 27);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<28>>() == 28);
  VISKORES_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<29>>() == 29);

  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<0>, TypePlaceholder<0>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<1>, TypePlaceholder<1>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<2>, TypePlaceholder<2>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<3>, TypePlaceholder<3>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<4>, TypePlaceholder<4>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<5>, TypePlaceholder<5>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<6>, TypePlaceholder<6>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<7>, TypePlaceholder<7>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<8>, TypePlaceholder<8>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<9>, TypePlaceholder<9>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<10>, TypePlaceholder<10>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<11>, TypePlaceholder<11>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<12>, TypePlaceholder<12>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<13>, TypePlaceholder<13>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<14>, TypePlaceholder<14>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<15>, TypePlaceholder<15>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<16>, TypePlaceholder<16>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<17>, TypePlaceholder<17>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<18>, TypePlaceholder<18>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<19>, TypePlaceholder<19>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<20>, TypePlaceholder<20>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<21>, TypePlaceholder<21>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<22>, TypePlaceholder<22>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<23>, TypePlaceholder<23>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<24>, TypePlaceholder<24>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<25>, TypePlaceholder<25>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<26>, TypePlaceholder<26>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<27>, TypePlaceholder<27>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<28>, TypePlaceholder<28>>::value));
  VISKORES_STATIC_ASSERT((std::is_same<VariantType::TypeAt<29>, TypePlaceholder<29>>::value));

  VISKORES_TEST_ASSERT(VariantType::CanStore<TypePlaceholder<2>>::value);
  VISKORES_TEST_ASSERT(!VariantType::CanStore<TypePlaceholder<100>>::value);
  VISKORES_TEST_ASSERT(variant.GetCanStore<TypePlaceholder<3>>());
  VISKORES_TEST_ASSERT(!variant.GetCanStore<TypePlaceholder<101>>());
}

void TestTriviallyCopyable()
{
#ifdef VISKORES_USE_STD_IS_TRIVIAL
  // Make sure base types are behaving as expected
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivially_constructible<float>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivially_copyable<float>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivial<float>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivially_constructible<int>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivially_copyable<int>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivial<int>::value);
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivially_constructible<NonTrivial>::value);
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivially_copyable<NonTrivial>::value);
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivial<NonTrivial>::value);
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivially_constructible<TrivialCopy>::value);
  VISKORES_STATIC_ASSERT(viskoresstd::is_trivially_copyable<TrivialCopy>::value);
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivial<TrivialCopy>::value);

  // A variant of trivially constructable things should be trivially constructable
  VISKORES_STATIC_ASSERT((viskoresstd::is_trivially_constructible<
                          viskores::exec::detail::VariantUnion<float, int>>::value));
  VISKORES_STATIC_ASSERT(
    (viskoresstd::is_trivially_constructible<viskores::exec::Variant<float, int>>::value));

  // A variant of trivially copyable things should be trivially copyable
  VISKORES_STATIC_ASSERT((viskoresstd::is_trivially_copyable<
                          viskores::exec::detail::VariantUnion<float, int, TrivialCopy>>::value));
  VISKORES_STATIC_ASSERT(
    (viskoresstd::is_trivially_copyable<viskores::exec::Variant<float, int, TrivialCopy>>::value));

  // A variant of any non-trivially constructable things is not trivially copyable
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::detail::VariantUnion<NonTrivial, float, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::detail::VariantUnion<float, NonTrivial, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::detail::VariantUnion<float, int, NonTrivial>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::Variant<NonTrivial, float, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::Variant<float, NonTrivial, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_constructible<
                          viskores::exec::Variant<float, int, NonTrivial>>::value));

  // A variant of any non-trivially copyable things is not trivially copyable
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_copyable<
                          viskores::exec::detail::VariantUnion<NonTrivial, float, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_copyable<
                          viskores::exec::detail::VariantUnion<float, NonTrivial, int>>::value));
  VISKORES_STATIC_ASSERT((!viskoresstd::is_trivially_copyable<
                          viskores::exec::detail::VariantUnion<float, int, NonTrivial>>::value));
  VISKORES_STATIC_ASSERT(
    (!viskoresstd::is_trivially_copyable<viskores::exec::Variant<NonTrivial, float, int>>::value));
  VISKORES_STATIC_ASSERT(
    (!viskoresstd::is_trivially_copyable<viskores::exec::Variant<float, NonTrivial, int>>::value));
  VISKORES_STATIC_ASSERT(
    (!viskoresstd::is_trivially_copyable<viskores::exec::Variant<float, int, NonTrivial>>::value));

  // A variant of trivial things should be trivial
  VISKORES_STATIC_ASSERT((viskoresstd::is_trivial<viskores::exec::Variant<float, int>>::value));
  VISKORES_STATIC_ASSERT(
    (!viskoresstd::is_trivial<viskores::exec::Variant<float, int, TrivialCopy>>::value));
  VISKORES_STATIC_ASSERT(
    (!viskoresstd::is_trivial<viskores::exec::Variant<float, int, NonTrivial>>::value));
#endif // VISKORES_USE_STD_IS_TRIVIAL
}

struct TestFunctor
{
  template <viskores::IdComponent Index>
  viskores::FloatDefault operator()(TypePlaceholder<Index>, viskores::Id expectedValue)
  {
    VISKORES_TEST_ASSERT(Index == expectedValue, "Index = ", Index, ", expected = ", expectedValue);
    return TestValue(expectedValue, viskores::FloatDefault{});
  }
};

struct TestFunctorModify
{
  template <viskores::IdComponent Index>
  void operator()(TypePlaceholder<Index>& object, viskores::Id expectedValue)
  {
    VISKORES_TEST_ASSERT(Index == expectedValue, "Index = ", Index, ", expected = ", expectedValue);
    VISKORES_TEST_ASSERT(object.Value == expectedValue);
    ++object.Value;
  }
};

void TestGet()
{
  std::cout << "Test Get" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>,
                                              TypePlaceholder<1>,
                                              viskores::Id,
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
                                              TypePlaceholder<21>,
                                              TypePlaceholder<22>,
                                              TypePlaceholder<23>,
                                              TypePlaceholder<24>,
                                              TypePlaceholder<25>,
                                              TypePlaceholder<26>,
                                              viskores::Float32,
                                              TypePlaceholder<28>,
                                              TypePlaceholder<29>>;

  {
    const viskores::Id expectedValue = TestValue(3, viskores::Id{});

    VariantType variant = expectedValue;
    VISKORES_TEST_ASSERT(variant.GetIndex() == 2);
    VISKORES_TEST_ASSERT(variant.IsType<viskores::Id>());
    VISKORES_TEST_ASSERT(!variant.IsType<viskores::Float32>());

    VISKORES_TEST_ASSERT(variant.Get<2>() == expectedValue);

    VISKORES_TEST_ASSERT(variant.Get<viskores::Id>() == expectedValue);

    // This line should compile, but will assert if you actually try to run it.
    //variant.Get<TypePlaceholder<100>>();
  }

  {
    const viskores::Float32 expectedValue = TestValue(4, viskores::Float32{});

    VariantType variant = expectedValue;
    VISKORES_TEST_ASSERT(variant.GetIndex() == 27);

    VISKORES_TEST_ASSERT(variant.Get<27>() == expectedValue);

    VISKORES_TEST_ASSERT(variant.Get<viskores::Float32>() == expectedValue);
  }
}

void TestCastAndCall()
{
  std::cout << "Test CastAndCall" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>,
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
                                              TypePlaceholder<21>,
                                              TypePlaceholder<22>,
                                              TypePlaceholder<23>,
                                              TypePlaceholder<24>,
                                              TypePlaceholder<25>,
                                              TypePlaceholder<26>,
                                              TypePlaceholder<27>,
                                              TypePlaceholder<28>,
                                              TypePlaceholder<29>>;
  viskores::FloatDefault result;

  VariantType variant0{ TypePlaceholder<0>{} };
  result = variant0.CastAndCall(TestFunctor(), 0);
  VISKORES_TEST_ASSERT(test_equal(result, TestValue(0, viskores::FloatDefault{})));

  VariantType variant1{ TypePlaceholder<1>{} };
  result = variant1.CastAndCall(TestFunctor(), 1);
  VISKORES_TEST_ASSERT(test_equal(result, TestValue(1, viskores::FloatDefault{})));

  const VariantType variant2{ TypePlaceholder<2>{} };
  result = variant2.CastAndCall(TestFunctor(), 2);
  VISKORES_TEST_ASSERT(test_equal(result, TestValue(2, viskores::FloatDefault{})));

  VariantType variant3{ TypePlaceholder<3>{} };
  result = variant3.CastAndCall(TestFunctor(), 3);
  VISKORES_TEST_ASSERT(test_equal(result, TestValue(3, viskores::FloatDefault{})));

  VariantType variant26{ TypePlaceholder<26>{} };
  result = variant26.CastAndCall(TestFunctor(), 26);
  VISKORES_TEST_ASSERT(test_equal(result, TestValue(26, viskores::FloatDefault{})));

  variant1.CastAndCall(TestFunctorModify{}, 1);
  VISKORES_TEST_ASSERT(variant1.Get<1>().Value == 2, "Variant object not updated.");

  variant1.CastAndCall([](auto& object) { ++object.Value; });
}

void TestCopyInvalid()
{
  std::cout << "Test copy invalid variant" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>, NonTrivial>;

  VariantType source;
  source.Reset();

  VariantType destination1(source);
  VISKORES_TEST_ASSERT(!destination1.IsValid());

  VariantType destination2(TypePlaceholder<0>{});
  destination2 = source;
  VISKORES_TEST_ASSERT(!destination2.IsValid());
}

struct CountConstructDestruct
{
  viskores::Id* Count;
  CountConstructDestruct(viskores::Id* count)
    : Count(count)
  {
    ++(*this->Count);
  }
  CountConstructDestruct(const CountConstructDestruct& src)
    : Count(src.Count)
  {
    ++(*this->Count);
  }
  ~CountConstructDestruct() { --(*this->Count); }
  CountConstructDestruct& operator=(const CountConstructDestruct&) = delete;
};

void TestCopyDestroy()
{
  std::cout << "Test copy destroy" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>,
                                              TypePlaceholder<1>,
                                              CountConstructDestruct,
                                              TypePlaceholder<3>,
                                              TypePlaceholder<4>>;
#ifdef VISKORES_USE_STD_IS_TRIVIAL
  VISKORES_STATIC_ASSERT(!viskoresstd::is_trivially_copyable<VariantType>::value);
#endif // VISKORES_USE_STD_IS_TRIVIAL
  viskores::Id count = 0;

  VariantType variant1 = CountConstructDestruct(&count);
  VISKORES_TEST_ASSERT(count == 1, count);
  VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant2{ variant1 };
    VISKORES_TEST_ASSERT(count == 2, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VISKORES_TEST_ASSERT(*variant2.Get<2>().Count == 2);
  }
  VISKORES_TEST_ASSERT(count == 1, count);
  VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant3{ VariantType(CountConstructDestruct(&count)) };
    VISKORES_TEST_ASSERT(count == 2, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VISKORES_TEST_ASSERT(*variant3.Get<2>().Count == 2);
  }
  VISKORES_TEST_ASSERT(count == 1, count);
  VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant4{ variant1 };
    VISKORES_TEST_ASSERT(count == 2, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VISKORES_TEST_ASSERT(*variant4.Get<2>().Count == 2);

    variant4 = TypePlaceholder<0>{};
    VISKORES_TEST_ASSERT(count == 1, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 1);

    variant4 = VariantType{ TypePlaceholder<1>{} };
    VISKORES_TEST_ASSERT(count == 1, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 1);

    variant4 = variant1;
    VISKORES_TEST_ASSERT(count == 2, count);
    VISKORES_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VISKORES_TEST_ASSERT(*variant4.Get<2>().Count == 2);
  }
}

void TestEmplace()
{
  std::cout << "Test Emplace" << std::endl;

  using VariantType =
    viskores::exec::Variant<viskores::Id, viskores::Id3, std::vector<viskores::Id>>;

  VariantType variant;
  variant.Emplace<viskores::Id>(TestValue(0, viskores::Id{}));
  VISKORES_TEST_ASSERT(variant.GetIndex() == 0);
  VISKORES_TEST_ASSERT(variant.Get<viskores::Id>() == TestValue(0, viskores::Id{}));

  variant.Emplace<1>(TestValue(1, viskores::Id{}));
  VISKORES_TEST_ASSERT(variant.GetIndex() == 1);
  VISKORES_TEST_ASSERT(variant.Get<viskores::Id3>() ==
                       viskores::Id3{ TestValue(1, viskores::Id{}) });

  variant.Emplace<1>(
    TestValue(2, viskores::Id{}), TestValue(3, viskores::Id{}), TestValue(4, viskores::Id{}));
  VISKORES_TEST_ASSERT(variant.GetIndex() == 1);
  VISKORES_TEST_ASSERT(variant.Get<viskores::Id3>() ==
                       viskores::Id3{ TestValue(2, viskores::Id{}),
                                      TestValue(3, viskores::Id{}),
                                      TestValue(4, viskores::Id{}) });

  variant.Emplace<2>(
    { TestValue(5, viskores::Id{}), TestValue(6, viskores::Id{}), TestValue(7, viskores::Id{}) });
  VISKORES_TEST_ASSERT(variant.GetIndex() == 2);
  VISKORES_TEST_ASSERT(variant.Get<std::vector<viskores::Id>>() ==
                       std::vector<viskores::Id>{ TestValue(5, viskores::Id{}),
                                                  TestValue(6, viskores::Id{}),
                                                  TestValue(7, viskores::Id{}) });
}

void TestConstructDestruct()
{
  std::cout << "Make sure constructors and destructors are called correctly" << std::endl;

  g_NonTrivialCount = 0;

  using VariantType = viskores::exec::Variant<NonTrivial, TrivialCopy>;

  {
    VariantType variant1 = NonTrivial{};
    VariantType variant2 = variant1;
    variant2 = NonTrivial{};
    NonTrivial nonTrivial;
    VariantType variant3 = nonTrivial;
    VariantType variant4;
    variant4.Emplace<NonTrivial>();
    VariantType variant5(VariantType(NonTrivial{}));
  }

  VISKORES_TEST_ASSERT(g_NonTrivialCount == 0);
}

void TestCopySelf()
{
  std::cout << "Make sure copying a Variant to itself works" << std::endl;

  using VariantType = viskores::exec::Variant<TypePlaceholder<0>, NonTrivial, TypePlaceholder<2>>;

  VariantType variant{ NonTrivial{} };
  VariantType& variantRef = variant;
  variant = variantRef;
  variant = variant.Get<NonTrivial>();
}

void RunTest()
{
  TestSize();
  TestIndexing();
  TestTriviallyCopyable();
  TestGet();
  TestCastAndCall();
  TestCopyInvalid();
  TestCopyDestroy();
  TestEmplace();
  TestConstructDestruct();
  TestCopySelf();
}

} // namespace test_variant

int UnitTestVariant(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(test_variant::RunTest, argc, argv);
}
