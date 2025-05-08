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
#ifndef viskores_rendering_AxisAnnotation3D_h
#define viskores_rendering_AxisAnnotation3D_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/Range.h>

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/AxisAnnotation.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/TextAnnotationBillboard.h>
#include <viskores/rendering/WorldAnnotator.h>

#include <memory>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT AxisAnnotation3D : public AxisAnnotation
{
private:
protected:
  viskores::Float64 TickMajorSize, TickMajorOffset;
  viskores::Float64 TickMinorSize, TickMinorOffset;
  int Axis;
  viskores::Vec3f_32 Invert;
  viskores::Vec3f_64 Point0, Point1;
  viskores::Range Range;
  viskores::Float64 FontScale;
  viskores::Float32 FontOffset;
  viskores::Float32 LineWidth;
  viskores::rendering::Color Color;
  std::vector<std::unique_ptr<TextAnnotationBillboard>> Labels;
  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation3D();

  AxisAnnotation3D(const AxisAnnotation3D&) = delete;

  AxisAnnotation3D& operator=(const AxisAnnotation3D&) = delete;

  VISKORES_CONT
  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  VISKORES_CONT
  void SetColor(viskores::rendering::Color c) { this->Color = c; }

  VISKORES_CONT
  void SetAxis(int a) { this->Axis = a; }

  void SetTickInvert(bool x, bool y, bool z);

  /// offset of 0 means the tick is inside the frame
  /// offset of 1 means the tick is outside the frame
  /// offset of 0.5 means the tick is centered on the frame
  VISKORES_CONT
  void SetMajorTickSize(viskores::Float64 size, viskores::Float64 offset)
  {
    this->TickMajorSize = size;
    this->TickMajorOffset = offset;
  }
  VISKORES_CONT
  void SetMinorTickSize(viskores::Float64 size, viskores::Float64 offset)
  {
    this->TickMinorSize = size;
    this->TickMinorOffset = offset;
  }

  VISKORES_CONT
  void SetWorldPosition(const viskores::Vec3f_64& point0, const viskores::Vec3f_64& point1)
  {
    this->Point0 = point0;
    this->Point1 = point1;
  }

  VISKORES_CONT
  void SetWorldPosition(viskores::Float64 x0,
                        viskores::Float64 y0,
                        viskores::Float64 z0,
                        viskores::Float64 x1,
                        viskores::Float64 y1,
                        viskores::Float64 z1)
  {
    this->SetWorldPosition(viskores::make_Vec(x0, y0, z0), viskores::make_Vec(x1, y1, z1));
  }

  void SetLabelFontScale(viskores::Float64 s);

  void SetLabelFontOffset(viskores::Float32 off) { this->FontOffset = off; }

  void SetRange(const viskores::Range& range) { this->Range = range; }

  void SetRange(viskores::Float64 lower, viskores::Float64 upper)
  {
    this->SetRange(viskores::Range(lower, upper));
  }

  void Render(const viskores::rendering::Camera& camera,
              const viskores::rendering::WorldAnnotator& worldAnnotator,
              viskores::rendering::Canvas& canvas) override;
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_AxisAnnotation3D_h
