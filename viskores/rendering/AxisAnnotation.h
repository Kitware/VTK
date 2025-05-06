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
#ifndef viskores_rendering_AxisAnnotation_h
#define viskores_rendering_AxisAnnotation_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Color.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT AxisAnnotation
{
protected:
  static void CalculateTicks(const viskores::Range& range,
                             bool minor,
                             std::vector<viskores::Float64>& positions,
                             std::vector<viskores::Float64>& proportions,
                             int modifyTickQuantity);
  static void CalculateTicksLogarithmic(const viskores::Range& range,
                                        bool minor,
                                        std::vector<viskores::Float64>& positions,
                                        std::vector<viskores::Float64>& proportions);

public:
  AxisAnnotation() = default;

  virtual ~AxisAnnotation() = default;

  virtual void Render(const viskores::rendering::Camera& camera,
                      const viskores::rendering::WorldAnnotator& worldAnnotator,
                      viskores::rendering::Canvas& canvas) = 0;
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_AxisAnnotation_h
