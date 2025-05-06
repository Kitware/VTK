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

#include <viskores/cont/internal/Hints.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DeviceAdapter.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/testing/Testing.h>

namespace UnitTestHintNamespace
{

void CheckFind()
{
  std::cout << "Empty list returns default.\n";
  VISKORES_TEST_ASSERT(
    viskores::cont::internal::HintFind<viskores::cont::internal::HintList<>,
                                       viskores::cont::internal::HintThreadsPerBlock<128>,
                                       viskores::cont::DeviceAdapterTagKokkos>::MaxThreads == 128);

  std::cout << "Find a hint that matches.\n";
  VISKORES_TEST_ASSERT(
    viskores::cont::internal::HintFind<
      viskores::cont::internal::HintList<viskores::cont::internal::HintThreadsPerBlock<128>>,
      viskores::cont::internal::HintThreadsPerBlock<0>,
      viskores::cont::DeviceAdapterTagKokkos>::MaxThreads == 128);
  VISKORES_TEST_ASSERT(
    viskores::cont::internal::HintFind<
      viskores::cont::internal::HintList<viskores::cont::internal::HintThreadsPerBlock<
        128,
        viskores::List<viskores::cont::DeviceAdapterTagKokkos>>>,
      viskores::cont::internal::HintThreadsPerBlock<0>,
      viskores::cont::DeviceAdapterTagKokkos>::MaxThreads == 128);

  std::cout << "Skip a hint that does not match.\n";
  VISKORES_TEST_ASSERT(
    (viskores::cont::internal::HintFind<
       viskores::cont::internal::HintList<viskores::cont::internal::HintThreadsPerBlock<
         128,
         viskores::List<viskores::cont::DeviceAdapterTagKokkos>>>,
       viskores::cont::internal::HintThreadsPerBlock<0>,
       viskores::cont::DeviceAdapterTagSerial>::MaxThreads == 0));

  std::cout << "Given a list of hints, pick the last one that matches\n";
  {
    using HList = viskores::cont::internal::HintList<
      viskores::cont::internal::HintThreadsPerBlock<64>,
      viskores::cont::internal::
        HintThreadsPerBlock<128, viskores::List<viskores::cont::DeviceAdapterTagCuda>>,
      viskores::cont::internal::
        HintThreadsPerBlock<256, viskores::List<viskores::cont::DeviceAdapterTagKokkos>>>;
    using HInit = viskores::cont::internal::HintThreadsPerBlock<0>;
    VISKORES_TEST_ASSERT(
      (viskores::cont::internal::HintFind<HList, HInit, viskores::cont::DeviceAdapterTagSerial>::
         MaxThreads == 64));
    VISKORES_TEST_ASSERT(
      (viskores::cont::internal::HintFind<HList, HInit, viskores::cont::DeviceAdapterTagCuda>::
         MaxThreads == 128));
    VISKORES_TEST_ASSERT(
      (viskores::cont::internal::HintFind<HList, HInit, viskores::cont::DeviceAdapterTagKokkos>::
         MaxThreads == 256));
  }
}

struct MyFunctor : viskores::exec::FunctorBase
{
  VISKORES_EXEC void operator()(viskores::Id viskoresNotUsed(index)) const
  {
    // NOP
  }

  VISKORES_EXEC void operator()(viskores::Id3 viskoresNotUsed(index)) const
  {
    // NOP
  }
};

void CheckSchedule()
{
  std::cout << "Schedule a functor using hints.\n";
  // There is no good way to see if the device adapter got or used the hints
  // as device adapters are free to ignore hints. This just tests that the
  // hints can be passed.
  using Hints =
    viskores::cont::internal::HintList<viskores::cont::internal::HintThreadsPerBlock<128>>;
  viskores::cont::Algorithm::Schedule(Hints{}, MyFunctor{}, 10);
  viskores::cont::Algorithm::Schedule(Hints{}, MyFunctor{}, viskores::Id3{ 2 });
}

void Run()
{
  CheckFind();
  CheckSchedule();
}

} // anonymous UnitTestHintNamespace

int UnitTestHints(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(UnitTestHintNamespace::Run, argc, argv);
}
