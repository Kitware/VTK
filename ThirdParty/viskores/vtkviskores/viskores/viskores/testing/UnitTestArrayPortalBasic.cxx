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

#include <viskores/internal/ArrayPortalBasic.h>
#include <viskores/internal/ArrayPortalHelpers.h>

#include <viskores/StaticAssert.h>

#include <viskores/testing/Testing.h>

#include <array>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

struct TypeTest
{
  template <typename T>
  void operator()(T) const
  {
    std::cout << "Creating data" << std::endl;
    std::array<T, ARRAY_SIZE> array;
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      array[static_cast<std::size_t>(index)] = TestValue(index, T{});
    }

    std::cout << "Construct read portal" << std::endl;
    using ReadPortalType = viskores::internal::ArrayPortalBasicRead<T>;
    VISKORES_STATIC_ASSERT((viskores::internal::PortalSupportsGets<ReadPortalType>::value));
    VISKORES_STATIC_ASSERT((!viskores::internal::PortalSupportsSets<ReadPortalType>::value));
    VISKORES_STATIC_ASSERT((viskores::internal::PortalSupportsIterators<ReadPortalType>::value));

    ReadPortalType readPortal(array.data(), ARRAY_SIZE);
    VISKORES_TEST_ASSERT(readPortal.GetNumberOfValues() == ARRAY_SIZE);
    VISKORES_TEST_ASSERT(readPortal.GetArray() == array.data());
    VISKORES_TEST_ASSERT(readPortal.GetIteratorBegin() == array.data());
    VISKORES_TEST_ASSERT(readPortal.GetIteratorEnd() == array.data() + ARRAY_SIZE);

    std::cout << "Check initial read data" << std::endl;
    CheckPortal(readPortal);

    std::cout << "Construct write portal" << std::endl;
    using WritePortalType = viskores::internal::ArrayPortalBasicWrite<T>;
    VISKORES_STATIC_ASSERT((viskores::internal::PortalSupportsGets<WritePortalType>::value));
    VISKORES_STATIC_ASSERT((viskores::internal::PortalSupportsSets<WritePortalType>::value));
    VISKORES_STATIC_ASSERT((viskores::internal::PortalSupportsIterators<WritePortalType>::value));

    WritePortalType writePortal(array.data(), ARRAY_SIZE);
    VISKORES_TEST_ASSERT(writePortal.GetNumberOfValues() == ARRAY_SIZE);
    VISKORES_TEST_ASSERT(writePortal.GetArray() == array.data());
    VISKORES_TEST_ASSERT(writePortal.GetIteratorBegin() == array.data());
    VISKORES_TEST_ASSERT(writePortal.GetIteratorEnd() == array.data() + ARRAY_SIZE);

    std::cout << "Check initial write data" << std::endl;
    CheckPortal(writePortal);

    std::cout << "Write new data" << std::endl;
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      writePortal.Set(index, TestValue(index + 10, T{}));
    }

    std::cout << "Check data written to array." << std::endl;
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VISKORES_TEST_ASSERT(
        test_equal(array[static_cast<std::size_t>(index)], TestValue(index + 10, T{})));
    }
  }
};

void Run()
{
  viskores::testing::Testing::TryTypes(TypeTest{});
}

} // anonymous namespace

int UnitTestArrayPortalBasic(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Run, argc, argv);
}
