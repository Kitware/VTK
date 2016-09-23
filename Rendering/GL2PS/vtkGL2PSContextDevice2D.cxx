/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSContextDevice2D.h"

#include "vtkBrush.h"
#include "vtkGL2PSUtilities.h"
#include "vtkMathTextUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkUnicodeString.h"

#include "vtk_gl2ps.h"

#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGL2PSContextDevice2D)

//-----------------------------------------------------------------------------
vtkGL2PSContextDevice2D::vtkGL2PSContextDevice2D()
{
}

//-----------------------------------------------------------------------------
vtkGL2PSContextDevice2D::~vtkGL2PSContextDevice2D()
{
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPoly(float *f, int n, unsigned char *colors,
                                       int nc_comps)
{
  // Only draw the quad if custom colors are defined or alpha is non-zero.
  // Postscript can't handle transparency, and the resulting output will have
  // a border around every quad otherwise.
  if ((colors || this->Pen->GetColorObject().GetAlpha() != 0) &&
      this->Pen->GetLineType() != vtkPen::NO_PEN)
  {
    this->Superclass::DrawPoly(f, n, colors, nc_comps);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPoints(float *points, int n,
                                         unsigned char *colors, int nc_comps)
{
  if (colors || this->Pen->GetColorObject().GetAlpha() != 0)
  {
    this->Superclass::DrawPoints(points, n, colors, nc_comps);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPointSprites(vtkImageData *sprite,
                                               float *points, int n,
                                               unsigned char *colors,
                                               int nc_comps)
{
  if (colors || this->Pen->GetColorObject().GetAlpha() != 0)
  {
    this->Superclass::DrawPointSprites(sprite, points, n, colors, nc_comps);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawMarkers(int shape, bool highlight,
                                          float *points, int n,
                                          unsigned char *colors, int nc_comps)
{
  switch (shape)
  {
    case VTK_MARKER_CROSS:
      this->DrawCrossMarkers(highlight, points, n, colors, nc_comps);
      break;
    default:
      // default is here for consistency -- Superclass defaults to plus for
      // unrecognized shapes.
    case VTK_MARKER_PLUS:
      this->DrawPlusMarkers(highlight, points, n, colors, nc_comps);
      break;
    case VTK_MARKER_SQUARE:
      this->DrawSquareMarkers(highlight, points, n, colors, nc_comps);
      break;
    case VTK_MARKER_CIRCLE:
      this->DrawCircleMarkers(highlight, points, n, colors, nc_comps);
      break;
    case VTK_MARKER_DIAMOND:
      this->DrawDiamondMarkers(highlight, points, n, colors, nc_comps);
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawQuad(float *points, int n)
{
  if (this->Brush->GetColorObject().GetAlpha() != 0)
  {
    this->Superclass::DrawQuad(points, n);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawQuadStrip(float *points, int n)
{
  if (this->Brush->GetColorObject().GetAlpha() != 0)
  {
    this->Superclass::DrawQuadStrip(points, n);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPolygon(float *points, int n)
{
  if (this->Brush->GetColorObject().GetAlpha() != 0)
  {
    this->Superclass::DrawPolygon(points, n);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawEllipseWedge(float x, float y,
                                               float outRx, float outRy,
                                               float inRx, float inRy,
                                               float startAngle, float stopAngle)
{
  if (this->Brush->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  // The path implementation can't handle start/stop angles. Defer to the
  // superclass in this case.
  if (std::fabs(startAngle) > 1e-5f || std::fabs(stopAngle - 360.0f) > 1e-5f)
  {
    this->Superclass::DrawEllipseWedge(x, y, outRx, outRy, inRx, inRy,
                                       startAngle, stopAngle);
  }

  vtkNew<vtkPath> path;
  this->AddEllipseToPath(path.GetPointer(), 0.f, 0.f, outRx, outRy, false);
  this->AddEllipseToPath(path.GetPointer(), 0.f, 0.f, inRx, inRy, true);

  std::stringstream label;
  label << "vtkGL2PSContextDevice2D::DrawEllipseWedge("
        << x << ", " << y << ", " << outRx << ", " << outRy << ", "
        << inRx << ", " << inRy << ", " << startAngle << ", " << stopAngle
        << ") path:";

  unsigned char color[4];
  this->Brush->GetColor(color);

  double rasterPos[3] = {static_cast<double>(x), static_cast<double>(y), 0.};

  this->TransformPoint(x, y);
  double windowPos[3] = {static_cast<double>(x), static_cast<double>(y), 0.};

  vtkGL2PSUtilities::DrawPath(path.GetPointer(), rasterPos, windowPos, color,
                              NULL, 0.0, -1.f, label.str().c_str());
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawEllipticArc(float x, float y,
                                              float rx, float ry,
                                              float startAngle,
                                              float stopAngle)
{
  if (this->Brush->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  // The path implementation can't handle start/stop angles. Defer to the
  // superclass in this case.
  if (fabs(startAngle) > 1e-5 || fabs(stopAngle - 360.0) > 1e-5)
  {
    this->Superclass::DrawEllipticArc(x, y, rx, ry, startAngle, stopAngle);
  }

  vtkNew<vtkPath> path;
  this->AddEllipseToPath(path.GetPointer(), 0.f, 0.f, rx, ry, false);
  this->TransformPath(path.GetPointer());

  double origin[3] = {x, y, 0.f};

  // Fill
  unsigned char fillColor[4];
  this->Brush->GetColor(fillColor);

  std::stringstream label;
  label << "vtkGL2PSContextDevice2D::DrawEllipticArc("
        << x << ", " << y << ", " << rx << ", " << ry << ", "
        << startAngle << ", " << stopAngle << ") fill:";

  vtkGL2PSUtilities::DrawPath(path.GetPointer(), origin, origin, fillColor,
                              NULL, 0.0, -1.f, label.str().c_str());

  // and stroke
  unsigned char strokeColor[4];
  this->Pen->GetColor(strokeColor);
  float strokeWidth = this->Pen->GetWidth();

  label.str("");
  label.clear();
  label << "vtkGL2PSContextDevice2D::DrawEllipticArc("
        << x << ", " << y << ", " << rx << ", " << ry << ", "
        << startAngle << ", " << stopAngle << ") stroke:";
  vtkGL2PSUtilities::DrawPath(path.GetPointer(), origin, origin, strokeColor,
                              NULL, 0.0, strokeWidth, label.str().c_str());
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawString(float *point,
                                         const vtkStdString &string)
{
  double p[3] = {point[0], point[1], 0.};
  vtkGL2PSUtilities::DrawString(string.c_str(), this->TextProp, p, 0.);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawString(float *point,
                                         const vtkUnicodeString &string)
{
  double p[3] = {point[0], point[1], 0.};
  vtkGL2PSUtilities::DrawString(string.utf8_str(), this->TextProp, p, 0.);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawMathTextString(float apoint[],
                                                 const vtkStdString &string)
{
  vtkNew<vtkPath> path;
  bool ok;
  if (vtkMathTextUtilities *mtu = vtkMathTextUtilities::GetInstance())
  {
    ok = mtu->StringToPath(string.c_str(), path.GetPointer(), this->TextProp,
                           this->RenderWindow->GetDPI());
  }
  else
  {
    vtkWarningMacro(<<"No vtkMathTextUtilities available! Enable the "
                    "vtkMatplotlib module to fix this.");
    return;
  }

  if (!ok)
  {
    return;
  }

  double origin[3] = {apoint[0], apoint[1], 0.f};
  double rotateAngle = this->TextProp->GetOrientation();
  double dcolor[3];
  this->TextProp->GetColor(dcolor);
  unsigned char color[4];
  color[0] = static_cast<unsigned char>(dcolor[0]*255);
  color[1] = static_cast<unsigned char>(dcolor[1]*255);
  color[2] = static_cast<unsigned char>(dcolor[2]*255);
  color[3] = static_cast<unsigned char>(this->TextProp->GetOpacity()*255);

  this->TransformPath(path.GetPointer());

  vtkGL2PSUtilities::DrawPath(path.GetPointer(), origin, origin, color, NULL,
                              rotateAngle, -1.f,
                              ("Pathified string: " + string).c_str());
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::ApplyPen(vtkPen *pen)
{
  this->Superclass::ApplyPen(pen);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetPointSize(float size)
{
  glPointSize(size);
  vtkOpenGLGL2PSHelper::SetPointSize(size);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetLineWidth(float width)
{
  glLineWidth(width);
  vtkOpenGLGL2PSHelper::SetLineWidth(width);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetLineType(int type)
{
  // Must call gl2psEnable(GL_LINE_STIPPLE) after glLineStipple. Let the
  // superclass handle setting the stipple type:
  this->Superclass::SetLineType(type);
  if (type == vtkPen::SOLID_LINE)
  {
    vtkOpenGLGL2PSHelper::DisableStipple();
  }
  else
  {
    vtkOpenGLGL2PSHelper::EnableStipple();
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawCrossMarkers(bool highlight, float *points,
                                               int n, unsigned char *colors,
                                               int nc_comps)
{
  float oldWidth = this->Pen->GetWidth();
  unsigned char oldColor[4];
  this->Pen->GetColor(oldColor);
  int oldLineType = this->Pen->GetLineType();

  float halfWidth = oldWidth * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  if (highlight)
  {
    this->Pen->SetWidth(1.5);
  }
  else
  {
    this->Pen->SetWidth(0.5);
  }
  this->Pen->SetLineType(vtkPen::SOLID_LINE);

  float curLine[4];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float *point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          VTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          vtkErrorMacro(<<"Invalid number of color components: " << nc_comps);
          break;
      }

      this->Pen->SetColor(color);
    }

    // The first line of the cross:
    curLine[0] = point[0] + deltaX;
    curLine[1] = point[1] + deltaY;
    curLine[2] = point[0] - deltaX;
    curLine[3] = point[1] - deltaY;
    this->DrawPoly(curLine, 2);

    // And the second:
    curLine[0] = point[0] + deltaX;
    curLine[1] = point[1] - deltaY;
    curLine[2] = point[0] - deltaX;
    curLine[3] = point[1] + deltaY;
    this->DrawPoly(curLine, 2);
  }

  this->Pen->SetWidth(oldWidth);
  this->Pen->SetColor(oldColor);
  this->Pen->SetLineType(oldLineType);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPlusMarkers(bool highlight, float *points,
                                              int n, unsigned char *colors,
                                              int nc_comps)
{
  float oldWidth = this->Pen->GetWidth();
  unsigned char oldColor[4];
  this->Pen->GetColor(oldColor);
  int oldLineType = this->Pen->GetLineType();

  float halfWidth = oldWidth * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  if (highlight)
  {
    this->Pen->SetWidth(1.5);
  }
  else
  {
    this->Pen->SetWidth(0.5);
  }
  this->Pen->SetLineType(vtkPen::SOLID_LINE);

  float curLine[4];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float *point = points + (i * 2);
    if  (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          VTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          vtkErrorMacro(<<"Invalid number of color components: " << nc_comps);
          break;
      }

      this->Pen->SetColor(color);
    }

    // The first line of the plus:
    curLine[0] = point[0] - deltaX;
    curLine[1] = point[1];
    curLine[2] = point[0] + deltaX;
    curLine[3] = point[1];
    this->DrawPoly(curLine, 2);

    // And the second:
    curLine[0] = point[0];
    curLine[1] = point[1] - deltaY;
    curLine[2] = point[0];
    curLine[3] = point[1] + deltaY;
    this->DrawPoly(curLine, 2);
  }

  this->Pen->SetWidth(oldWidth);
  this->Pen->SetColor(oldColor);
  this->Pen->SetLineType(oldLineType);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawSquareMarkers(bool /*highlight*/,
                                                float *points,
                                                int n, unsigned char *colors,
                                                int nc_comps)
{
  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  float halfWidth = this->GetPen()->GetWidth() * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  float quad[8];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float *point = points + (i * 2);
    if  (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          VTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          vtkErrorMacro(<<"Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    quad[0] = point[0] - deltaX;
    quad[1] = point[1] - deltaY;
    quad[2] = point[0] + deltaX;
    quad[3] = quad[1];
    quad[4] = quad[2];
    quad[5] = point[1] + deltaY;
    quad[6] = quad[0];
    quad[7] = quad[5];

    this->DrawQuad(quad,4);
  }

  this->Brush->SetColor(oldColor);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawCircleMarkers(bool /*highlight*/,
                                                float *points,
                                                int n, unsigned char *colors,
                                                int nc_comps)
{
  float radius = this->GetPen()->GetWidth() * 0.475;

  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float *point = points + (i * 2);
    if  (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          VTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          vtkErrorMacro(<<"Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    this->DrawEllipseWedge(point[0], point[1], radius, radius, 0, 0,
                           0, 360);
  }

  this->Brush->SetColor(oldColor);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawDiamondMarkers(bool /*highlight*/,
                                                 float *points,
                                                 int n, unsigned char *colors,
                                                 int nc_comps)
{
  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  float halfWidth = this->GetPen()->GetWidth() * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  float quad[8];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float *point = points + (i * 2);
    if  (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          VTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          vtkErrorMacro(<<"Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    quad[0] = point[0] - deltaX;
    quad[1] = point[1];
    quad[2] = point[0];
    quad[3] = point[1] - deltaY;
    quad[4] = point[0] + deltaX;
    quad[5] = point[1];
    quad[6] = point[0];
    quad[7] = point[1] + deltaY;

    this->DrawQuad(quad,4);
  }

  this->Brush->SetColor(oldColor);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::AddEllipseToPath(vtkPath *path, float x, float y,
                                               float rx, float ry, bool reverse)
{
  if (rx < 1e-5 || ry < 1e-5)
  {
    return;
  }

  // method based on http://www.tinaja.com/glib/ellipse4.pdf
  const static float MAGIC = (4.0/3.0) * (sqrt(2.0) - 1);

  if (!reverse)
  {
    path->InsertNextPoint(x - rx,      y,           0, vtkPath::MOVE_TO);
    path->InsertNextPoint(x - rx,      ry * MAGIC,  0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(-rx * MAGIC, y + ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x,           y + ry,      0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(rx * MAGIC,  y + ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx,      ry * MAGIC,  0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx,      y,           0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(x + rx,      -ry * MAGIC, 0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(rx * MAGIC,  y - ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x,           y - ry,      0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(-rx * MAGIC, y - ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx,      -ry * MAGIC, 0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx,      y,           0, vtkPath::CUBIC_CURVE);
  }
  else
  {
    path->InsertNextPoint(x - rx,      y,           0, vtkPath::MOVE_TO);
    path->InsertNextPoint(x - rx,      -ry * MAGIC, 0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(-rx * MAGIC, y - ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x,           y - ry,      0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(rx * MAGIC,  y - ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx,      -ry * MAGIC, 0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx,      y,           0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(x + rx,      ry * MAGIC,  0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(rx * MAGIC,  y + ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x,           y + ry,      0, vtkPath::CUBIC_CURVE);

    path->InsertNextPoint(-rx * MAGIC, y + ry,      0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx,      ry * MAGIC,  0, vtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx,      y,           0, vtkPath::CUBIC_CURVE);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::TransformPath(vtkPath *path) const
{
  // Transform the path with the modelview matrix:
  float modelview[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

  // Transform the 2D path.
  float newPoint[3] = {0, 0, 0};
  vtkPoints *points = path->GetPoints();
  for (vtkIdType i = 0; i < path->GetNumberOfPoints(); ++i)
  {
    double *point = points->GetPoint(i);
    newPoint[0] = modelview[0] * point[0] + modelview[4] * point[1]
        + modelview[12];
    newPoint[1] = modelview[1] * point[0] + modelview[5] * point[1]
        + modelview[13];
    points->SetPoint(i, newPoint);
  }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::TransformPoint(float &x, float &y) const
{
  float modelview[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

  float inX = x;
  float inY = y;
  x = modelview[0] * inX + modelview[4] * inY + modelview[12];
  y = modelview[1] * inX + modelview[5] * inY + modelview[13];
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::TransformSize(float &dx, float &dy) const
{
  float modelview[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

  dx /= modelview[0];
  dy /= modelview[5];
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
