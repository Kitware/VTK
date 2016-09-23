/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLTextActor3D.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkPath.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"

#include <sstream>
#include <string>

vtkStandardNewMacro(vtkOpenGLTextActor3D)

//------------------------------------------------------------------------------
void vtkOpenGLTextActor3D::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkOpenGLTextActor3D::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  vtkOpenGLGL2PSHelper *gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case vtkOpenGLGL2PSHelper::Capture:
        return this->RenderGL2PS(vp, gl2ps);
      case vtkOpenGLGL2PSHelper::Background:
        return 0; // No render.
      case vtkOpenGLGL2PSHelper::Inactive:
        break; // normal render.
    }
  }

  return this->Superclass::RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
vtkOpenGLTextActor3D::vtkOpenGLTextActor3D()
{
}

//------------------------------------------------------------------------------
vtkOpenGLTextActor3D::~vtkOpenGLTextActor3D()
{
}

//------------------------------------------------------------------------------
int vtkOpenGLTextActor3D::RenderGL2PS(vtkViewport *vp,
                                      vtkOpenGLGL2PSHelper *gl2ps)
{
  vtkRenderer *ren = vtkRenderer::SafeDownCast(vp);
  if (!ren)
  {
    vtkWarningMacro("Viewport is not a renderer.");
    return 0;
  }

  // Get path
  std::string input = this->Input && this->Input[0] ? this->Input : "";
  vtkNew<vtkPath> textPath;

  vtkTextRenderer *tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkWarningMacro(<<"Cannot generate path data from 3D text string '"
                    << input << "': Text renderer unavailable.");
    return 0;
  }

  if (!tren->StringToPath(this->TextProperty, input, textPath.GetPointer(),
                          vtkTextActor3D::GetRenderedDPI()))
  {
    vtkWarningMacro(<<"Failed to generate path data from 3D text string '"
                    << input << "': StringToPath failed.");
    return 0;
  }

  // Get actor info
  vtkMatrix4x4 *actorMatrix = this->GetMatrix();
  double actorBounds[6];
  this->GetBounds(actorBounds);
  double textPos[3] = {(actorBounds[1] + actorBounds[0]) * 0.5,
                       (actorBounds[3] + actorBounds[2]) * 0.5,
                       (actorBounds[5] + actorBounds[4]) * 0.5};

  double *fgColord = this->TextProperty->GetColor();
  unsigned char fgColor[4] = {
    static_cast<unsigned char>(fgColord[0] * 255),
    static_cast<unsigned char>(fgColord[1] * 255),
    static_cast<unsigned char>(fgColord[2] * 255),
    static_cast<unsigned char>(this->TextProperty->GetOpacity() * 255)};

  // Draw the background quad as a path:
  if (this->TextProperty->GetBackgroundOpacity() > 0.f)
  {
    double *bgColord = this->TextProperty->GetBackgroundColor();
    unsigned char bgColor[4] = {
      static_cast<unsigned char>(bgColord[0] * 255),
      static_cast<unsigned char>(bgColord[1] * 255),
      static_cast<unsigned char>(bgColord[2] * 255),
      static_cast<unsigned char>(this->TextProperty->GetBackgroundOpacity() *
                                 255)};

    // Get the camera so we can calculate an offset to place the background
    // behind the text.
    vtkCamera *cam = ren->GetActiveCamera();
    vtkMatrix4x4 *mat = cam->GetCompositeProjectionTransformMatrix(
          ren->GetTiledAspectRatio(), 0., 1.);
    double forward[3] = {mat->GetElement(2, 0),
                         mat->GetElement(2, 1),
                         mat->GetElement(2, 2)};
    vtkMath::Normalize(forward);
    double bgPos[3] = {textPos[0] - (forward[0] * 0.0001),
                       textPos[1] - (forward[1] * 0.0001),
                       textPos[2] - (forward[2] * 0.0001)};

    vtkTextRenderer::Metrics metrics;
    if (tren->GetMetrics(this->TextProperty, input, metrics,
                         vtkTextActor3D::GetRenderedDPI()))
    {
      vtkNew<vtkPath> bgPath;
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopLeft.GetX()),
                              static_cast<double>(metrics.TopLeft.GetY()),
                              0., vtkPath::MOVE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopRight.GetX()),
                              static_cast<double>(metrics.TopRight.GetY()),
                              0., vtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.BottomRight.GetX()),
                              static_cast<double>(metrics.BottomRight.GetY()),
                              0., vtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.BottomLeft.GetX()),
                              static_cast<double>(metrics.BottomLeft.GetY()),
                              0., vtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopLeft.GetX()),
                              static_cast<double>(metrics.TopLeft.GetY()),
                              0., vtkPath::LINE_TO);

      std::ostringstream bgLabel;
      bgLabel << "vtkOpenGLTextActor3D::RenderGL2PS background for string: '"
              << input << "'.";
      gl2ps->Draw3DPath(bgPath.GetPointer(), actorMatrix, bgPos, bgColor, ren,
                        bgLabel.str().c_str());
    }
  }

  // Draw the text path:
  std::ostringstream label;
  label << "vtkOpenGLTextActor3D::RenderGL2PS path for string: '"
        << input << "'.";
  gl2ps->Draw3DPath(textPath.GetPointer(), actorMatrix, textPos, fgColor, ren,
                    label.str().c_str());

  return 1;
}
