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

#include <viskores/Hash.h>

#include <viskores/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

VISKORES_CONT
static void CheckUnique(std::vector<viskores::HashType> hashes)
{
  std::sort(hashes.begin(), hashes.end());

  for (std::size_t index = 1; index < hashes.size(); ++index)
  {
    VISKORES_TEST_ASSERT(hashes[index - 1] != hashes[index], "Found duplicate hashes.");
  }
}

template <typename VecType>
VISKORES_CONT static void DoHashTest(VecType&&)
{
  std::cout << "Test hash for " << viskores::testing::TypeName<VecType>::Name() << std::endl;

  const viskores::Id NUM_HASHES = 100;
  std::cout << "  Make sure the first " << NUM_HASHES << " values are unique." << std::endl;
  // There is a small probability that two values of these 100 could be the same. If this test
  // fails we could just be unlucky (and have to use a different set of 100 hashes), but it is
  // suspicious and you should double check the hashes.
  std::vector<viskores::HashType> hashes;
  hashes.reserve(NUM_HASHES);
  for (viskores::Id index = 0; index < NUM_HASHES; ++index)
  {
    hashes.push_back(viskores::Hash(TestValue(index, VecType())));
  }
  CheckUnique(hashes);

  std::cout << "  Try close values that should have different hashes." << std::endl;
  hashes.clear();
  VecType vec = TestValue(5, VecType());
  hashes.push_back(viskores::Hash(vec));
  vec[0] = vec[0] + 1;
  hashes.push_back(viskores::Hash(vec));
  vec[1] = vec[1] - 1;
  hashes.push_back(viskores::Hash(vec));
  CheckUnique(hashes);
}

VISKORES_CONT
static void TestHash()
{
  DoHashTest(viskores::Id2());
  DoHashTest(viskores::Id3());
  DoHashTest(viskores::Vec<viskores::Id, 10>());
  DoHashTest(viskores::IdComponent2());
  DoHashTest(viskores::IdComponent3());
  DoHashTest(viskores::Vec<viskores::IdComponent, 10>());
}

} // anonymous namespace

int UnitTestHash(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestHash, argc, argv);
}
