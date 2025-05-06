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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/resampling/Probe.h>

namespace
{

viskores::cont::DataSet MakeInputDataSet()
{
  std::vector<viskores::Float32> pvec(16), cvec(9);
  for (std::size_t i = 0; i < 16; ++i)
  {
    pvec[i] = static_cast<viskores::Float32>(i) * 0.3f;
  }
  for (std::size_t i = 0; i < 9; ++i)
  {
    cvec[i] = static_cast<viskores::Float32>(i) * 0.7f;
  }

  auto input = viskores::cont::DataSetBuilderUniform::Create(
    viskores::Id2(4, 4), viskores::make_Vec(0.0f, 0.0f), viskores::make_Vec(1.0f, 1.0f));
  input.AddPointField("pointdata", pvec);
  input.AddCellField("celldata", cvec);
  return input;
}

viskores::cont::DataSet MakeGeometryDataSet()
{
  auto geometry = viskores::cont::DataSetBuilderUniform::Create(
    viskores::Id2(9, 9), viskores::make_Vec(0.7f, 0.7f), viskores::make_Vec(0.35f, 0.35f));
  return geometry;
}

viskores::cont::DataSet ConvertDataSetUniformToExplicit(const viskores::cont::DataSet& uds)
{
  viskores::filter::clean_grid::CleanGrid toUnstructured;
  toUnstructured.SetMergePoints(true);
  return toUnstructured.Execute(uds);
}

const std::vector<viskores::Float32>& GetExpectedPointData()
{
  static std::vector<viskores::Float32> expected = { 1.05f,
                                                     1.155f,
                                                     1.26f,
                                                     1.365f,
                                                     1.47f,
                                                     1.575f,
                                                     1.68f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     1.47f,
                                                     1.575f,
                                                     1.68f,
                                                     1.785f,
                                                     1.89f,
                                                     1.995f,
                                                     2.1f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     1.89f,
                                                     1.995f,
                                                     2.1f,
                                                     2.205f,
                                                     2.31f,
                                                     2.415f,
                                                     2.52f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     2.31f,
                                                     2.415f,
                                                     2.52f,
                                                     2.625f,
                                                     2.73f,
                                                     2.835f,
                                                     2.94f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     2.73f,
                                                     2.835f,
                                                     2.94f,
                                                     3.045f,
                                                     3.15f,
                                                     3.255f,
                                                     3.36f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     3.15f,
                                                     3.255f,
                                                     3.36f,
                                                     3.465f,
                                                     3.57f,
                                                     3.675f,
                                                     3.78f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     3.57f,
                                                     3.675f,
                                                     3.78f,
                                                     3.885f,
                                                     3.99f,
                                                     4.095f,
                                                     4.2f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32() };
  return expected;
}

const std::vector<viskores::Float32>& GetExpectedCellData()
{
  static std::vector<viskores::Float32> expected = { 0.0f,
                                                     0.7f,
                                                     0.7f,
                                                     0.7f,
                                                     1.4f,
                                                     1.4f,
                                                     1.4f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     2.1f,
                                                     2.8f,
                                                     2.8f,
                                                     2.8f,
                                                     3.5f,
                                                     3.5f,
                                                     3.5f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     2.1f,
                                                     2.8f,
                                                     2.8f,
                                                     2.8f,
                                                     3.5f,
                                                     3.5f,
                                                     3.5f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     2.1f,
                                                     2.8f,
                                                     2.8f,
                                                     2.8f,
                                                     3.5f,
                                                     3.5f,
                                                     3.5f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     4.2f,
                                                     4.9f,
                                                     4.9f,
                                                     4.9f,
                                                     5.6f,
                                                     5.6f,
                                                     5.6f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     4.2f,
                                                     4.9f,
                                                     4.9f,
                                                     4.9f,
                                                     5.6f,
                                                     5.6f,
                                                     5.6f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     4.2f,
                                                     4.9f,
                                                     4.9f,
                                                     4.9f,
                                                     5.6f,
                                                     5.6f,
                                                     5.6f,
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32(),
                                                     viskores::Nan32() };
  return expected;
}

const std::vector<viskores::UInt8>& GetExpectedHiddenPoints()
{
  static std::vector<viskores::UInt8> expected = {
    0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
  };
  return expected;
}

const std::vector<viskores::UInt8>& GetExpectedHiddenCells()
{
  static std::vector<viskores::UInt8> expected = { 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                                   0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                                   0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                                   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  return expected;
}

template <typename T>
void TestResultArray(const viskores::cont::ArrayHandle<T>& result, const std::vector<T>& expected)
{
  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "Incorrect field size");

  auto portal = result.ReadPortal();
  viskores::Id size = portal.GetNumberOfValues();
  for (viskores::Id i = 0; i < size; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]),
                         "Incorrect field value");
  }
}

class TestProbe
{
private:
  using FieldArrayType = viskores::cont::ArrayHandle<viskores::Float32>;
  using HiddenArrayType = viskores::cont::ArrayHandle<viskores::UInt8>;

  static void ExplicitToUnifrom()
  {
    std::cout << "Testing Probe Explicit to Uniform:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = MakeGeometryDataSet();

    viskores::filter::resampling::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("pointdata").GetData()),
                    GetExpectedPointData());
    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("celldata").GetData()),
                    GetExpectedCellData());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetPointField("HIDDEN").GetData()),
                    GetExpectedHiddenPoints());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetCellField("HIDDEN").GetData()),
                    GetExpectedHiddenCells());
  }

  static void UniformToExplict()
  {
    std::cout << "Testing Probe Uniform to Explicit:\n";

    auto input = MakeInputDataSet();
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    viskores::filter::resampling::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("pointdata").GetData()),
                    GetExpectedPointData());
    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("celldata").GetData()),
                    GetExpectedCellData());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetPointField("HIDDEN").GetData()),
                    GetExpectedHiddenPoints());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetCellField("HIDDEN").GetData()),
                    GetExpectedHiddenCells());
  }

  static void ExplicitToExplict()
  {
    std::cout << "Testing Probe Explicit to Explicit:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    viskores::filter::resampling::Probe probe;
    probe.SetGeometry(geometry);
    probe.SetFieldsToPass({ "pointdata", "celldata" });
    auto output = probe.Execute(input);

    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("pointdata").GetData()),
                    GetExpectedPointData());
    TestResultArray(viskores::cont::Cast<FieldArrayType>(output.GetField("celldata").GetData()),
                    GetExpectedCellData());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetPointField("HIDDEN").GetData()),
                    GetExpectedHiddenPoints());
    TestResultArray(viskores::cont::Cast<HiddenArrayType>(output.GetCellField("HIDDEN").GetData()),
                    GetExpectedHiddenCells());
  }

public:
  static void Run()
  {
    ExplicitToUnifrom();
    UniformToExplict();
    ExplicitToExplict();
  }
};

} // anonymous namespace

int UnitTestProbe(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestProbe::Run, argc, argv);
}
