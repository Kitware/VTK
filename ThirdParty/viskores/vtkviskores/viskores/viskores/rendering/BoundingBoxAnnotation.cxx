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

#include <viskores/rendering/BoundingBoxAnnotation.h>

namespace viskores
{
namespace rendering
{

BoundingBoxAnnotation::BoundingBoxAnnotation()
  : Color(0.5, 0.5, 0.5)
  , Extents(-1, 1, -1, 1, -1, 1)
{
}

BoundingBoxAnnotation::~BoundingBoxAnnotation() {}

void BoundingBoxAnnotation::Render(const viskores::rendering::Camera&,
                                   const WorldAnnotator& annotator)
{
  //win->SetupForWorldSpace();

  viskores::Float32 linewidth = 1.0;

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
}
}
} // namespace viskores::rendering
