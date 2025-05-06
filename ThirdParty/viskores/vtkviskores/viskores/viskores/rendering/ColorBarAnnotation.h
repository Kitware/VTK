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
#ifndef viskores_rendering_ColorBarAnnotation_h
#define viskores_rendering_ColorBarAnnotation_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/AxisAnnotation2D.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT ColorBarAnnotation
{
protected:
  viskores::cont::ColorTable ColorTable;
  viskores::rendering::AxisAnnotation2D Axis;
  viskores::Bounds Position;
  bool Horizontal;
  std::string FieldName;

public:
  ColorBarAnnotation();

  VISKORES_CONT
  void SetColorTable(const viskores::cont::ColorTable& colorTable)
  {
    this->ColorTable = colorTable;
  }

  VISKORES_CONT
  void SetRange(const viskores::Range& range, viskores::IdComponent numTicks);

  VISKORES_CONT
  void SetFieldName(const std::string& fieldName);

  VISKORES_CONT
  void SetRange(viskores::Float64 l, viskores::Float64 h, viskores::IdComponent numTicks)
  {
    this->SetRange(viskores::Range(l, h), numTicks);
  }


  VISKORES_CONT
  void SetPosition(const viskores::Bounds& position);

  void Render(const viskores::rendering::Camera& camera,
              const viskores::rendering::WorldAnnotator& worldAnnotator,
              viskores::rendering::Canvas& canvas);
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_ColorBarAnnotation_h
