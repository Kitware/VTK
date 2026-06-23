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

#include <viskores/LowerBound.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

using IdArray = viskores::cont::ArrayHandle<viskores::Id>;

struct TestLowerBound
{
  struct Impl : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn needles, WholeArrayIn haystack, FieldOut results);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    template <typename HaystackPortal>
    VISKORES_EXEC viskores::Id operator()(viskores::Id needle, const HaystackPortal& haystack) const
    {
      return viskores::LowerBound(haystack, needle);
    }
  };

  static void Check(const IdArray& needles,
                    const IdArray& haystack,
                    const std::vector<viskores::Id>& expected)
  {
    VISKORES_TEST_ASSERT(needles.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()));

    IdArray results;
    viskores::cont::Invoker invoke;
    invoke(Impl{}, needles, haystack, results);

    auto resultsPortal = results.ReadPortal();
    VISKORES_TEST_ASSERT(results.GetNumberOfValues() == needles.GetNumberOfValues());
    for (viskores::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }

  static void Run()
  {
    // Needles query positions before, inside, and after the sorted haystack range.
    IdArray needles =
      viskores::cont::make_ArrayHandle<viskores::Id>({ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 });
    // Haystack includes duplicates to exercise lower-bound insertion before equal values.
    IdArray haystack =
      viskores::cont::make_ArrayHandle<viskores::Id>({ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 });
    std::vector<viskores::Id> expected{ 0, 0, 1, 4, 4, 6, 9, 9, 9, 11 };
    Check(needles, haystack, expected);

    IdArray emptyHaystack;
    emptyHaystack.Allocate(0);
    std::vector<viskores::Id> emptyHaystackExpected(
      static_cast<size_t>(needles.GetNumberOfValues()), 0);
    // All lower-bound results in an empty haystack should point to the beginning.
    Check(needles, emptyHaystack, emptyHaystackExpected);

    IdArray emptyNeedles;
    emptyNeedles.Allocate(0);
    // An empty input domain should produce an empty output array.
    Check(emptyNeedles, haystack, std::vector<viskores::Id>{});
  }
};

void RunLowerBoundTest()
{
  std::cout << "Testing lower bound." << std::endl;
  TestLowerBound::Run();
}

} // anon namespace

int UnitTestLowerBound(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunLowerBoundTest, argc, argv);
}
