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

#include <viskores/Types.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/ColorTableMap.h>
#include <viskores/cont/ColorTableSamples.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <iostream>

namespace
{

template <viskores::IdComponent N>
void CheckColors(viskores::cont::ArrayHandle<viskores::Vec<viskores::UInt8, N>> result,
                 const std::vector<viskores::Vec<viskores::UInt8, N>>& expected)
{
  using Vec = viskores::Vec<viskores::UInt8, N>;

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()));
  auto portal = result.ReadPortal();
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    Vec resultValue = portal.Get(index);
    Vec expectedValue = expected[static_cast<std::size_t>(index)];
    VISKORES_TEST_ASSERT(
      resultValue == expectedValue, "Expected color ", expectedValue, " but got ", resultValue);
  }
}

void TestConstructors()
{
  viskores::Range inValidRange{ 1.0, 0.0 };
  viskores::Range range{ 0.0, 1.0 };
  viskores::Vec<float, 3> rgb1{ 0.0f, 0.0f, 0.0f };
  viskores::Vec<float, 3> rgb2{ 1.0f, 1.0f, 1.0f };
  auto rgbspace = viskores::ColorSpace::RGB;
  auto hsvspace = viskores::ColorSpace::HSV;
  auto diverging = viskores::ColorSpace::Diverging;

  viskores::cont::ColorTable table(rgbspace);
  VISKORES_TEST_ASSERT(table.GetColorSpace() == rgbspace, "color space not saved");
  VISKORES_TEST_ASSERT(table.GetRange() == inValidRange, "default range incorrect");

  viskores::cont::ColorTable tableRGB(range, rgb1, rgb2, hsvspace);
  VISKORES_TEST_ASSERT(tableRGB.GetColorSpace() == hsvspace, "color space not saved");
  VISKORES_TEST_ASSERT(tableRGB.GetRange() == range, "color range not saved");

  viskores::Vec<float, 4> rgba1{ 0.0f, 0.0f, 0.0f, 1.0f };
  viskores::Vec<float, 4> rgba2{ 1.0f, 1.0f, 1.0f, 0.0f };
  viskores::cont::ColorTable tableRGBA(range, rgba1, rgba2, diverging);
  VISKORES_TEST_ASSERT(tableRGBA.GetColorSpace() == diverging, "color space not saved");
  VISKORES_TEST_ASSERT(tableRGBA.GetRange() == range, "color range not saved");

  //verify we can store a vector of tables
  std::vector<viskores::cont::ColorTable> tables;
  tables.push_back(table);
  tables.push_back(tableRGB);
  tables.push_back(tableRGBA);
  tables.push_back(tableRGBA);
  tables.push_back(tableRGB);
  tables.push_back(table);
}

void TestLoadPresets()
{
  viskores::Range range{ 0.0, 1.0 };
  auto rgbspace = viskores::ColorSpace::RGB;
  auto hsvspace = viskores::ColorSpace::HSV;
  auto labspace = viskores::ColorSpace::Lab;
  auto diverging = viskores::ColorSpace::Diverging;

  {
    viskores::cont::ColorTable table(rgbspace);
    VISKORES_TEST_ASSERT(table.LoadPreset("Cool to Warm"));
    VISKORES_TEST_ASSERT(table.GetColorSpace() == diverging,
                         "color space not switched when loading preset");
    VISKORES_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
    VISKORES_TEST_ASSERT(table.GetNumberOfPoints() == 3);

    VISKORES_TEST_ASSERT(table.LoadPreset(viskores::cont::ColorTable::Preset::CoolToWarmExtended));
    VISKORES_TEST_ASSERT(table.GetColorSpace() == labspace,
                         "color space not switched when loading preset");
    VISKORES_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
    VISKORES_TEST_ASSERT(table.GetNumberOfPoints() == 35);

    table.SetColorSpace(hsvspace);
    VISKORES_TEST_ASSERT((table.LoadPreset("no table with this name") == false),
                         "failed to error out on bad preset table name");
    //verify that after a failure we still have the previous preset loaded
    VISKORES_TEST_ASSERT(table.GetColorSpace() == hsvspace,
                         "color space not switched when loading preset");
    VISKORES_TEST_ASSERT(table.GetRange() == range, "color range not correct after failing preset");
    VISKORES_TEST_ASSERT(table.GetNumberOfPoints() == 35);
  }


  //verify that we can get the presets
  std::set<std::string> names = viskores::cont::ColorTable::GetPresets();
  VISKORES_TEST_ASSERT(names.size() == 18, "incorrect number of names in preset set");

  VISKORES_TEST_ASSERT(names.count("Inferno") == 1, "names should contain inferno");
  VISKORES_TEST_ASSERT(names.count("Black-Body Radiation") == 1,
                       "names should contain black-body radiation");
  VISKORES_TEST_ASSERT(names.count("Viridis") == 1, "names should contain viridis");
  VISKORES_TEST_ASSERT(names.count("Black - Blue - White") == 1,
                       "names should contain black, blue and white");
  VISKORES_TEST_ASSERT(names.count("Blue to Orange") == 1, "names should contain samsel fire");
  VISKORES_TEST_ASSERT(names.count("Jet") == 1, "names should contain jet");

  // verify that we can load in all the listed color tables
  for (auto&& name : names)
  {
    viskores::cont::ColorTable table(name);
    VISKORES_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset ", name);
  }

  auto presetEnum = { viskores::cont::ColorTable::Preset::Default,
                      viskores::cont::ColorTable::Preset::CoolToWarm,
                      viskores::cont::ColorTable::Preset::CoolToWarmExtended,
                      viskores::cont::ColorTable::Preset::Viridis,
                      viskores::cont::ColorTable::Preset::Inferno,
                      viskores::cont::ColorTable::Preset::Plasma,
                      viskores::cont::ColorTable::Preset::BlackBodyRadiation,
                      viskores::cont::ColorTable::Preset::XRay,
                      viskores::cont::ColorTable::Preset::Green,
                      viskores::cont::ColorTable::Preset::BlackBlueWhite,
                      viskores::cont::ColorTable::Preset::BlueToOrange,
                      viskores::cont::ColorTable::Preset::GrayToRed,
                      viskores::cont::ColorTable::Preset::ColdAndHot,
                      viskores::cont::ColorTable::Preset::BlueGreenOrange,
                      viskores::cont::ColorTable::Preset::YellowGrayBlue,
                      viskores::cont::ColorTable::Preset::RainbowUniform,
                      viskores::cont::ColorTable::Preset::Jet,
                      viskores::cont::ColorTable::Preset::RainbowDesaturated };
  for (viskores::cont::ColorTable::Preset preset : presetEnum)
  {
    viskores::cont::ColorTable table(preset);
    VISKORES_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset");
  }
}

void TestClamping()
{
  std::cout << "Test Clamping" << std::endl;

  viskores::Range range{ 0.0, 1.0 };
  viskores::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
  viskores::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
  auto rgbspace = viskores::ColorSpace::RGB;

  viskores::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
  VISKORES_TEST_ASSERT(table.GetClamping() == true, "clamping not setup properly");

  auto field = viskores::cont::make_ArrayHandle({ -1, 0, 1, 2 });

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //verify that we clamp the values to the expected range
  CheckColors(colors, { { 0, 255, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 255 } });
}

void TestRangeColors()
{
  std::cout << "Test default ranges" << std::endl;

  viskores::Range range{ -1.0, 2.0 };
  viskores::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
  viskores::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
  auto rgbspace = viskores::ColorSpace::RGB;

  viskores::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
  table.SetClampingOff();
  VISKORES_TEST_ASSERT(table.GetClamping() == false, "clamping not setup properly");

  auto field = viskores::cont::make_ArrayHandle({ -2, -1, 2, 3 });

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //verify that both the above and below range colors are used,
  //and that the default value of both is 0,0,0
  CheckColors(colors, { { 0, 0, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 0, 0, 0 } });


  std::cout << "Test specified ranges" << std::endl;
  //verify that we can specify custom above and below range colors
  table.SetAboveRangeColor(viskores::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
  table.SetBelowRangeColor(viskores::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green
  const bool ran2 = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran2, "color table failed to execute");
  CheckColors(colors, { { 0, 0, 255 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 0 } });
}

void TestRescaleRange()
{
  std::cout << "Test Rescale Range" << std::endl;
  viskores::Range range{ -100.0, 100.0 };

  //implement a blue2yellow color table
  viskores::Vec<float, 3> rgb1{ 0.0f, 0.0f, 1.0f };
  viskores::Vec<float, 3> rgb2{ 1.0f, 1.0f, 0.0f };
  auto lab = viskores::ColorSpace::Lab;

  viskores::cont::ColorTable table(range, rgb1, rgb2, lab);
  table.AddPoint(0.0, viskores::Vec<float, 3>{ 0.5f, 0.5f, 0.5f });
  VISKORES_TEST_ASSERT(table.GetRange() == range, "custom range not saved");

  viskores::cont::ColorTable newTable = table.MakeDeepCopy();
  VISKORES_TEST_ASSERT(newTable.GetRange() == range, "custom range not saved");

  viskores::Range normalizedRange{ 0.0, 50.0 };
  newTable.RescaleToRange(normalizedRange);
  VISKORES_TEST_ASSERT(table.GetRange() == range, "deep copy not working properly");
  VISKORES_TEST_ASSERT(newTable.GetRange() == normalizedRange, "rescale of range failed");
  VISKORES_TEST_ASSERT(newTable.GetNumberOfPoints() == 3,
                       "rescaled has incorrect number of control points");

  //Verify that the rescaled color table generates correct colors
  auto field = viskores::cont::make_ArrayHandle({ 0, 10, 20, 30, 40, 50 });

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  const bool ran = viskores::cont::ColorTableMap(field, newTable, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //values confirmed with ParaView 5.4
  CheckColors(colors,
              { { 0, 0, 255 },
                { 105, 69, 204 },
                { 126, 109, 153 },
                { 156, 151, 117 },
                { 207, 202, 87 },
                { 255, 255, 0 } });
}

void TestAddPoints()
{
  std::cout << "Test Add Points" << std::endl;

  viskores::Range range{ -20, 20.0 };
  auto rgbspace = viskores::ColorSpace::RGB;

  viskores::cont::ColorTable table(rgbspace);
  table.AddPoint(-10.0, viskores::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
  table.AddPoint(-20.0, viskores::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
  table.AddPoint(20.0, viskores::Vec<float, 3>{ 0.0f, 0.0f, 0.0f });
  table.AddPoint(0.0, viskores::Vec<float, 3>{ 0.0f, 0.0f, 1.0f });

  VISKORES_TEST_ASSERT(table.GetRange() == range, "adding points to make range expand properly");
  VISKORES_TEST_ASSERT(table.GetNumberOfPoints() == 4,
                       "adding points caused number of control points to be wrong");

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  auto field = viskores::cont::make_ArrayHandle({ 10.0f, -5.0f, -15.0f });
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  CheckColors(colors, { { 0, 0, 128 }, { 0, 128, 255 }, { 128, 255, 255 } });
}

void TestAddSegments()
{
  std::cout << "Test Add Segments" << std::endl;

  viskores::Range range{ 0.0, 50.0 };
  auto diverging = viskores::ColorSpace::Diverging;

  viskores::cont::ColorTable table(viskores::cont::ColorTable::Preset::CoolToWarm);
  VISKORES_TEST_ASSERT(table.GetColorSpace() == diverging,
                       "color space not switched when loading preset");


  //Opacity Ramp from 0 to 1
  table.AddSegmentAlpha(0.0, 0.0f, 1.0, 1.0f);
  VISKORES_TEST_ASSERT(table.GetNumberOfPointsAlpha() == 2, "incorrect number of alpha points");

  table.RescaleToRange(range);

  //Verify that the opacity points have moved
  viskores::Vec<double, 4> opacityData;
  table.GetPointAlpha(1, opacityData);
  VISKORES_TEST_ASSERT(test_equal(opacityData[0], range.Max), "rescale to range failed on opacity");
  VISKORES_TEST_ASSERT(opacityData[1] == 1.0, "rescale changed opacity values");
  VISKORES_TEST_ASSERT(opacityData[2] == 0.5, "rescale modified mid/sharp of opacity");
  VISKORES_TEST_ASSERT(opacityData[3] == 0.0, "rescale modified mid/sharp of opacity");


  viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
  auto field = viskores::cont::make_ArrayHandle({ 0, 10, 20, 30, 40, 50 });
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //values confirmed with ParaView 5.4
  CheckColors(colors,
              { { 59, 76, 192, 0 },
                { 124, 159, 249, 51 },
                { 192, 212, 245, 102 },
                { 242, 203, 183, 153 },
                { 238, 133, 104, 204 },
                { 180, 4, 38, 255 } });
}

void TestRemovePoints()
{
  std::cout << "Test Remove Points" << std::endl;

  auto hsv = viskores::ColorSpace::HSV;

  viskores::cont::ColorTable table(hsv);
  //implement Blue to Red Rainbow color table
  table.AddSegment(0,
                   viskores::Vec<float, 3>{ 0.0f, 0.0f, 1.0f },
                   1., //second points color should be replaced by following segment
                   viskores::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

  table.AddPoint(-10.0, viskores::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
  table.AddPoint(-20.0, viskores::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
  table.AddPoint(20.0, viskores::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

  VISKORES_TEST_ASSERT(table.RemovePoint(-10.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePoint(-20.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePoint(20.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePoint(20.) == false, "can't remove a point that doesn't exist");

  VISKORES_TEST_ASSERT((table.GetRange() == viskores::Range{ 0.0, 1.0 }),
                       "removing points didn't update range");
  table.RescaleToRange(viskores::Range{ 0.0, 50.0 });

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  auto field = viskores::cont::make_ArrayHandle({ 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f });
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //values confirmed with ParaView 5.4
  CheckColors(colors,
              { { 0, 0, 255 },
                { 0, 204, 255 },
                { 0, 255, 102 },
                { 102, 255, 0 },
                { 255, 204, 0 },
                { 255, 0, 0 } });

  std::cout << "  Change Color Space" << std::endl;
  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors_rgb;
  table.SetColorSpace(viskores::ColorSpace::RGB);
  viskores::cont::ColorTableMap(field, table, colors_rgb);

  CheckColors(colors_rgb,
              { { 0, 0, 255 },
                { 51, 0, 204 },
                { 102, 0, 153 },
                { 153, 0, 102 },
                { 204, 0, 51 },
                { 255, 0, 0 } });
}

void TestOpacityOnlyPoints()
{
  std::cout << "Test Opacity Only Points" << std::endl;

  auto hsv = viskores::ColorSpace::HSV;

  viskores::cont::ColorTable table(hsv);
  //implement only a color table
  table.AddPointAlpha(0.0, 0.0f, 0.75f, 0.25f);
  table.AddPointAlpha(1.0, 1.0f);

  table.AddPointAlpha(10.0, 0.5f, 0.5f, 0.0f);
  table.AddPointAlpha(-10.0, 0.0f);
  table.AddPointAlpha(-20.0, 1.0f);
  table.AddPointAlpha(20.0, 0.5f);

  VISKORES_TEST_ASSERT(table.RemovePointAlpha(10.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePointAlpha(-10.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePointAlpha(-20.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePointAlpha(20.) == true, "failed to remove a existing point");
  VISKORES_TEST_ASSERT(table.RemovePointAlpha(20.) == false,
                       "can't remove a point that doesn't exist");

  VISKORES_TEST_ASSERT((table.GetRange() == viskores::Range{ 0.0, 1.0 }),
                       "removing points didn't update range");
  table.RescaleToRange(viskores::Range{ 0.0, 50.0 });

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
  auto field = viskores::cont::make_ArrayHandle({ 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f });
  const bool ran = viskores::cont::ColorTableMap(field, table, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //values confirmed with ParaView 5.4
  CheckColors(colors,
              { { 0, 0, 0, 0 },
                { 0, 0, 0, 1 },
                { 0, 0, 0, 11 },
                { 0, 0, 0, 52 },
                { 0, 0, 0, 203 },
                { 0, 0, 0, 255 } });
}

void TestWorkletTransport()
{
  std::cout << "Test Worklet Transport" << std::endl;

  using namespace viskores::worklet::colorconversion;

  viskores::cont::ColorTable table(viskores::cont::ColorTable::Preset::Green);
  VISKORES_TEST_ASSERT((table.GetRange() == viskores::Range{ 0.0, 1.0 }),
                       "loading linear green table failed with wrong range");
  VISKORES_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                       "loading linear green table failed with number of control points");

  auto samples = viskores::cont::make_ArrayHandle({ 0.0, 0.5, 1.0 });

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
  viskores::cont::Invoker invoke;
  invoke(TransferFunction{}, samples, table, colors);

  CheckColors(colors, { { 14, 28, 31, 255 }, { 21, 150, 21, 255 }, { 255, 251, 230, 255 } });
}

void TestSampling()
{
  std::cout << "Test Sampling" << std::endl;

  viskores::cont::ColorTable table(viskores::cont::ColorTable::Preset::Green);
  VISKORES_TEST_ASSERT((table.GetRange() == viskores::Range{ 0.0, 1.0 }),
                       "loading linear green table failed with wrong range");
  VISKORES_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                       "loading linear green table failed with number of control points");

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
  constexpr viskores::Id nvals = 3;
  table.Sample(nvals, colors);

  CheckColors(colors, { { 14, 28, 31, 255 }, { 21, 150, 21, 255 }, { 255, 251, 230, 255 } });
}

void TestLookupTable()
{
  std::cout << "Test Lookup Table" << std::endl;

  //build a color table with clamping off and verify that sampling works
  viskores::Range range{ 0.0, 50.0 };
  viskores::cont::ColorTable table(viskores::cont::ColorTable::Preset::CoolToWarm);
  table.RescaleToRange(range);
  table.SetClampingOff();
  table.SetAboveRangeColor(viskores::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
  table.SetBelowRangeColor(viskores::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

  viskores::cont::ColorTableSamplesRGB samples;
  table.Sample(256, samples);
  VISKORES_TEST_ASSERT((samples.Samples.GetNumberOfValues() == 260), "invalid sample length");

  viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
  auto field = viskores::cont::make_ArrayHandle({ -1, 0, 10, 20, 30, 40, 50, 60 });
  const bool ran = viskores::cont::ColorTableMap(field, samples, colors);
  VISKORES_TEST_ASSERT(ran, "color table failed to execute");

  //values confirmed with ParaView 5.11
  CheckColors(colors,
              { { 0, 0, 255 },
                { 59, 76, 192 },
                { 124, 159, 249 },
                { 192, 212, 245 },
                { 242, 203, 183 },
                { 238, 133, 104 },
                { 180, 4, 38 },
                { 255, 0, 0 } });
}

void Run()
{
  TestConstructors();
  TestLoadPresets();
  TestClamping();
  TestRangeColors();

  TestRescaleRange(); //uses Lab
  TestAddPoints();    //uses RGB
  TestAddSegments();  //uses Diverging && opacity
  TestRemovePoints(); //use HSV

  TestOpacityOnlyPoints();

  TestWorkletTransport();
  TestSampling();
  TestLookupTable();
}

} // anonymous namespace

int UnitTestColorTable(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
