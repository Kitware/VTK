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

#include <viskores/VecFlat.h>

#include <viskores/VecAxisAlignedPointCoordinates.h>

#include <viskores/cont/Logging.h>

#include <viskores/testing/Testing.h>

namespace
{

template <typename T>
void CheckTraits(const T&, viskores::IdComponent numComponents)
{
  VISKORES_TEST_ASSERT((std::is_same<typename viskores::TypeTraits<T>::DimensionalityTag,
                                     viskores::TypeTraitsVectorTag>::value));
  VISKORES_TEST_ASSERT(viskores::VecTraits<T>::NUM_COMPONENTS == numComponents);
}

void TryBasicVec()
{
  using NestedVecType = viskores::Vec<viskores::Vec<viskores::Id, 2>, 3>;
  std::cout << "Trying " << viskores::cont::TypeToString<NestedVecType>() << std::endl;

  NestedVecType nestedVec = { { 0, 1 }, { 2, 3 }, { 4, 5 } };
  std::cout << "  original: " << nestedVec << std::endl;

  auto flatVec = viskores::make_VecFlat(nestedVec);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 6);
  VISKORES_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 6);
  VISKORES_TEST_ASSERT(flatVec[0] == 0);
  VISKORES_TEST_ASSERT(flatVec[1] == 1);
  VISKORES_TEST_ASSERT(flatVec[2] == 2);
  VISKORES_TEST_ASSERT(flatVec[3] == 3);
  VISKORES_TEST_ASSERT(flatVec[4] == 4);
  VISKORES_TEST_ASSERT(flatVec[5] == 5);

  flatVec = viskores::VecFlat<NestedVecType>{ 5, 4, 3, 2, 1, 0 };
  std::cout << "  flat backward: " << flatVec << std::endl;
  VISKORES_TEST_ASSERT(flatVec[0] == 5);
  VISKORES_TEST_ASSERT(flatVec[1] == 4);
  VISKORES_TEST_ASSERT(flatVec[2] == 3);
  VISKORES_TEST_ASSERT(flatVec[3] == 2);
  VISKORES_TEST_ASSERT(flatVec[4] == 1);
  VISKORES_TEST_ASSERT(flatVec[5] == 0);

  nestedVec = flatVec;
  std::cout << "  nested backward: " << nestedVec << std::endl;
  VISKORES_TEST_ASSERT(nestedVec[0][0] == 5);
  VISKORES_TEST_ASSERT(nestedVec[0][1] == 4);
  VISKORES_TEST_ASSERT(nestedVec[1][0] == 3);
  VISKORES_TEST_ASSERT(nestedVec[1][1] == 2);
  VISKORES_TEST_ASSERT(nestedVec[2][0] == 1);
  VISKORES_TEST_ASSERT(nestedVec[2][1] == 0);
}

void TryScalar()
{
  using ScalarType = viskores::Id;
  std::cout << "Trying " << viskores::cont::TypeToString<ScalarType>() << std::endl;

  ScalarType scalar = TestValue(0, ScalarType{});
  std::cout << "  original: " << scalar << std::endl;

  auto flatVec = viskores::make_VecFlat(scalar);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 1);
  VISKORES_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 1);
  VISKORES_TEST_ASSERT(test_equal(flatVec[0], TestValue(0, ScalarType{})));
}

void TrySpecialVec()
{
  using NestedVecType = viskores::Vec<viskores::VecAxisAlignedPointCoordinates<1>, 2>;
  std::cout << "Trying " << viskores::cont::TypeToString<NestedVecType>() << std::endl;

  NestedVecType nestedVec = { { { 0, 0, 0 }, { 1, 1, 1 } }, { { 1, 1, 1 }, { 1, 1, 1 } } };
  std::cout << "  original: " << nestedVec << std::endl;

  auto flatVec = viskores::make_VecFlat(nestedVec);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 12);
  VISKORES_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 12);
  VISKORES_TEST_ASSERT(test_equal(flatVec[0], nestedVec[0][0][0]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[1], nestedVec[0][0][1]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[2], nestedVec[0][0][2]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[3], nestedVec[0][1][0]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[4], nestedVec[0][1][1]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[5], nestedVec[0][1][2]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[6], nestedVec[1][0][0]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[7], nestedVec[1][0][1]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[8], nestedVec[1][0][2]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[9], nestedVec[1][1][0]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[10], nestedVec[1][1][1]));
  VISKORES_TEST_ASSERT(test_equal(flatVec[11], nestedVec[1][1][2]));
}

void DoTest()
{
  TryBasicVec();
  TryScalar();
  TrySpecialVec();
}

} // anonymous namespace

int UnitTestVecFlat(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(DoTest, argc, argv);
}
