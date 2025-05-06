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

#include <viskores/rendering/TextAnnotationBillboard.h>

#include <viskores/Matrix.h>

namespace viskores
{
namespace rendering
{

TextAnnotationBillboard::TextAnnotationBillboard(const std::string& text,
                                                 const viskores::rendering::Color& color,
                                                 viskores::Float32 scalar,
                                                 const viskores::Vec3f_32& position,
                                                 viskores::Float32 angleDegrees)
  : TextAnnotation(text, color, scalar)
  , Position(position)
  , Angle(angleDegrees)
{
}

void TextAnnotationBillboard::SetPosition(const viskores::Vec3f_32& position)
{
  this->Position = position;
}

void TextAnnotationBillboard::SetPosition(viskores::Float32 xpos,
                                          viskores::Float32 ypos,
                                          viskores::Float32 zpos)
{
  this->SetPosition(viskores::make_Vec(xpos, ypos, zpos));
}

void TextAnnotationBillboard::Render(const viskores::rendering::Camera& camera,
                                     const viskores::rendering::WorldAnnotator& worldAnnotator,
                                     viskores::rendering::Canvas& canvas) const
{
  using MatrixType = viskores::Matrix<viskores::Float32, 4, 4>;
  using VectorType = viskores::Vec3f_32;

  MatrixType viewMatrix = camera.CreateViewMatrix();
  MatrixType projectionMatrix =
    camera.CreateProjectionMatrix(canvas.GetWidth(), canvas.GetHeight());

  VectorType screenPos = viskores::Transform3DPointPerspective(
    viskores::MatrixMultiply(projectionMatrix, viewMatrix), this->Position);

  canvas.SetViewToScreenSpace(camera, true);
  MatrixType translateMatrix =
    viskores::Transform3DTranslate(screenPos[0], screenPos[1], -screenPos[2]);

  viskores::Float32 windowAspect =
    viskores::Float32(canvas.GetWidth()) / viskores::Float32(canvas.GetHeight());

  MatrixType scaleMatrix = viskores::Transform3DScale(1.f / windowAspect, 1.f, 1.f);

  MatrixType viewportMatrix;
  viskores::MatrixIdentity(viewportMatrix);
  //if view type == 2D?
  {
    viskores::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(canvas.GetWidth(), canvas.GetHeight(), vl, vr, vb, vt);
    viskores::Float32 xs = (vr - vl);
    viskores::Float32 ys = (vt - vb);
    viewportMatrix = viskores::Transform3DScale(2.f / xs, 2.f / ys, 1.f);
  }

  MatrixType rotateMatrix = viskores::Transform3DRotateZ(this->Angle * viskores::Pi_180f());

  viskores::Matrix<viskores::Float32, 4, 4> fullTransformMatrix = viskores::MatrixMultiply(
    translateMatrix,
    viskores::MatrixMultiply(scaleMatrix, viskores::MatrixMultiply(viewportMatrix, rotateMatrix)));

  VectorType origin =
    viskores::Transform3DPointPerspective(fullTransformMatrix, VectorType(0, 0, 0));
  VectorType right = viskores::Transform3DVector(fullTransformMatrix, VectorType(1, 0, 0));
  VectorType up = viskores::Transform3DVector(fullTransformMatrix, VectorType(0, 1, 0));

  // scale depth from (1, -1) to (0, 1);
  viskores::Float32 depth = screenPos[2] * .5f + .5f;
  worldAnnotator.AddText(
    origin, right, up, this->Scale, this->Anchor, this->TextColor, this->Text, depth);

  canvas.SetViewToWorldSpace(camera, true);
}
}
} // viskores::rendering
