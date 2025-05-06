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
#include <viskores/worklet/MaskIndices.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
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
  using ControlSignature = void(CellSetIn cellset, FieldInOutPoint outPointId);
  using ExecutionSignature = void(InputIndex, _2);
  using InputDomain = _1;

  using MaskType = viskores::worklet::MaskIndices;

  VISKORES_EXEC void operator()(viskores::Id pointId, viskores::Id& outPointId) const
  {
    outPointId = pointId;
  }
};

template <typename CellSetType>
void RunTest(const CellSetType& cellset, const viskores::cont::ArrayHandle<viskores::Id>& indices)
{
  viskores::Id numPoints = cellset.GetNumberOfPoints();
  viskores::cont::ArrayHandle<viskores::Id> outPointId;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<viskores::Id>(-1, numPoints),
                            outPointId);

  viskores::worklet::DispatcherMapTopology<Worklet> dispatcher(
    viskores::worklet::MaskIndices{ indices });
  dispatcher.Invoke(cellset, outPointId);

  viskores::cont::ArrayHandle<viskores::Int8> stencil;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<viskores::Int8>(0, numPoints),
                            stencil);

  // Check that output that should be written was.
  for (viskores::Id i = 0; i < indices.GetNumberOfValues(); ++i)
  {
    // All unmasked indices should have been copied to the output.
    viskores::Id unMaskedIndex = indices.ReadPortal().Get(i);
    viskores::Id writtenValue = outPointId.ReadPortal().Get(unMaskedIndex);
    VISKORES_TEST_ASSERT(unMaskedIndex == writtenValue,
                         "Did not pass unmasked index. Expected ",
                         unMaskedIndex,
                         ". Got ",
                         writtenValue);

    // Mark index as passed.
    stencil.WritePortal().Set(unMaskedIndex, 1);
  }

  // Check that output that should not be written was not.
  for (viskores::Id i = 0; i < numPoints; ++i)
  {
    if (stencil.ReadPortal().Get(i) == 0)
    {
      viskores::Id foundValue = outPointId.ReadPortal().Get(i);
      VISKORES_TEST_ASSERT(foundValue == -1,
                           "Expected index ",
                           i,
                           " to be unwritten but was filled with ",
                           foundValue);
    }
  }
}

void TestMaskIndices()
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
  std::cout << "Testing with random indices " << iterations << " times\n";
  std::cout << "Seed: " << seed << std::endl;
  for (int i = 1; i <= iterations; ++i)
  {
    std::cout << "iteration: " << i << "\n";

    viskores::Id count = countDistribution(generator);
    viskores::cont::ArrayHandle<viskores::Id> indices;
    indices.Allocate(count);

    // Note that it is possible that the same index will be written twice, which is generally
    // a bad idea with MaskIndices. However, the worklet will write the same value for each
    // instance, so we should still get the correct result.
    {
      auto portal = indices.WritePortal();
      std::cout << "using indices:";
      for (viskores::Id j = 0; j < count; ++j)
      {
        auto val = ptidDistribution(generator);
        std::cout << " " << val;
        portal.Set(j, val);
      }
      std::cout << "\n";
    }

    RunTest(cellset, indices);
  }
}

} // anonymous namespace

int UnitTestMaskIndices(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMaskIndices, argc, argv);
}
