/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDFContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDFContextDevice2D.h"

#include "vtkAbstractMapper.h" // for VTK_SCALAR_MODE defines
#include "vtkBrush.h"
#include "vtkCellIterator.h"
#include "vtkCellTypes.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkImageBlend.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageFlip.h"
#include "vtkIntArray.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTransform.h"
#include "vtkUnicodeString.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVectorOperators.h"

#include <vtk_libharu.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{

static void GetPointBounds(
  float* points, int numPoints, HPDF_REAL bbox[4], const float radius = 0.f)
{
  bbox[0] = static_cast<HPDF_REAL>(points[0]);
  bbox[1] = static_cast<HPDF_REAL>(points[0]);
  bbox[2] = static_cast<HPDF_REAL>(points[1]);
  bbox[3] = static_cast<HPDF_REAL>(points[1]);

  for (int i = 1; i < numPoints; ++i)
  {
    bbox[0] = std::min(bbox[0], static_cast<HPDF_REAL>(points[i * 2]));
    bbox[1] = std::max(bbox[1], static_cast<HPDF_REAL>(points[i * 2]));
    bbox[2] = std::min(bbox[2], static_cast<HPDF_REAL>(points[i * 2 + 1]));
    bbox[3] = std::max(bbox[3], static_cast<HPDF_REAL>(points[i * 2 + 1]));
  }

  bbox[0] -= radius;
  bbox[1] += radius;
  bbox[2] -= radius;
  bbox[3] += radius;
}

static void PolygonToShading(
  float* points, int numPoints, unsigned char* colors, int nc_comps, HPDF_Shading shading)
{
  assert(numPoints >= 3);

  // First triangle
  for (int ptIdx = 0; ptIdx < 3; ++ptIdx)
  {
    const float* pt = points + ptIdx * 2;
    const unsigned char* color = colors + ptIdx * nc_comps;
    HPDF_Shading_AddVertexRGB(shading, HPDF_FREE_FORM_TRI_MESH_EDGEFLAG_NO_CONNECTION, pt[0], pt[1],
      color[0], color[1], color[2]);
  }

  // Fan-out additional verts
  for (int ptIdx = 3; ptIdx < numPoints; ++ptIdx)
  {
    const float* pt = points + ptIdx * 2;
    const unsigned char* color = colors + ptIdx * nc_comps;
    HPDF_Shading_AddVertexRGB(
      shading, HPDF_FREE_FORM_TRI_MESH_EDGEFLAG_AC, pt[0], pt[1], color[0], color[1], color[2]);
  }
}

static void LineSegmentToShading(const float p1[2], const unsigned char rgb1[3], const float p2[2],
  const unsigned char rgb2[3], float radius, HPDF_Shading shading)
{
  float pDy = p2[1] - p1[1];
  float pDx = p2[0] - p1[0];
  float nDx = -pDy;
  float nDy = pDx;

  if (nDx == 0.f && nDy == 0.f)
  {
    return; // Points are coincident. Avoid division by zero below:
  }

  float tmpInvNorm = 1.f / std::sqrt(nDx * nDx + nDy * nDy);
  nDx *= tmpInvNorm * radius;
  nDy *= tmpInvNorm * radius;

  float quad[8] = { p1[0] + nDx, p1[1] + nDy, p1[0] - nDx, p1[1] - nDy, p2[0] - nDx, p2[1] - nDy,
    p2[0] + nDx, p2[1] + nDy };
  unsigned char color[12] = { rgb1[0], rgb1[1], rgb1[2], rgb1[0], rgb1[1], rgb1[2], rgb2[0],
    rgb2[1], rgb2[2], rgb2[0], rgb2[1], rgb2[2] };
  PolygonToShading(quad, 4, color, 3, shading);
}

static void PolyLineToShading(const float* points, int numPoints, const unsigned char* color,
  int nc_comps, float radius, HPDF_Shading shading)
{
  for (int i = 0; i < numPoints - 1; ++i)
  {
    const int n = i + 1;
    LineSegmentToShading(
      points + 2 * i, color + nc_comps * i, points + 2 * n, color + nc_comps * n, radius, shading);
  }
}

std::pair<double, double> GetScaleFactor(vtkMatrix3x3* mat)
{
  auto sign = [](double x) -> double { return x >= 0. ? 1. : -1; };

  auto a = mat->GetData()[0];
  auto b = mat->GetData()[1];
  auto c = mat->GetData()[3];
  auto d = mat->GetData()[4];

  double sx = sign(a) * std::sqrt(a * a + b * b);
  double sy = sign(d) * std::sqrt(c * c + d * d);

  return std::make_pair(sx, sy);
}

} // end anon namespace

// Need to be able to use vtkColor3f in a std::map. Must be outside of the anon
// namespace to work.
static bool operator<(const vtkColor3f& a, const vtkColor3f& b)
{
  for (int i = 0; i < 3; ++i)
  {
    if (a[i] < b[i])
    {
      return true;
    }
    else if (a[i] > b[i])
    {
      return false;
    }
  }
  return false;
}

struct TextHelper
{
  struct Line
  {
    Line(std::string&& str, HPDF_REAL width)
      : String(std::move(str))
      , Width(width)
    {
    }

    Line(const std::string& str, HPDF_REAL width)
      : String(str)
      , Width(width)
    {
    }

    std::string String;
    HPDF_REAL Width;
  };

  HPDF_Doc Document;
  HPDF_Page Page;
  vtkTextProperty* TextProp;
  HPDF_Font Font;
  const std::string& String;
  vtkMatrix3x3* Transform;
  double ScaleX;
  double ScaleY;

  HPDF_REAL FontSize;
  HPDF_Box FontBBox;
  HPDF_REAL BBoxWidth;
  HPDF_REAL BBoxHeight;
  HPDF_REAL Theta;
  HPDF_REAL SineTheta;
  HPDF_REAL CosineTheta;
  HPDF_REAL LineHeight;
  HPDF_REAL Leading;
  HPDF_REAL Ascent;
  HPDF_REAL Descent;
  std::vector<Line> Lines;
  bool Valid;

  TextHelper(
    HPDF_Doc doc, HPDF_Page page, vtkTextProperty* tprop, const std::string& str, vtkMatrix3x3* mat)
    : Document{ doc }
    , Page{ page }
    , TextProp{ tprop }
    , Font{ nullptr }
    , String{ str }
    , Transform(mat)
    , ScaleX{ 0 }
    , ScaleY{ 0 }
    , FontSize{ 0.f }
    , BBoxWidth{ 0.f }
    , BBoxHeight{ 0.f }
    , Theta{ vtkMath::RadiansFromDegrees(static_cast<float>(tprop->GetOrientation())) }
    , SineTheta(std::sin(this->Theta))
    , CosineTheta(std::cos(this->Theta))
    , LineHeight{ 0.f }
    , Leading{ 0.f }
    , Ascent{ 0.f }
    , Descent{ 0.f }
    , Valid{ false }
  {
    std::tie(this->ScaleX, this->ScaleY) = GetScaleFactor(this->Transform);

    if (this->LoadFont() && this->SplitStrings() && this->ComputeBBox())
    {
      this->Valid = true;
    }
  }

  void DrawText(const float ptIn[2]) const
  {
    assert(this->Valid);

    // Copy point since we'll modify it:
    std::array<float, 2> pt = { { ptIn[0], ptIn[1] } };

    this->JustifyStartPoint(pt.data());

    // Prepare text state
    HPDF_Page_BeginText(this->Page);
    HPDF_Page_SetFontAndSize(this->Page, this->Font, this->FontSize);
    HPDF_Page_SetTextRenderingMode(this->Page, HPDF_FILL);
    HPDF_Page_SetTextLeading(this->Page, this->Leading);

    // Initialize text matrix
    HPDF_Page_SetTextMatrix(this->Page, this->CosineTheta, this->SineTheta, -this->SineTheta,
      this->CosineTheta, pt[0], pt[1]);

    // Draw lines:
    switch (this->TextProp->GetJustification())
    {
      default:
      case VTK_TEXT_LEFT:
        this->PrintLeftJustifiedText();
        break;

      case VTK_TEXT_CENTERED:
        this->PrintCenterJustifiedText();
        break;

      case VTK_TEXT_RIGHT:
        this->PrintRightJustifiedText();
        break;
    }

    HPDF_Page_EndText(this->Page);
  }

private:
  bool LoadFont()
  {
    int family = this->TextProp->GetFontFamily();
    if (family == VTK_FONT_FILE)
    {
      const char* fontName =
        HPDF_LoadTTFontFromFile(this->Document, this->TextProp->GetFontFile(), true);
      this->Font = HPDF_GetFont(this->Document, fontName, "StandardEncoding");
    }
    else
    {
      std::ostringstream fontStr;
      bool isBold = this->TextProp->GetBold() != 0;
      bool isItalic = this->TextProp->GetItalic() != 0;

      switch (family)
      {
        case VTK_ARIAL:
          fontStr << "Helvetica";
          if (isBold || isItalic)
          {
            fontStr << "-";
          }
          if (isBold)
          {
            fontStr << "Bold";
          }
          if (isItalic)
          {
            fontStr << "Oblique";
          }
          break;

        case VTK_COURIER:
          fontStr << "Courier";
          if (isBold || isItalic)
          {
            fontStr << "-";
          }
          if (isBold)
          {
            fontStr << "Bold";
          }
          if (isItalic)
          {
            fontStr << "Oblique";
          }
          break;

        case VTK_TIMES:
          fontStr << "Times-";
          if (isBold && isItalic)
          {
            fontStr << "BoldItalic";
          }
          else if (isBold)
          {
            fontStr << "Bold";
          }
          else if (isItalic)
          {
            fontStr << "Italic";
          }
          else
          {
            fontStr << "Roman";
          }
          break;

        default:
          // Garbage in, garbage out:
          vtkGenericWarningMacro("Unknown font code (" << family << ")");
          return false;
      }

      this->Font = HPDF_GetFont(this->Document, fontStr.str().c_str(), "StandardEncoding");
    }

    if (!this->Font)
    {
      vtkGenericWarningMacro("Error preparing libharu font object.");
      return false;
    }

    // Reduce the font size by the current y scale factor:
    this->FontSize = static_cast<HPDF_REAL>(this->TextProp->GetFontSize());
    this->FontSize /= this->ScaleY;

    // Had to dig to find this info, so commenting it here:
    // The font's bbox is the box containing "all glyphs if placed with their
    // origins coincident. It is independent of fontsize.
    // In libharu, the textHeight is computed as:
    // (bbox.top - bbox.bottom) / 1000 * fontSize
    // In VTK, the default leading is:
    // (textHeight) * tprop->LineSpacing
    // From this, we can compute the leading needed for libharu:
    HPDF_REAL fontScale = this->FontSize / 1000.f;
    this->FontBBox = HPDF_Font_GetBBox(this->Font);
    this->LineHeight = (this->FontBBox.top - this->FontBBox.bottom) * fontScale;
    this->Leading = this->LineHeight * this->TextProp->GetLineSpacing();

    this->Ascent = HPDF_Font_GetAscent(this->Font) * fontScale;
    this->Descent = HPDF_Font_GetDescent(this->Font) * fontScale;

    return true;
  }

  HPDF_REAL ComputeLineWidth(const std::string& str)
  {
    HPDF_TextWidth widthAttr = HPDF_Font_TextWidth(this->Font,
      reinterpret_cast<const HPDF_BYTE*>(str.c_str()), static_cast<HPDF_UINT>(str.size()));

    HPDF_REAL wordSpace = HPDF_Page_GetWordSpace(this->Page);
    HPDF_REAL charSpace = HPDF_Page_GetCharSpace(this->Page);

    return (wordSpace * widthAttr.numwords + charSpace * widthAttr.numchars +
      widthAttr.width * this->FontSize / 1000);
  }

  bool SplitStrings()
  {
    this->BBoxWidth = 0;

    std::string::const_iterator it = this->String.begin();
    std::string::const_iterator end = this->String.end();
    std::string::const_iterator itEnd = std::find(it, end, '\n');

    while (itEnd != end)
    {
      std::string tmp{ it, itEnd };
      HPDF_REAL width = this->ComputeLineWidth(tmp);
      this->BBoxWidth = std::max(this->BBoxWidth, width);
      this->Lines.emplace_back(std::move(tmp), width);

      it = itEnd;
      std::advance(it, 1);
      itEnd = std::find(it, end, '\n');
    }

    // Last line:
    {
      std::string tmp{ it, itEnd };
      HPDF_REAL width = this->ComputeLineWidth(tmp);
      if (width > 0) // Skip empty trailing lines:
      {
        this->BBoxWidth = std::max(this->BBoxWidth, width);
        this->Lines.emplace_back(std::move(tmp), width);
      }
    }
    return true;
  }

  bool ComputeBBox()
  {
    std::size_t nLines = this->Lines.size();
    switch (nLines)
    {
      case 0:
        this->BBoxHeight = 0;
        break;

      case 1:
        this->BBoxHeight = this->Ascent;
        break;

      default:
        this->BBoxHeight = this->LineHeight + this->Leading * (nLines - 1);
        break;
    }

    return true;
  }

  // Move the baseline of the first line to the appropriate location given
  // the justification parameters
  void JustifyStartPoint(float pt[2]) const
  {
    std::array<float, 2> offset = { { 0.f, -this->Ascent } };

    switch (this->TextProp->GetJustification())
    {
      default:
      case VTK_TEXT_LEFT:
        break;

      case VTK_TEXT_CENTERED:
        offset[0] -= this->BBoxWidth * 0.5f;
        break;

      case VTK_TEXT_RIGHT:
        offset[0] -= this->BBoxWidth;
        break;
    }

    switch (this->TextProp->GetVerticalJustification())
    {
      case VTK_TEXT_BOTTOM:
        offset[1] += this->BBoxHeight;
        break;

      case VTK_TEXT_CENTERED:
        offset[1] += this->BBoxHeight * 0.5f;
        break;

      default:
      case VTK_TEXT_TOP:
        break;
    }

    // Account for tprop rotation:
    std::array<float, 2> tmp = { {
      offset[0] * this->CosineTheta - offset[1] * this->SineTheta,
      offset[0] * this->SineTheta + offset[1] * this->CosineTheta,
    } };

    pt[0] += tmp[0];
    pt[1] += tmp[1];
  }

  void PrintLeftJustifiedText() const
  {
    for (const auto& line : this->Lines)
    {
      HPDF_Page_ShowText(this->Page, line.String.c_str());
      HPDF_Page_MoveToNextLine(this->Page);
    }
  }

  void PrintCenterJustifiedText() const
  {
    HPDF_REAL currentOffset = 0; // for centering
    for (size_t i = 0; i < this->Lines.size(); ++i)
    {
      const Line& line = this->Lines[i];

      if (i == 0)
      { // Center the first line:
        currentOffset = (this->BBoxWidth - line.Width) * 0.5f;
        HPDF_Page_MoveTextPos(this->Page, currentOffset, 0);
      }
      else
      {
        // This line's offset:
        HPDF_REAL lineOffset = (this->BBoxWidth - line.Width) * 0.5;

        // The incremental change to effect this lines offset relative to the
        // current offset:
        HPDF_REAL incrOffset = lineOffset - currentOffset;

        // Center current line and advance to new line:
        HPDF_Page_MoveTextPos(this->Page, incrOffset, -this->Leading);

        // Update for next iteration:
        currentOffset = lineOffset;
      }

      HPDF_Page_ShowText(this->Page, line.String.c_str());
    }
  }

  void PrintRightJustifiedText() const
  {
    HPDF_REAL currentOffset = 0; // for centering
    for (size_t i = 0; i < this->Lines.size(); ++i)
    {
      const Line& line = this->Lines[i];

      if (i == 0)
      { // Right justify the first line:
        currentOffset = this->BBoxWidth - line.Width;
        HPDF_Page_MoveTextPos(this->Page, currentOffset, 0);
      }
      else
      {
        // This line's offset:
        HPDF_REAL lineOffset = this->BBoxWidth - line.Width;

        // The incremental change to effect this lines offset relative to the
        // current offset:
        HPDF_REAL incrOffset = lineOffset - currentOffset;

        // Center current line and advance to new line:
        HPDF_Page_MoveTextPos(this->Page, incrOffset, -this->Leading);

        // Update for next iteration:
        currentOffset = lineOffset;
      }

      HPDF_Page_ShowText(this->Page, line.String.c_str());
    }
  }
};

//------------------------------------------------------------------------------
// vtkPDFContextDevice2D::Details
//------------------------------------------------------------------------------

struct vtkPDFContextDevice2D::Details
{
  Details()
    : Page(nullptr)
  {
  }

  HPDF_Doc Document;
  HPDF_Page Page;

  std::map<unsigned char, HPDF_ExtGState> AlphaGStateMap;

  float computeWorldRadius(const float x1, const float y1, const float x2, const float y2,
    vtkMatrix4x4* matrix, const float penWidth)
  {
    // Attempt to compute the radius in world space.  For this computation
    // we make the assumption that the following expression holds true:
    //
    //     worldDiagonalLength       worldRadius
    //    ---------------------  =  -------------
    //     screenDiagonalLength      screenRadius
    //
    // Then we further assume that this->Matrix can take the points of our
    // polydata bounding box and transform them into top-level pixel coords
    // of our render window.
    double transMat[9];
    double* mat = matrix->GetData();
    vtkPDFContextDevice2D::Matrix4ToMatrix3(mat, transMat);

    const float boundPt1[3] = { static_cast<float>(x1), static_cast<float>(y1), 1.0f };

    const float boundPt2[3] = { static_cast<float>(x2), static_cast<float>(y2), 1.0f };

    float boundPt1Px[3];
    float boundPt2Px[3];

    vtkMatrix3x3::MultiplyPoint(transMat, boundPt1, boundPt1Px);
    vtkMatrix3x3::MultiplyPoint(transMat, boundPt2, boundPt2Px);

    // Compute the length of the bounding box diagonal in world space
    const float wdx = boundPt1[0] - boundPt2[0];
    const float wdy = boundPt1[1] - boundPt2[1];
    const float worldDiag = std::sqrt(wdx * wdx + wdy * wdy);

    // Do the same as above, but in screen space
    const float sdx = boundPt1Px[0] - boundPt2Px[0];
    const float sdy = boundPt1Px[1] - boundPt2Px[1];
    const float screenDiag = std::sqrt(sdx * sdx + sdy * sdy);

    // Pen width is twice the radius in screen space, so get that in world space
    const float worldPenWidth = penWidth * (worldDiag / screenDiag);

    return worldPenWidth * 0.5f;
  }
};

//------------------------------------------------------------------------------
// vtkPDFContextDevice2D
//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkPDFContextDevice2D);
vtkCxxSetObjectMacro(vtkPDFContextDevice2D, Renderer, vtkRenderer);

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetHaruObjects(void* doc, void* page)
{
  if (page && doc)
  {
    this->Impl->Document = *static_cast<HPDF_Doc*>(doc);
    this->Impl->Page = *static_cast<HPDF_Page*>(page);
  }
  else
  {
    this->Impl->Document = nullptr;
    this->Impl->Page = nullptr;
  }
  this->Impl->AlphaGStateMap.clear();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPoly(float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(nc_comps == 0 || colors != nullptr);
  assert(n > 0);
  assert(points != nullptr);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
  {
    return;
  }

  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  this->PushGraphicsState();
  this->ApplyPenState();

  if (colors == nullptr)
  {
    HPDF_Page_MoveTo(this->Impl->Page, points[0], points[1]);
    for (int i = 1; i < n; ++i)
    {
      HPDF_Page_LineTo(this->Impl->Page, points[i * 2], points[i * 2 + 1]);
    }
    this->Stroke();
  }
  else
  {
    vtkVector2f width = this->GetUnscaledPenWidth() * 0.5f;
    const float radius = std::max(width[0], width[1]) * 0.5f;
    HPDF_REAL bbox[4];
    GetPointBounds(points, n, bbox, radius);

    HPDF_Shading shading = HPDF_Shading_New(this->Impl->Document,
      HPDF_SHADING_FREE_FORM_TRIANGLE_MESH, HPDF_CS_DEVICE_RGB, bbox[0], bbox[1], bbox[2], bbox[3]);
    PolyLineToShading(points, n, colors, nc_comps, radius, shading);
    HPDF_Page_SetShading(this->Impl->Page, shading);
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawLines(float* f, int n, unsigned char* colors, int nc_comps)
{
  assert(nc_comps == 0 || colors != nullptr);
  assert(n > 0);
  assert(f != nullptr);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
  {
    return;
  }

  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  this->PushGraphicsState();

  if (colors == nullptr)
  {
    this->ApplyPenState();

    for (int i = 0; i < n / 2; ++i)
    {
      HPDF_Page_MoveTo(this->Impl->Page, f[i * 4], f[i * 4 + 1]);
      HPDF_Page_LineTo(this->Impl->Page, f[i * 4 + 2], f[i * 4 + 3]);
    }
    this->Stroke();
  }
  else
  {
    vtkVector2f width = this->GetUnscaledPenWidth();
    const float radius = std::max(width[0], width[1]) * 0.5f;
    HPDF_REAL bbox[4];
    GetPointBounds(f, n, bbox, radius);

    HPDF_Shading shading = HPDF_Shading_New(this->Impl->Document,
      HPDF_SHADING_FREE_FORM_TRIANGLE_MESH, HPDF_CS_DEVICE_RGB, bbox[0], bbox[1], bbox[2], bbox[3]);

    for (int i = 0; i < n / 2; ++i)
    {
      const float* p1 = f + i * 4;
      const unsigned char* rgb1 = colors + i * 2 * nc_comps;
      const float* p2 = f + i * 6;
      const unsigned char* rgb2 = colors + (i * 2 + 1) * nc_comps;
      LineSegmentToShading(p1, rgb1, p2, rgb2, radius, shading);
    }

    HPDF_Page_SetShading(this->Impl->Page, shading);
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPoints(float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(nc_comps == 0 || colors != nullptr);
  assert(n > 0);
  assert(points != nullptr);

  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  this->PushGraphicsState();
  this->ApplyPenStateAsFill();

  const vtkVector2f width = this->GetUnscaledPenWidth();
  const vtkVector2f halfWidth = width * 0.5f;

  for (int i = 0; i < n; ++i)
  {
    if (nc_comps > 0)
    {
      this->ApplyFillColor(colors + i * nc_comps, nc_comps);
    }
    float originX = points[i * 2] - halfWidth[0];
    float originY = points[i * 2 + 1] - halfWidth[1];
    HPDF_Page_Rectangle(this->Impl->Page, originX, originY, width[0], width[1]);
    this->Fill();
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPointSprites(
  vtkImageData* spriteIn, float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(points);
  assert(n > 0);
  assert(nc_comps == 0 || colors);
  assert(spriteIn);

  vtkImageData* rgb = this->PrepareImageData(spriteIn);
  if (!rgb)
  {
    vtkErrorMacro("Unsupported point sprite format.");
    return;
  }

  assert(rgb->GetScalarType() == VTK_UNSIGNED_CHAR);
  assert(rgb->GetNumberOfScalarComponents() == 3);

  int dims[3];
  rgb->GetDimensions(dims);
  vtkIdType numPoints = rgb->GetNumberOfPoints();
  unsigned char* bufIn = static_cast<unsigned char*>(rgb->GetScalarPointer());

  const float sizeFactor = this->Pen->GetWidth() / static_cast<float>(std::max(dims[0], dims[1]));
  const float width = dims[0] * sizeFactor;
  const float height = dims[1] * sizeFactor;
  const float halfWidth = width * 0.5f;
  const float halfHeight = height * 0.5f;

  this->PushGraphicsState();

  // The HPDF_Images are cleaned up by libharu when we finish writing the file.
  typedef std::map<vtkColor3f, HPDF_Image> SpriteMap;
  SpriteMap spriteMap;

  for (int i = 0; i < n; ++i)
  {
    const float* p = points + 2 * i;

    vtkColor3f color;
    unsigned char alpha = 255;
    if (colors)
    {
      unsigned char* c = colors + nc_comps * i;
      switch (nc_comps)
      {
        case 3:
          color.Set(c[0] / 255.f, c[1] / 255.f, c[2] / 255.f);
          break;

        case 4:
          color.Set(c[0] / 255.f, c[1] / 255.f, c[2] / 255.f);
          alpha = c[3];
          break;

        default:
          vtkErrorMacro("Unsupported number of color components: " << nc_comps);
          continue;
      }
    }
    else
    {
      vtkColor4ub penColor = this->Pen->GetColorObject();
      color.Set(penColor[0] / 255.f, penColor[1] / 255.f, penColor[2] / 255.f);
      alpha = penColor[3];
    }

    HPDF_Image sprite;

    SpriteMap::iterator it = spriteMap.find(color);
    if (it != spriteMap.end())
    {
      sprite = it->second;
    }
    else
    {
      std::vector<unsigned char> coloredBuf;
      coloredBuf.reserve(numPoints * 3);
      // Using int since we're iterating to j < 0 (and vtkIdType is unsigned).
      // It's very unlikely that numPoints will be larger than INT_MAX, but
      // we'll check anyway. This throws a warning (taut-compare) when
      // vtkIdType is a 32-bit integer, so we disable it in that case.
#ifdef VTK_USE_64BIT_IDS
      if (numPoints > static_cast<vtkIdType>(VTK_INT_MAX))
      {
        vtkErrorMacro("FIXME: Image data too large for indexing with int.");
        this->PopGraphicsState();
        rgb->UnRegister(this);
        return;
      }
#endif // VTK_USE_64BIT_IDS
      for (int j = static_cast<int>(numPoints) - 1; j >= 0; --j)
      {
        unsigned char* pointColor = bufIn + 3 * j;
        // This is what the OpenGL implementation does:
        coloredBuf.push_back(pointColor[0] * color[0]);
        coloredBuf.push_back(pointColor[1] * color[1]);
        coloredBuf.push_back(pointColor[2] * color[2]);
      }

      sprite = HPDF_LoadRawImageFromMem(
        this->Impl->Document, coloredBuf.data(), dims[0], dims[1], HPDF_CS_DEVICE_RGB, 8);
      spriteMap.insert(std::make_pair(color, sprite));
    }

    this->ApplyFillAlpha(alpha);
    HPDF_Page_DrawImage(
      this->Impl->Page, sprite, p[0] - halfWidth, p[1] - halfHeight, width, height);
  }

  rgb->UnRegister(this);

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawMarkers(
  int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(points);
  assert(n > 0);
  assert(nc_comps == 0 || colors);

  this->PushGraphicsState();

  switch (shape)
  {
    case VTK_MARKER_CROSS:
      this->DrawCrossMarkers(highlight, points, n, colors, nc_comps);
      break;

    default:
      // default is here for consistency with old impl -- defaults to plus for
      // unrecognized shapes.
      VTK_FALLTHROUGH;
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

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawQuad(float* p, int n)
{
  assert(n > 0);
  assert(p != nullptr);

  if (this->Brush->GetColorObject().GetAlpha() == 0 && this->Brush->GetTexture() == nullptr)
  {
    return;
  }

  this->PushGraphicsState();
  this->ApplyBrushState();
  this->RegisterTexturePoints(p, n);

  size_t numQuads = n / 4;
  for (size_t quad = 0; quad < numQuads; ++quad)
  {
    const size_t i = quad * 8; // (4 verts / quad) * (2 floats / vert)

    HPDF_Page_MoveTo(this->Impl->Page, p[i], p[i + 1]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 2], p[i + 3]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 4], p[i + 5]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 6], p[i + 7]);
    HPDF_Page_ClosePath(this->Impl->Page);
  }

  this->Fill();
  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawQuadStrip(float* p, int n)
{
  assert(n > 0);
  assert(p != nullptr);

  if (this->Brush->GetColorObject().GetAlpha() == 0 && this->Brush->GetTexture() == nullptr)
  {
    return;
  }

  this->PushGraphicsState();
  this->ApplyBrushState();
  this->RegisterTexturePoints(p, n);

  size_t numQuads = n / 2 - 1;
  for (size_t quad = 0; quad < numQuads; ++quad)
  {
    const size_t i = quad * 4;

    HPDF_Page_MoveTo(this->Impl->Page, p[i], p[i + 1]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 2], p[i + 3]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 4], p[i + 5]);
    HPDF_Page_LineTo(this->Impl->Page, p[i + 6], p[i + 7]);
    HPDF_Page_ClosePath(this->Impl->Page);
  }

  this->Fill();
  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPolygon(float* f, int n)
{
  assert(n > 0);
  assert(f != nullptr);

  if (this->Brush->GetColorObject().GetAlpha() == 0 && this->Brush->GetTexture() == nullptr)
  {
    return;
  }

  this->PushGraphicsState();
  this->ApplyBrushState();
  this->RegisterTexturePoints(f, n);

  HPDF_Page_MoveTo(this->Impl->Page, f[0], f[1]);

  for (int i = 1; i < n; ++i)
  {
    HPDF_Page_LineTo(this->Impl->Page, f[i * 2], f[i * 2 + 1]);
  }

  HPDF_Page_ClosePath(this->Impl->Page);

  this->Fill();

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawColoredPolygon(
  float* points, int numPoints, unsigned char* colors, int nc_comps)
{
  assert(numPoints > 0);
  assert(points != nullptr);

  // Just use the standard draw method if there is a texture or colors are not
  // specified:
  if (this->Brush->GetTexture() != nullptr || nc_comps == 0)
  {
    this->DrawPolygon(points, numPoints);
    return;
  }

  // If all of the points have the same color, use a more compact method to
  // draw the poly:
  bool sameColor = true;
  for (int i = 1; i < numPoints && sameColor; ++i)
  {
    sameColor = std::equal(colors, colors + nc_comps, colors + (i * nc_comps));
  }
  if (sameColor)
  {
    const vtkColor4ub oldBrush = this->Brush->GetColorObject();
    switch (nc_comps)
    {
      case 4:
        this->Brush->SetOpacity(colors[3]);
        VTK_FALLTHROUGH;
      case 3:
        this->Brush->SetColor(colors);
        break;

      default:
        vtkWarningMacro("Unsupported number of color components: " << nc_comps);
        return;
    }

    this->DrawPolygon(points, numPoints);
    this->Brush->SetColor(oldBrush);
    return;
  }

  this->PushGraphicsState();

  HPDF_REAL bbox[4];
  GetPointBounds(points, numPoints, bbox);

  HPDF_Shading shading = HPDF_Shading_New(this->Impl->Document,
    HPDF_SHADING_FREE_FORM_TRIANGLE_MESH, HPDF_CS_DEVICE_RGB, bbox[0], bbox[1], bbox[2], bbox[3]);

  PolygonToShading(points, numPoints, colors, nc_comps, shading);

  HPDF_Page_SetShading(this->Impl->Page, shading);

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx,
  float inRy, float startAngle, float stopAngle)
{
  assert("pre: positive_outRx" && outRx >= 0.0f);
  assert("pre: positive_outRy" && outRy >= 0.0f);
  assert("pre: positive_inRx" && inRx >= 0.0f);
  assert("pre: positive_inRy" && inRy >= 0.0f);
  assert("pre: ordered_rx" && inRx <= outRx);
  assert("pre: ordered_ry" && inRy <= outRy);

  this->PushGraphicsState();
  this->ApplyBrushState();

  // Register the bounds of the outer ellipse:
  float bounds[8] = { x - outRx, y - outRy, x - outRx, y + outRy, x + outRx, y + outRy, x + outRx,
    y - outRy };
  this->RegisterTexturePoints(bounds, 4);

  // If we're drawing a complete ellipse, just use the built-in ellipse call:
  if (std::fabs(stopAngle - startAngle) >= 360.f)
  {
    HPDF_Page_Ellipse(this->Impl->Page, x, y, outRx, outRy);
    if (inRx > 0.f || inRy > 0.f)
    {
      HPDF_Page_Ellipse(this->Impl->Page, x, y, inRx, inRy);
      this->FillEvenOdd();
    }
    else
    {
      this->Fill();
    }
  }
  // If we're drawing circles, use the built-in arc calls:
  else if (inRx == inRy && outRx == outRy)
  {
    // VTK  uses 0 degrees = East with CCW rotation, but
    // Haru uses 0 degrees = North with CW rotation. Adjust for this:
    float hStart = -(stopAngle - 90.f);
    float hStop = -(startAngle - 90.f);

    HPDF_Page_Arc(this->Impl->Page, x, y, outRx, hStart, hStop);
    if (inRx > 0.f)
    {
      HPDF_Page_Arc(this->Impl->Page, x, y, inRx, hStart, hStop);
      this->FillEvenOdd();
    }
    else
    {
      this->Fill();
    }
  }
  else
  {
    // Haru doesn't support drawing ellipses that have start/stop angles.
    // You can either do an ellipse or a circle with start/stop, but not both.
    // We if have to do both, we'll need to rasterize the path.
    this->DrawEllipticArcSegments(x, y, outRx, outRy, startAngle, stopAngle, true);
    if (inRx > 0 || inRy > 0.f)
    {
      this->DrawEllipticArcSegments(x, y, inRx, inRy, stopAngle, startAngle, false);
      HPDF_Page_ClosePath(this->Impl->Page);
      this->FillEvenOdd();
    }
    else
    {
      HPDF_Page_ClosePath(this->Impl->Page);
      this->Fill();
    }
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawEllipticArc(
  float x, float y, float rX, float rY, float startAngle, float stopAngle)
{
  assert("pre: positive_rX" && rX >= 0);
  assert("pre: positive_rY" && rY >= 0);

  this->PushGraphicsState();
  this->ApplyPenState();
  this->ApplyBrushState();

  // If we're drawing a complete ellipse, just use the built-in ellipse call:
  if (std::fabs(stopAngle - startAngle) >= 360.f)
  {
    HPDF_Page_Ellipse(this->Impl->Page, x, y, rX, rY);
    this->Fill(true);
  }
  // If we're drawing circles, use the built-in arc calls:
  else if (rX == rY)
  {
    // VTK  uses 0 degrees = East with CCW rotation, but
    // Haru uses 0 degrees = North with CW rotation. Adjust for this:
    float hStart = -(stopAngle - 90.f);
    float hStop = -(startAngle - 90.f);

    HPDF_Page_Arc(this->Impl->Page, x, y, rX, hStart, hStop);
    HPDF_Page_ClosePath(this->Impl->Page);
    this->Fill();
    HPDF_Page_Arc(this->Impl->Page, x, y, rX, hStart, hStop);
    this->Stroke();
  }
  else
  {
    // Haru doesn't support drawing ellipses that have start/stop angles.
    // You can either do an ellipse or a circle with start/stop, but not both.
    // We if have to do both, we'll need to rasterize the path.
    this->DrawEllipticArcSegments(x, y, rX, rY, startAngle, stopAngle, true);
    HPDF_Page_ClosePath(this->Impl->Page);
    this->Fill();
    this->DrawEllipticArcSegments(x, y, rX, rY, startAngle, stopAngle, true);
    this->Stroke();
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawString(float* point, const vtkStdString& string)
{
  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkGenericWarningMacro("vtkTextRenderer unavailable. Link to "
                           "vtkRenderingFreeType to get the default "
                           "implementation.");
    return;
  }

  int backend = tren->DetectBackend(string);

  this->PushGraphicsState();

  if (backend != vtkTextRenderer::MathText)
  {
    vtkNew<vtkMatrix3x3> mat;
    this->GetMatrix(mat);
    TextHelper helper(this->Impl->Document, this->Impl->Page, this->TextProp, string, mat);
    if (!helper.Valid)
    {
      vtkErrorMacro("Error preparing to draw string: " << string);
      this->PopGraphicsState();
      return;
    }

    this->ApplyTextPropertyState();

    helper.DrawText(point);
  }
  else
  {
    vtkNew<vtkPath> path;
    int dpi = this->Renderer->GetRenderWindow()->GetDPI();
    if (!tren->StringToPath(this->TextProp, string, path, dpi, backend))
    {
      vtkErrorMacro("Error generating path for MathText string '" << string << "'.");
      this->PopGraphicsState();
      return;
    }

    this->ApplyTextPropertyState();
    this->DrawPath(path, point[0], point[1]);
    this->FillEvenOdd();

    float bbox[4];
    this->ComputeStringBounds(string, bbox);
    HPDF_Page_SetRGBStroke(this->Impl->Page, 1, 0, 0);
    HPDF_Page_Rectangle(this->Impl->Page, bbox[0], bbox[1] - bbox[3], bbox[2], bbox[3]);
    HPDF_Page_Stroke(this->Impl->Page);
  }

  this->PopGraphicsState();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ComputeStringBounds(const vtkStdString& string, float bounds[4])
{
  vtkNew<vtkMatrix3x3> mat;
  this->GetMatrix(mat);
  TextHelper helper(this->Impl->Document, this->Impl->Page, this->TextProp, string, mat);
  if (!helper.Valid)
  {
    vtkErrorMacro("Error determining bounding box for string '" << string << "'.");
    std::fill(bounds, bounds + 4, 0.f);
    return;
  }

  bounds[0] = 0.f;
  bounds[1] = 0.f;
  bounds[2] = helper.BBoxWidth;
  bounds[3] = helper.BBoxHeight;
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawString(float* point, const vtkUnicodeString& string)
{
  this->DrawString(point, std::string(string.utf8_str()));
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ComputeStringBounds(const vtkUnicodeString& string, float bounds[4])
{
  this->ComputeStringBounds(string.utf8_str(), bounds);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ComputeJustifiedStringBounds(const char* string, float bounds[4])
{
  this->ComputeStringBounds(string, bounds);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawMathTextString(float* point, const vtkStdString& str)
{
  this->DrawString(point, str);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawImage(float p[2], float scale, vtkImageData* image)
{
  assert(p);
  assert(image);

  int dims[3];
  image->GetDimensions(dims);
  dims[0] *= scale;
  dims[1] *= scale;
  this->DrawImage(vtkRectf(p[0], p[1], dims[0], dims[1]), image);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawImage(const vtkRectf& pos, vtkImageData* image)
{
  assert(image);

  vtkImageData* rgb = this->PrepareImageData(image);
  if (!rgb)
  {
    return;
  }

  assert(rgb->GetScalarType() == VTK_UNSIGNED_CHAR);
  assert(rgb->GetNumberOfScalarComponents() == 3);

  int dims[3];
  rgb->GetDimensions(dims);
  const HPDF_BYTE* buf = static_cast<HPDF_BYTE*>(rgb->GetScalarPointer());

  HPDF_Image pdfImage =
    HPDF_LoadRawImageFromMem(this->Impl->Document, buf, dims[0], dims[1], HPDF_CS_DEVICE_RGB, 8);

  HPDF_Page_DrawImage(this->Impl->Page, pdfImage, pos[0], pos[1], pos[2], pos[3]);

  rgb->UnRegister(this);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetColor4(unsigned char[4])
{
  // This is how the OpenGL2 impl handles this...
  vtkErrorMacro("color cannot be set this way.");
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  this->Brush->SetTexture(image);
  this->Brush->SetTextureProperties(properties);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetPointSize(float size)
{
  this->Pen->SetWidth(size);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetLineWidth(float width)
{
  this->Pen->SetWidth(width);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPolyData(
  float p[2], float scale, vtkPolyData* polyData, vtkUnsignedCharArray* colors, int scalarMode)
{
  // Do nothing if the supported cell types do not exist in the dataset:
  vtkNew<vtkCellTypes> types;
  polyData->GetCellTypes(types);
  if (!types->IsType(VTK_LINE) && !types->IsType(VTK_POLY_LINE) && !types->IsType(VTK_TRIANGLE) &&
    !types->IsType(VTK_QUAD) && !types->IsType(VTK_POLYGON))
  {
    return;
  }

  double bounds[6];
  polyData->GetBounds(bounds);

  float radius = this->Impl->computeWorldRadius(
    bounds[0], bounds[2], bounds[1], bounds[3], this->Matrix->GetMatrix(), this->Pen->GetWidth());

  // Adjust bounds for transform, account for pen width:
  bounds[0] = (bounds[0] + p[0]) * scale - radius;
  bounds[1] = (bounds[1] + p[0]) * scale + radius;
  bounds[2] = (bounds[2] + p[1]) * scale - radius;
  bounds[3] = (bounds[3] + p[1]) * scale + radius;

  // Accumulate all triangles in a shading object:
  HPDF_Shading shading =
    HPDF_Shading_New(this->Impl->Document, HPDF_SHADING_FREE_FORM_TRIANGLE_MESH, HPDF_CS_DEVICE_RGB,
      bounds[0], bounds[1], bounds[2], bounds[3]);

  // Temporary buffers:
  std::vector<float> verts;
  std::vector<unsigned char> vertColors;

  vtkCellIterator* cell = polyData->NewCellIterator();
  cell->InitTraversal();
  for (; !cell->IsDoneWithTraversal(); cell->GoToNextCell())
  {
    // To match the original implementation on the OpenGL2 backend, we only
    // handle polygons and lines:
    int cellType = cell->GetCellType();
    switch (cellType)
    {
      case VTK_LINE:
      case VTK_POLY_LINE:
      case VTK_TRIANGLE:
      case VTK_QUAD:
      case VTK_POLYGON:
        break;

      default:
        continue;
    }

    // Allocate temporary arrays:
    vtkIdType numPoints = cell->GetNumberOfPoints();
    if (numPoints == 0)
    {
      continue;
    }
    verts.resize(static_cast<size_t>(numPoints) * 2);
    vertColors.resize(static_cast<size_t>(numPoints) * 4);

    vtkIdType cellId = cell->GetCellId();
    vtkIdList* pointIds = cell->GetPointIds();
    vtkPoints* points = cell->GetPoints();

    for (vtkIdType i = 0; i < numPoints; ++i)
    {
      const size_t vertsIdx = 2 * static_cast<size_t>(i);
      const size_t colorIdx = 4 * static_cast<size_t>(i);

      const double* point = points->GetPoint(i);
      verts[vertsIdx] = (static_cast<float>(point[0]) + p[0]) * scale;
      verts[vertsIdx + 1] = (static_cast<float>(point[1]) + p[1]) * scale;

      if (scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA)
      {
        colors->GetTypedTuple(pointIds->GetId(i), vertColors.data() + colorIdx);
      }
      else
      {
        colors->GetTypedTuple(cellId, vertColors.data() + colorIdx);
      }
    }

    if (cellType == VTK_LINE || cellType == VTK_POLY_LINE)
    {
      PolyLineToShading(verts.data(), numPoints, vertColors.data(), 4, radius, shading);
    }
    else
    {
      PolygonToShading(verts.data(), numPoints, vertColors.data(), 4, shading);
    }
  }
  cell->Delete();

  HPDF_Page_SetShading(this->Impl->Page, shading);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetLineType(int type)
{
  this->Pen->SetLineType(type);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetMatrix(vtkMatrix3x3* mat3)
{
  double mat4[16];
  vtkPDFContextDevice2D::Matrix3ToMatrix4(mat3, mat4);
  this->Matrix->SetMatrix(mat4);
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::GetMatrix(vtkMatrix3x3* mat3)
{
  vtkPDFContextDevice2D::Matrix4ToMatrix3(this->Matrix->GetMatrix()->GetData(), mat3);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::MultiplyMatrix(vtkMatrix3x3* mat3)
{
  double mat4[16];
  vtkPDFContextDevice2D::Matrix3ToMatrix4(mat3, mat4);
  this->Matrix->Concatenate(mat4);
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::PushMatrix()
{
  this->Matrix->Push();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::PopMatrix()
{
  this->Matrix->Pop();
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::SetClipping(int* x)
{
  this->ClipBox[0] = static_cast<HPDF_REAL>(x[0]);
  this->ClipBox[1] = static_cast<HPDF_REAL>(x[1]);
  this->ClipBox[2] = static_cast<HPDF_REAL>(x[2]);
  this->ClipBox[3] = static_cast<HPDF_REAL>(x[3]);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::EnableClipping(bool enable)
{
  if (enable)
  {
    this->PushGraphicsState();
    HPDF_Page_Rectangle(
      this->Impl->Page, this->ClipBox[0], this->ClipBox[1], this->ClipBox[2], this->ClipBox[3]);
    HPDF_Page_Clip(this->Impl->Page);
    // Prevent the clip path from being drawn:
    HPDF_Page_EndPath(this->Impl->Page);
  }
  else
  {
    this->PopGraphicsState();
  }
}

//------------------------------------------------------------------------------
vtkPDFContextDevice2D::vtkPDFContextDevice2D()
  : Impl(new Details)
  , Renderer(nullptr)
  , IsInTexturedFill(false)
{
  std::fill(this->ClipBox, this->ClipBox + 4, 0.f);
  std::fill(this->TextureBounds, this->TextureBounds + 4, 0.f);
}

//------------------------------------------------------------------------------
vtkPDFContextDevice2D::~vtkPDFContextDevice2D()
{
  this->SetRenderer(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::PushGraphicsState()
{
  HPDF_Page_GSave(this->Impl->Page);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::PopGraphicsState()
{
  HPDF_Page_GRestore(this->Impl->Page);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyPenState()
{
  const vtkVector2f width = this->GetUnscaledPenWidth();
  this->ApplyStrokeColor(this->Pen->GetColorObject().GetData(), 4);
  this->ApplyLineWidth(std::max(width[0], width[1]));
  this->ApplyLineType(this->Pen->GetLineType());
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyStrokeColor(unsigned char* color, int numComps)
{
  HPDF_Page_SetRGBStroke(this->Impl->Page, static_cast<HPDF_REAL>(color[0] / 255.0),
    static_cast<HPDF_REAL>(color[1] / 255.0), static_cast<HPDF_REAL>(color[2] / 255.0));

  unsigned char alpha = numComps > 3 ? color[3] : 255;
  auto gstateIter = this->Impl->AlphaGStateMap.find(alpha);
  if (gstateIter == this->Impl->AlphaGStateMap.end())
  {
    HPDF_ExtGState gstate = HPDF_CreateExtGState(this->Impl->Document);
    HPDF_ExtGState_SetAlphaFill(gstate, alpha / 255.f);
    auto tmp = this->Impl->AlphaGStateMap.insert(std::make_pair(alpha, gstate));
    gstateIter = tmp.first;
  }
  HPDF_ExtGState alphaState = gstateIter->second;
  HPDF_Page_SetExtGState(this->Impl->Page, alphaState);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyLineWidth(float width)
{
  HPDF_Page_SetLineWidth(this->Impl->Page, width);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyLineType(int type)
{
  // These match the OpenGL2 implementation:
  static const HPDF_UINT16 noPen[] = { 0, 10 };
  static const HPDF_UINT noPenLen = 2;

  static const HPDF_UINT16 dash[] = { 8 };
  static const HPDF_UINT dashLen = 1;

  static const HPDF_UINT16 dot[] = { 1, 7 };
  static const HPDF_UINT16 denseDot[] = { 1, 3 };
  static const HPDF_UINT dotLen = 2;

  static const HPDF_UINT16 dashDot[] = { 4, 6, 2, 4 };
  static const HPDF_UINT dashDotLen = 4;

  // This is dash-dot-dash, but eh. It matches the OpenGL2 0x1C47 pattern.
  static const HPDF_UINT16 dashDotDot[] = { 3, 3, 1, 3, 3, 3 };
  static const HPDF_UINT dashDotDotLen = 6;

  switch (type)
  {
    default:
      vtkErrorMacro("Unknown line type: " << type);
      VTK_FALLTHROUGH;

    case vtkPen::NO_PEN:
      HPDF_Page_SetDash(this->Impl->Page, noPen, noPenLen, 0);
      break;

    case vtkPen::SOLID_LINE:
      HPDF_Page_SetDash(this->Impl->Page, nullptr, 0, 0);
      break;

    case vtkPen::DASH_LINE:
      HPDF_Page_SetDash(this->Impl->Page, dash, dashLen, 0);
      break;

    case vtkPen::DOT_LINE:
      HPDF_Page_SetDash(this->Impl->Page, dot, dotLen, 0);
      break;

    case vtkPen::DASH_DOT_LINE:
      HPDF_Page_SetDash(this->Impl->Page, dashDot, dashDotLen, 0);
      break;

    case vtkPen::DASH_DOT_DOT_LINE:
      HPDF_Page_SetDash(this->Impl->Page, dashDotDot, dashDotDotLen, 0);
      break;

    case vtkPen::DENSE_DOT_LINE:
      HPDF_Page_SetDash(this->Impl->Page, denseDot, dotLen, 0);
      break;
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Stroke()
{
  HPDF_Page_Stroke(this->Impl->Page);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyPenStateAsFill()
{
  this->ApplyFillColor(this->Pen->GetColorObject().GetData(), 4);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyBrushState()
{
  this->ApplyFillColor(this->Brush->GetColorObject().GetData(), 4);

  if (this->Brush->GetTexture())
  {
    this->BeginClipPathForTexture();
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyTextPropertyState()
{
  unsigned char rgba[4] = { static_cast<unsigned char>(this->TextProp->GetColor()[0] * 255.),
    static_cast<unsigned char>(this->TextProp->GetColor()[1] * 255.),
    static_cast<unsigned char>(this->TextProp->GetColor()[2] * 255.),
    static_cast<unsigned char>(this->TextProp->GetOpacity() * 255.) };

  this->ApplyFillColor(rgba, 4);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyFillColor(unsigned char* color, int numComps)
{
  HPDF_Page_SetRGBFill(this->Impl->Page, static_cast<HPDF_REAL>(color[0] / 255.0),
    static_cast<HPDF_REAL>(color[1] / 255.0), static_cast<HPDF_REAL>(color[2] / 255.0));
  if (numComps > 3)
  {
    this->ApplyFillAlpha(color[3]);
  }
  else
  {
    this->ApplyFillAlpha(255);
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyFillAlpha(unsigned char alpha)
{
  auto gstateIter = this->Impl->AlphaGStateMap.find(alpha);
  if (gstateIter == this->Impl->AlphaGStateMap.end())
  {
    HPDF_ExtGState gstate = HPDF_CreateExtGState(this->Impl->Document);
    HPDF_ExtGState_SetAlphaFill(gstate, alpha / 255.f);
    auto tmp = this->Impl->AlphaGStateMap.insert(std::make_pair(alpha, gstate));
    gstateIter = tmp.first;
  }
  HPDF_ExtGState alphaState = gstateIter->second;
  HPDF_Page_SetExtGState(this->Impl->Page, alphaState);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Fill(bool stroke)
{
  if (this->IsInTexturedFill)
  {
    this->FillTexture();
    return;
  }

  if (stroke)
  {
    HPDF_Page_FillStroke(this->Impl->Page);
  }
  else
  {
    HPDF_Page_Fill(this->Impl->Page);
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::FillEvenOdd(bool stroke)
{
  if (this->IsInTexturedFill)
  {
    this->FillTexture();
    return;
  }

  if (stroke)
  {
    HPDF_Page_EofillStroke(this->Impl->Page);
  }
  else
  {
    HPDF_Page_Eofill(this->Impl->Page);
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::BeginClipPathForTexture()
{
  assert(!this->IsInTexturedFill);
  this->IsInTexturedFill = true;
  this->TextureBounds[0] = this->TextureBounds[2] = VTK_INT_MAX;
  this->TextureBounds[1] = this->TextureBounds[3] = VTK_INT_MIN;
  this->PushGraphicsState(); // so we can pop the clip path
  this->ApplyFillAlpha(255); // Match the OpenGL implementation
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::RegisterTexturePoints(float* data, int numPoints)
{
  if (this->IsInTexturedFill)
  {
    for (int i = 0; i < numPoints; ++i)
    {
      this->TextureBounds[0] = std::min(this->TextureBounds[0], data[2 * i]);
      this->TextureBounds[1] = std::max(this->TextureBounds[1], data[2 * i]);
      this->TextureBounds[2] = std::min(this->TextureBounds[2], data[2 * i + 1]);
      this->TextureBounds[3] = std::max(this->TextureBounds[3], data[2 * i + 1]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::FillTexture()
{
  assert(this->IsInTexturedFill);

  this->IsInTexturedFill = false;

  if (this->TextureBounds[0] == VTK_INT_MAX || this->TextureBounds[1] == VTK_INT_MIN ||
    this->TextureBounds[2] == VTK_INT_MAX || this->TextureBounds[3] == VTK_INT_MIN)
  { // No geometry to texture:
    this->PopGraphicsState();
    return;
  }

  // Use current path for clipping
  HPDF_Page_Clip(this->Impl->Page);
  HPDF_Page_EndPath(this->Impl->Page);

  // Prepare texture image
  vtkImageData* image = this->Brush->GetTexture();
  assert(image);

  vtkImageData* rgb = this->PrepareImageData(image);
  if (!rgb)
  {
    return;
  }

  assert(rgb->GetScalarType() == VTK_UNSIGNED_CHAR);
  assert(rgb->GetNumberOfScalarComponents() == 3);

  int dims[3];
  rgb->GetDimensions(dims);
  const HPDF_BYTE* buf = static_cast<HPDF_BYTE*>(rgb->GetScalarPointer());

  HPDF_Image pdfImage =
    HPDF_LoadRawImageFromMem(this->Impl->Document, buf, dims[0], dims[1], HPDF_CS_DEVICE_RGB, 8);

  const bool isTiled = ((this->Brush->GetTextureProperties() & vtkBrush::Repeat) != 0);

  // tile across TextureBounds if repeating
  if (isTiled)
  {
    float x = this->TextureBounds[0];
    while (x < this->TextureBounds[1])
    {
      float y = this->TextureBounds[2];
      while (y < this->TextureBounds[3])
      {
        HPDF_Page_DrawImage(this->Impl->Page, pdfImage, x, y, dims[0], dims[1]);
        y += dims[1];
      }
      x += dims[0];
    }
  }
  else
  { // stretch across texture bounds if stretched
    HPDF_Page_DrawImage(this->Impl->Page, pdfImage, this->TextureBounds[0], this->TextureBounds[2],
      this->TextureBounds[1] - this->TextureBounds[0],
      this->TextureBounds[3] - this->TextureBounds[2]);
  }

  rgb->UnRegister(this);
  this->PopGraphicsState(); // unset clip path
}

//------------------------------------------------------------------------------
vtkImageData* vtkPDFContextDevice2D::PrepareImageData(vtkImageData* in)
{
  int numComps = in->GetNumberOfScalarComponents();

  // We'll only handle RGB / RGBA:
  if (numComps != 3 && numComps != 4)
  {
    vtkWarningMacro("Images with " << numComps << " components not supported.");
    return nullptr;
  }

  // Need to convert scalar type?
  if (in->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkNew<vtkImageCast> cast;
    cast->SetInputData(in);
    cast->SetOutputScalarTypeToUnsignedChar();
    cast->Update();
    in = cast->GetOutput();
    in->Register(this);
  }
  else
  {
    in->Register(this); // Keep refcounts consistent
  }

  if (in->GetNumberOfScalarComponents() == 4)
  { // If RGBA, blend into brush color -- Haru doesn't support RGBA.

    vtkNew<vtkImageData> background;
    { // Fill the background image with brush color, saturate alpha
      std::array<unsigned char, 4> bgColor;
      this->Brush->GetColor(bgColor.data());
      bgColor[3] = 255; // Saturate alpha
      background->SetExtent(in->GetExtent());
      background->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
      auto* scalars = vtkUnsignedCharArray::SafeDownCast(background->GetPointData()->GetScalars());
      for (int comp = 0; comp < 4; ++comp)
      {
        scalars->FillComponent(comp, bgColor[comp]);
      }
    }

    // Blend the input image over the background color:
    vtkNew<vtkImageBlend> blender;
    blender->AddInputData(0, background);
    blender->AddInputData(0, in);
    in->UnRegister(this); // Remove ref++ from above
    blender->SetBlendModeToNormal();

    vtkNew<vtkImageExtractComponents> extract;
    extract->SetInputConnection(blender->GetOutputPort(0));
    extract->SetComponents(0, 1, 2);
    extract->Update();
    in = extract->GetOutput();
    in->Register(this);
  }

  // Finally, flip the image (Haru expects them this way)
  vtkNew<vtkImageFlip> flip;
  flip->SetInputData(in);
  in->UnRegister(this);
  flip->SetFilteredAxis(1); // y axis
  flip->Update();
  in = flip->GetOutput();
  in->Register(this);

  return in;
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawEllipticArcSegments(
  float x, float y, float rX, float rY, float startAngle, float stopAngle, bool startPath)
{
  // Adapted from OpenGL implementation:
  int numSegments = this->GetNumberOfArcIterations(rX, rY, startAngle, stopAngle);

  // step in radians:
  float step =
    vtkMath::RadiansFromDegrees(stopAngle - startAngle) / static_cast<float>(numSegments);

  float rstart = vtkMath::RadiansFromDegrees(startAngle);

  if (startPath)
  {
    HPDF_Page_MoveTo(this->Impl->Page, rX * std::cos(rstart) + x, rY * std::sin(rstart) + y);
  }
  else
  {
    HPDF_Page_LineTo(this->Impl->Page, rX * std::cos(rstart) + x, rY * std::sin(rstart) + y);
  }

  for (int i = 1; i <= numSegments; ++i)
  {
    float angle = rstart + i * step;
    HPDF_Page_LineTo(this->Impl->Page, rX * std::cos(angle) + x, rY * std::sin(angle) + y);
  }
}

//------------------------------------------------------------------------------
int vtkPDFContextDevice2D::GetNumberOfArcIterations(
  float rX, float rY, float startAngle, float stopAngle)
{
  // Copied from original OpenGL implementation:
  assert("pre: positive_rX" && rX >= 0.0f);
  assert("pre: positive_rY" && rY >= 0.0f);
  assert("pre: not_both_null" && (rX > 0.0 || rY > 0.0));

  // 1.0: pixel precision. 0.5 (subpixel precision, useful with multisampling)
  double error = 4.0; // experience shows 4.0 is visually enough.

  // The tessellation is the most visible on the biggest radius.
  double maxRadius;
  if (rX >= rY)
  {
    maxRadius = rX;
  }
  else
  {
    maxRadius = rY;
  }

  if (error > maxRadius)
  {
    // to make sure the argument of asin() is in a valid range.
    error = maxRadius;
  }

  // Angle of a sector so that its chord is `error' pixels.
  // This is will be our maximum angle step.
  double maxStep = 2.0 * asin(error / (2.0 * maxRadius));

  // ceil because we want to make sure we don't underestimate the number of
  // iterations by 1.
  return static_cast<int>(
    ceil(vtkMath::RadiansFromDegrees(std::fabs(stopAngle - startAngle)) / maxStep));
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawCrossMarkers(
  bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  const float markerSize = this->Pen->GetWidth();
  const float delta = markerSize * 0.5f;

  this->ApplyLineWidth(highlight ? 1.5f : 0.5f);
  this->ApplyLineType(vtkPen::SOLID_LINE);
  if (!colors)
  {
    this->ApplyStrokeColor(this->Pen->GetColorObject().GetData(), 4);
  }

  for (int i = 0; i < n; ++i)
  {
    float* p = points + i * 2;
    if (colors)
    {
      if (i != 0)
      {
        this->Stroke();
      }
      this->ApplyStrokeColor(colors + i * nc_comps, nc_comps);
    }
    HPDF_Page_MoveTo(this->Impl->Page, p[0] + delta, p[1] + delta);
    HPDF_Page_LineTo(this->Impl->Page, p[0] - delta, p[1] - delta);
    HPDF_Page_MoveTo(this->Impl->Page, p[0] + delta, p[1] - delta);
    HPDF_Page_LineTo(this->Impl->Page, p[0] - delta, p[1] + delta);
  }
  this->Stroke();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPlusMarkers(
  bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  const float markerSize = this->Pen->GetWidth();
  const float delta = markerSize * 0.5f;

  this->ApplyLineWidth(highlight ? 1.5f : 0.5f);
  this->ApplyLineType(vtkPen::SOLID_LINE);
  if (!colors)
  {
    this->ApplyStrokeColor(this->Pen->GetColorObject().GetData(), 4);
  }

  for (int i = 0; i < n; ++i)
  {
    float* p = points + i * 2;
    if (colors)
    {
      if (i != 0)
      {
        this->Stroke();
      }
      this->ApplyStrokeColor(colors + i * nc_comps, nc_comps);
    }
    HPDF_Page_MoveTo(this->Impl->Page, p[0], p[1] + delta);
    HPDF_Page_LineTo(this->Impl->Page, p[0], p[1] - delta);
    HPDF_Page_MoveTo(this->Impl->Page, p[0] + delta, p[1]);
    HPDF_Page_LineTo(this->Impl->Page, p[0] - delta, p[1]);
  }
  this->Stroke();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawSquareMarkers(
  bool, float* points, int n, unsigned char* colors, int nc_comps)
{
  const float markerSize = this->Pen->GetWidth();
  const float delta = markerSize * 0.5f;

  if (!colors)
  {
    this->ApplyFillColor(this->Pen->GetColorObject().GetData(), 4);
  }

  for (int i = 0; i < n; ++i)
  {
    float* p = points + i * 2;
    if (colors)
    {
      if (i != 0)
      {
        this->Fill();
      }
      this->ApplyFillColor(colors + i * nc_comps, nc_comps);
    }
    HPDF_Page_Rectangle(this->Impl->Page, p[0] - delta, p[1] - delta, markerSize, markerSize);
  }
  this->Fill();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawCircleMarkers(
  bool, float* points, int n, unsigned char* colors, int nc_comps)
{
  const float markerSize = this->Pen->GetWidth();
  const float radius = markerSize * 0.5f;

  if (!colors)
  {
    this->ApplyFillColor(this->Pen->GetColorObject().GetData(), 4);
  }

  for (int i = 0; i < n; ++i)
  {
    float* p = points + i * 2;
    if (colors)
    {
      if (i != 0)
      {
        this->Fill();
      }
      this->ApplyFillColor(colors + i * nc_comps, nc_comps);
    }
    HPDF_Page_Ellipse(this->Impl->Page, p[0], p[1], radius, radius);
  }
  this->Fill();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawDiamondMarkers(
  bool, float* points, int n, unsigned char* colors, int nc_comps)
{
  const float markerSize = this->Pen->GetWidth();
  const float radius = markerSize * 0.5f;

  if (!colors)
  {
    this->ApplyFillColor(this->Pen->GetColorObject().GetData(), 4);
  }

  for (int i = 0; i < n; ++i)
  {
    float* p = points + i * 2;
    if (colors)
    {
      if (i != 0)
      {
        this->Fill();
      }
      this->ApplyFillColor(colors + i * nc_comps, nc_comps);
    }
    HPDF_Page_MoveTo(this->Impl->Page, p[0] + radius, p[1]);
    HPDF_Page_LineTo(this->Impl->Page, p[0], p[1] + radius);
    HPDF_Page_LineTo(this->Impl->Page, p[0] - radius, p[1]);
    HPDF_Page_LineTo(this->Impl->Page, p[0], p[1] - radius);
    HPDF_Page_ClosePath(this->Impl->Page);
  }
  this->Fill();
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::DrawPath(vtkPath* path, float originX, float originY)
{
  // The text renderer always uses floats to generate paths, so we'll optimize
  // a bit here:
  vtkFloatArray* points = vtkArrayDownCast<vtkFloatArray>(path->GetPoints()->GetData());
  vtkIntArray* codes = path->GetCodes();

  if (!points)
  {
    vtkErrorMacro("This method expects the path point precision to be floats.");
    return;
  }

  vtkIdType numTuples = points->GetNumberOfTuples();
  if (numTuples != codes->GetNumberOfTuples() || codes->GetNumberOfComponents() != 1 ||
    points->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("Invalid path data.");
    return;
  }

  if (numTuples == 0)
  { // Nothing to do.
    return;
  }

  typedef vtkPath::ControlPointType CodeEnum;
  typedef vtkIntArray::ValueType CodeType;
  CodeType* code = codes->GetPointer(0);
  CodeType* codeEnd = code + numTuples;

  typedef vtkFloatArray::ValueType PointType;
  PointType* point = points->GetPointer(0);

  // These are only used in an assertion, ifdef silences warning on non-debug
  // builds
#ifndef NDEBUG
  PointType* pointBegin = point;
  CodeType* codeBegin = code;
#endif

  HPDF_Page page = this->Impl->Page;

  // Translate to origin:
  HPDF_Page_Concat(page, 1.f, 0.f, 0.f, 1.f, originX, originY);

  while (code < codeEnd)
  {
    assert("Sanity check" && (code - codeBegin) * 3 == point - pointBegin);

    switch (static_cast<CodeEnum>(*code))
    {
      case vtkPath::MOVE_TO:
        HPDF_Page_MoveTo(page, point[0], point[1]);
        point += 3;
        ++code;
        break;

      case vtkPath::LINE_TO:
        HPDF_Page_LineTo(page, point[0], point[1]);
        point += 3;
        ++code;
        break;

      case vtkPath::CONIC_CURVE:
        HPDF_Page_CurveTo3(page, point[0], point[1], point[3], point[4]);
        point += 6;
        assert(CodeEnum(code[1]) == vtkPath::CONIC_CURVE);
        code += 2;
        break;

      case vtkPath::CUBIC_CURVE:
        HPDF_Page_CurveTo(page, point[0], point[1], point[3], point[4], point[6], point[7]);
        point += 9;
        assert(CodeEnum(code[1]) == vtkPath::CUBIC_CURVE);
        assert(CodeEnum(code[2]) == vtkPath::CUBIC_CURVE);
        code += 3;
        break;

      default:
        throw std::runtime_error("Unknown control code.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::ApplyTransform()
{
  // The HPDF API for transform management is lacking. There's no clear way
  // to simply *set* the transform, we can only concatenate multiple transforms
  // together. Nor is there a way to push/pop a matrix stack. So we'll just
  // invert the current transform to unapply it before applying the new one.
  HPDF_TransMatrix oldTrans = HPDF_Page_GetTransMatrix(this->Impl->Page);
  double oldInvTransMat3[9];
  vtkPDFContextDevice2D::HPDFTransformToMatrix3(
    oldTrans.a, oldTrans.b, oldTrans.c, oldTrans.d, oldTrans.x, oldTrans.y, oldInvTransMat3);
  vtkMatrix3x3::Invert(oldInvTransMat3, oldInvTransMat3);

  // Multiply the inverse current transform with the new one:
  double newTransMat3[9];
  double* mat = this->Matrix->GetMatrix()->GetData();
  vtkPDFContextDevice2D::Matrix4ToMatrix3(mat, newTransMat3);

  vtkMatrix3x3::Multiply3x3(oldInvTransMat3, newTransMat3, newTransMat3);

  // Test if the new transform is identity, within tolerance:
  bool isIdentity = true;
  const double tol = 1e-6;
  for (size_t i = 0; i < 3 && isIdentity; ++i)
  {
    for (size_t j = 0; j < 3 && isIdentity; ++j)
    {
      const double val = newTransMat3[i * 3 + j];
      const double exp = (i == j) ? 1. : 0.;
      if (std::fabs(val - exp) > tol)
      {
        isIdentity = false;
      }
    }
  }

  // Do nothing if the transform would have no effect.
  if (isIdentity)
  {
    return;
  }

  // Otherwise, write the new transform out.
  float hpdfMat[6];
  vtkPDFContextDevice2D::Matrix3ToHPDFTransform(newTransMat3, hpdfMat);
  HPDF_Page_Concat(
    this->Impl->Page, hpdfMat[0], hpdfMat[1], hpdfMat[2], hpdfMat[3], hpdfMat[4], hpdfMat[5]);
}

//------------------------------------------------------------------------------
vtkVector2f vtkPDFContextDevice2D::GetUnscaledPenWidth()
{
  float width = this->GetPen()->GetWidth();
  vtkNew<vtkMatrix3x3> mat;
  this->GetMatrix(mat);
  double sx;
  double sy;
  std::tie(sx, sy) = GetScaleFactor(mat);

  return vtkVector2f{ static_cast<float>(width / sx), static_cast<float>(width / sy) };
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Matrix3ToMatrix4(vtkMatrix3x3* mat3, double mat4[16])
{
  const double* mat3Data = mat3->GetData();
  mat4[0] = mat3Data[0];
  mat4[1] = mat3Data[1];
  mat4[2] = 0.;
  mat4[3] = mat3Data[2];
  mat4[4] = mat3Data[3];
  mat4[5] = mat3Data[4];
  mat4[6] = 0.;
  mat4[7] = mat3Data[5];
  mat4[8] = 0.;
  mat4[9] = 0.;
  mat4[10] = 1.;
  mat4[11] = 0.;
  mat4[12] = 0.;
  mat4[13] = 0.;
  mat4[14] = 0.;
  mat4[15] = 1.;
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Matrix4ToMatrix3(double mat4[16], vtkMatrix3x3* mat3)
{
  double* mat3Data = mat3->GetData();
  vtkPDFContextDevice2D::Matrix4ToMatrix3(mat4, mat3Data);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Matrix4ToMatrix3(double mat4[16], double mat3[9])
{
  mat3[0] = mat4[0];
  mat3[1] = mat4[1];
  mat3[2] = mat4[3];
  mat3[3] = mat4[4];
  mat3[4] = mat4[5];
  mat3[5] = mat4[7];
  mat3[6] = 0.;
  mat3[7] = 0.;
  mat3[8] = 1.;
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Matrix4ToHPDFTransform(const double mat4[16], float hpdfMat[6])
{
  hpdfMat[0] = static_cast<float>(mat4[0]);
  hpdfMat[1] = static_cast<float>(mat4[4]);
  hpdfMat[2] = static_cast<float>(mat4[1]);
  hpdfMat[3] = static_cast<float>(mat4[5]);
  hpdfMat[4] = static_cast<float>(mat4[3]);
  hpdfMat[5] = static_cast<float>(mat4[7]);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::Matrix3ToHPDFTransform(const double mat3[9], float hpdfMat[6])
{
  hpdfMat[0] = static_cast<float>(mat3[0]);
  hpdfMat[1] = static_cast<float>(mat3[3]);
  hpdfMat[2] = static_cast<float>(mat3[1]);
  hpdfMat[3] = static_cast<float>(mat3[4]);
  hpdfMat[4] = static_cast<float>(mat3[2]);
  hpdfMat[5] = static_cast<float>(mat3[5]);
}

//------------------------------------------------------------------------------
void vtkPDFContextDevice2D::HPDFTransformToMatrix3(
  float a, float b, float c, float d, float x, float y, double mat3[9])
{
  mat3[0] = static_cast<double>(a);
  mat3[1] = static_cast<double>(c);
  mat3[2] = static_cast<double>(x);
  mat3[3] = static_cast<double>(b);
  mat3[4] = static_cast<double>(d);
  mat3[5] = static_cast<double>(y);
  mat3[6] = 0.;
  mat3[7] = 0.;
  mat3[8] = 1.;
}
