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
#ifndef viskores_rendering_View2D_h
#define viskores_rendering_View2D_h

#include <viskores/rendering/View.h>

#include <viskores/rendering/AxisAnnotation2D.h>
#include <viskores/rendering/ColorBarAnnotation.h>

namespace viskores
{
namespace rendering
{

/// @brief A view for a 3D data set.
///
/// 2D data are rendered directly on the X-Y plane.
class VISKORES_RENDERING_EXPORT View2D : public viskores::rendering::View
{
public:
  View2D(
    const viskores::rendering::Scene& scene,
    const viskores::rendering::Mapper& mapper,
    const viskores::rendering::Canvas& canvas,
    const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
    const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  View2D(
    const viskores::rendering::Scene& scene,
    const viskores::rendering::Mapper& mapper,
    const viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
    const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  void Paint() override;

  void RenderScreenAnnotations() override;

  void RenderWorldAnnotations() override;

private:
  void UpdateCameraProperties();

  // 2D-specific annotations
  viskores::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  viskores::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  viskores::rendering::ColorBarAnnotation ColorBarAnnotation;
};
}
} // namespace viskores::rendering

#endif //viskores_rendering_View2D_h
