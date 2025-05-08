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
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>
#include <viskores/interop/anari/ANARIScene.h>
// viskores
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/vector_analysis/Gradient.h>
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
  contourFilter.SetActiveField(tangle_field.GetName());
  auto tangleIso = contourFilter.Execute(tangle);

  viskores::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField(tangle_field.GetName());
  gradientFilter.SetOutputFieldName("Gradient");
  auto tangleGrad = gradientFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  viskores::interop::anari::ANARIScene scene(d);

  auto& mVol = scene.AddMapper(viskores::interop::anari::ANARIMapperVolume(d));
  mVol.SetName("volume");

  auto& mIso = scene.AddMapper(viskores::interop::anari::ANARIMapperTriangles(d));
  mIso.SetName("isosurface");
  mIso.SetCalculateNormals(true);

  auto& mGrad = scene.AddMapper(viskores::interop::anari::ANARIMapperGlyphs(d));
  mGrad.SetName("gradient");

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       scene.GetANARIWorld(),
                       viskores::Vec3f_32(-0.05, 1.43, 1.87),
                       viskores::Vec3f_32(0.32, -0.53, -0.79),
                       viskores::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/scene-empty-mappers.png");

  // Render a frame ///////////////////////////////////////////////////////////

  mVol.SetActor({ tangle.GetCellSet(), tangle.GetCoordinateSystem(), tangle.GetField("tangle") });
  mIso.SetActor(
    { tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle") });
  mGrad.SetActor(
    { tangleGrad.GetCellSet(), tangleGrad.GetCoordinateSystem(), tangleGrad.GetField("Gradient") });

  setColorMap(d, mVol);
  setColorMap(d, mIso);
  setColorMap(d, mGrad);

  renderTestANARIImage(d,
                       scene.GetANARIWorld(),
                       viskores::Vec3f_32(-0.05, 1.43, 1.87),
                       viskores::Vec3f_32(0.32, -0.53, -0.79),
                       viskores::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/scene.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIScene(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
