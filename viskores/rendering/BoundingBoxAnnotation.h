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
#ifndef viskores_rendering_BoundingBoxAnnotation_h
#define viskores_rendering_BoundingBoxAnnotation_h

#include <viskores/Bounds.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT BoundingBoxAnnotation
{
private:
  viskores::rendering::Color Color;
  viskores::Bounds Extents;

public:
  BoundingBoxAnnotation();

  virtual ~BoundingBoxAnnotation();

  VISKORES_CONT
  const viskores::Bounds& GetExtents() const { return this->Extents; }

  VISKORES_CONT
  void SetExtents(const viskores::Bounds& extents) { this->Extents = extents; }

  VISKORES_CONT
  const viskores::rendering::Color& GetColor() const { return this->Color; }

  VISKORES_CONT
  void SetColor(viskores::rendering::Color c) { this->Color = c; }

  virtual void Render(const viskores::rendering::Camera&, const WorldAnnotator& annotator);
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_BoundingBoxAnnotation_h
