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

#ifndef viskores_rendering_LineRendererBatcher_h
#define viskores_rendering_LineRendererBatcher_h

#include <string>
#include <vector>

#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT LineRendererBatcher
{
public:
  VISKORES_CONT
  LineRendererBatcher();

  VISKORES_CONT
  void BatchLine(const viskores::Vec3f_64& start,
                 const viskores::Vec3f_64& end,
                 const viskores::rendering::Color& color);

  VISKORES_CONT
  void BatchLine(const viskores::Vec3f_32& start,
                 const viskores::Vec3f_32& end,
                 const viskores::rendering::Color& color);

  void Render(const viskores::rendering::Canvas* canvas) const;

private:
  std::vector<viskores::Vec3f_32> Starts;
  std::vector<viskores::Vec3f_32> Ends;
  std::vector<viskores::Vec4f_32> Colors;
};
}
} // namespace viskores::rendering

#endif // viskores_rendering_LineRendererBatcher_h
