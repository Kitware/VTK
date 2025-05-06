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

#include <viskores/VecFromPortalPermute.h>

#include <viskores/VecVariable.h>

#include <viskores/testing/Testing.h>

namespace UnitTestVecFromPortalPermuteNamespace
{

static constexpr viskores::IdComponent ARRAY_SIZE = 10;

template <typename T>
void CheckType(T, T)
{
  // Check passes if this function is called correctly.
}

template <typename T>
class TestPortal
{
public:
  using ValueType = T;

  VISKORES_EXEC
  viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VISKORES_EXEC
  ValueType Get(viskores::Id index) const { return TestValue(index, ValueType()); }
};

struct VecFromPortalPermuteTestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    using PortalType = TestPortal<T>;
    using IndexVecType = viskores::VecVariable<viskores::Id, ARRAY_SIZE>;
    using VecType = viskores::VecFromPortalPermute<IndexVecType, PortalType>;
    using TTraits = viskores::TypeTraits<VecType>;
    using VTraits = viskores::VecTraits<VecType>;

    std::cout << "Checking VecFromPortal traits" << std::endl;

    // The statements will fail to compile if the traits is not working as
    // expected.
    CheckType(typename TTraits::DimensionalityTag(), viskores::TypeTraitsVectorTag());
    CheckType(typename VTraits::ComponentType(), T());
    CheckType(typename VTraits::HasMultipleComponents(),
              viskores::VecTraitsTagMultipleComponents());
    CheckType(typename VTraits::IsSizeStatic(), viskores::VecTraitsTagSizeVariable());

    std::cout << "Checking VecFromPortal contents" << std::endl;

    PortalType portal;

    for (viskores::Id offset = 0; offset < ARRAY_SIZE; offset++)
    {
      for (viskores::IdComponent length = 0; 2 * length + offset < ARRAY_SIZE; length++)
      {
        IndexVecType indices;
        for (viskores::IdComponent index = 0; index < length; index++)
        {
          indices.Append(offset + 2 * index);
        }

        VecType vec(&indices, portal);

        VISKORES_TEST_ASSERT(vec.GetNumberOfComponents() == length, "Wrong length.");
        VISKORES_TEST_ASSERT(VTraits::GetNumberOfComponents(vec) == length, "Wrong length.");

        viskores::Vec<T, ARRAY_SIZE> copyDirect;
        vec.CopyInto(copyDirect);

        viskores::Vec<T, ARRAY_SIZE> copyTraits;
        VTraits::CopyInto(vec, copyTraits);

        for (viskores::IdComponent index = 0; index < length; index++)
        {
          T expected = TestValue(2 * index + offset, T());
          VISKORES_TEST_ASSERT(test_equal(vec[index], expected), "Wrong value.");
          VISKORES_TEST_ASSERT(test_equal(VTraits::GetComponent(vec, index), expected),
                               "Wrong value.");
          VISKORES_TEST_ASSERT(test_equal(copyDirect[index], expected), "Wrong copied value.");
          VISKORES_TEST_ASSERT(test_equal(copyTraits[index], expected), "Wrong copied value.");
        }
      }
    }
  }
};

void VecFromPortalPermuteTest()
{
  viskores::testing::Testing::TryTypes(VecFromPortalPermuteTestFunctor(),
                                       viskores::TypeListCommon());
}

} // namespace UnitTestVecFromPortalPermuteNamespace

int UnitTestVecFromPortalPermute(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(
    UnitTestVecFromPortalPermuteNamespace::VecFromPortalPermuteTest, argc, argv);
}
