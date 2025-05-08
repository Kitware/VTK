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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <vector>
#include <viskores/filter/mesh_info/CellMeasures.h>

namespace
{

struct CheckCellMeasuresFunctor
{
  template <typename ArrayType>
  void operator()(const ArrayType& resultArrayHandle,
                  const std::vector<viskores::Float32>& expected) const
  {
    VISKORES_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() ==
                           static_cast<viskores::Id>(expected.size()),
                         "Wrong number of entries in the output dataset");

    auto portal = resultArrayHandle.ReadPortal();
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(portal.Get(static_cast<viskores::Id>(i)), expected[i]),
                           "Wrong result for CellMeasure filter");
    }
  }
};

void TestCellMeasuresFilter(viskores::cont::DataSet& dataset,
                            const char* msg,
                            const std::vector<viskores::Float32>& expected,
                            const viskores::filter::mesh_info::IntegrationType& type)
{
  std::cout << "Testing CellMeasures Filter on " << msg << "\n";

  viskores::filter::mesh_info::CellMeasures vols;
  vols.SetMeasure(type);
  viskores::cont::DataSet outputData = vols.Execute(dataset);

  VISKORES_TEST_ASSERT(vols.GetCellMeasureName() == "measure");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfCells() == static_cast<viskores::Id>(expected.size()),
                       "Wrong number of cells in the output dataset");

  // Check that the empty measure name above produced a field with the expected name.
  auto result = outputData.GetField(vols.GetCellMeasureName()).GetData();
  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "Output field could not be found or was improper.");

  viskores::cont::CastAndCall(
    result.ResetTypes(viskores::TypeListFieldScalar{}, VISKORES_DEFAULT_STORAGE_LIST{}),
    CheckCellMeasuresFunctor{},
    expected);
}

void TestCellMeasures()
{
  using viskores::filter::mesh_info::IntegrationType;

  viskores::cont::testing::MakeTestDataSet factory;
  viskores::cont::DataSet data;

  data = factory.Make3DExplicitDataSet2();
  TestCellMeasuresFilter(data, "explicit dataset 2", { -1.f }, IntegrationType::AllMeasures);

  data = factory.Make3DExplicitDataSet3();
  TestCellMeasuresFilter(data, "explicit dataset 3", { -1.f / 6.f }, IntegrationType::AllMeasures);

  data = factory.Make3DExplicitDataSet4();
  TestCellMeasuresFilter(data, "explicit dataset 4", { -1.f, -1.f }, IntegrationType::AllMeasures);

  data = factory.Make3DExplicitDataSet5();
  TestCellMeasuresFilter(data,
                         "explicit dataset 5",
                         { 1.f, 1.f / 3.f, 1.f / 6.f, -1.f / 2.f },
                         IntegrationType::AllMeasures);

  data = factory.Make3DExplicitDataSet6();
  TestCellMeasuresFilter(data,
                         "explicit dataset 6 (only volume)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.083426f, 0.25028f },
                         IntegrationType::Volume);
  TestCellMeasuresFilter(
    data,
    "explicit dataset 6 (all)",
    { 0.999924f, 0.999924f, 0.f, 0.f, 3.85516f, 1.00119f, 0.083426f, 0.25028f },
    IntegrationType::AllMeasures);
}

} // anonymous namespace

int UnitTestCellMeasuresFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellMeasures, argc, argv);
}
