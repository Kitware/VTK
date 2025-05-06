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

#include <viskores/TypeTraits.h>

#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

namespace
{

struct TypeTraitTest
{
  template <typename T>
  void operator()(T t) const
  {
    // If you get compiler errors here, it could be a TypeTraits instance
    // has missing or malformed tags.
    this->TestDimensionality(t, typename viskores::TypeTraits<T>::DimensionalityTag());
    this->TestNumeric(t, typename viskores::TypeTraits<T>::NumericTag());
  }

private:
  template <typename T>
  void TestDimensionality(T, viskores::TypeTraitsScalarTag) const
  {
    std::cout << "  scalar" << std::endl;
    VISKORES_TEST_ASSERT(viskores::VecTraits<T>::NUM_COMPONENTS == 1,
                         "Scalar type does not have one component.");
  }
  template <typename T>
  void TestDimensionality(T, viskores::TypeTraitsVectorTag) const
  {
    std::cout << "  vector" << std::endl;
    VISKORES_TEST_ASSERT(viskores::VecTraits<T>::NUM_COMPONENTS > 1,
                         "Vector type does not have multiple components.");
  }

  template <typename T>
  void TestNumeric(T, viskores::TypeTraitsIntegerTag) const
  {
    std::cout << "  integer" << std::endl;
    using VT = typename viskores::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    VISKORES_TEST_ASSERT(value == 2, "Integer does not round to integer.");
  }
  template <typename T>
  void TestNumeric(T, viskores::TypeTraitsRealTag) const
  {
    std::cout << "  real" << std::endl;
    using VT = typename viskores::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    VISKORES_TEST_ASSERT(test_equal(float(value), float(2.001)),
                         "Real does not hold floaing point number.");
  }
};

static void TestTypeTraits()
{
  TypeTraitTest test;
  viskores::testing::Testing::TryTypes(test);
  std::cout << "viskores::Vec<viskores::FloatDefault, 5>" << std::endl;
  test(viskores::Vec<viskores::FloatDefault, 5>());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestTypeTraits(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestTypeTraits, argc, argv);
}
