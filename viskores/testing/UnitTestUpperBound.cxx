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

#include <viskores/UpperBound.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

using IdArray = viskores::cont::ArrayHandle<viskores::Id>;

struct TestUpperBound
{
  struct Impl : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn needles, WholeArrayIn haystack, FieldOut results);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    template <typename HaystackPortal>
    VISKORES_EXEC viskores::Id operator()(viskores::Id needle, const HaystackPortal& haystack) const
    {
      return viskores::UpperBound(haystack, needle);
    }
  };

  static void Run()
  {
    IdArray needles =
      viskores::cont::make_ArrayHandle<viskores::Id>({ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 });
    IdArray haystack =
      viskores::cont::make_ArrayHandle<viskores::Id>({ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 });
    IdArray results;

    std::vector<viskores::Id> expected{ 0, 1, 4, 4, 6, 9, 9, 9, 11, 11 };

    viskores::cont::Invoker invoke;
    invoke(Impl{}, needles, haystack, results);

    // Verify:
    auto resultsPortal = results.ReadPortal();
    for (viskores::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }
};

void RunUpperBoundTest()
{
  std::cout << "Testing upper bound." << std::endl;
  TestUpperBound::Run();
}

} // anon namespace

int UnitTestUpperBound(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunUpperBoundTest, argc, argv);
}
