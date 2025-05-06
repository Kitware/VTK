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
#ifndef viskores_rendering_WorldAnnotator_h
#define viskores_rendering_WorldAnnotator_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/Types.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/LineRendererBatcher.h>

namespace viskores
{
namespace rendering
{

class Canvas;

class VISKORES_RENDERING_EXPORT WorldAnnotator
{
public:
  explicit WorldAnnotator(const viskores::rendering::Canvas* canvas);

  void AddLine(const viskores::Vec3f_64& point0,
               const viskores::Vec3f_64& point1,
               viskores::Float32 lineWidth,
               const viskores::rendering::Color& color,
               bool inFront = false) const;

  VISKORES_CONT
  void AddLine(viskores::Float64 x0,
               viskores::Float64 y0,
               viskores::Float64 z0,
               viskores::Float64 x1,
               viskores::Float64 y1,
               viskores::Float64 z1,
               viskores::Float32 lineWidth,
               const viskores::rendering::Color& color,
               bool inFront = false) const
  {
    this->AddLine(
      viskores::make_Vec(x0, y0, z0), viskores::make_Vec(x1, y1, z1), lineWidth, color, inFront);
  }

  VISKORES_CONT
  void BeginLineRenderingBatch() const;

  VISKORES_CONT
  void EndLineRenderingBatch() const;

  void AddText(const viskores::Vec3f_32& origin,
               const viskores::Vec3f_32& right,
               const viskores::Vec3f_32& up,
               viskores::Float32 scale,
               const viskores::Vec2f_32& anchor,
               const viskores::rendering::Color& color,
               const std::string& text,
               viskores::Float32 depth = 0.f) const;

  VISKORES_CONT
  void AddText(viskores::Float32 originX,
               viskores::Float32 originY,
               viskores::Float32 originZ,
               viskores::Float32 rightX,
               viskores::Float32 rightY,
               viskores::Float32 rightZ,
               viskores::Float32 upX,
               viskores::Float32 upY,
               viskores::Float32 upZ,
               viskores::Float32 scale,
               viskores::Float32 anchorX,
               viskores::Float32 anchorY,
               const viskores::rendering::Color& color,
               const std::string& text) const
  {
    this->AddText(viskores::make_Vec(originX, originY, originZ),
                  viskores::make_Vec(rightX, rightY, rightZ),
                  viskores::make_Vec(upX, upY, upZ),
                  scale,
                  viskores::make_Vec(anchorX, anchorY),
                  color,
                  text);
  }

private:
  const viskores::rendering::Canvas* Canvas;
  mutable viskores::rendering::LineRendererBatcher LineBatcher;
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_WorldAnnotator_h
