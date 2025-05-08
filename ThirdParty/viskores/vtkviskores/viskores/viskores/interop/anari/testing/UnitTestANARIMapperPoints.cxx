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

// viskores::anari
#include <viskores/interop/anari/ANARIMapperPoints.h>
// viskores
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/io/EncodePNG.h>
#include <viskores/source/Tangle.h>
// std
#include <cstdlib>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Create Viskores datasets /////////////////////////////////////////////////////

  viskores::source::Tangle source;
  source.SetPointDimensions({ 32 });
  auto tangle = source.Execute();

  auto& tangle_field = tangle.GetField("tangle");
  viskores::Range range;
  tangle_field.GetRange(&range);
  const auto isovalue = range.Center();

  viskores::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValue(isovalue);
  contourFilter.SetActiveField("tangle");
  auto tangleIso = contourFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  viskores::interop::anari::ANARIActor actor(
    tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle"));

  viskores::interop::anari::ANARIMapperPoints mIso(d, actor);
  setColorMap(d, mIso);

  auto surface = mIso.GetANARISurface();
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);

  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(-0.05, 1.43, 1.87),
                       viskores::Vec3f_32(0.32, -0.53, -0.79),
                       viskores::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/points.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIMapperPoints(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
