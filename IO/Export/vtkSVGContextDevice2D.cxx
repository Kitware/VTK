/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSVGContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSVGContextDevice2D.h"

#include "vtkAssume.h"
#include "vtkBase64OutputStream.h"
#include "vtkBrush.h"
#include "vtkColor.h"
#include "vtkFloatArray.h"
#include "vtkFreeTypeTools.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTransform.h"
#include "vtkUnicodeString.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkXMLDataElement.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace
{

std::string ColorToString(const unsigned char* rgb)
{
  std::ostringstream out;
  out << "#";
  for (int i = 0; i < 3; ++i)
  {
    out << std::setw(2) << std::right << std::setfill('0') << std::hex
        << static_cast<unsigned int>(rgb[i]);
  }
  return out.str();
}

// Bbox is xmin, xmax, ymin, ymax. Writes:
// "xmin,ymin,xmax,ymax"
std::string BBoxToString(const std::array<int, 4>& bbox)
{
  std::ostringstream out;
  out << bbox[0] << "," << bbox[2] << "," << bbox[1] << "," << bbox[3];
  return out.str();
}

std::string Transform2DToString(const std::array<double, 9> xform)
{
  std::ostringstream out;
  out << "matrix(" << xform[0] << "," << xform[3] << "," << xform[1] << "," << xform[4] << ","
      << xform[2] << "," << xform[5] << ")";
  return out.str();
}

struct EllipseHelper
{
  EllipseHelper(float cx, float cy, float rx, float ry)
    : X(0.f)
    , Y(0.f)
    , Cx(cx)
    , Cy(cy)
    , Rx(rx)
    , Ry(ry)
  {
  }

  void UpdateDegrees(float degrees) { this->UpdateRadians(vtkMath::RadiansFromDegrees(degrees)); }

  void UpdateRadians(float radians)
  {
    this->X = this->Cx + std::cos(radians) * this->Rx;
    this->Y = this->Cy + std::sin(radians) * this->Ry;
  }

  float X;
  float Y;

private:
  float Cx;
  float Cy;
  float Rx;
  float Ry;
};

struct FontKey
{
  vtkSmartPointer<vtkTextProperty> TextProperty;

  explicit FontKey(vtkTextProperty* tprop)
    : TextProperty(vtkSmartPointer<vtkTextProperty>::New())
  {
    // Clone into an internal tprop. The property will likely be modified by
    // the time we get around to writing out definitions.
    this->TextProperty->ShallowCopy(tprop);

    // Blank out properties that we don't care about for raw outlines:
    this->TextProperty->SetFontSize(0);
    this->TextProperty->SetOrientation(0.);
  }

  FontKey(const FontKey& o)
    : TextProperty(o.TextProperty)
  {
  }

  bool operator<(const FontKey& other) const
  {
    const int thisFontFamily = this->TextProperty->GetFontFamily();
    const int otherFontFamily = other.TextProperty->GetFontFamily();
    if (thisFontFamily < otherFontFamily)
    {
      return true;
    }
    else if (thisFontFamily > otherFontFamily)
    {
      return false;
    }

    const bool thisBold = this->TextProperty->GetBold() != 0;
    const bool otherBold = other.TextProperty->GetBold() != 0;
    if (thisBold < otherBold)
    {
      return true;
    }
    else if (thisBold > otherBold)
    {
      return false;
    }

    const bool thisItalic = this->TextProperty->GetItalic() != 0;
    const bool otherItalic = other.TextProperty->GetItalic() != 0;
    if (thisItalic < otherItalic)
    {
      return true;
    }
    else if (thisItalic > otherItalic)
    {
      return false;
    }

    if (thisFontFamily == VTK_FONT_FILE)
    {
      const char* thisFile = this->TextProperty->GetFontFile();
      const char* otherFile = other.TextProperty->GetFontFile();
      if (thisFile < otherFile)
      {
        return true;
      }
      else if (thisFile > otherFile)
      {
        return false;
      }
    }

    return false;
  }
};

struct FontInfo
{
  using CharType = vtkUnicodeString::value_type;
  using KerningPairType = std::pair<CharType, CharType>;

  explicit FontInfo(const std::string& svgId)
    : SVGId(svgId)
  {
  }

  void ProcessString(const vtkUnicodeString& str)
  {
    vtkUnicodeString::const_iterator it = str.begin();
    vtkUnicodeString::const_iterator end = str.end();
    if (it == end)
    {
      return;
    }

    vtkUnicodeString::const_iterator next = it;
    std::advance(next, 1);
    while (next != end)
    {
      this->Chars.insert(*it);
      this->KerningPairs.insert(std::make_pair(*it, *next));
      std::advance(it, 1);
      std::advance(next, 1);
    }

    // Last char:
    this->Chars.insert(*it);
  }

  std::string SVGId;
  std::set<CharType> Chars;
  std::set<KerningPairType> KerningPairs;

private:
  FontInfo(const FontInfo&) = delete;
  void operator=(const FontInfo&) = delete;
};

struct ImageInfo
{
  explicit ImageInfo(vtkImageData* img)
  {
    vtkNew<vtkPNGWriter> pngWriter;
    pngWriter->WriteToMemoryOn();
    pngWriter->SetCompressionLevel(0);
    pngWriter->SetInputData(img);
    pngWriter->Write();

    vtkUnsignedCharArray* png = pngWriter->GetResult();
    if (!png || png->GetNumberOfValues() == 0)
    {
      return;
    }

    std::ostringstream base64Stream;
    base64Stream << "data:image/png;base64,";

    vtkNew<vtkBase64OutputStream> base64Encoder;
    base64Encoder->SetStream(&base64Stream);
    if (!base64Encoder->StartWriting() ||
      !base64Encoder->Write(png->GetPointer(0), png->GetNumberOfValues()) ||
      !base64Encoder->EndWriting())
    {
      return;
    }

    int* dims = img->GetDimensions();
    this->Size[0] = dims[0];
    this->Size[1] = dims[1];

    this->PNGBase64 = base64Stream.str();
  }

  ImageInfo(ImageInfo&& o)
    : Size(std::move(o.Size))
    , Id(std::move(o.Id))
    , PNGBase64(std::move(o.PNGBase64))
  {
  }

  bool operator<(const ImageInfo& other) const
  {
    if (this->Size[0] < other.Size[0])
    {
      return true;
    }
    else if (this->Size[0] > other.Size[0])
    {
      return false;
    }

    if (this->Size[1] < other.Size[1])
    {
      return true;
    }
    else if (this->Size[1] > other.Size[1])
    {
      return false;
    }

    if (this->PNGBase64 < other.PNGBase64)
    {
      return true;
    }

    return false;
  }

  std::array<int, 2> Size;
  std::string Id;
  std::string PNGBase64;

private:
  ImageInfo(const ImageInfo&) = delete;
  void operator=(const ImageInfo&) = delete;
};

struct PatternInfo
{
  explicit PatternInfo(const ImageInfo& img, int textureProperty)
    // We only care about Repeat and Stretch, since SVG doesn't allow control
    // over Nearest/Linear interpolation.
    : TextureProperty(textureProperty & (vtkBrush::Repeat | vtkBrush::Stretch))
    , ImageSize(img.Size)
    , ImageId(img.Id)
  {
  }

  PatternInfo(PatternInfo&& o)
    : TextureProperty(std::move(o.TextureProperty))
    , ImageSize(std::move(o.ImageSize))
    , ImageId(std::move(o.ImageId))
    , PatternId(std::move(o.PatternId))
  {
  }

  bool operator<(const PatternInfo& other) const
  {
    if (this->TextureProperty < other.TextureProperty)
    {
      return true;
    }
    else if (this->TextureProperty > other.TextureProperty)
    {
      return false;
    }

    if (this->ImageId < other.ImageId)
    {
      return true;
    }

    return false;
  }

  int TextureProperty;
  std::array<int, 2> ImageSize;
  std::string ImageId;
  std::string PatternId;

private:
  PatternInfo(const PatternInfo&) = delete;
  void operator=(const PatternInfo&) = delete;
};

struct ClipRectInfo
{
  explicit ClipRectInfo(const std::array<int, 4>& rect)
    : Rect(rect)
  {
  }

  ClipRectInfo(ClipRectInfo&& o)
    : Rect(std::move(o.Rect))
    , Id(std::move(o.Id))
  {
  }

  bool operator<(const ClipRectInfo& other) const
  {
    for (size_t i = 0; i < this->Rect.size(); ++i)
    {
      if (this->Rect[i] < other.Rect[i])
      {
        return true;
      }
      else if (this->Rect[i] > other.Rect[i])
      {
        return false;
      }
    }

    return false;
  }

  std::array<int, 4> Rect; // x, y, w, h
  std::string Id;

private:
  ClipRectInfo(const ClipRectInfo&) = delete;
  void operator=(const ClipRectInfo&) = delete;
};

// SVG's y axis is inverted compared to VTK's:
struct YConverter
{
  float Height;

  explicit YConverter(float height)
    : Height(height)
  {
  }

  float operator()(float inY) { return this->Height - inY; }
};

} // end anon namespace

// Need to be able to use vtkColor4f in a std::map. Must be outside of the anon
// namespace to work.
static bool operator<(const vtkColor4f& a, const vtkColor4f& b)
{
  for (int i = 0; i < 4; ++i)
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

struct vtkSVGContextDevice2D::Details
{
  using FontMapType = std::map<FontKey, FontInfo*>;
  using ImageSetType = std::set<ImageInfo>;
  using PatternSetType = std::set<PatternInfo>;
  using ClipRectSetType = std::set<ClipRectInfo>;

  FontMapType FontMap;
  ImageSetType ImageSet;
  PatternSetType PatternSet;
  ClipRectSetType ClipRectSet;

  ~Details() { this->FreeFontMap(); }

  void FreeFontMap()
  {
    for (auto& it : this->FontMap)
    {
      delete it.second;
    }
    this->FontMap.clear();
  }

  const ImageInfo& GetImageInfo(vtkImageData* img)
  {
    ImageInfo newInfo(img);

    auto insertResult = this->ImageSet.insert(std::move(newInfo));
    const ImageInfo& info = *insertResult.first;
    if (insertResult.second)
    {
      std::ostringstream id;
      id << "vtkEmbeddedImage" << this->ImageSet.size();
      // This is safe; setting the id won't change the sort order in the set.
      const_cast<ImageInfo&>(info).Id = id.str();
    }

    return info;
  }

  const PatternInfo& GetPatternInfo(vtkImageData* texture, int textureProperty)
  {
    const ImageInfo& imageInfo = this->GetImageInfo(texture);
    PatternInfo newInfo(imageInfo, textureProperty);

    auto insertResult = this->PatternSet.insert(std::move(newInfo));
    const PatternInfo& info = *insertResult.first;
    if (insertResult.second)
    {
      std::ostringstream id;
      id << "vtkPattern" << this->PatternSet.size();
      // This is safe; setting the id won't change the sort order in the set.
      const_cast<PatternInfo&>(info).PatternId = id.str();
    }

    return info;
  }

  const ClipRectInfo& GetClipRectInfo(const std::array<int, 4>& rect)
  {
    ClipRectInfo newInfo(rect);

    auto insertResult = this->ClipRectSet.insert(std::move(newInfo));
    const ClipRectInfo& info = *insertResult.first;
    if (insertResult.second)
    {
      std::ostringstream id;
      id << "vtkClipRect" << this->ClipRectSet.size();
      // This is safe; setting the id won't change the sort order in the set.
      const_cast<ClipRectInfo&>(info).Id = id.str();
    }

    return info;
  }

  FontInfo& GetFontInfo(vtkTextProperty* tprop)
  {
    FontKey key(tprop);
    FontMapType::const_iterator it = this->FontMap.find(key);
    if (it == this->FontMap.end())
    {
      std::ostringstream tmp;
      tmp << "vtkExportedFont-" << std::hex << this << "_" << std::dec << this->FontMap.size()
          << "_" << tprop->GetFontFamilyAsString();
      std::string id = tmp.str();
      auto result = this->FontMap.insert(std::make_pair(key, new FontInfo(id)));
      it = result.first;
    }

    return *it->second;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSVGContextDevice2D);
vtkCxxSetObjectMacro(vtkSVGContextDevice2D, Viewport, vtkViewport);

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetSVGContext(vtkXMLDataElement* context, vtkXMLDataElement* defs)
{
  this->ContextNode = context;
  this->ActiveNode = context;
  this->DefinitionNode = defs;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::GenerateDefinitions()
{
  if (this->EmbedFonts)
  {
    this->WriteFonts();
  }

  this->WriteImages();
  this->WritePatterns(); // Must come after images
  this->WriteClipRects();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::Begin(vtkViewport* vp)
{
  // Recreate the pen/brush to reset state:
  this->Pen->Delete();
  this->Pen = vtkPen::New();
  this->Brush->Delete();
  this->Brush = vtkBrush::New();

  this->SetViewport(vp);
  this->CanvasHeight = static_cast<float>(vp->GetVTKWindow()->GetSize()[1]);
  std::fill(this->ClipRect.begin(), this->ClipRect.end(), 0);
  std::fill(this->ActiveNodeClipRect.begin(), this->ActiveNodeClipRect.end(), 0);
  std::fill(this->ActiveNodeTransform.begin(), this->ActiveNodeTransform.end(), 0.);
  this->ActiveNodeTransform[0] = 1.;
  this->ActiveNodeTransform[4] = 1.;
  this->ActiveNodeTransform[8] = 1.;
  this->Matrix->Identity();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::End()
{
  this->SetViewport(nullptr);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawPoly(float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!colors)
  {
    vtkNew<vtkXMLDataElement> polyLine;
    polyLine->SetName("polyline");
    this->ActiveNode->AddNestedElement(polyLine);
    this->ApplyPenStateToNode(polyLine);

    YConverter y(this->CanvasHeight);

    std::ostringstream verts;
    verts << "\n";
    for (int i = 0; i < n; ++i)
    {
      verts << points[i * 2] << "," << y(points[i * 2 + 1]) << "\n";
    }
    polyLine->SetAttribute("points", verts.str().c_str());
  }
  else
  {
    this->PushGraphicsState();
    this->ApplyPenStippleToNode(this->ActiveNode);
    this->ApplyPenWidthToNode(this->ActiveNode);
    bool useAlpha = nc_comps == 4;
    if (!useAlpha)
    {
      this->ApplyPenOpacityToNode(this->ActiveNode);
    }

    for (int i = 0; i < n - 1; ++i)
    {
      const vtkVector2f p1(points + i * 2);
      const vtkColor4ub c1(colors + i * nc_comps);
      const vtkVector2f p2(points + (i + 1) * 2);
      const vtkColor4ub c2(colors + (i + 1) * nc_comps);

      this->DrawLineGradient(p1, c1, p2, c2, useAlpha);
    }

    this->PopGraphicsState();
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawLines(float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!colors)
  {
    // Use path instead of lines for a more efficient/compact representation:
    vtkNew<vtkXMLDataElement> path;
    path->SetName("path");
    this->ActiveNode->AddNestedElement(path);
    this->ApplyPenStateToNode(path);

    YConverter y(this->CanvasHeight);

    std::ostringstream d;
    d << "\n";
    int numLines = n / 2;
    for (int i = 0; i < numLines; ++i)
    {
      const float* p1 = points + i * 4;
      const float* p2 = points + i * 4 + 2;
      d << "M" << p1[0] << "," << y(p1[1]) << "L" << p2[0] << "," << y(p2[1]) << "\n";
    }
    path->SetAttribute("d", d.str().c_str());
  }
  else
  {
    this->PushGraphicsState();
    this->ApplyPenStippleToNode(this->ActiveNode);
    this->ApplyPenWidthToNode(this->ActiveNode);
    bool useAlpha = nc_comps == 4;
    if (!useAlpha)
    {
      this->ApplyPenOpacityToNode(this->ActiveNode);
    }

    const int numLines = n / 2;
    for (int i = 0; i < numLines; ++i)
    {
      const vtkVector2f p1(points + i * 4);
      const vtkVector2f p2(points + i * 4 + 2);
      const vtkColor4ub c1(colors + i * 2 * nc_comps);
      const vtkColor4ub c2(colors + (i * 2 + 1) * nc_comps);

      this->DrawLineGradient(p1, c1, p2, c2, useAlpha);
    }

    this->PopGraphicsState();
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawPoints(float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!colors)
  {
    // Use path instead of rects for a more efficient/compact representation.
    vtkNew<vtkXMLDataElement> path;
    path->SetName("path");
    this->ActiveNode->AddNestedElement(path);

    this->ApplyPenAsFillColorToNode(path);
    this->ApplyPenAsFillOpacityToNode(path);

    YConverter y(this->CanvasHeight);

    float deltaX;
    float deltaY;
    this->GetScaledPenWidth(deltaX, deltaY);
    deltaX *= 0.5f;
    deltaY *= 0.5f;

    std::ostringstream d;
    d << "\n";
    for (int i = 0; i < n; ++i)
    {
      const float* p = points + i * 2;
      d << "M" << p[0] - deltaX << "," << y(p[1] - deltaY)
        << "\n"
           "L"
        << p[0] + deltaX << "," << y(p[1] - deltaY) << "\n"
        << p[0] + deltaX << "," << y(p[1] + deltaY) << "\n"
        << p[0] - deltaX << "," << y(p[1] + deltaY) << "\nz\n";
    }
    path->SetAttribute("d", d.str().c_str());
  }
  else
  {
    const float width = this->GetScaledPenWidth();
    const float halfWidth = width * 0.5f;
    const bool useAlpha = nc_comps == 4;

    if (!useAlpha)
    {
      this->PushGraphicsState();
      this->ApplyPenAsFillOpacityToNode(this->ActiveNode);
    }

    YConverter y(this->CanvasHeight);

    for (int i = 0; i < n; ++i)
    {
      const float* p = points + i * 2;
      const unsigned char* c = colors + i * nc_comps;

      vtkNew<vtkXMLDataElement> point;
      this->ActiveNode->AddNestedElement(point);

      point->SetName("rect");
      point->SetFloatAttribute("x", p[0] - halfWidth);
      point->SetFloatAttribute("y", y(p[1] - halfWidth));
      point->SetFloatAttribute("width", width);
      point->SetFloatAttribute("height", width);
      point->SetAttribute("fill", ColorToString(c).c_str());
      if (useAlpha && c[3] != 255)
      {
        point->SetFloatAttribute("fill-opacity", c[3] / 255.f);
      }
    }

    if (!useAlpha)
    {
      this->PopGraphicsState();
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawPointSprites(
  vtkImageData* spriteIn, float* points, int n, unsigned char* colors, int nc_comps)
{
  if (nc_comps != 3 && nc_comps != 4)
  {
    vtkErrorMacro("Unsupported number of components: " << nc_comps);
    return;
  }

  vtkImageData* rgba = this->PreparePointSprite(spriteIn);
  if (!rgba)
  {
    vtkErrorMacro("Unsupported point sprite format.");
    return;
  }

  assert(rgba->GetScalarType() == VTK_UNSIGNED_CHAR);
  assert(rgba->GetNumberOfScalarComponents() == 4);

  int dims[3];
  rgba->GetDimensions(dims);
  vtkIdType numPoints = rgba->GetNumberOfPoints();
  vtkUnsignedCharArray* colorArray =
    vtkArrayDownCast<vtkUnsignedCharArray>(rgba->GetPointData()->GetScalars());
  const float sizeFactor =
    this->GetScaledPenWidth() / static_cast<float>(std::max(dims[0], dims[1]));
  const float spriteWidth = dims[0] * sizeFactor;
  const float spriteHeight = dims[1] * sizeFactor;
  const float halfWidth = spriteWidth * 0.5f;
  const float halfHeight = spriteHeight * 0.5f;
  const float brushAlpha = this->Brush->GetOpacity() / 255.f;
  YConverter y(this->CanvasHeight);

  typedef std::map<vtkColor4f, std::string> SpriteMap;
  SpriteMap spriteMap;

  for (int i = 0; i < n; ++i)
  {
    const float* p = points + 2 * i;

    vtkColor4f color;
    if (colors)
    {
      unsigned char* c = colors + nc_comps * i;
      switch (nc_comps)
      {
        case 3:
          color.Set(c[0] / 255.f, c[1] / 255.f, c[2] / 255.f, brushAlpha);
          break;

        case 4:
          color.Set(c[0] / 255.f, c[1] / 255.f, c[2] / 255.f, c[3] / 255.f);
          break;

        default:
          vtkErrorMacro("Unsupported number of color components: " << nc_comps);
          continue;
      }
    }
    else
    {
      vtkColor4ub penColor = this->Pen->GetColorObject();
      color = vtkColor4f(
        penColor[0] / 255.f, penColor[1] / 255.f, penColor[2] / 255.f, penColor[3] / 255.f);
    }

    std::string sprite;
    SpriteMap::iterator it = spriteMap.find(color);
    if (it != spriteMap.end())
    {
      sprite = it->second;
    }
    else
    {
      vtkNew<vtkUnsignedCharArray> spriteColor;
      spriteColor->SetNumberOfComponents(4);
      spriteColor->SetNumberOfTuples(numPoints);

      for (vtkIdType t = 0; t < numPoints; ++t)
      {
        // This is what the OpenGL implementation does:
        for (int c = 0; c < 4; ++c)
        {
          spriteColor->SetTypedComponent(
            t, c, static_cast<unsigned char>(colorArray->GetTypedComponent(t, c) * color[c] + .5f));
        }
      }

      vtkNew<vtkImageData> spriteImage;
      spriteImage->ShallowCopy(rgba);
      spriteImage->GetPointData()->SetScalars(spriteColor);

      const ImageInfo& info = this->Impl->GetImageInfo(spriteImage);
      sprite = info.Id;

      spriteMap.insert(std::make_pair(color, sprite));
    }

    const float xScale = spriteWidth / dims[0];
    const float yScale = spriteHeight / dims[1];

    // Offset the coordinates to center the sprite on the anchor:
    const float anchorX = p[0] - halfWidth;
    const float anchorY = y(p[1] - halfHeight);

    // Construct a matrix representing the following transformation:
    //
    // [X] = [T3] [T2] [S] [T1]
    //
    // [X]  = final transform
    // [T1] = translate(-pos.X, -pos.Y); Move to origin to prepare for scaling.
    // [S]  = scale(xScale, yScale); Resize the image to match the input rect.
    // [T2] = translate(0, -pos.H); Anchor at bottom corner instead of top
    // [T3] = translate(pos.X, pos.Y); Move back to anchor point
    std::ostringstream xform;
    xform << "matrix(" << xScale << ",0,0," << yScale << "," << anchorX - xScale * anchorX << ","
          << anchorY - (yScale * anchorY + spriteHeight) << ")";

    vtkNew<vtkXMLDataElement> use;
    this->ActiveNode->AddNestedElement(use);
    use->SetName("use");
    use->SetFloatAttribute("x", anchorX);
    use->SetFloatAttribute("y", anchorY); // YConverter already applied
    use->SetFloatAttribute("width", spriteWidth);
    use->SetFloatAttribute("height", spriteHeight);
    use->SetAttribute("transform", xform.str().c_str());
    use->SetAttribute("xlink:href", (std::string("#") + sprite).c_str());
  }

  rgba->UnRegister(this);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawMarkers(
  int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  bool fill = false;
  bool stroke = false;
  float strokeWidth = 0.f;

  std::string markerId;
  switch (shape)
  {
    case VTK_MARKER_CROSS:
      markerId = this->AddCrossSymbol(highlight);
      stroke = true;
      strokeWidth = highlight ? 1.5f : 1.f;
      break;

    default:
      // default is here for consistency with old impl -- defaults to plus for
      // unrecognized shapes.
      VTK_FALLTHROUGH;
    case VTK_MARKER_PLUS:
      markerId = this->AddPlusSymbol(highlight);
      stroke = true;
      strokeWidth = highlight ? 1.5f : 1.f;
      break;

    case VTK_MARKER_SQUARE:
      markerId = this->AddSquareSymbol(highlight);
      fill = true;
      break;

    case VTK_MARKER_CIRCLE:
      markerId = this->AddCircleSymbol(highlight);
      fill = true;
      break;

    case VTK_MARKER_DIAMOND:
      markerId = this->AddDiamondSymbol(highlight);
      fill = true;
      break;
  }

  const float width = this->GetScaledPenWidth();
  const float halfWidth = width * 0.5f;
  YConverter y(this->CanvasHeight);

  // Adjust stroke width for scaling. Symbols are defined in a unit square.
  strokeWidth /= width;

  markerId = std::string("#") + markerId;

  if (!colors)
  {
    this->PushGraphicsState();
    if (stroke)
    {
      this->ApplyPenColorToNode(this->ActiveNode);
      this->ApplyPenOpacityToNode(this->ActiveNode);
      this->ApplyPenStippleToNode(this->ActiveNode);
      this->ActiveNode->SetFloatAttribute("stroke-width", strokeWidth);
    }
    if (fill)
    {
      this->ApplyPenAsFillColorToNode(this->ActiveNode);
      this->ApplyPenAsFillOpacityToNode(this->ActiveNode);
    }

    for (int i = 0; i < n; ++i)
    {
      const float* p = points + i * 2;

      vtkNew<vtkXMLDataElement> node;
      this->ActiveNode->AddNestedElement(node);
      node->SetName("use");
      node->SetFloatAttribute("x", p[0] - halfWidth);
      node->SetFloatAttribute("y", y(p[1]) - halfWidth);
      node->SetFloatAttribute("width", width);
      node->SetFloatAttribute("height", width);
      node->SetAttribute("xlink:href", markerId.c_str());
    }

    this->PopGraphicsState();
  }
  else
  {
    const bool useAlpha = nc_comps == 4;

    if (!useAlpha)
    {
      this->PushGraphicsState();
      if (stroke)
      {
        this->ApplyPenOpacityToNode(this->ActiveNode);
      }
      if (fill)
      {
        this->ApplyPenAsFillOpacityToNode(this->ActiveNode);
      }
    }

    for (int i = 0; i < n; ++i)
    {
      const float* p = points + i * 2;
      const unsigned char* c = colors + i * nc_comps;
      const std::string colStr = ColorToString(c);

      vtkNew<vtkXMLDataElement> node;
      this->ActiveNode->AddNestedElement(node);
      node->SetName("use");
      node->SetFloatAttribute("x", p[0] - halfWidth);
      node->SetFloatAttribute("y", y(p[1]) - halfWidth);
      node->SetFloatAttribute("width", width);
      node->SetFloatAttribute("height", width);
      node->SetAttribute("xlink:href", markerId.c_str());
      if (stroke)
      {
        node->SetAttribute("stroke", colStr.c_str());
        node->SetFloatAttribute("stroke-width", strokeWidth);
      }
      if (fill)
      {
        node->SetAttribute("fill", colStr.c_str());
      }
      if (useAlpha && c[3] != 255)
      {
        const float a = c[3] / 255.f;
        if (stroke)
        {
          node->SetFloatAttribute("stroke-opacity", a);
        }
        if (fill)
        {
          node->SetFloatAttribute("fill-opacity", a);
        }
      }
    }

    if (!useAlpha)
    {
      this->PopGraphicsState();
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawQuad(float* points, int n)
{
  this->DrawPolygon(points, n);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawQuadStrip(float* points, int n)
{
  if (n < 4 || n % 2 != 0)
  { // Must be at least one quad, and a whole number of quads.
    return;
  }

  // Combine all into a path that traces the exterior (Even verts on one side,
  // odd verts on the other):
  vtkNew<vtkXMLDataElement> path;
  path->SetName("path");
  this->ActiveNode->AddNestedElement(path);

  this->ApplyBrushStateToNode(path);

  YConverter y(this->CanvasHeight);
  std::ostringstream d;
  d << "\nM" << points[0] << "," << y(points[1]) << "\nL\n";
  for (int i = 2; i < n; i += 2)
  {
    d << points[i * 2] << "," << y(points[i * 2 + 1]) << "\n";
  }

  for (int i = n - 1; i >= 0; i -= 2)
  {
    d << points[i * 2] << "," << y(points[i * 2 + 1]) << "\n";
  }
  d << "z";

  path->SetAttribute("d", d.str().c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawPolygon(float* points, int n)
{
  vtkNew<vtkXMLDataElement> path;
  path->SetName("path");
  this->ActiveNode->AddNestedElement(path);

  this->ApplyBrushStateToNode(path);

  YConverter y(this->CanvasHeight);
  std::ostringstream d;
  d << "\nM" << points[0] << "," << y(points[1]) << "\nL";
  for (int i = 1; i < n; ++i)
  {
    d << points[i * 2] << "," << y(points[i * 2 + 1]) << "\n";
  }
  d << "z";

  path->SetAttribute("d", d.str().c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawColoredPolygon(
  float* points, int numPoints, unsigned char* colors, int nc_comps)
{
  assert(numPoints > 0);
  assert(nc_comps >= 3 && nc_comps <= 4);
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

  const bool useAlpha = nc_comps == 4;
  const vtkVector2f p0(points);
  const vtkColor4ub c0(colors);

  // We may have 3 or 4 components, so initialize these with a sane alpha value:
  vtkColor4ub c1{ 0, 0, 0, 255 };
  vtkColor4ub c2{ 0, 0, 0, 255 };

  for (int i = 1; i < numPoints - 1; ++i)
  {
    const vtkVector2f p1(points + 2 * i);
    const vtkVector2f p2(points + 2 * (i + 1));
    std::copy_n(colors + nc_comps * i, nc_comps, c1.GetData());
    std::copy_n(colors + nc_comps * (i + 1), nc_comps, c2.GetData());

    this->DrawTriangleGradient(p0, c0, p1, c1, p2, c2, useAlpha);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawEllipseWedge(float cx, float cy, float outRx, float outRy,
  float inRx, float inRy, float startAngle, float stopAngle)
{
  if (stopAngle < startAngle)
  {
    std::swap(startAngle, stopAngle);
  }

  const float arcLength = stopAngle - startAngle;
  const bool isArc = arcLength < 359.99f;
  const bool isFilled = inRx == 0.f && inRy == 0.f;
  const bool isCircle = inRx == inRy && outRx == outRy;
  const int largeArcFlag = (arcLength >= 180.f) ? 1 : 0;
  const int sweepFlag = 0;
  YConverter y(this->CanvasHeight);

  if (!isArc)
  {
    if (isFilled)
    {
      // Easy case: full ellipse/circle:
      if (isCircle)
      {
        vtkNew<vtkXMLDataElement> circle;
        this->ActiveNode->AddNestedElement(circle);
        this->ApplyBrushStateToNode(circle);
        circle->SetName("circle");
        circle->SetFloatAttribute("cx", cx);
        circle->SetFloatAttribute("cy", y(cy));
        circle->SetFloatAttribute("r", outRx);
      }
      else // !isCircle
      {
        vtkNew<vtkXMLDataElement> ellipse;
        this->ActiveNode->AddNestedElement(ellipse);
        this->ApplyBrushStateToNode(ellipse);
        ellipse->SetName("ellipse");
        ellipse->SetFloatAttribute("cx", cx);
        ellipse->SetFloatAttribute("cy", y(cy));
        ellipse->SetFloatAttribute("rx", outRx);
        ellipse->SetFloatAttribute("ry", outRy);
      }
    }
    else // !isFilled
    {
      vtkNew<vtkXMLDataElement> path;
      this->ActiveNode->AddNestedElement(path);
      this->ApplyBrushStateToNode(path);
      path->SetName("path");
      path->SetAttribute("fill-rule", "evenodd");

      std::ostringstream d;

      // Outer ellipse:
      EllipseHelper helper(cx, cy, outRx, outRy);
      helper.UpdateDegrees(0.f);
      d << "M" << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(180.f);
      d << "A" << outRx << "," << outRy << " 0 1 1 " << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(360.f);
      d << "A" << outRx << "," << outRy << " 0 1 1 " << helper.X << "," << y(helper.Y) << "\nz\n";

      // Inner ellipse:
      helper = EllipseHelper(cx, cy, inRx, inRy);
      helper.UpdateDegrees(0.f);
      d << "M" << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(180.f);
      d << "A" << inRx << "," << inRy << " 0 1 1 " << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(360.f);
      d << "A" << inRx << "," << inRy << " 0 1 1 " << helper.X << "," << y(helper.Y) << "\nz\n";

      path->SetAttribute("d", d.str().c_str());
    }
  }
  else // isArc
  {
    if (isFilled)
    {
      vtkNew<vtkXMLDataElement> path;
      this->ActiveNode->AddNestedElement(path);
      this->ApplyBrushStateToNode(path);
      path->SetName("path");

      std::ostringstream d;
      EllipseHelper helper(cx, cy, outRx, outRy);

      d << "M" << cx << "," << y(cy) << "\n";
      helper.UpdateDegrees(startAngle);
      d << "L" << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(stopAngle);
      d << "A" << outRx << "," << outRy << " 0 " << largeArcFlag << " " << sweepFlag << " "
        << helper.X << "," << y(helper.Y) << "\nz\n";
      path->SetAttribute("d", d.str().c_str());
    }
    else // !isFilled
    {
      vtkNew<vtkXMLDataElement> path;
      this->ActiveNode->AddNestedElement(path);
      this->ApplyBrushStateToNode(path);
      path->SetName("path");
      path->SetAttribute("fill-rule", "evenodd");

      std::ostringstream d;

      // Outer ellipse
      EllipseHelper helper(cx, cy, outRx, outRy);
      helper.UpdateDegrees(startAngle);
      d << "M" << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(stopAngle);
      d << "A" << outRx << "," << outRy << " 0 " << largeArcFlag << " " << sweepFlag << " "
        << helper.X << "," << y(helper.Y) << "\n";
      path->SetAttribute("d", d.str().c_str());

      // Inner ellipse
      const int innerSweepFlag = 1;
      helper = EllipseHelper(cx, cy, inRx, inRy);
      helper.UpdateDegrees(stopAngle);
      d << "L" << helper.X << "," << y(helper.Y) << "\n";
      helper.UpdateDegrees(startAngle);
      d << "A" << inRx << "," << inRy << " 0 " << largeArcFlag << " " << innerSweepFlag << " "
        << helper.X << "," << y(helper.Y) << "\nz\n";
      path->SetAttribute("d", d.str().c_str());
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawEllipticArc(
  float cx, float cy, float rX, float rY, float startAngle, float stopAngle)
{
  if (stopAngle < startAngle)
  {
    std::swap(startAngle, stopAngle);
  }

  const float arcLength = stopAngle - startAngle;
  const bool isArc = arcLength < 360.f;
  const bool isCircle = rX == rY;
  const int largeArcFlag = (arcLength >= 180.f) ? 1 : 0;
  const int sweepFlag = 0;
  YConverter y(this->CanvasHeight);

  if (!isArc)
  {
    // Easy case: full ellipse/circle:
    if (isCircle)
    {
      vtkNew<vtkXMLDataElement> circle;
      this->ActiveNode->AddNestedElement(circle);
      this->ApplyPenStateToNode(circle);
      this->ApplyBrushStateToNode(circle);
      circle->SetName("circle");
      circle->SetFloatAttribute("cx", cx);
      circle->SetFloatAttribute("cy", y(cy));
      circle->SetFloatAttribute("r", rX);
    }
    else // !isCircle
    {
      vtkNew<vtkXMLDataElement> ellipse;
      this->ActiveNode->AddNestedElement(ellipse);
      this->ApplyPenStateToNode(ellipse);
      this->ApplyBrushStateToNode(ellipse);
      ellipse->SetName("ellipse");
      ellipse->SetFloatAttribute("cx", cx);
      ellipse->SetFloatAttribute("cy", y(cy));
      ellipse->SetFloatAttribute("rx", rX);
      ellipse->SetFloatAttribute("ry", rY);
    }
  }
  else // isArc
  {
    vtkNew<vtkXMLDataElement> path;
    this->ActiveNode->AddNestedElement(path);
    this->ApplyPenStateToNode(path);
    this->ApplyBrushStateToNode(path);
    path->SetName("path");

    std::ostringstream d;
    EllipseHelper helper(cx, cy, rX, rY);
    helper.UpdateDegrees(startAngle);
    d << "M" << helper.X << "," << y(helper.Y) << "\n";
    helper.UpdateDegrees(stopAngle);
    d << "A" << rX << "," << rY << " 0 " << largeArcFlag << " " << sweepFlag << " " << helper.X
      << "," << y(helper.Y) << "\n";
    path->SetAttribute("d", d.str().c_str());
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawString(float* point, const vtkStdString& string)
{
  this->DrawString(point, vtkUnicodeString::from_utf8(string));
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ComputeStringBounds(const vtkStdString& string, float bounds[4])
{
  this->ComputeStringBounds(vtkUnicodeString::from_utf8(string), bounds);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawString(float* point, const vtkUnicodeString& string)
{
  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkErrorMacro("vtkTextRenderer unavailable. Link to vtkRenderingFreeType "
                  "to get the default implementation.");
    return;
  }

  int backend = this->TextAsPath ? vtkTextRenderer::Default : tren->DetectBackend(string);

  if (backend == vtkTextRenderer::FreeType)
  {
    // Embed freetype text and fonts in the SVG:
    FontInfo& info = this->Impl->GetFontInfo(this->TextProp);
    info.ProcessString(string);

    vtkNew<vtkXMLDataElement> text;
    this->ActiveNode->AddNestedElement(text);
    text->SetName("text");
    this->ApplyTextPropertyStateToNode(text, point[0], point[1]);
    // Position is encoded in the transform:
    text->SetFloatAttribute("x", 0.f);
    text->SetFloatAttribute("y", 0.f);

    std::string utf8String = string.utf8_str();
    text->SetCharacterData(utf8String.c_str(), static_cast<int>(utf8String.size()));
  }
  else
  {
    // Export other text (e.g. MathText) as a path:
    vtkNew<vtkPath> tPath;
    int dpi = this->Viewport->GetVTKWindow()->GetDPI();
    if (!tren->StringToPath(this->TextProp, string, tPath, dpi, backend))
    {
      vtkErrorMacro("Error generating path for MathText string '" << string << "'.");
      return;
    }

    vtkNew<vtkXMLDataElement> path;
    this->ActiveNode->AddNestedElement(path);
    path->SetName("path");
    this->ApplyTextPropertyStateToNodeForPath(path, point[0], point[1]);

    std::ostringstream d;
    this->DrawPath(tPath, d);
    path->SetAttribute("d", d.str().c_str());
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ComputeStringBounds(const vtkUnicodeString& string, float bounds[4])
{
  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkErrorMacro("vtkTextRenderer unavailable. Link to vtkRenderingFreeType "
                  "to get the default implementation.");
    std::fill(bounds, bounds + 4, 0.f);
    return;
  }

  assert(this->Viewport && this->Viewport->GetVTKWindow());
  int dpi = this->Viewport->GetVTKWindow()->GetDPI();

  vtkTextRenderer::Metrics m;
  if (!tren->GetMetrics(this->TextProp, string, m, dpi))
  {
    vtkErrorMacro("Error computing bbox for string '" << string << "'.");
    std::fill(bounds, bounds + 4, 0.f);
    return;
  }

  bounds[0] = 0.f;
  bounds[1] = 0.f;
  bounds[2] = static_cast<float>(m.BoundingBox[1] - m.BoundingBox[0] + 1);
  bounds[3] = static_cast<float>(m.BoundingBox[3] - m.BoundingBox[2] + 1);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ComputeJustifiedStringBounds(const char* string, float bounds[4])
{
  this->ComputeStringBounds(vtkUnicodeString::from_utf8(string), bounds);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawMathTextString(float* point, const vtkStdString& str)
{
  this->DrawString(point, str);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawImage(float p[2], float scale, vtkImageData* image)
{
  int dims[3];
  image->GetDimensions(dims);
  dims[0] *= scale;
  dims[1] *= scale;
  this->DrawImage(vtkRectf(p[0], p[1], dims[0], dims[1]), image);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawImage(const vtkRectf& pos, vtkImageData* image)
{
  const ImageInfo& info = this->Impl->GetImageInfo(image);
  YConverter y(this->CanvasHeight);

  const float xScale = pos.GetWidth() / info.Size[0];
  const float yScale = pos.GetHeight() / info.Size[1];

  // Construct a matrix representing the following transformation:
  //
  // [X] = [T3] [T2] [S] [T1]
  //
  // [X]  = final transform
  // [T1] = translate(-pos.X, -pos.Y); Move to origin to prepare for scaling.
  // [S]  = scale(xScale, yScale); Resize the image to match the input rect.
  // [T2] = translate(0, -pos.H); Anchor at bottom corner instead of top
  // [T3] = translate(pos.X, pos.Y); Move back to anchor point
  std::ostringstream xform;
  xform << "matrix(" << xScale << ",0,0," << yScale << "," << pos.GetX() - xScale * pos.GetX()
        << "," << y(pos.GetY()) - (yScale * y(pos.GetY()) + pos.GetHeight()) << ")";

  vtkNew<vtkXMLDataElement> use;
  this->ActiveNode->AddNestedElement(use);
  use->SetName("use");
  use->SetFloatAttribute("x", pos.GetX());
  use->SetFloatAttribute("y", y(pos.GetY()));
  use->SetFloatAttribute("width", pos.GetWidth());
  use->SetFloatAttribute("height", pos.GetHeight());
  use->SetAttribute("transform", xform.str().c_str());
  use->SetAttribute("xlink:href", (std::string("#") + info.Id).c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetColor4(unsigned char[])
{
  // This is how the OpenGL2 impl handles this...
  vtkErrorMacro("color cannot be set this way.");
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  this->Brush->SetTexture(image);
  this->Brush->SetTextureProperties(properties);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetPointSize(float size)
{
  this->Pen->SetWidth(size);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetLineWidth(float width)
{
  this->Pen->SetWidth(width);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetLineType(int type)
{
  this->Pen->SetLineType(type);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetMatrix(vtkMatrix3x3* m)
{
  // Adjust the transform to account for the fact that SVG's y-axis is reversed:
  std::array<double, 9> mat3;
  vtkSVGContextDevice2D::AdjustMatrixForSVG(m->GetData(), mat3.data());

  std::array<double, 16> mat4;
  vtkSVGContextDevice2D::Matrix3ToMatrix4(mat3.data(), mat4.data());

  this->Matrix->SetMatrix(mat4.data());
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::GetMatrix(vtkMatrix3x3* mat3)
{
  vtkSVGContextDevice2D::Matrix4ToMatrix3(this->Matrix->GetMatrix()->GetData(), mat3->GetData());
  vtkSVGContextDevice2D::AdjustMatrixForSVG(mat3->GetData(), mat3->GetData());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::MultiplyMatrix(vtkMatrix3x3* m)
{
  // Adjust the transform to account for the fact that SVG's y-axis is reversed:
  std::array<double, 9> mat3;
  vtkSVGContextDevice2D::AdjustMatrixForSVG(m->GetData(), mat3.data());

  std::array<double, 16> mat4;
  vtkSVGContextDevice2D::Matrix3ToMatrix4(mat3.data(), mat4.data());
  this->Matrix->Concatenate(mat4.data());
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::PushMatrix()
{
  this->Matrix->Push();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::PopMatrix()
{
  this->Matrix->Pop();
  this->ApplyTransform();
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetClipping(int* x)
{
  if (!std::equal(this->ClipRect.begin(), this->ClipRect.end(), x))
  {
    std::copy(x, x + this->ClipRect.size(), this->ClipRect.begin());
    this->SetupClippingAndTransform();
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::EnableClipping(bool enable)
{
  if (enable != this->IsClipping)
  {
    this->IsClipping = enable;
    this->SetupClippingAndTransform();
  }
}

//------------------------------------------------------------------------------
vtkSVGContextDevice2D::vtkSVGContextDevice2D()
  : Impl(new Details)
  , Viewport(nullptr)
  , ContextNode(nullptr)
  , ActiveNode(nullptr)
  , DefinitionNode(nullptr)
  , CanvasHeight(0.f)
  , SubdivisionThreshold(1.f)
  , IsClipping(false)
  , ActiveNodeIsClipping(false)
  , EmbedFonts(false)
  , TextAsPath(true)
{
  std::fill(this->ClipRect.begin(), this->ClipRect.end(), 0);
  std::fill(this->ActiveNodeClipRect.begin(), this->ActiveNodeClipRect.end(), 0);

  std::fill(this->ActiveNodeTransform.begin(), this->ActiveNodeTransform.end(), 0.);
  this->ActiveNodeTransform[0] = 1.;
  this->ActiveNodeTransform[4] = 1.;
  this->ActiveNodeTransform[8] = 1.;
}

//------------------------------------------------------------------------------
vtkSVGContextDevice2D::~vtkSVGContextDevice2D()
{
  this->SetViewport(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::PushGraphicsState()
{
  vtkNew<vtkXMLDataElement> newGState;
  newGState->SetName("g");
  this->ActiveNode->AddNestedElement(newGState);
  this->ActiveNode = newGState;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::PopGraphicsState()
{
  if (this->ActiveNode == this->ContextNode)
  {
    vtkErrorMacro("Internal error: Attempting to pop graphics state past "
                  "context node. This likely means there's a pop with no "
                  "corresponding push.");
    return;
  }

  vtkXMLDataElement* oldActive = this->ActiveNode;
  this->ActiveNode = this->ActiveNode->GetParent();

  // If the old active node is empty, remove it completely:
  if (oldActive->GetNumberOfNestedElements() == 0)
  {
    this->ActiveNode->RemoveNestedElement(oldActive);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::SetupClippingAndTransform()
{
  // To manage transforms and clipping, we don't push/pop/concatenate transforms
  // in the output, and instead only push a single <g> element under the
  // ContextNode with the current transform and clipping formation. Any other
  // calls to PushGraphicsState (for instance, setting a common color to a
  // collection of primitives) should be popped before changing transform or
  // clipping info.

  // If we're more than one node nested under ContextNode, that's an error.
  // See above.
  if (this->ContextNode != this->ActiveNode && this->ContextNode != this->ActiveNode->GetParent())
  {
    vtkErrorMacro("This method must only be called when there is, at most, one "
                  "<g> element between ActiveNode and ContextNode.");
    return;
  }

  // Have the transform/clipping settings actually changed?
  double* mat4 = this->Matrix->GetMatrix()->GetData();
  const bool isClippingChanged = this->IsClipping == this->ActiveNodeIsClipping;
  const bool clipRectChanged =
    !std::equal(this->ClipRect.begin(), this->ClipRect.end(), this->ActiveNodeClipRect.begin());
  const bool transformChanged =
    vtkSVGContextDevice2D::Transform2DEqual(this->ActiveNodeTransform.data(), mat4);
  if (!isClippingChanged && (!this->IsClipping || !clipRectChanged) && !transformChanged)
  {
    return;
  }

  // Sync the cached values:
  vtkSVGContextDevice2D::Matrix4ToMatrix3(mat4, this->ActiveNodeTransform.data());
  std::copy(this->ClipRect.begin(), this->ClipRect.end(), this->ActiveNodeClipRect.begin());
  this->ActiveNodeIsClipping = this->IsClipping;

  // Strip the old transform/clip node out if needed:
  if (this->ActiveNode != this->ContextNode)
  {
    this->PopGraphicsState();
  }
  assert(this->ActiveNode == this->ContextNode);

  // If no clipping or transform is present, no need for a new <g> element,
  // just add new primitives to the ContextNode directly.
  const std::array<double, 9> ident = { { 1., 0., 0., 0., 1., 0., 0., 0., 1. } };
  const bool isIdentity = vtkSVGContextDevice2D::Transform2DEqual(ident.data(), mat4);

  if (!this->IsClipping && isIdentity)
  {
    return;
  }

  // Finally, add new gstate with transform and clipping info.
  this->PushGraphicsState();
  if (!isIdentity)
  {
    this->ActiveNode->SetAttribute(
      "transform", Transform2DToString(this->ActiveNodeTransform).c_str());
  }
  if (this->IsClipping)
  {
    const ClipRectInfo& info = this->Impl->GetClipRectInfo(this->ClipRect);
    this->ActiveNode->SetAttribute("clip-path", (std::string("url(#") + info.Id + ")").c_str());
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenStateToNode(vtkXMLDataElement* node)
{
  this->ApplyPenColorToNode(node);
  this->ApplyPenOpacityToNode(node);
  this->ApplyPenWidthToNode(node);
  this->ApplyPenStippleToNode(node);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenColorToNode(vtkXMLDataElement* node)
{
  node->SetAttribute("stroke", ColorToString(this->Pen->GetColor()).c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenOpacityToNode(vtkXMLDataElement* node)
{
  if (this->Pen->GetOpacity() != 255)
  {
    node->SetFloatAttribute("stroke-opacity", this->Pen->GetOpacity() / 255.f);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenWidthToNode(vtkXMLDataElement* node)
{
  float width = this->GetScaledPenWidth();
  if (std::fabs(width - 1.f) > 1e-5)
  {
    node->SetFloatAttribute("stroke-width", width);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenStippleToNode(vtkXMLDataElement* node)
{
  // These match the OpenGL2 implementation:
  switch (this->Pen->GetLineType())
  {
    default:
      vtkErrorMacro("Unknown line type: " << this->Pen->GetLineType());
      VTK_FALLTHROUGH;

    case vtkPen::NO_PEN:
      node->SetAttribute("stroke-dasharray", "0,10");
      break;

    case vtkPen::SOLID_LINE:
      node->RemoveAttribute("stroke-dasharray");
      break;

    case vtkPen::DASH_LINE:
      node->SetAttribute("stroke-dasharray", "8");
      break;

    case vtkPen::DOT_LINE:
      node->SetAttribute("stroke-dasharray", "1,7");
      break;

    case vtkPen::DASH_DOT_LINE:
      node->SetAttribute("stroke-dasharray", "4,6,2,4");
      break;

    case vtkPen::DASH_DOT_DOT_LINE:
      // This is dash-dot-dash, but eh. It matches the OpenGL2 0x1C47 pattern.
      node->SetAttribute("stroke-dasharray", "3,3,1,3,3,3");
      break;

    case vtkPen::DENSE_DOT_LINE:
      node->SetAttribute("stroke-dasharray", "1,3");
      break;
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenAsFillColorToNode(vtkXMLDataElement* node)
{
  node->SetAttribute("fill", ColorToString(this->Pen->GetColor()).c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyPenAsFillOpacityToNode(vtkXMLDataElement* node)
{
  if (this->Pen->GetOpacity() != 255)
  {
    node->SetFloatAttribute("fill-opacity", this->Pen->GetOpacity() / 255.f);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyBrushStateToNode(vtkXMLDataElement* node)
{
  if (!this->Brush->GetTexture())
  {
    this->ApplyBrushColorToNode(node);
    this->ApplyBrushOpacityToNode(node);
  }
  else
  {
    // Do not apply brush opacity; this matches the OpenGL2 implementation.
    this->ApplyBrushTextureToNode(node);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyBrushColorToNode(vtkXMLDataElement* node)
{
  node->SetAttribute("fill", ColorToString(this->Brush->GetColor()).c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyBrushOpacityToNode(vtkXMLDataElement* node)
{
  if (this->Brush->GetOpacity() != 255)
  {
    node->SetFloatAttribute("fill-opacity", this->Brush->GetOpacity() / 255.f);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyBrushTextureToNode(vtkXMLDataElement* node)
{
  vtkImageData* img = this->Brush->GetTexture();
  int prop = this->Brush->GetTextureProperties();

  const PatternInfo& info = this->Impl->GetPatternInfo(img, prop);
  std::ostringstream fill;
  fill << "url(#" << info.PatternId << ")";
  node->SetAttribute("fill", fill.str().c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyTextPropertyStateToNode(vtkXMLDataElement* node, float x, float y)
{
  vtkFreeTypeTools* ftt = vtkFreeTypeTools::GetInstance();
  if (!ftt)
  {
    vtkErrorMacro("Error embedding fonts: No vtkFreeTypeTools instance "
                  "available.");
    return;
  }

  YConverter yConv(this->CanvasHeight);

  using FaceMetrics = vtkFreeTypeTools::FaceMetrics;
  FaceMetrics faceMetrics = ftt->GetFaceMetrics(this->TextProp);

  vtkVector3d colord;
  this->TextProp->GetColor(colord.GetData());
  vtkColor3ub color = {
    static_cast<unsigned char>((colord[0] * 255.) + 0.5),
    static_cast<unsigned char>((colord[1] * 255.) + 0.5),
    static_cast<unsigned char>((colord[2] * 255.) + 0.5),
  };

  std::ostringstream xform;
  xform << "translate(" << x << "," << yConv(y) << ")";
  if (this->TextProp->GetOrientation() != 0.)
  {
    xform << "rotate(" << this->TextProp->GetOrientation() << ") ";
  }

  std::ostringstream fontSize;
  fontSize << this->TextProp->GetFontSize() << "pt";

  node->SetAttribute("fill", ColorToString(color.GetData()).c_str());
  node->SetFloatAttribute("fill-opacity", static_cast<float>(this->TextProp->GetOpacity()));
  node->SetAttribute("font-family", faceMetrics.FamilyName.c_str());
  node->SetAttribute("font-size", fontSize.str().c_str());
  node->SetAttribute("font-style", this->TextProp->GetItalic() != 0 ? "italic" : "normal");
  node->SetAttribute("font-weight", this->TextProp->GetBold() != 0 ? "bold" : "normal");

  switch (this->TextProp->GetJustification())
  {
    default:
    case VTK_TEXT_LEFT:
      break;

    case VTK_TEXT_CENTERED:
      node->SetAttribute("text-anchor", "middle");
      break;

    case VTK_TEXT_RIGHT:
      node->SetAttribute("text-anchor", "right");
      break;
  }

  switch (this->TextProp->GetVerticalJustification())
  {
    default:
    case VTK_TEXT_BOTTOM:
      node->SetAttribute("alignment-baseline", "bottom");
      break;

    case VTK_TEXT_CENTERED:
      if (this->TextProp->GetUseTightBoundingBox())
      {
        node->SetAttribute("alignment-baseline", "middle");
      }
      else
      {
        node->SetAttribute("alignment-baseline", "central");
      }
      break;

    case VTK_TEXT_TOP:
      node->SetAttribute("alignment-baseline", "top");
      break;
  }

  node->SetAttribute("transform", xform.str().c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyTextPropertyStateToNodeForPath(
  vtkXMLDataElement* node, float x, float y)
{
  vtkVector3d colord;
  this->TextProp->GetColor(colord.GetData());
  vtkColor3ub color = {
    static_cast<unsigned char>((colord[0] * 255.) + 0.5),
    static_cast<unsigned char>((colord[1] * 255.) + 0.5),
    static_cast<unsigned char>((colord[2] * 255.) + 0.5),
  };

  YConverter yConv(this->CanvasHeight);

  std::ostringstream xform;
  xform << "translate(" << x << "," << yConv(y) << ")";

  node->SetAttribute("fill", ColorToString(color.GetData()).c_str());
  node->SetFloatAttribute("fill-opacity", static_cast<float>(this->TextProp->GetOpacity()));

  node->SetAttribute("transform", xform.str().c_str());
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::ApplyTransform()
{
  this->SetupClippingAndTransform();
}

//------------------------------------------------------------------------------
std::string vtkSVGContextDevice2D::AddCrossSymbol(bool)
{
  std::ostringstream idStream;
  idStream << "Cross";

  std::string id = idStream.str();

  if (!this->DefinitionNode->FindNestedElementWithNameAndId("symbol", id.c_str()))
  {
    vtkNew<vtkXMLDataElement> symbol;
    this->DefinitionNode->AddNestedElement(symbol);

    symbol->SetName("symbol");
    symbol->SetId(id.c_str());
    symbol->SetAttribute("id", id.c_str());
    symbol->SetAttribute("viewBox", "0,0 1,1");

    vtkNew<vtkXMLDataElement> path;
    symbol->AddNestedElement(path);

    path->SetName("path");
    path->SetAttribute("d", "M0,0L1,1M0,1L1,0");
  }

  return id;
}

//------------------------------------------------------------------------------
std::string vtkSVGContextDevice2D::AddPlusSymbol(bool)
{
  std::ostringstream idStream;
  idStream << "Plus";

  std::string id = idStream.str();

  if (!this->DefinitionNode->FindNestedElementWithNameAndId("symbol", id.c_str()))
  {
    vtkNew<vtkXMLDataElement> symbol;
    this->DefinitionNode->AddNestedElement(symbol);

    symbol->SetName("symbol");
    symbol->SetId(id.c_str());
    symbol->SetAttribute("id", id.c_str());
    symbol->SetAttribute("viewBox", "0,0 1,1");

    vtkNew<vtkXMLDataElement> path;
    symbol->AddNestedElement(path);

    path->SetName("path");
    path->SetAttribute("d", "M0.5,0L0.5,1M0,0.5L1,0.5");
  }

  return id;
}

//------------------------------------------------------------------------------
std::string vtkSVGContextDevice2D::AddSquareSymbol(bool)
{
  std::ostringstream idStream;
  idStream << "Square";

  std::string id = idStream.str();

  if (!this->DefinitionNode->FindNestedElementWithNameAndId("symbol", id.c_str()))
  {
    vtkNew<vtkXMLDataElement> symbol;
    this->DefinitionNode->AddNestedElement(symbol);

    symbol->SetName("symbol");
    symbol->SetId(id.c_str());
    symbol->SetAttribute("id", id.c_str());
    symbol->SetAttribute("viewBox", "0,0 1,1");

    vtkNew<vtkXMLDataElement> rect;
    symbol->AddNestedElement(rect);

    rect->SetName("rect");
    rect->SetFloatAttribute("x", 0.f);
    rect->SetFloatAttribute("y", 0.f);
    rect->SetFloatAttribute("width", 1.f);
    rect->SetFloatAttribute("height", 1.f);
  }

  return id;
}

//------------------------------------------------------------------------------
std::string vtkSVGContextDevice2D::AddCircleSymbol(bool)
{
  std::ostringstream idStream;
  idStream << "Circle";

  std::string id = idStream.str();

  if (!this->DefinitionNode->FindNestedElementWithNameAndId("symbol", id.c_str()))
  {
    vtkNew<vtkXMLDataElement> symbol;
    this->DefinitionNode->AddNestedElement(symbol);

    symbol->SetName("symbol");
    symbol->SetId(id.c_str());
    symbol->SetAttribute("id", id.c_str());
    symbol->SetAttribute("viewBox", "0,0 1,1");

    vtkNew<vtkXMLDataElement> circle;
    symbol->AddNestedElement(circle);

    circle->SetName("circle");
    circle->SetFloatAttribute("cx", 0.5f);
    circle->SetFloatAttribute("cy", 0.5f);
    circle->SetFloatAttribute("r", 0.5f);
  }

  return id;
}

//------------------------------------------------------------------------------
std::string vtkSVGContextDevice2D::AddDiamondSymbol(bool)
{
  std::ostringstream idStream;
  idStream << "Diamond";

  std::string id = idStream.str();

  if (!this->DefinitionNode->FindNestedElementWithNameAndId("symbol", id.c_str()))
  {
    vtkNew<vtkXMLDataElement> symbol;
    this->DefinitionNode->AddNestedElement(symbol);

    symbol->SetName("symbol");
    symbol->SetId(id.c_str());
    symbol->SetAttribute("id", id.c_str());
    symbol->SetAttribute("viewBox", "0,0 1,1");

    vtkNew<vtkXMLDataElement> path;
    symbol->AddNestedElement(path);

    path->SetName("path");
    path->SetAttribute("d", "M0,.5L.5,1 1,.5 .5,0z");
  }

  return id;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawPath(vtkPath* path, std::ostream& out)
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

  // Use a lambda to invert the y positions for SVG:
  auto y = [](float yIn) -> float { return -yIn; };

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

  // Track the last code so we can save a little space by chaining draw commands
  int lastCode = -1;

  while (code < codeEnd)
  {
    assert("Sanity check" && (code - codeBegin) * 3 == point - pointBegin);

    switch (static_cast<CodeEnum>(*code))
    {
      case vtkPath::MOVE_TO:
        if (lastCode != *code)
        {
          lastCode = *code;
          out << "M";
        }
        out << point[0] << "," << y(point[1]) << "\n";
        point += 3;
        ++code;
        break;

      case vtkPath::LINE_TO:
        if (lastCode != *code)
        {
          lastCode = *code;
          out << "L";
        }
        out << point[0] << "," << y(point[1]) << "\n";
        point += 3;
        ++code;
        break;

      case vtkPath::CONIC_CURVE:
        assert(CodeEnum(code[1]) == vtkPath::CONIC_CURVE);
        if (lastCode != *code)
        {
          lastCode = *code;
          out << "Q";
        }
        out << point[0] << "," << y(point[1]) << " " << point[3] << "," << y(point[4]) << "\n";
        point += 6;
        code += 2;
        break;

      case vtkPath::CUBIC_CURVE:
        assert(CodeEnum(code[1]) == vtkPath::CUBIC_CURVE);
        assert(CodeEnum(code[2]) == vtkPath::CUBIC_CURVE);
        if (lastCode != *code)
        {
          lastCode = *code;
          out << "C";
        }
        out << point[0] << "," << y(point[1]) << " " << point[3] << "," << y(point[4]) << " "
            << point[6] << "," << y(point[7]) << "\n";
        point += 9;
        code += 3;
        break;

      default:
        vtkErrorMacro("Unknown control code.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawLineGradient(const vtkVector2f& p1, const vtkColor4ub& c1,
  const vtkVector2f& p2, const vtkColor4ub& c2, bool useAlpha)
{
  const vtkColor4ub aveColor = { static_cast<unsigned char>(static_cast<int>(c1[0] + c2[0]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[1] + c2[1]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[2] + c2[2]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[3] + c2[3]) / 2) };

  // If the colors are more or less the same, go ahead and draw this segment.
  // Same if the segment is small enough to fit on a single pixel.
  if (this->LengthLessThanTolerance(p1, p2) || this->ColorsAreClose(c1, c2, useAlpha))
  {
    YConverter y(this->CanvasHeight);
    vtkNew<vtkXMLDataElement> line;
    this->ActiveNode->AddNestedElement(line);
    line->SetName("line");
    line->SetFloatAttribute("x1", p1[0]);
    line->SetFloatAttribute("y1", y(p1[1]));
    line->SetFloatAttribute("x2", p2[0]);
    line->SetFloatAttribute("y2", y(p2[1]));
    this->ApplyPenWidthToNode(line);
    line->SetAttribute("stroke", ColorToString(aveColor.GetData()).c_str());
    if (useAlpha && aveColor[3] != 255)
    {
      line->SetFloatAttribute("stroke-opacity", aveColor[3] / 255.f);
    }
    // FIXME: Disable gradient stipple for now, we'd need to account for offsets
    //  this->ApplyPenStippleToNode(node);

    return;
  }

  // Otherwise, subdivide into two more line segments:
  const vtkVector2f avePos = (p1 + p2) * 0.5;

  this->DrawLineGradient(p1, c1, avePos, aveColor, useAlpha);
  this->DrawLineGradient(avePos, aveColor, p2, c2, useAlpha);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::DrawTriangleGradient(const vtkVector2f& p1, const vtkColor4ub& c1,
  const vtkVector2f& p2, const vtkColor4ub& c2, const vtkVector2f& p3, const vtkColor4ub& c3,
  bool useAlpha)
{
  // If the colors are more or less the same, go ahead and draw this triangle.
  // Same if the triangle is small enough to fit on a single pixel.
  if (this->AreaLessThanTolerance(p1, p2, p3) || this->ColorsAreClose(c1, c2, c3, useAlpha))
  {
    YConverter y(this->CanvasHeight);
    const vtkColor4ub aveColor = { static_cast<unsigned char>(
                                     static_cast<int>(c1[0] + c2[0] + c3[0]) / 3),
      static_cast<unsigned char>(static_cast<int>(c1[1] + c2[1] + c3[1]) / 3),
      static_cast<unsigned char>(static_cast<int>(c1[2] + c2[2] + c3[2]) / 3),
      static_cast<unsigned char>(static_cast<int>(c1[3] + c2[3] + c3[3]) / 3) };
    vtkNew<vtkXMLDataElement> polygon;
    this->ActiveNode->AddNestedElement(polygon);
    polygon->SetName("polygon");
    polygon->SetAttribute("fill", ColorToString(aveColor.GetData()).c_str());
    if (useAlpha && aveColor[3] != 255)
    {
      polygon->SetFloatAttribute("fill-opacity", aveColor[3] / 255.f);
    }

    // This should disable antialiasing on supported viewers (works on webkit).
    // Helps prevent visible boundaries between polygons:
    polygon->SetAttribute("shape-rendering", "crispEdges");

    std::ostringstream points;
    points << p1[0] << "," << y(p1[1]) << " " << p2[0] << "," << y(p2[1]) << " " << p3[0] << ","
           << y(p3[1]);
    polygon->SetAttribute("points", points.str().c_str());

    return;
  }

  // Otherwise, subdivide into 4 triangles:
  //           v1
  //            +
  //           /|
  //          / |
  //         /  |
  //        /   |
  //   v12 +----+ v13
  //      /|   /|
  //     / |  / |
  //    /  | /  |
  //   /   |/   |
  //  +----+----+
  // v2   v23   v3
  const vtkVector2f p12 = (p1 + p2) * 0.5;
  const vtkVector2f p23 = (p2 + p3) * 0.5;
  const vtkVector2f p13 = (p1 + p3) * 0.5;
  const vtkColor4ub c12 = { static_cast<unsigned char>(static_cast<int>(c1[0] + c2[0]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[1] + c2[1]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[2] + c2[2]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[3] + c2[3]) / 2) };
  const vtkColor4ub c23 = { static_cast<unsigned char>(static_cast<int>(c2[0] + c3[0]) / 2),
    static_cast<unsigned char>(static_cast<int>(c2[1] + c3[1]) / 2),
    static_cast<unsigned char>(static_cast<int>(c2[2] + c3[2]) / 2),
    static_cast<unsigned char>(static_cast<int>(c2[3] + c3[3]) / 2) };
  const vtkColor4ub c13 = { static_cast<unsigned char>(static_cast<int>(c1[0] + c3[0]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[1] + c3[1]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[2] + c3[2]) / 2),
    static_cast<unsigned char>(static_cast<int>(c1[3] + c3[3]) / 2) };

  this->DrawTriangleGradient(p1, c1, p12, c12, p13, c13, useAlpha);
  this->DrawTriangleGradient(p2, c2, p12, c12, p23, c23, useAlpha);
  this->DrawTriangleGradient(p3, c3, p13, c13, p23, c23, useAlpha);
  this->DrawTriangleGradient(p12, c12, p13, c13, p23, c23, useAlpha);
}

//------------------------------------------------------------------------------
bool vtkSVGContextDevice2D::AreaLessThanTolerance(
  const vtkVector2f& p1, const vtkVector2f& p2, const vtkVector2f& p3)
{
  return this->LengthLessThanTolerance(p1, p2) && this->LengthLessThanTolerance(p1, p3) &&
    this->LengthLessThanTolerance(p2, p3);
}

//------------------------------------------------------------------------------
bool vtkSVGContextDevice2D::LengthLessThanTolerance(const vtkVector2f& p1, const vtkVector2f& p2)
{
  return (p2 - p1).SquaredNorm() < this->SubdivisionThreshold;
}

//------------------------------------------------------------------------------
bool vtkSVGContextDevice2D::ColorsAreClose(
  const vtkColor4ub& c1, const vtkColor4ub& c2, bool useAlpha)
{
  const std::array<unsigned char, 4> tol = { { 16, 8, 32, 32 } };
  int comps = useAlpha ? 4 : 3;
  for (int i = 0; i < comps; ++i)
  {
    if (std::abs(c1[i] - c2[i]) > tol[i])
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkSVGContextDevice2D::ColorsAreClose(
  const vtkColor4ub& c1, const vtkColor4ub& c2, const vtkColor4ub& c3, bool useAlpha)
{
  return (this->ColorsAreClose(c1, c2, useAlpha) && this->ColorsAreClose(c2, c3, useAlpha) &&
    this->ColorsAreClose(c1, c3, useAlpha));
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::WriteFonts()
{
  vtkFreeTypeTools* ftt = vtkFreeTypeTools::GetInstance();
  if (!ftt)
  {
    vtkErrorMacro("Error embedding fonts: No vtkFreeTypeTools instance "
                  "available.");
    return;
  }

  using FaceMetrics = vtkFreeTypeTools::FaceMetrics;
  using GlyphOutline = vtkFreeTypeTools::GlyphOutline;

  for (const auto& fontEntry : this->Impl->FontMap)
  {
    const FontKey& key = fontEntry.first;
    const FontInfo* info = fontEntry.second;
    FaceMetrics faceMetrics = ftt->GetFaceMetrics(key.TextProperty);

    // We only embed scalable fonts for now.
    if (!faceMetrics.Scalable)
    {
      vtkWarningMacro("Cannot embed non-scalable fonts (referring to font file: "
        << key.TextProperty->GetFontFile() << ")");
      continue;
    }

    vtkNew<vtkXMLDataElement> font;
    this->DefinitionNode->AddNestedElement(font);
    font->SetName("font");
    font->SetAttribute("id", info->SVGId.c_str());
    font->SetIntAttribute("horiz-adv-x", faceMetrics.HorizAdvance);

    vtkNew<vtkXMLDataElement> face;
    font->AddNestedElement(face);
    face->SetName("font-face");
    face->SetAttribute("font-family", faceMetrics.FamilyName.c_str());
    face->SetAttribute("font-style", faceMetrics.Italic ? "italic" : "normal");
    face->SetAttribute("font-weight", faceMetrics.Bold ? "bold" : "normal");
    face->SetAttribute("font-size", "all");
    face->SetIntAttribute("units-per-em", faceMetrics.UnitsPerEM);
    face->SetIntAttribute("ascent", faceMetrics.Ascender);
    face->SetIntAttribute("descent", faceMetrics.Descender);
    face->SetAttribute("bbox", BBoxToString(faceMetrics.BoundingBox).c_str());
    face->SetAttribute("alphabetic", "0");

    for (auto charId : info->Chars)
    {
      GlyphOutline glyphInfo = ftt->GetUnscaledGlyphOutline(key.TextProperty, charId);
      vtkUnicodeString unicode(1, charId);

      vtkNew<vtkXMLDataElement> glyph;
      face->AddNestedElement(glyph);
      glyph->SetName("glyph");
      glyph->SetAttributeEncoding(VTK_ENCODING_UTF_8);
      glyph->SetAttribute("unicode", unicode.utf8_str());
      glyph->SetIntAttribute("horiz-adv-x", glyphInfo.HorizAdvance);

      std::ostringstream d;
      this->DrawPath(glyphInfo.Path, d);
      glyph->SetAttribute("d", d.str().c_str());
    }

    for (auto charPair : info->KerningPairs)
    {
      const vtkUnicodeString unicode1(1, charPair.first);
      const vtkUnicodeString unicode2(1, charPair.second);
      std::array<int, 2> kerning =
        ftt->GetUnscaledKerning(key.TextProperty, charPair.first, charPair.second);

      if (std::abs(kerning[0]) == 0)
      {
        continue;
      }

      vtkNew<vtkXMLDataElement> hkern;
      font->AddNestedElement(hkern);
      hkern->SetName("hkern");
      hkern->SetAttributeEncoding(VTK_ENCODING_UTF_8);
      hkern->SetAttribute("u1", unicode1.utf8_str());
      hkern->SetAttribute("u2", unicode2.utf8_str());
      hkern->SetIntAttribute("k", -kerning[0]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::WriteImages()
{
  for (const ImageInfo& info : this->Impl->ImageSet)
  {
    vtkNew<vtkXMLDataElement> image;
    this->DefinitionNode->AddNestedElement(image);
    image->SetName("image");
    image->SetAttribute("id", info.Id.c_str());
    image->SetIntAttribute("width", info.Size[0]);
    image->SetIntAttribute("height", info.Size[1]);
    image->SetAttribute("xlink:href", info.PNGBase64.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::WritePatterns()
{
  for (const PatternInfo& info : this->Impl->PatternSet)
  {
    vtkNew<vtkXMLDataElement> pattern;
    this->DefinitionNode->AddNestedElement(pattern);
    pattern->SetName("pattern");
    pattern->SetAttribute("id", info.PatternId.c_str());

    // We only care about Repeat and Stretch, since SVG doesn't allow control
    // over Nearest/Linear interpolation.
    const bool isTiled = ((info.TextureProperty & vtkBrush::Repeat) != 0);
    if (isTiled)
    {
      pattern->SetIntAttribute("width", info.ImageSize[0]);
      pattern->SetIntAttribute("height", info.ImageSize[1]);
      pattern->SetAttribute("patternUnits", "userSpaceOnUse");
    }
    else // Stretched
    {
      std::ostringstream viewBox;
      viewBox << "0,0," << info.ImageSize[0] << "," << info.ImageSize[1];
      pattern->SetIntAttribute("width", 1);
      pattern->SetIntAttribute("height", 1);
      pattern->SetAttribute("viewBox", viewBox.str().c_str());
      pattern->SetAttribute("preserveAspectRatio", "none");
    }

    vtkNew<vtkXMLDataElement> use;
    pattern->AddNestedElement(use);
    use->SetName("use");
    use->SetFloatAttribute("x", 0);
    use->SetFloatAttribute("y", 0);
    use->SetIntAttribute("width", info.ImageSize[0]);
    use->SetIntAttribute("height", info.ImageSize[1]);
    use->SetAttribute("xlink:href", (std::string("#") + info.ImageId).c_str());
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::WriteClipRects()
{
  for (const auto& info : this->Impl->ClipRectSet)
  {
    vtkNew<vtkXMLDataElement> clipPath;
    this->DefinitionNode->AddNestedElement(clipPath);
    clipPath->SetName("clipPath");
    clipPath->SetAttribute("id", info.Id.c_str());

    // Get rect
    vtkNew<vtkXMLDataElement> rect;
    clipPath->AddNestedElement(rect);
    rect->SetName("rect");
    rect->SetAttribute("fill", "#000");
    rect->SetIntAttribute("x", info.Rect[0]);
    rect->SetIntAttribute("y", info.Rect[1]);
    rect->SetIntAttribute("width", info.Rect[2]);
    rect->SetIntAttribute("height", info.Rect[3]);
  }
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::AdjustMatrixForSVG(const double in[9], double out[9])
{
  // Adjust the transform to account for the fact that SVG's y-axis is reversed:
  //
  // [S] = [T]^-1 [V] [T]
  //
  // [S] is the transform in SVG space (stored in this->Matrix).
  // [V] is the transform in VTK space (inputs from Context2D API).
  //       | 1  0  0 |
  // [T] = | 0 -1  h | where h = viewport height.
  //       | 0  0  1 | [T] flips the y axis.
  // Also, [T] = [T]^-1 in this case.

  std::array<double, 9> tmpMat3;
  std::array<double, 9> VTKToSVG;
  vtkSVGContextDevice2D::GetSVGMatrix(VTKToSVG.data());
  vtkMatrix3x3::Multiply3x3(VTKToSVG.data(), in, tmpMat3.data());
  vtkMatrix3x3::Multiply3x3(tmpMat3.data(), VTKToSVG.data(), out);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::GetSVGMatrix(double svg[9])
{
  svg[0] = 1.;
  svg[1] = 0.;
  svg[2] = 0.;
  svg[3] = 0.;
  svg[4] = -1.;
  svg[5] = this->CanvasHeight;
  svg[6] = 0.;
  svg[7] = 0.;
  svg[8] = 1.;
}

//------------------------------------------------------------------------------
bool vtkSVGContextDevice2D::Transform2DEqual(const double mat3[9], const double mat4[16])
{
  const double tol = 1e-5;

  const std::array<size_t, 6> mat3Map = { { 0, 1, 2, 3, 4, 5 } };
  const std::array<size_t, 6> mat4Map = { { 0, 1, 3, 4, 5, 7 } };

  for (size_t i = 0; i < 6; ++i)
  {
    if (std::fabs(mat3[mat3Map[i]] - mat4[mat4Map[i]]) > tol)
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::Matrix3ToMatrix4(const double mat3[9], double mat4[16])
{
  mat4[0] = mat3[0];
  mat4[1] = mat3[1];
  mat4[2] = 0.;
  mat4[3] = mat3[2];
  mat4[4] = mat3[3];
  mat4[5] = mat3[4];
  mat4[6] = 0.;
  mat4[7] = mat3[5];
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
void vtkSVGContextDevice2D::Matrix4ToMatrix3(const double mat4[16], double mat3[9])
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
float vtkSVGContextDevice2D::GetScaledPenWidth()
{
  float x, y;
  this->GetScaledPenWidth(x, y);
  return (x + y) * 0.5f;
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::GetScaledPenWidth(float& x, float& y)
{
  x = y = this->Pen->GetWidth();
  this->TransformSize(x, y);
}

//------------------------------------------------------------------------------
void vtkSVGContextDevice2D::TransformSize(float& x, float& y)
{
  // Get current 3x3 SVG transform:
  std::array<double, 9> m;
  vtkSVGContextDevice2D::Matrix4ToMatrix3(this->Matrix->GetMatrix()->GetData(), m.data());

  // Invert it (we want to go from local space --> global space)
  vtkMatrix3x3::Invert(m.data(), m.data());

  // Extract the scale values:
  const double xScale = std::copysign(std::sqrt(m[0] * m[0] + m[1] * m[1]), m[0]);
  const double yScale = std::copysign(std::sqrt(m[3] * m[3] + m[4] * m[4]), m[4]);

  x *= static_cast<float>(xScale);
  y *= static_cast<float>(yScale);
}

//------------------------------------------------------------------------------
vtkImageData* vtkSVGContextDevice2D::PreparePointSprite(vtkImageData* in)
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

  if (in->GetNumberOfScalarComponents() == 3)
  { // If RGB, append a constant alpha.
    vtkNew<vtkImageData> rgba;
    rgba->ShallowCopy(in);

    vtkUnsignedCharArray* data =
      vtkArrayDownCast<vtkUnsignedCharArray>(rgba->GetPointData()->GetScalars());
    if (!data)
    {
      vtkErrorMacro("Internal error: vtkImageCast failed.");
      in->UnRegister(this);
      return nullptr;
    }

    vtkIdType numTuples = data->GetNumberOfTuples();
    vtkNew<vtkUnsignedCharArray> newData;
    newData->SetNumberOfComponents(4);
    newData->SetNumberOfTuples(numTuples);

    // Give the compiler an explicit hint about array strides so it can optimize
    // the memory accesses below:
    VTK_ASSUME(data->GetNumberOfComponents() == 3);
    VTK_ASSUME(newData->GetNumberOfComponents() == 4);

    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      newData->SetTypedComponent(t, 0, data->GetTypedComponent(t, 0));
      newData->SetTypedComponent(t, 1, data->GetTypedComponent(t, 1));
      newData->SetTypedComponent(t, 2, data->GetTypedComponent(t, 2));
      newData->SetTypedComponent(t, 3, 255);
    }
    rgba->GetPointData()->SetScalars(newData);

    in->UnRegister(this);
    in = rgba;
    in->Register(this);
  }

  return in;
}
