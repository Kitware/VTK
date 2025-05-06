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

#include <viskores/cont/ArrayPortalToIterators.h>

#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/ArrayPortalFromIterators.h>

#include <viskores/VecTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr viskores::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

  static ValueType ExpectedValue(viskores::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<viskores::Id>(value)));
  }

  class ReadOnlyArrayPortal
  {
  public:
    using ValueType = T;

    VISKORES_CONT
    ReadOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    VISKORES_CONT
    viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    VISKORES_CONT
    ValueType Get(viskores::Id index) const { return ExpectedValue(index, this->Value); }

  private:
    ComponentType Value;
  };

  class WriteOnlyArrayPortal
  {
  public:
    using ValueType = T;

    VISKORES_CONT
    WriteOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    VISKORES_CONT
    viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    VISKORES_CONT
    void Set(viskores::Id index, const ValueType& value) const
    {
      VISKORES_TEST_ASSERT(value == ExpectedValue(index, this->Value),
                           "Set unexpected value in array portal.");
    }

  private:
    ComponentType Value;
  };

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    std::cout << "    Check distance" << std::endl;
    VISKORES_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                         "Distance between begin and end incorrect.");

    std::cout << "    Write expected value in iterator." << std::endl;
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
    std::cout << "    Check distance" << std::endl;
    VISKORES_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                         "Distance between begin and end incorrect.");

    std::cout << "    Read expected value from iterator." << std::endl;
    viskores::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      VISKORES_TEST_ASSERT(ValueType(*iter) == ExpectedValue(index, value),
                           "Got bad value from iterator.");
      index++;
    }
    return true;
  }

  void TestIteratorRead()
  {
    using ArrayPortalType = ReadOnlyArrayPortal;
    using GetIteratorsType = viskores::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType READ_VALUE = 23;
    ArrayPortalType portal(READ_VALUE);

    std::cout << "  Testing read-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    CheckIterator(iterators.GetBegin(), iterators.GetEnd(), READ_VALUE);

    std::cout << "  Testing read-only iterators with convenience functions." << std::endl;
    CheckIterator(viskores::cont::ArrayPortalToIteratorBegin(portal),
                  viskores::cont::ArrayPortalToIteratorEnd(portal),
                  READ_VALUE);
  }

  void TestIteratorWrite()
  {
    using ArrayPortalType = WriteOnlyArrayPortal;
    using GetIteratorsType = viskores::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType WRITE_VALUE = 63;
    ArrayPortalType portal(WRITE_VALUE);

    std::cout << "  Testing write-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    FillIterator(iterators.GetBegin(), iterators.GetEnd(), WRITE_VALUE);

    std::cout << "  Testing write-only iterators with convenience functions." << std::endl;
    FillIterator(viskores::cont::ArrayPortalToIteratorBegin(portal),
                 viskores::cont::ArrayPortalToIteratorEnd(portal),
                 WRITE_VALUE);
  }

  void TestSimpleIterators()
  {
    std::array<T, ARRAY_SIZE> array;
    T* begin = array.data();
    T* end = begin + ARRAY_SIZE;
    const T* cbegin = begin;
    const T* cend = end;
    viskores::cont::ArrayHandle<T> arrayHandle =
      viskores::cont::make_ArrayHandle(begin, ARRAY_SIZE, viskores::CopyFlag::Off);

    std::cout
      << "  Testing ArrayPortalToIterators(ArrayPortalFromIterators) gets back simple iterator."
      << std::endl;
    {
      auto portal = viskores::cont::internal::ArrayPortalFromIterators<T*>(begin, end);
      auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);
      VISKORES_TEST_ASSERT(viskores::cont::TypeToString(begin) ==
                             viskores::cont::TypeToString(iter),
                           "Expected iterator type ",
                           viskores::cont::TypeToString(begin),
                           " but got ",
                           viskores::cont::TypeToString(iter));
      VISKORES_STATIC_ASSERT((std::is_same<T*, decltype(iter)>::value));
    }
    {
      auto portal = viskores::cont::internal::ArrayPortalFromIterators<const T*>(cbegin, cend);
      auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);
      VISKORES_TEST_ASSERT(viskores::cont::TypeToString(cbegin) ==
                             viskores::cont::TypeToString(iter),
                           "Expected iterator type ",
                           viskores::cont::TypeToString(cbegin),
                           " but got ",
                           viskores::cont::TypeToString(iter));
      VISKORES_STATIC_ASSERT((std::is_same<const T*, decltype(iter)>::value));
    }

    std::cout << "  Testing that basic ArrayHandle has simple iterators." << std::endl;
    {
      auto portal = arrayHandle.WritePortal();
      auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);
      VISKORES_TEST_ASSERT(viskores::cont::TypeToString(begin) ==
                             viskores::cont::TypeToString(iter),
                           "Expected iterator type ",
                           viskores::cont::TypeToString(begin),
                           " but got ",
                           viskores::cont::TypeToString(iter));
      VISKORES_STATIC_ASSERT((std::is_same<T*, decltype(iter)>::value));
    }
    {
      auto portal = arrayHandle.ReadPortal();
      auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);
      VISKORES_TEST_ASSERT(viskores::cont::TypeToString(cbegin) ==
                             viskores::cont::TypeToString(iter),
                           "Expected iterator type ",
                           viskores::cont::TypeToString(cbegin),
                           " but got ",
                           viskores::cont::TypeToString(iter));
      VISKORES_STATIC_ASSERT((std::is_same<const T*, decltype(iter)>::value));
    }
  }

  void operator()()
  {
    TestIteratorRead();
    TestIteratorWrite();
    TestSimpleIterators();
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

// Defines minimal API needed for ArrayPortalToIterators to detect and
// use custom iterators:
struct SpecializedIteratorAPITestPortal
{
  using IteratorType = int;
  IteratorType GetIteratorBegin() const { return 32; }
  IteratorType GetIteratorEnd() const { return 13; }
};

void TestCustomIterator()
{
  std::cout << "  Testing custom iterator detection." << std::endl;

  // Dummy portal type for this test:
  using PortalType = SpecializedIteratorAPITestPortal;
  using ItersType = viskores::cont::ArrayPortalToIterators<PortalType>;

  PortalType portal;
  ItersType iters{ portal };

  VISKORES_TEST_ASSERT(
    std::is_same<typename ItersType::IteratorType, typename PortalType::IteratorType>::value);
  VISKORES_TEST_ASSERT(
    std::is_same<decltype(iters.GetBegin()), typename PortalType::IteratorType>::value);
  VISKORES_TEST_ASSERT(
    std::is_same<decltype(iters.GetEnd()), typename PortalType::IteratorType>::value);
  VISKORES_TEST_ASSERT(iters.GetBegin() == 32);
  VISKORES_TEST_ASSERT(iters.GetEnd() == 13);

  // Convenience API, too:
  VISKORES_TEST_ASSERT(std::is_same<decltype(viskores::cont::ArrayPortalToIteratorBegin(portal)),
                                    typename PortalType::IteratorType>::value);
  VISKORES_TEST_ASSERT(std::is_same<decltype(viskores::cont::ArrayPortalToIteratorEnd(portal)),
                                    typename PortalType::IteratorType>::value);
  VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorBegin(portal) == 32);
  VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorEnd(portal) == 13);
}

void TestArrayPortalToIterators()
{
  viskores::testing::Testing::TryTypes(TestFunctor());
  TestCustomIterator();
}

} // Anonymous namespace

int UnitTestArrayPortalToIterators(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayPortalToIterators, argc, argv);
}
