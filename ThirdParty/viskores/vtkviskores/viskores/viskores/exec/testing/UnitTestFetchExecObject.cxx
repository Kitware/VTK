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

#include <viskores/exec/arg/FetchTagExecObject.h>

#include <viskores/exec/testing/ThreadIndicesTesting.h>

#include <viskores/testing/Testing.h>

#define EXPECTED_NUMBER 67

namespace
{

struct TestExecutionObject
{
  TestExecutionObject()
    : Number(static_cast<viskores::Int32>(0xDEADDEAD))
  {
  }
  TestExecutionObject(viskores::Int32 number)
    : Number(number)
  {
  }
  viskores::Int32 Number;
};

void TryInvocation()
{
  TestExecutionObject execObjectStore(EXPECTED_NUMBER);

  using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagExecObject,
                                               viskores::exec::arg::AspectTagDefault,
                                               TestExecutionObject>;

  FetchType fetch;

  viskores::exec::arg::ThreadIndicesTesting indices(0);

  TestExecutionObject execObject = fetch.Load(indices, execObjectStore);
  VISKORES_TEST_ASSERT(execObject.Number == EXPECTED_NUMBER, "Did not load object correctly.");

  execObject.Number = -1;

  // This should be a no-op.
  fetch.Store(indices, execObjectStore, execObject);

  // Data in Invocation should not have changed.
  VISKORES_TEST_ASSERT(execObjectStore.Number == EXPECTED_NUMBER,
                       "Fetch changed read-only execution object.");
}

void TestExecObjectFetch()
{
  TryInvocation();
}

} // anonymous namespace

int UnitTestFetchExecObject(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
