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
#include <viskores/cont/testing/TestingSerialization.h>

using namespace viskores::cont::testing::serialization;

namespace
{

using FieldTypeList = viskores::List<viskores::Float32, viskores::Vec3f>;
using CellSetTypes = viskores::List<viskores::cont::CellSetExplicit<>,
                                    viskores::cont::CellSetSingleType<>,
                                    viskores::cont::CellSetStructured<1>,
                                    viskores::cont::CellSetStructured<2>,
                                    viskores::cont::CellSetStructured<3>>;

using DataSetWrapper = viskores::cont::DataSetWithCellSetTypes<CellSetTypes>;

VISKORES_CONT void TestEqualDataSetWrapper(const DataSetWrapper& ds1, const DataSetWrapper& ds2)
{
  VISKORES_TEST_ASSERT(test_equal_DataSets(ds1.DataSet, ds2.DataSet, CellSetTypes{}));
}

VISKORES_CONT void TestEqualDataSet(const viskores::cont::DataSet& ds1,
                                    const viskores::cont::DataSet& ds2)
{
  VISKORES_TEST_ASSERT(test_equal_DataSets(ds1, ds2, CellSetTypes{}));
}

void RunTest(const viskores::cont::DataSet& ds)
{
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
  TestSerialization(viskores::cont::SerializableDataSet<FieldTypeList, CellSetTypes>(ds),
                    TestEqualDataSetWrapper);
  VISKORES_DEPRECATED_SUPPRESS_END
  TestSerialization(DataSetWrapper(ds), TestEqualDataSetWrapper);
  TestSerialization(ds, TestEqualDataSet);
}

void TestDataSetSerialization()
{
  viskores::cont::testing::MakeTestDataSet makeDS;

  std::cout << "Testing 1D Uniform DataSet #0\n";
  RunTest(makeDS.Make1DUniformDataSet0());
  std::cout << "Testing 1D Uniform DataSet #1\n";
  RunTest(makeDS.Make1DUniformDataSet1());

  std::cout << "Testing 2D Uniform DataSet #0\n";
  RunTest(makeDS.Make2DUniformDataSet0());
  std::cout << "Testing 2D Uniform DataSet #1\n";
  RunTest(makeDS.Make2DUniformDataSet1());

  std::cout << "Testing 3D Uniform DataSet #0\n";
  RunTest(makeDS.Make3DUniformDataSet0());
  std::cout << "Testing 3D Uniform DataSet #1\n";
  RunTest(makeDS.Make3DUniformDataSet1());
  std::cout << "Testing 3D Uniform DataSet #2\n";
  RunTest(makeDS.Make3DUniformDataSet2());

  std::cout << "Testing 3D Regular DataSet #0\n";
  RunTest(makeDS.Make3DRegularDataSet0());
  std::cout << "Testing 3D Regular DataSet #1\n";
  RunTest(makeDS.Make3DRegularDataSet1());

  std::cout << "Testing 2D Rectilinear DataSet #0\n";
  RunTest(makeDS.Make2DRectilinearDataSet0());
  std::cout << "Testing 3D Rectilinear DataSet #0\n";
  RunTest(makeDS.Make3DRectilinearDataSet0());

  std::cout << "Testing 1D Explicit DataSet #0\n";
  RunTest(makeDS.Make1DExplicitDataSet0());

  std::cout << "Testing 2D Explicit DataSet #0\n";
  RunTest(makeDS.Make2DExplicitDataSet0());

  std::cout << "Testing 3D Explicit DataSet #0\n";
  RunTest(makeDS.Make3DExplicitDataSet0());
  std::cout << "Testing 3D Explicit DataSet #1\n";
  RunTest(makeDS.Make3DExplicitDataSet1());
  std::cout << "Testing 3D Explicit DataSet #2\n";
  RunTest(makeDS.Make3DExplicitDataSet2());
  std::cout << "Testing 3D Explicit DataSet #3\n";
  RunTest(makeDS.Make3DExplicitDataSet3());
  std::cout << "Testing 3D Explicit DataSet #4\n";
  RunTest(makeDS.Make3DExplicitDataSet4());
  std::cout << "Testing 3D Explicit DataSet #5\n";
  RunTest(makeDS.Make3DExplicitDataSet5());
  std::cout << "Testing 3D Explicit DataSet #6\n";
  RunTest(makeDS.Make3DExplicitDataSet6());

  std::cout << "Testing 3D Polygonal DataSet #0\n";
  RunTest(makeDS.Make3DExplicitDataSetPolygonal());

  std::cout << "Testing Cow Nose DataSet\n";
  RunTest(makeDS.Make3DExplicitDataSetCowNose());
}

} // anonymous namespace

int UnitTestSerializationDataSet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDataSetSerialization, argc, argv);
}
