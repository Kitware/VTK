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

#include <viskores/exec/arg/WorkIndex.h>

#include <viskores/exec/arg/FetchTagArrayDirectIn.h>

#include <viskores/exec/testing/ThreadIndicesTesting.h>

#include <viskores/testing/Testing.h>

namespace
{

void TestWorkIndexFetch()
{
  std::cout << "Trying WorkIndex fetch." << std::endl;

  using FetchType = viskores::exec::arg::Fetch<
    viskores::exec::arg::FetchTagArrayDirectIn, // Not used but probably common.
    viskores::exec::arg::AspectTagWorkIndex,
    viskores::internal::NullType>;

  FetchType fetch;

  for (viskores::Id index = 0; index < 10; index++)
  {
    viskores::exec::arg::ThreadIndicesTesting indices(index);

    viskores::Id value = fetch.Load(indices, viskores::internal::NullType());
    VISKORES_TEST_ASSERT(value == index, "Fetch did not give correct work index.");

    value++;

    // This should be a no-op.
    fetch.Store(indices, viskores::internal::NullType(), value);
  }
}

} // anonymous namespace

int UnitTestFetchWorkIndex(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestWorkIndexFetch, argc, argv);
}
