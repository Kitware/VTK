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

#include <viskores/rendering/Color.h>

namespace viskores
{
namespace rendering
{

viskores::rendering::Color viskores::rendering::Color::black(0, 0, 0, 1);
viskores::rendering::Color viskores::rendering::Color::white(1, 1, 1, 1);

viskores::rendering::Color viskores::rendering::Color::red(1, 0, 0, 1);
viskores::rendering::Color viskores::rendering::Color::green(0, 1, 0, 1);
viskores::rendering::Color viskores::rendering::Color::blue(0, 0, 1, 1);

viskores::rendering::Color viskores::rendering::Color::cyan(0, 1, 1, 1);
viskores::rendering::Color viskores::rendering::Color::magenta(1, 0, 1, 1);
viskores::rendering::Color viskores::rendering::Color::yellow(1, 1, 0, 1);

viskores::rendering::Color viskores::rendering::Color::gray10(.1f, .1f, .1f, 1);
viskores::rendering::Color viskores::rendering::Color::gray20(.2f, .2f, .2f, 1);
viskores::rendering::Color viskores::rendering::Color::gray30(.3f, .3f, .3f, 1);
viskores::rendering::Color viskores::rendering::Color::gray40(.4f, .4f, .4f, 1);
viskores::rendering::Color viskores::rendering::Color::gray50(.5f, .5f, .5f, 1);
viskores::rendering::Color viskores::rendering::Color::gray60(.6f, .6f, .6f, 1);
viskores::rendering::Color viskores::rendering::Color::gray70(.7f, .7f, .7f, 1);
viskores::rendering::Color viskores::rendering::Color::gray80(.8f, .8f, .8f, 1);
viskores::rendering::Color viskores::rendering::Color::gray90(.9f, .9f, .9f, 1);
}
} // namespace viskores::rendering
