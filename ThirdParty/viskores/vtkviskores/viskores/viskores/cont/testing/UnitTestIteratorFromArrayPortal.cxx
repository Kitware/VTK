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

#include <viskores/cont/internal/IteratorFromArrayPortal.h>

#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/internal/ArrayPortalFromIterators.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr viskores::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

  ValueType ExpectedValue(viskores::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<viskores::Id>(value)));
  }

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    viskores::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      *iter = ExpectedValue(index, value);
      index++;
    }
  }

  template <class IteratorType>
  bool CheckIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    viskores::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      if (ValueType(*iter) != ExpectedValue(index, value))
      {
        return false;
      }
      index++;
    }
    return true;
  }

  template <class PortalType>
  bool CheckPortal(const PortalType& portal, const ComponentType& value)
  {
    viskores::cont::ArrayPortalToIterators<PortalType> iterators(portal);
    return CheckIterator(iterators.GetBegin(), iterators.GetEnd(), value);
  }

  ComponentType ORIGINAL_VALUE() { return 39; }

  template <class ArrayPortalType>
  void TestIteratorRead(ArrayPortalType portal)
  {
    using IteratorType = viskores::cont::internal::IteratorFromArrayPortal<ArrayPortalType>;

    IteratorType begin = viskores::cont::internal::make_IteratorBegin(portal);
    IteratorType end = viskores::cont::internal::make_IteratorEnd(portal);
    VISKORES_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                         "Distance between begin and end incorrect.");
    VISKORES_TEST_ASSERT(std::distance(end, begin) == -ARRAY_SIZE,
                         "Distance between begin and end incorrect.");

    std::cout << "    Check forward iteration." << std::endl;
    VISKORES_TEST_ASSERT(CheckIterator(begin, end, ORIGINAL_VALUE()), "Forward iteration wrong");

    std::cout << "    Check backward iteration." << std::endl;
    IteratorType middle = end;
    for (viskores::Id index = portal.GetNumberOfValues() - 1; index >= 0; index--)
    {
      middle--;
      ValueType value = *middle;
      VISKORES_TEST_ASSERT(value == ExpectedValue(index, ORIGINAL_VALUE()),
                           "Backward iteration wrong");
    }

    std::cout << "    Check advance" << std::endl;
    middle = begin + ARRAY_SIZE / 2;
    VISKORES_TEST_ASSERT(std::distance(begin, middle) == ARRAY_SIZE / 2, "Bad distance to middle.");
    VISKORES_TEST_ASSERT(ValueType(*middle) == ExpectedValue(ARRAY_SIZE / 2, ORIGINAL_VALUE()),
                         "Bad value at middle.");
  }

  template <class ArrayPortalType>
  void TestIteratorWrite(ArrayPortalType portal)
  {
    using IteratorType = viskores::cont::internal::IteratorFromArrayPortal<ArrayPortalType>;

    IteratorType begin = viskores::cont::internal::make_IteratorBegin(portal);
    IteratorType end = viskores::cont::internal::make_IteratorEnd(portal);

    static const ComponentType WRITE_VALUE = 73;

    std::cout << "    Write values to iterator." << std::endl;
    FillIterator(begin, end, WRITE_VALUE);

    std::cout << "    Check values in portal." << std::endl;
    VISKORES_TEST_ASSERT(CheckPortal(portal, WRITE_VALUE),
                         "Did not get correct values when writing to iterator.");
  }

  void TestOperators()
  {
    struct Functor
    {
      VISKORES_EXEC ValueType operator()(viskores::Id index) const
      {
        return TestValue(index, ValueType{});
      }
    };
    Functor functor;

    auto array = viskores::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);
    auto portal = array.ReadPortal();

    VISKORES_TEST_ASSERT(test_equal(portal.Get(0), functor(0)));
    ::CheckPortal(portal);

    // Normally, you would use `ArrayPortalToIterators`, but we want to test this
    // class specifically.
    using IteratorType = viskores::cont::internal::IteratorFromArrayPortal<decltype(portal)>;
    IteratorType begin{ portal };
    IteratorType end{ portal, ARRAY_SIZE };

    VISKORES_TEST_ASSERT(test_equal(*begin, functor(0)));
    VISKORES_TEST_ASSERT(test_equal(begin[0], functor(0)));
    VISKORES_TEST_ASSERT(test_equal(begin[3], functor(3)));

    IteratorType iter = begin;
    VISKORES_TEST_ASSERT(test_equal(*iter, functor(0)));
    VISKORES_TEST_ASSERT(test_equal(*(iter++), functor(0)));
    VISKORES_TEST_ASSERT(test_equal(*iter, functor(1)));
    VISKORES_TEST_ASSERT(test_equal(*(++iter), functor(2)));
    VISKORES_TEST_ASSERT(test_equal(*iter, functor(2)));

    VISKORES_TEST_ASSERT(test_equal(*(iter--), functor(2)));
    VISKORES_TEST_ASSERT(test_equal(*iter, functor(1)));
    VISKORES_TEST_ASSERT(test_equal(*(--iter), functor(0)));
    VISKORES_TEST_ASSERT(test_equal(*iter, functor(0)));

    VISKORES_TEST_ASSERT(test_equal(*(iter += 3), functor(3)));
    VISKORES_TEST_ASSERT(test_equal(*(iter -= 3), functor(0)));

    VISKORES_TEST_ASSERT(end - begin == ARRAY_SIZE);

    VISKORES_TEST_ASSERT(test_equal(*(iter + 3), functor(3)));
    VISKORES_TEST_ASSERT(test_equal(*(3 + iter), functor(3)));
    iter += 3;
    VISKORES_TEST_ASSERT(test_equal(*(iter - 3), functor(0)));

    VISKORES_TEST_ASSERT(iter == (begin + 3));
    VISKORES_TEST_ASSERT(!(iter != (begin + 3)));
    VISKORES_TEST_ASSERT(iter != begin);
    VISKORES_TEST_ASSERT(!(iter == begin));

    VISKORES_TEST_ASSERT(!(iter < begin));
    VISKORES_TEST_ASSERT(!(iter < (begin + 3)));
    VISKORES_TEST_ASSERT((iter < end));

    VISKORES_TEST_ASSERT(!(iter <= begin));
    VISKORES_TEST_ASSERT((iter <= (begin + 3)));
    VISKORES_TEST_ASSERT((iter <= end));

    VISKORES_TEST_ASSERT((iter > begin));
    VISKORES_TEST_ASSERT(!(iter > (begin + 3)));
    VISKORES_TEST_ASSERT(!(iter > end));

    VISKORES_TEST_ASSERT((iter >= begin));
    VISKORES_TEST_ASSERT((iter >= (begin + 3)));
    VISKORES_TEST_ASSERT(!(iter >= end));
  }

  void operator()()
  {
    ValueType array[ARRAY_SIZE];

    FillIterator(array, array + ARRAY_SIZE, ORIGINAL_VALUE());

    ::viskores::cont::internal::ArrayPortalFromIterators<ValueType*> portal(array,
                                                                            array + ARRAY_SIZE);
    ::viskores::cont::internal::ArrayPortalFromIterators<const ValueType*> const_portal(
      array, array + ARRAY_SIZE);

    std::cout << "  Test read from iterator." << std::endl;
    TestIteratorRead(portal);

    std::cout << "  Test read from const iterator." << std::endl;
    TestIteratorRead(const_portal);

    std::cout << "  Test write to iterator." << std::endl;
    TestIteratorWrite(portal);

    std::cout << "  Test operators." << std::endl;
    TestOperators();
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayIteratorFromArrayPortal()
{
  viskores::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestIteratorFromArrayPortal(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayIteratorFromArrayPortal, argc, argv);
}
