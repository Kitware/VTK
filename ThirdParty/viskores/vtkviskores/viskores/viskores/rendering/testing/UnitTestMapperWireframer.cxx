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
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

viskores::cont::DataSet Make3DUniformDataSet(viskores::Id size = 64)
{
  viskores::Float32 center = static_cast<viskores::Float32>(-size) / 2.0f;
  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet dataSet = builder.Create(viskores::Id3(size, size, size),
                                                   viskores::Vec3f_32(center, center, center),
                                                   viskores::Vec3f_32(1.0f, 1.0f, 1.0f));
  const char* fieldName = "pointvar";
  viskores::Id numValues = dataSet.GetNumberOfPoints();
  viskores::cont::ArrayHandleCounting<viskores::Float32> fieldValues(
    0.0f, 10.0f / static_cast<viskores::Float32>(numValues), numValues);
  viskores::cont::ArrayHandle<viskores::Float32> scalarField;
  viskores::cont::ArrayCopy(fieldValues, scalarField);
  dataSet.AddPointField(fieldName, scalarField);
  return dataSet;
}

viskores::cont::DataSet Make2DExplicitDataSet()
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;
  const int nVerts = 5;
  using CoordType = viskores::Vec3f_32;
  std::vector<CoordType> coords(nVerts);
  CoordType coordinates[nVerts] = { CoordType(0.f, 0.f, 0.f),
                                    CoordType(1.f, .5f, 0.f),
                                    CoordType(2.f, 1.f, 0.f),
                                    CoordType(3.f, 1.7f, 0.f),
                                    CoordType(4.f, 3.f, 0.f) };

  std::vector<viskores::Float32> cellVar;
  cellVar.push_back(10);
  cellVar.push_back(12);
  cellVar.push_back(13);
  cellVar.push_back(14);
  std::vector<viskores::Float32> pointVar;
  pointVar.push_back(10);
  pointVar.push_back(12);
  pointVar.push_back(13);
  pointVar.push_back(14);
  pointVar.push_back(15);
  dataSet.AddCoordinateSystem(viskores::cont::make_CoordinateSystem(
    "coordinates", coordinates, nVerts, viskores::CopyFlag::On));
  viskores::cont::CellSetSingleType<> cellSet;

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  connectivity.Allocate(8);
  auto connPortal = connectivity.WritePortal();
  connPortal.Set(0, 0);
  connPortal.Set(1, 1);

  connPortal.Set(2, 1);
  connPortal.Set(3, 2);

  connPortal.Set(4, 2);
  connPortal.Set(5, 3);

  connPortal.Set(6, 3);
  connPortal.Set(7, 4);

  cellSet.Fill(nVerts, viskores::CELL_SHAPE_LINE, 2, connectivity);
  dataSet.SetCellSet(cellSet);
  dataSet.AddPointField("pointVar", pointVar);
  dataSet.AddCellField("cellVar", cellVar);

  return dataSet;
}

void RenderTests()
{

  viskores::cont::testing::MakeTestDataSet maker;

  {
    viskores::rendering::testing::RenderTestOptions testOptions;
    testOptions.Mapper = viskores::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { viskores::rendering::Color::black };
    testOptions.AllowAnyDevice = false;

    viskores::rendering::testing::RenderTest(
      maker.Make3DRegularDataSet0(), "pointvar", "rendering/wireframer/wf_reg3D.png", testOptions);
    viskores::rendering::testing::RenderTest(maker.Make3DRectilinearDataSet0(),
                                             "pointvar",
                                             "rendering/wireframer/wf_rect3D.png",
                                             testOptions);
    testOptions.ViewDimension = 2;
    viskores::rendering::testing::RenderTest(
      Make2DExplicitDataSet(), "cellVar", "rendering/wireframer/wf_lines2D.png", testOptions);
  }

  // These tests are very fickle on multiple machines and on different devices
  // Need to boost the maximum number of allowable error pixels manually
  {
    viskores::rendering::testing::RenderTestOptions testOptions;
    testOptions.Mapper = viskores::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { viskores::rendering::Color::black };
    testOptions.AllowedPixelErrorRatio = 0.05f;
    testOptions.AllowAnyDevice = false;
    viskores::rendering::testing::RenderTest(
      Make3DUniformDataSet(), "pointvar", "rendering/wireframer/wf_uniform3D.png", testOptions);
    viskores::rendering::testing::RenderTest(maker.Make3DExplicitDataSet4(),
                                             "pointvar",
                                             "rendering/wireframer/wf_expl3D.png",
                                             testOptions);
  }

  //
  // Test the 1D cell set line plot with multiple lines
  //
  {
    viskores::rendering::testing::RenderTestOptions testOptions;
    testOptions.ViewDimension = 1;
    testOptions.Mapper = viskores::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { viskores::rendering::Color::red, viskores::rendering::Color::green };
    testOptions.AllowAnyDevice = false;

    viskores::cont::DataSet dataSet0 = maker.Make1DUniformDataSet0();
    viskores::rendering::testing::RenderTest(
      { { dataSet0, "pointvar" }, { dataSet0, "pointvar2" } },
      "rendering/wireframer/wf_lines1D.png",
      testOptions);

    //test log y and title
    testOptions.LogY = true;
    testOptions.Title = "1D Test Plot";
    viskores::cont::DataSet dataSet1 = maker.Make1DUniformDataSet1();
    viskores::rendering::testing::RenderTest(
      dataSet1, "pointvar", "rendering/wireframer/wf_linesLogY1D.png", testOptions);
  }
}

} //namespace

int UnitTestMapperWireframer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
