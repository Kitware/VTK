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
#include "vtkOpenGLRenderWindow.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkUnicodeString.h"

#include "vtk_gl2ps.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGL2PSContextDevice2D)

//-----------------------------------------------------------------------------
vtkGL2PSContextDevice2D::vtkGL2PSContextDevice2D()
{
  this->StippleOn = false;
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
  if ((colors || this->Pen->GetColorObject().Alpha() != 0) &&
      this->Pen->GetLineType() != vtkPen::NO_PEN)
    {
    this->Superclass::DrawPoly(f, n, colors, nc_comps);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPoints(float *points, int n,
                                         unsigned char *colors, int nc_comps)
{
  if (colors || this->Pen->GetColorObject().Alpha() != 0)
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
  if (colors || this->Pen->GetColorObject().Alpha() != 0)
    {
    this->Superclass::DrawPointSprites(sprite, points, n, colors, nc_comps);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawQuad(float *points, int n)
{
  if (this->Brush->GetColorObject().Alpha() != 0)
    {
    this->Superclass::DrawQuad(points, n);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawQuadStrip(float *points, int n)
{
  if (this->Brush->GetColorObject().Alpha() != 0)
    {
    this->Superclass::DrawQuadStrip(points, n);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawPolygon(float *points, int n)
{
  if (this->Brush->GetColorObject().Alpha() != 0)
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
  if (this->Brush->GetColorObject().Alpha() != 0)
    {
    this->Superclass::DrawEllipseWedge(x, y, outRx, outRy, inRx, inRy,
                                       startAngle, stopAngle);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawEllipticArc(float x, float y,
                                              float rX, float rY,
                                              float startAngle, float stopAngle)
{
  if (this->Brush->GetColorObject().Alpha() != 0)
    {
    this->Superclass::DrawEllipticArc(x, y, rX, rY, startAngle, stopAngle);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawString(float *point,
                                         const vtkStdString &string)
{
  double p[3] = {point[0], point[1], 0.f};
  vtkGL2PSUtilities::DrawString(string.c_str(), this->TextProp, p);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::DrawString(float *point,
                                         const vtkUnicodeString &string)
{
  double p[3] = {point[0], point[1], 0.f};
  vtkGL2PSUtilities::DrawString(string.utf8_str(), this->TextProp, p);
}

void vtkGL2PSContextDevice2D::DrawMathTextString(float apoint[],
                                                 const vtkStdString &string)
{
  vtkNew<vtkPath> path;
  bool ok;
  if (vtkMathTextUtilities::GetInstance())
    {
    ok = vtkMathTextUtilities::GetInstance()->StringToPath(string.c_str(),
                                                           path.GetPointer(),
                                                           this->TextProp);
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
  double scale[2] = {1.0, 1.0};
  double rotateAngle = this->TextProp->GetOrientation();
  int *renWinSize = this->RenderWindow->GetSize();
  double windowSize[2];
  windowSize[0] = static_cast<double>(renWinSize[0]);
  windowSize[1] = static_cast<double>(renWinSize[1]);
  double dcolor[3];
  this->TextProp->GetColor(dcolor);
  unsigned char color[3];
  color[0] = static_cast<unsigned char>(dcolor[0]*255);
  color[1] = static_cast<unsigned char>(dcolor[1]*255);
  color[2] = static_cast<unsigned char>(dcolor[2]*255);

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

  vtkGL2PSUtilities::DrawPath(path.GetPointer(), origin, windowSize, origin,
                              scale, rotateAngle, color);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::ApplyPen(vtkPen *pen)
{
  this->Superclass::ApplyPen(pen);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetPointSize(float size)
{
  gl2psPointSize(size);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetLineWidth(float width)
{
  gl2psLineWidth(width);
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::SetLineType(int type)
{
  // Must call gl2psEnable(GL_LINE_STIPPLE) after glLineStipple. Let the
  // superclass handle setting the stipple type:
  this->Superclass::SetLineType(type);
  if (type == vtkPen::SOLID_LINE && this->StippleOn)
    {
    gl2psDisable(GL2PS_LINE_STIPPLE);
    }
  else if (!this->StippleOn)
    {
    gl2psEnable(GL2PS_LINE_STIPPLE);
    }
}

//-----------------------------------------------------------------------------
void vtkGL2PSContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StippleOn: " << this->StippleOn << endl;
}
