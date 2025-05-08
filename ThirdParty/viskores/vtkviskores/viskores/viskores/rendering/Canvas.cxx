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

#include <viskores/rendering/Canvas.h>

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/io/DecodePNG.h>
#include <viskores/io/EncodePNG.h>
#include <viskores/io/FileUtils.h>
#include <viskores/io/ImageUtils.h>
#include <viskores/rendering/BitmapFontFactory.h>
#include <viskores/rendering/LineRenderer.h>
#include <viskores/rendering/TextRenderer.h>
#include <viskores/rendering/TextRendererBatcher.h>
#include <viskores/rendering/WorldAnnotator.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <fstream>
#include <iostream>

namespace viskores
{
namespace rendering
{
namespace internal
{

struct ClearBuffers : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_CONT
  ClearBuffers() {}

  VISKORES_EXEC
  void operator()(viskores::Vec4f_32& color, viskores::Float32& depth) const
  {
    color[0] = 0.f;
    color[1] = 0.f;
    color[2] = 0.f;
    color[3] = 0.f;
    // The depth is set to slightly larger than 1.0f, ensuring this color value always fails a
    // depth check
    depth = VISKORES_DEFAULT_CANVAS_DEPTH;
  }
}; // struct ClearBuffers

struct BlendBackground : public viskores::worklet::WorkletMapField
{
  viskores::Vec4f_32 BackgroundColor;

  VISKORES_CONT
  BlendBackground(const viskores::Vec4f_32& backgroundColor)
    : BackgroundColor(backgroundColor)
  {
  }

  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  VISKORES_EXEC void operator()(viskores::Vec4f_32& color) const
  {
    if (color[3] >= 1.f)
      return;

    viskores::Float32 alpha = BackgroundColor[3] * (1.f - color[3]);
    color[0] = color[0] + BackgroundColor[0] * alpha;
    color[1] = color[1] + BackgroundColor[1] * alpha;
    color[2] = color[2] + BackgroundColor[2] * alpha;
    color[3] = alpha + color[3];
  }
}; // struct BlendBackground

struct DrawColorSwatch : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_CONT
  DrawColorSwatch(viskores::Id2 dims,
                  viskores::Id2 xBounds,
                  viskores::Id2 yBounds,
                  const viskores::Vec4f_32 color)
    : Color(color)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    SwatchBottomLeft[0] = xBounds[0];
    SwatchBottomLeft[1] = yBounds[0];
    SwatchWidth = xBounds[1] - xBounds[0];
    SwatchHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer>
  VISKORES_EXEC void operator()(const viskores::Id& index, FrameBuffer& frameBuffer) const
  {
    // local bar coord
    viskores::Id x = index % SwatchWidth;
    viskores::Id y = index / SwatchWidth;

    // offset to global image coord
    x += SwatchBottomLeft[0];
    y += SwatchBottomLeft[1];

    viskores::Id offset = y * ImageWidth + x;
    frameBuffer.Set(offset, Color);
  }

  viskores::Id ImageWidth;
  viskores::Id ImageHeight;
  viskores::Id2 SwatchBottomLeft;
  viskores::Id SwatchWidth;
  viskores::Id SwatchHeight;
  const viskores::Vec4f_32 Color;
}; // struct DrawColorSwatch

struct DrawColorBar : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  DrawColorBar(viskores::Id2 dims, viskores::Id2 xBounds, viskores::Id2 yBounds, bool horizontal)
    : Horizontal(horizontal)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    BarBottomLeft[0] = xBounds[0];
    BarBottomLeft[1] = yBounds[0];
    BarWidth = xBounds[1] - xBounds[0];
    BarHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer, typename ColorMap>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                FrameBuffer& frameBuffer,
                                const ColorMap& colorMap) const
  {
    // local bar coord
    viskores::Id x = index % BarWidth;
    viskores::Id y = index / BarWidth;
    viskores::Id sample = Horizontal ? x : y;


    const viskores::Vec4ui_8 color = colorMap.Get(sample);

    viskores::Float32 normalizedHeight = Horizontal
      ? static_cast<viskores::Float32>(y) / static_cast<viskores::Float32>(BarHeight)
      : static_cast<viskores::Float32>(x) / static_cast<viskores::Float32>(BarWidth);
    // offset to global image coord
    x += BarBottomLeft[0];
    y += BarBottomLeft[1];

    viskores::Id offset = y * ImageWidth + x;
    // If the colortable has alpha values, we blend each color sample with translucent white.
    // The height of the resultant translucent bar indicates the opacity.

    constexpr viskores::Float32 conversionToFloatSpace = (1.0f / 255.0f);
    viskores::Float32 alpha = color[3] * conversionToFloatSpace;
    if (alpha < 1 && normalizedHeight <= alpha)
    {
      constexpr viskores::Float32 intensity = 0.4f;
      constexpr viskores::Float32 inverseIntensity = (1.0f - intensity);
      alpha *= inverseIntensity;
      viskores::Vec4f_32 blendedColor(
        1.0f * intensity + (color[0] * conversionToFloatSpace) * alpha,
        1.0f * intensity + (color[1] * conversionToFloatSpace) * alpha,
        1.0f * intensity + (color[2] * conversionToFloatSpace) * alpha,
        1.0f);
      frameBuffer.Set(offset, blendedColor);
    }
    else
    {
      // make sure this is opaque
      viskores::Vec4f_32 fColor((color[0] * conversionToFloatSpace),
                                (color[1] * conversionToFloatSpace),
                                (color[2] * conversionToFloatSpace),
                                1.0f);
      frameBuffer.Set(offset, fColor);
    }
  }

  viskores::Id ImageWidth;
  viskores::Id ImageHeight;
  viskores::Id2 BarBottomLeft;
  viskores::Id BarWidth;
  viskores::Id BarHeight;
  bool Horizontal;
}; // struct DrawColorBar

} // namespace internal

struct Canvas::CanvasInternals
{

  CanvasInternals(viskores::Id width, viskores::Id height)
    : Width(width)
    , Height(height)
  {
    BackgroundColor.Components[0] = 0.f;
    BackgroundColor.Components[1] = 0.f;
    BackgroundColor.Components[2] = 0.f;
    BackgroundColor.Components[3] = 1.f;

    ForegroundColor.Components[0] = 1.f;
    ForegroundColor.Components[1] = 1.f;
    ForegroundColor.Components[2] = 1.f;
    ForegroundColor.Components[3] = 1.f;
  }

  viskores::Id Width;
  viskores::Id Height;
  viskores::rendering::Color BackgroundColor;
  viskores::rendering::Color ForegroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
  viskores::rendering::BitmapFont Font;
  FontTextureType FontTexture;
  viskores::Matrix<viskores::Float32, 4, 4> ModelView;
  viskores::Matrix<viskores::Float32, 4, 4> Projection;
  std::shared_ptr<viskores::rendering::TextRendererBatcher> TextBatcher;
};

Canvas::Canvas(viskores::Id width, viskores::Id height)
  : Internals(new CanvasInternals(0, 0))
{
  viskores::MatrixIdentity(Internals->ModelView);
  viskores::MatrixIdentity(Internals->Projection);
  this->ResizeBuffers(width, height);
}

Canvas::~Canvas() {}

viskores::rendering::Canvas* Canvas::NewCopy() const
{
  return new viskores::rendering::Canvas(*this);
}

viskores::Id Canvas::GetWidth() const
{
  return Internals->Width;
}

viskores::Id Canvas::GetHeight() const
{
  return Internals->Height;
}

const Canvas::ColorBufferType& Canvas::GetColorBuffer() const
{
  return Internals->ColorBuffer;
}

Canvas::ColorBufferType& Canvas::GetColorBuffer()
{
  return Internals->ColorBuffer;
}

const Canvas::DepthBufferType& Canvas::GetDepthBuffer() const
{
  return Internals->DepthBuffer;
}

Canvas::DepthBufferType& Canvas::GetDepthBuffer()
{
  return Internals->DepthBuffer;
}

viskores::cont::DataSet Canvas::GetDataSet(const std::string& colorFieldName,
                                           const std::string& depthFieldName) const
{
  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet dataSet =
    builder.Create(viskores::Id2(this->GetWidth(), this->GetHeight()));
  if (!colorFieldName.empty())
  {
    dataSet.AddPointField(colorFieldName, this->GetColorBuffer());
  }
  if (!depthFieldName.empty())
  {
    dataSet.AddPointField(depthFieldName, this->GetDepthBuffer());
  }
  return dataSet;
}

viskores::cont::DataSet Canvas::GetDataSet(const char* colorFieldName,
                                           const char* depthFieldName) const
{
  return this->GetDataSet((colorFieldName != nullptr) ? std::string(colorFieldName) : std::string(),
                          (depthFieldName != nullptr) ? std::string(depthFieldName)
                                                      : std::string());
}

const viskores::rendering::Color& Canvas::GetBackgroundColor() const
{
  return Internals->BackgroundColor;
}

void Canvas::SetBackgroundColor(const viskores::rendering::Color& color)
{
  Internals->BackgroundColor = color;
}

const viskores::rendering::Color& Canvas::GetForegroundColor() const
{
  return Internals->ForegroundColor;
}

void Canvas::SetForegroundColor(const viskores::rendering::Color& color)
{
  Internals->ForegroundColor = color;
}

void Canvas::Clear()
{
  internal::ClearBuffers worklet;
  viskores::worklet::DispatcherMapField<internal::ClearBuffers> dispatcher(worklet);
  dispatcher.Invoke(this->GetColorBuffer(), this->GetDepthBuffer());
}

void Canvas::BlendBackground()
{
  internal::BlendBackground worklet(GetBackgroundColor().Components);
  viskores::worklet::DispatcherMapField<internal::BlendBackground> dispatcher(worklet);
  dispatcher.Invoke(this->GetColorBuffer());
}

void Canvas::ResizeBuffers(viskores::Id width, viskores::Id height)
{
  VISKORES_ASSERT(width >= 0);
  VISKORES_ASSERT(height >= 0);

  viskores::Id numPixels = width * height;
  if (Internals->ColorBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->ColorBuffer.Allocate(numPixels);
  }
  if (Internals->DepthBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->DepthBuffer.Allocate(numPixels);
  }

  Internals->Width = width;
  Internals->Height = height;
}

void Canvas::AddColorSwatch(const viskores::Vec2f_64& point0,
                            const viskores::Vec2f_64& viskoresNotUsed(point1),
                            const viskores::Vec2f_64& point2,
                            const viskores::Vec2f_64& viskoresNotUsed(point3),
                            const viskores::rendering::Color& color) const
{
  viskores::Float64 width = static_cast<viskores::Float64>(this->GetWidth());
  viskores::Float64 height = static_cast<viskores::Float64>(this->GetHeight());

  viskores::Id2 x, y;
  x[0] = static_cast<viskores::Id>(((point0[0] + 1.) / 2.) * width + .5);
  x[1] = static_cast<viskores::Id>(((point2[0] + 1.) / 2.) * width + .5);
  y[0] = static_cast<viskores::Id>(((point0[1] + 1.) / 2.) * height + .5);
  y[1] = static_cast<viskores::Id>(((point2[1] + 1.) / 2.) * height + .5);

  viskores::Id2 dims(this->GetWidth(), this->GetHeight());

  viskores::Id totalPixels = (x[1] - x[0]) * (y[1] - y[0]);
  viskores::cont::ArrayHandleCounting<viskores::Id> iterator(0, 1, totalPixels);
  viskores::worklet::DispatcherMapField<internal::DrawColorSwatch> dispatcher(
    internal::DrawColorSwatch(dims, x, y, color.Components));
  dispatcher.Invoke(iterator, this->GetColorBuffer());
}

void Canvas::AddColorSwatch(const viskores::Float64 x0,
                            const viskores::Float64 y0,
                            const viskores::Float64 x1,
                            const viskores::Float64 y1,
                            const viskores::Float64 x2,
                            const viskores::Float64 y2,
                            const viskores::Float64 x3,
                            const viskores::Float64 y3,
                            const viskores::rendering::Color& color) const
{
  this->AddColorSwatch(viskores::make_Vec(x0, y0),
                       viskores::make_Vec(x1, y1),
                       viskores::make_Vec(x2, y2),
                       viskores::make_Vec(x3, y3),
                       color);
}

void Canvas::AddLine(const viskores::Vec2f_64& point0,
                     const viskores::Vec2f_64& point1,
                     viskores::Float32 linewidth,
                     const viskores::rendering::Color& color) const
{
  viskores::rendering::Canvas* self = const_cast<viskores::rendering::Canvas*>(this);
  viskores::rendering::LineRendererBatcher lineBatcher;
  LineRenderer renderer(
    self, viskores::MatrixMultiply(Internals->Projection, Internals->ModelView), &lineBatcher);
  renderer.RenderLine(point0, point1, linewidth, color);
  lineBatcher.Render(self);
}

void Canvas::AddLine(viskores::Float64 x0,
                     viskores::Float64 y0,
                     viskores::Float64 x1,
                     viskores::Float64 y1,
                     viskores::Float32 linewidth,
                     const viskores::rendering::Color& color) const
{
  this->AddLine(viskores::make_Vec(x0, y0), viskores::make_Vec(x1, y1), linewidth, color);
}

void Canvas::AddColorBar(const viskores::Bounds& bounds,
                         const viskores::cont::ColorTable& colorTable,
                         bool horizontal) const
{
  viskores::Float64 width = static_cast<viskores::Float64>(this->GetWidth());
  viskores::Float64 height = static_cast<viskores::Float64>(this->GetHeight());

  viskores::Id2 x, y;
  x[0] = static_cast<viskores::Id>(((bounds.X.Min + 1.) / 2.) * width + .5);
  x[1] = static_cast<viskores::Id>(((bounds.X.Max + 1.) / 2.) * width + .5);
  y[0] = static_cast<viskores::Id>(((bounds.Y.Min + 1.) / 2.) * height + .5);
  y[1] = static_cast<viskores::Id>(((bounds.Y.Max + 1.) / 2.) * height + .5);
  viskores::Id barWidth = x[1] - x[0];
  viskores::Id barHeight = y[1] - y[0];

  viskores::Id numSamples = horizontal ? barWidth : barHeight;
  viskores::cont::ArrayHandle<viskores::Vec4ui_8> colorMap;

  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(viskores::cont::DeviceAdapterTagSerial{});
    colorTable.Sample(static_cast<viskores::Int32>(numSamples), colorMap);
  }

  viskores::Id2 dims(this->GetWidth(), this->GetHeight());

  viskores::Id totalPixels = (x[1] - x[0]) * (y[1] - y[0]);
  viskores::cont::ArrayHandleCounting<viskores::Id> iterator(0, 1, totalPixels);
  viskores::worklet::DispatcherMapField<internal::DrawColorBar> dispatcher(
    internal::DrawColorBar(dims, x, y, horizontal));
  dispatcher.Invoke(iterator, this->GetColorBuffer(), colorMap);
}

void Canvas::AddColorBar(viskores::Float32 x,
                         viskores::Float32 y,
                         viskores::Float32 width,
                         viskores::Float32 height,
                         const viskores::cont::ColorTable& colorTable,
                         bool horizontal) const
{
  this->AddColorBar(viskores::Bounds(viskores::Range(x, x + width),
                                     viskores::Range(y, y + height),
                                     viskores::Range(0, 0)),
                    colorTable,
                    horizontal);
}

viskores::Id2 Canvas::GetScreenPoint(
  viskores::Float32 x,
  viskores::Float32 y,
  viskores::Float32 z,
  const viskores::Matrix<viskores::Float32, 4, 4>& transform) const
{
  viskores::Vec4f_32 point(x, y, z, 1.0f);
  point = viskores::MatrixMultiply(transform, point);

  viskores::Id2 pixelPos;
  viskores::Float32 width = static_cast<viskores::Float32>(Internals->Width);
  viskores::Float32 height = static_cast<viskores::Float32>(Internals->Height);
  pixelPos[0] = static_cast<viskores::Id>(viskores::Round((1.0f + point[0]) * width * 0.5f + 0.5f));
  pixelPos[1] =
    static_cast<viskores::Id>(viskores::Round((1.0f + point[1]) * height * 0.5f + 0.5f));
  return pixelPos;
}

void Canvas::AddText(const viskores::Matrix<viskores::Float32, 4, 4>& transform,
                     viskores::Float32 scale,
                     const viskores::Vec2f_32& anchor,
                     const viskores::rendering::Color& color,
                     const std::string& text,
                     const viskores::Float32& depth) const
{
  if (!this->EnsureFontLoaded() || !this->Internals->TextBatcher)
  {
    return;
  }

  viskores::rendering::Canvas* self = const_cast<viskores::rendering::Canvas*>(this);
  viskores::rendering::TextRenderer fontRenderer(
    self, Internals->Font, Internals->FontTexture, this->Internals->TextBatcher.get());
  fontRenderer.RenderText(transform, scale, anchor, color, text, depth);
}

void Canvas::AddText(const viskores::Vec2f_32& position,
                     viskores::Float32 scale,
                     viskores::Float32 angle,
                     viskores::Float32 windowAspect,
                     const viskores::Vec2f_32& anchor,
                     const viskores::rendering::Color& color,
                     const std::string& text) const
{
  viskores::Matrix<viskores::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  viskores::Matrix<viskores::Float32, 4, 4> scaleMatrix =
    Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  viskores::Vec3f_32 rotationAxis(0.0f, 0.0f, 1.0f);
  viskores::Matrix<viskores::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  viskores::Matrix<viskores::Float32, 4, 4> transform = viskores::MatrixMultiply(
    translationMatrix, viskores::MatrixMultiply(scaleMatrix, rotationMatrix));

  this->AddText(transform, scale, anchor, color, text, 0.f);
}

void Canvas::AddText(viskores::Float32 x,
                     viskores::Float32 y,
                     viskores::Float32 scale,
                     viskores::Float32 angle,
                     viskores::Float32 windowAspect,
                     viskores::Float32 anchorX,
                     viskores::Float32 anchorY,
                     const viskores::rendering::Color& color,
                     const std::string& text) const
{
  this->AddText(viskores::make_Vec(x, y),
                scale,
                angle,
                windowAspect,
                viskores::make_Vec(anchorX, anchorY),
                color,
                text);
}

void Canvas::BeginTextRenderingBatch() const
{
  if (!this->EnsureFontLoaded() || this->Internals->TextBatcher)
  {
    return;
  }

  this->Internals->TextBatcher =
    std::make_shared<viskores::rendering::TextRendererBatcher>(this->Internals->FontTexture);
}

void Canvas::EndTextRenderingBatch() const
{
  if (!this->Internals->TextBatcher)
  {
    return;
  }

  this->Internals->TextBatcher->Render(this);
  this->Internals->TextBatcher.reset();
}

bool Canvas::EnsureFontLoaded() const
{
  if (!Internals->FontTexture.IsValid())
  {
    if (!LoadFont())
    {
      return false;
    }
  }
  return true;
}

bool Canvas::LoadFont() const
{
  Internals->Font = BitmapFontFactory::CreateLiberation2Sans();
  const std::vector<unsigned char>& rawPNG = Internals->Font.GetRawImageData();
  std::vector<unsigned char> rgba;
  unsigned long textureWidth, textureHeight;
  auto error = io::DecodePNG(rgba, textureWidth, textureHeight, &rawPNG[0], rawPNG.size());
  if (error != 0)
  {
    return false;
  }
  viskores::Id numValues = static_cast<viskores::Id>(textureWidth * textureHeight);
  viskores::cont::ArrayHandle<UInt8> alpha;
  alpha.Allocate(numValues);
  auto alphaPortal = alpha.WritePortal();
  for (viskores::Id i = 0; i < numValues; ++i)
  {
    alphaPortal.Set(i, rgba[static_cast<std::size_t>(i * 4 + 3)]);
  }
  Internals->FontTexture =
    FontTextureType(viskores::Id(textureWidth), viskores::Id(textureHeight), alpha);
  Internals->FontTexture.SetFilterMode(TextureFilterMode::Linear);
  Internals->FontTexture.SetWrapMode(TextureWrapMode::Clamp);
  return true;
}

const viskores::Matrix<viskores::Float32, 4, 4>& Canvas::GetModelView() const
{
  return Internals->ModelView;
}

const viskores::Matrix<viskores::Float32, 4, 4>& Canvas::GetProjection() const
{
  return Internals->Projection;
}

void Canvas::SetViewToWorldSpace(const viskores::rendering::Camera& camera,
                                 bool viskoresNotUsed(clip))
{
  Internals->ModelView = camera.CreateViewMatrix();
  Internals->Projection = camera.CreateProjectionMatrix(GetWidth(), GetHeight());
}

void Canvas::SetViewToScreenSpace(const viskores::rendering::Camera& viskoresNotUsed(camera),
                                  bool viskoresNotUsed(clip))
{
  viskores::MatrixIdentity(Internals->ModelView);
  viskores::MatrixIdentity(Internals->Projection);
  Internals->Projection[2][2] = -1.0f;
}

void Canvas::SaveAs(const std::string& fileName) const
{
  this->RefreshColorBuffer();
  ColorBufferType::ReadPortalType colorPortal = GetColorBuffer().ReadPortal();
  viskores::Id width = GetWidth();
  viskores::Id height = GetHeight();

  if (viskores::io::EndsWith(fileName, ".png"))
  {
    std::vector<unsigned char> img(static_cast<size_t>(4 * width * height));
    for (viskores::Id yIndex = height - 1; yIndex >= 0; yIndex--)
    {
      for (viskores::Id xIndex = 0; xIndex < width; xIndex++)
      {
        viskores::Vec4f_32 tuple = colorPortal.Get(yIndex * width + xIndex);
        // y = 0 is the top of a .png file.
        size_t idx = static_cast<size_t>(4 * width * (height - 1 - yIndex) + 4 * xIndex);
        img[idx + 0] = (unsigned char)(tuple[0] * 255);
        img[idx + 1] = (unsigned char)(tuple[1] * 255);
        img[idx + 2] = (unsigned char)(tuple[2] * 255);
        img[idx + 3] = (unsigned char)(tuple[3] * 255);
      }
    }

    viskores::io::SavePNG(
      fileName, img, static_cast<unsigned long>(width), static_cast<unsigned long>(height));
    return;
  }

  std::ofstream of(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
  of << "P6" << std::endl << width << " " << height << std::endl << 255 << std::endl;
  for (viskores::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (viskores::Id xIndex = 0; xIndex < width; xIndex++)
    {
      viskores::Vec4f_32 tuple = colorPortal.Get(yIndex * width + xIndex);
      of << (unsigned char)(tuple[0] * 255);
      of << (unsigned char)(tuple[1] * 255);
      of << (unsigned char)(tuple[2] * 255);
    }
  }
  of.close();
}

viskores::rendering::WorldAnnotator* Canvas::CreateWorldAnnotator() const
{
  return new viskores::rendering::WorldAnnotator(this);
}
}
} // viskores::rendering
