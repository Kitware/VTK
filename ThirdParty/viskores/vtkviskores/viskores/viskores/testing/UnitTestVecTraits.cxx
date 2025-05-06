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
#include <viskores/testing/VecTraitsTests.h>

#include <viskores/testing/Testing.h>

namespace
{

static constexpr viskores::Id MAX_VECTOR_SIZE = 5;
static constexpr viskores::Id VecInit[MAX_VECTOR_SIZE] = { 42, 54, 67, 12, 78 };

struct TypeWithoutVecTraits
{
  viskores::Id Value = -1;

  TypeWithoutVecTraits() = default;

  TypeWithoutVecTraits(viskores::Id value)
    : Value(value)
  {
  }

  operator viskores::Id() const { return this->Value; }
};

struct TestVecTypeFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    using Traits = viskores::VecTraits<T>;
    using ComponentType = typename Traits::ComponentType;
    VISKORES_TEST_ASSERT(Traits::NUM_COMPONENTS <= MAX_VECTOR_SIZE,
                         "Need to update test for larger vectors.");
    T inVector;
    for (viskores::IdComponent index = 0; index < Traits::NUM_COMPONENTS; index++)
    {
      Traits::SetComponent(inVector, index, ComponentType(VecInit[index]));
    }
    T outVector;
    viskores::testing::TestVecType<Traits::NUM_COMPONENTS>(inVector, outVector);
    viskores::VecC<ComponentType> outVecC(outVector);
    viskores::testing::TestVecType<Traits::NUM_COMPONENTS>(viskores::VecC<ComponentType>(inVector),
                                                           outVecC);
    viskores::VecCConst<ComponentType> outVecCConst(outVector);
    viskores::testing::TestVecType<Traits::NUM_COMPONENTS>(
      viskores::VecCConst<ComponentType>(inVector), outVecCConst);
  }
};

void TestVecTraits()
{
  TestVecTypeFunctor test;
  viskores::testing::Testing::TryTypes(test);
  std::cout << "viskores::Vec<viskores::FloatDefault, 5>" << std::endl;
  test(viskores::Vec<viskores::FloatDefault, 5>());
  std::cout << "TypeWithoutVecTraits" << std::endl;
  test(TypeWithoutVecTraits{});

  viskores::testing::TestVecComponentsTag<viskores::Id3>();
  viskores::testing::TestVecComponentsTag<viskores::Vec3f>();
  viskores::testing::TestVecComponentsTag<viskores::Vec4f>();
  viskores::testing::TestVecComponentsTag<viskores::VecC<viskores::FloatDefault>>();
  viskores::testing::TestVecComponentsTag<viskores::VecCConst<viskores::Id>>();
  viskores::testing::TestScalarComponentsTag<viskores::Id>();
  viskores::testing::TestScalarComponentsTag<viskores::FloatDefault>();
  viskores::testing::TestScalarComponentsTag<TypeWithoutVecTraits>();
}

} // anonymous namespace

int UnitTestVecTraits(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestVecTraits, argc, argv);
}
