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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/connected_components/CellSetConnectivity.h>
#include <viskores/filter/contour/Contour.h>

#include <viskores/source/Tangle.h>

namespace
{

class TestCellSetConnectivity
{
public:
  static void TestTangleIsosurface()
  {
    viskores::source::Tangle tangle;
    tangle.SetCellDimensions({ 4, 4, 4 });
    viskores::cont::DataSet dataSet = tangle.Execute();

    viskores::filter::contour::Contour filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("tangle");
    viskores::cont::DataSet iso = filter.Execute(dataSet);

    viskores::filter::connected_components::CellSetConnectivity connectivity;
    const viskores::cont::DataSet output = connectivity.Execute(iso);

    viskores::cont::ArrayHandle<viskores::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = viskores::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VISKORES_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                         "Wrong number of connected components");
  }

  static void TestExplicitDataSet()
  {
    viskores::cont::DataSet dataSet =
      viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

    viskores::filter::connected_components::CellSetConnectivity connectivity;
    const viskores::cont::DataSet output = connectivity.Execute(dataSet);

    viskores::cont::ArrayHandle<viskores::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = viskores::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VISKORES_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                         "Wrong number of connected components");
  }

  static void TestUniformDataSet()
  {
    viskores::cont::DataSet dataSet =
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
    viskores::filter::connected_components::CellSetConnectivity connectivity;
    const viskores::cont::DataSet output = connectivity.Execute(dataSet);

    viskores::cont::ArrayHandle<viskores::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = viskores::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VISKORES_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                         "Wrong number of connected components");
  }

  void operator()() const
  {
    TestCellSetConnectivity::TestTangleIsosurface();
    TestCellSetConnectivity::TestExplicitDataSet();
    TestCellSetConnectivity::TestUniformDataSet();
  }
};
}

int UnitTestCellSetConnectivityFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
