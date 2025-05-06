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

#include <viskores/filter/field_transform/FieldToColors.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
namespace
{
void TestFieldToColors()
{
  //faux input field
  constexpr viskores::Id nvals = 8;
  constexpr viskores::FloatDefault data[nvals] = { -1, 0, 10, 20, 30, 40, 50, 60 };

  //build a color table with clamping off and verify that sampling works
  viskores::Range range{ 0.0, 50.0 };
  viskores::cont::ColorTable table(viskores::cont::ColorTable::Preset::CoolToWarm);
  table.RescaleToRange(range);
  table.SetClampingOff();
  table.SetAboveRangeColor(viskores::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
  table.SetBelowRangeColor(viskores::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

  viskores::cont::DataSet ds =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();
  ds.AddPointField("faux", data, nvals);

  viskores::filter::field_transform::FieldToColors ftc(table);
  ftc.SetOutputToRGBA();
  ftc.SetActiveField("faux");
  ftc.SetOutputFieldName("colors");

  auto rgbaResult = ftc.Execute(ds);
  VISKORES_TEST_ASSERT(rgbaResult.HasPointField("colors"), "Field missing.");
  viskores::cont::Field Result = rgbaResult.GetPointField("colors");
  viskores::cont::ArrayHandle<viskores::Vec4ui_8> resultRGBAHandle;
  Result.GetData().AsArrayHandle(resultRGBAHandle);

  //values confirmed with ParaView 5.11
  const viskores::Vec4ui_8 correct_diverging_rgba_values[nvals] = {
    { 0, 0, 255, 255 },     { 59, 76, 192, 255 },   { 124, 159, 249, 255 }, { 192, 212, 245, 255 },
    { 242, 203, 183, 255 }, { 238, 133, 104, 255 }, { 180, 4, 38, 255 },    { 255, 0, 0, 255 }
  };
  auto portalRGBA = resultRGBAHandle.ReadPortal();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGBA.Get(static_cast<viskores::Id>(i));
    VISKORES_TEST_ASSERT(result == correct_diverging_rgba_values[i],
                         "incorrect value when interpolating between values");
  }

  //Now verify that we can switching our output mode
  ftc.SetOutputToRGB();
  auto rgbResult = ftc.Execute(ds);
  VISKORES_TEST_ASSERT(rgbResult.HasPointField("colors"), "Field missing.");
  Result = rgbResult.GetPointField("colors");
  viskores::cont::ArrayHandle<viskores::Vec3ui_8> resultRGBHandle;
  Result.GetData().AsArrayHandle(resultRGBHandle);

  //values confirmed with ParaView 5.11
  const viskores::Vec3ui_8 correct_diverging_rgb_values[nvals] = {
    { 0, 0, 255 },     { 59, 76, 192 },   { 124, 159, 249 }, { 192, 212, 245 },
    { 242, 203, 183 }, { 238, 133, 104 }, { 180, 4, 38 },    { 255, 0, 0 }
  };
  auto portalRGB = resultRGBHandle.ReadPortal();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGB.Get(static_cast<viskores::Id>(i));
    VISKORES_TEST_ASSERT(result == correct_diverging_rgb_values[i],
                         "incorrect value when interpolating between values");
  }
}
}

int UnitTestFieldToColors(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFieldToColors, argc, argv);
}
