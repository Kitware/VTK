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

#include <viskores/cont/testing/Testing.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::rendering::Canvas canvas;
  canvas.SetBackgroundColor(viskores::rendering::Color::white);
  canvas.Clear();
  canvas.AddLine(-0.8, 0.8, 0.8, 0.8, 1.0f, viskores::rendering::Color::black);
  canvas.AddLine(0.8, 0.8, 0.8, -0.8, 1.0f, viskores::rendering::Color::black);
  canvas.AddLine(0.8, -0.8, -0.8, -0.8, 1.0f, viskores::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, -0.8, 0.8, 1.0f, viskores::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, 0.8, 0.8, 1.0f, viskores::rendering::Color::black);
  canvas.AddLine(-0.8, 0.8, 0.8, -0.8, 1.0f, viskores::rendering::Color::black);
  viskores::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, viskores::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  canvas.SaveAs("canvas.pnm");
}

} //namespace

int UnitTestCanvas(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
