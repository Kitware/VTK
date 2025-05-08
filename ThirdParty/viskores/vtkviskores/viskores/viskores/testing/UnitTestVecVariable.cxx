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

#include <viskores/VecVariable.h>

#include <viskores/testing/Testing.h>

namespace
{

struct VecVariableTestFunctor
{
  // You will get a compile fail if this does not pass
  template <typename NumericTag>
  void CheckNumericTag(NumericTag, NumericTag) const
  {
    std::cout << "NumericTag pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckDimensionalityTag(viskores::TypeTraitsVectorTag) const
  {
    std::cout << "VectorTag pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  template <typename T>
  void CheckComponentType(T, T) const
  {
    std::cout << "ComponentType pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckHasMultipleComponents(viskores::VecTraitsTagMultipleComponents) const
  {
    std::cout << "MultipleComponents pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckVariableSize(viskores::VecTraitsTagSizeVariable) const
  {
    std::cout << "VariableSize" << std::endl;
  }

  template <typename T>
  void operator()(T) const
  {
    static constexpr viskores::IdComponent SIZE = 5;
    using VecType = viskores::Vec<T, SIZE>;
    using VecVariableType = viskores::VecVariable<T, SIZE>;
    using TTraits = viskores::TypeTraits<VecVariableType>;
    using VTraits = viskores::VecTraits<VecVariableType>;

    std::cout << "Check NumericTag." << std::endl;
    this->CheckNumericTag(typename TTraits::NumericTag(),
                          typename viskores::TypeTraits<T>::NumericTag());

    std::cout << "Check DimensionalityTag." << std::endl;
    this->CheckDimensionalityTag(typename TTraits::DimensionalityTag());

    std::cout << "Check ComponentType." << std::endl;
    this->CheckComponentType(typename VTraits::ComponentType(), T());

    std::cout << "Check MultipleComponents." << std::endl;
    this->CheckHasMultipleComponents(typename VTraits::HasMultipleComponents());

    std::cout << "Check VariableSize." << std::endl;
    this->CheckVariableSize(typename VTraits::IsSizeStatic());

    VecType source = TestValue(0, VecType());

    VecVariableType vec1(source);
    VecType vecCopy;
    vec1.CopyInto(vecCopy);
    VISKORES_TEST_ASSERT(test_equal(vec1, vecCopy), "Bad init or copyinto.");

    viskores::VecVariable<T, SIZE + 1> vec2;
    for (viskores::IdComponent setIndex = 0; setIndex < SIZE; setIndex++)
    {
      VISKORES_TEST_ASSERT(vec2.GetNumberOfComponents() == setIndex,
                           "Report wrong number of components");
      vec2.Append(source[setIndex]);
    }
    VISKORES_TEST_ASSERT(test_equal(vec2, vec1), "Bad values from Append.");
  }
};

void TestVecVariable()
{
  viskores::testing::Testing::TryTypes(VecVariableTestFunctor(), viskores::TypeListFieldScalar());
}

} // anonymous namespace

int UnitTestVecVariable(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestVecVariable, argc, argv);
}
