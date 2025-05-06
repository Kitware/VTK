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

#include <viskores/cont/StorageList.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

enum TypeId
{
  BASIC
};

TypeId GetTypeId(viskores::cont::StorageTagBasic)
{
  return BASIC;
}

struct TestFunctor
{
  std::vector<TypeId> FoundTypes;

  template <typename T>
  VISKORES_CONT void operator()(T)
  {
    this->FoundTypes.push_back(GetTypeId(T()));
  }
};

template <viskores::IdComponent N>
void CheckSame(const viskores::Vec<TypeId, N>& expected, const std::vector<TypeId>& found)
{
  VISKORES_TEST_ASSERT(static_cast<viskores::IdComponent>(found.size()) == N,
                       "Got wrong number of items.");

  for (viskores::IdComponent index = 0; index < N; index++)
  {
    viskores::UInt32 i = static_cast<viskores::UInt32>(index);
    VISKORES_TEST_ASSERT(expected[index] == found[i], "Got wrong type.");
  }
}

template <viskores::IdComponent N, typename List>
void TryList(const viskores::Vec<TypeId, N>& expected, List)
{
  TestFunctor functor;
  viskores::ListForEach(functor, List());
  CheckSame(expected, functor.FoundTypes);
}

void TestLists()
{
  std::cout << "StorageListBasic" << std::endl;
  TryList(viskores::Vec<TypeId, 1>(BASIC), viskores::cont::StorageListBasic());
}

} // anonymous namespace

int UnitTestStorageList(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestLists, argc, argv);
}
