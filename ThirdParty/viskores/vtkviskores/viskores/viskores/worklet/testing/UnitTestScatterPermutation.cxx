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
#include <viskores/worklet/ScatterPermutation.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

class Worklet : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldOutPoint outPointId,
                                FieldOutPoint outVisit);
  using ExecutionSignature = void(InputIndex, VisitIndex, _2, _3);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterPermutation<>;

  VISKORES_CONT
  static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id>& permutation)
  {
    return ScatterType(permutation);
  }

  VISKORES_EXEC void operator()(viskores::Id pointId,
                                viskores::IdComponent visit,
                                viskores::Id& outPointId,
                                viskores::IdComponent& outVisit) const
  {
    outPointId = pointId;
    outVisit = visit;
  }
};

template <typename CellSetType>
void RunTest(const CellSetType& cellset,
             const viskores::cont::ArrayHandle<viskores::Id>& permutation)
{
  viskores::cont::ArrayHandle<viskores::Id> outPointId;
  viskores::cont::ArrayHandle<viskores::IdComponent> outVisit;

  viskores::worklet::DispatcherMapTopology<Worklet> dispatcher(Worklet::MakeScatter(permutation));
  dispatcher.Invoke(cellset, outPointId, outVisit);

  for (viskores::Id i = 0; i < permutation.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(outPointId.ReadPortal().Get(i) == permutation.ReadPortal().Get(i),
                         "output point ids do not match the permutation");
    VISKORES_TEST_ASSERT(outVisit.ReadPortal().Get(i) == 0, "incorrect visit index");
  }
}

void TestScatterPermutation()
{
  viskores::cont::DataSet dataset =
    viskores::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  auto cellset = dataset.GetCellSet();
  viskores::Id numberOfPoints = cellset.GetNumberOfPoints();

  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));

  std::default_random_engine generator;
  generator.seed(seed);
  std::uniform_int_distribution<viskores::Id> countDistribution(1, 2 * numberOfPoints);
  std::uniform_int_distribution<viskores::Id> ptidDistribution(0, numberOfPoints - 1);

  const int iterations = 5;
  std::cout << "Testing with random permutations " << iterations << " times\n";
  std::cout << "Seed: " << seed << std::endl;
  for (int i = 1; i <= iterations; ++i)
  {
    std::cout << "iteration: " << i << "\n";

    viskores::Id count = countDistribution(generator);
    viskores::cont::ArrayHandle<viskores::Id> permutation;
    permutation.Allocate(count);

    auto portal = permutation.WritePortal();
    std::cout << "using permutation:";
    for (viskores::Id j = 0; j < count; ++j)
    {
      auto val = ptidDistribution(generator);
      std::cout << " " << val;
      portal.Set(j, val);
    }
    std::cout << "\n";

    RunTest(cellset, permutation);
  }
}

} // anonymous namespace

int UnitTestScatterPermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestScatterPermutation, argc, argv);
}
