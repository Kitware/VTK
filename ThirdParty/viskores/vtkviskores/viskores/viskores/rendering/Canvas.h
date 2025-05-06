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

#ifndef viskores_rendering_Canvas_h
#define viskores_rendering_Canvas_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/Matrix.h>
#include <viskores/Types.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/BitmapFont.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/Texture2D.h>

#define VISKORES_DEFAULT_CANVAS_DEPTH 1.001f

namespace viskores
{
namespace rendering
{

class WorldAnnotator;

/// @brief Represents the image space that is the target of rendering.
class VISKORES_RENDERING_EXPORT Canvas
{
public:
  using ColorBufferType = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  using DepthBufferType = viskores::cont::ArrayHandle<viskores::Float32>;
  using FontTextureType = viskores::rendering::Texture2D<1>;

  /// Construct a canvas of a given width and height.
  Canvas(viskores::Id width = 1024, viskores::Id height = 1024);
  virtual ~Canvas();

  /// Create a new `Canvas` object of the same subtype as this one.
  virtual viskores::rendering::Canvas* NewCopy() const;

  /// @brief Clear out the image buffers.
  virtual void Clear();

  /// @brief Blend the foreground data with the background color.
  ///
  /// When a render is started, it is given a zeroed background rather than the
  /// background color specified by `SetBackgroundColor()`. This is because when
  /// blending pixel fragments of transparent objects the background color can
  /// interfere. Call this method after the render is completed for the final
  /// blend to get the proper background color.
  virtual void BlendBackground();

  /// @brief The width of the image.
  VISKORES_CONT
  viskores::Id GetWidth() const;

  /// @brief The height of the image.
  VISKORES_CONT
  viskores::Id GetHeight() const;

  /// @brief Get the color channels of the image.
  VISKORES_CONT
  const ColorBufferType& GetColorBuffer() const;

  /// @copydoc GetColorBuffer
  VISKORES_CONT
  ColorBufferType& GetColorBuffer();

  /// @brief Get the depth channel of the image.
  VISKORES_CONT
  const DepthBufferType& GetDepthBuffer() const;

  /// @copydoc GetDepthBuffer
  VISKORES_CONT
  DepthBufferType& GetDepthBuffer();

  /// \brief Gets the image in this `Canvas` as a `viskores::cont::DataSet`.
  ///
  /// The returned `DataSet` will be a uniform structured 2D grid. The color and depth
  /// buffers will be attached as field with the given names. If the name for the color
  /// or depth field is empty, then that respective field will not be added.
  ///
  /// The arrays of the color and depth buffer are shallow copied. Thus, changes in
  /// the `Canvas` may cause unexpected behavior in the `DataSet`.
  ///
  VISKORES_CONT viskores::cont::DataSet GetDataSet(
    const std::string& colorFieldName = "color",
    const std::string& depthFieldName = "depth") const;
  /// @copydoc GetDataSet
  VISKORES_CONT viskores::cont::DataSet GetDataSet(const char* colorFieldName,
                                                   const char* depthFieldName = "depth") const;

  /// @brief Change the size of the image.
  VISKORES_CONT
  void ResizeBuffers(viskores::Id width, viskores::Id height);

  /// @brief Specify the background color.
  VISKORES_CONT
  const viskores::rendering::Color& GetBackgroundColor() const;

  /// @copydoc GetBackgroundColor
  VISKORES_CONT
  void SetBackgroundColor(const viskores::rendering::Color& color);

  /// @brief Specify the foreground color used for annotations.
  VISKORES_CONT
  const viskores::rendering::Color& GetForegroundColor() const;

  /// @copydoc GetForegroundColor
  VISKORES_CONT
  void SetForegroundColor(const viskores::rendering::Color& color);

  VISKORES_CONT
  viskores::Id2 GetScreenPoint(viskores::Float32 x,
                               viskores::Float32 y,
                               viskores::Float32 z,
                               const viskores::Matrix<viskores::Float32, 4, 4>& transfor) const;

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  virtual void RefreshColorBuffer() const {}
  virtual void RefreshDepthBuffer() const {}

  virtual void SetViewToWorldSpace(const viskores::rendering::Camera& camera, bool clip);
  virtual void SetViewToScreenSpace(const viskores::rendering::Camera& camera, bool clip);
  virtual void SetViewportClipping(const viskores::rendering::Camera&, bool) {}

  /// @brief Save the rendered image.
  ///
  /// If the filename ends with ".png", it will be saved in the portable network
  /// graphic format. Otherwise, the file will be saved in Netbpm portable pixmap format.
  virtual void SaveAs(const std::string& fileName) const;

  /// Creates a WorldAnnotator of a type that is paired with this Canvas. Other
  /// types of world annotators might work, but this provides a default.
  ///
  /// The WorldAnnotator is created with the C++ new keyword (so it should be
  /// deleted with delete later). A pointer to the created WorldAnnotator is
  /// returned.
  ///
  virtual viskores::rendering::WorldAnnotator* CreateWorldAnnotator() const;

  VISKORES_CONT
  virtual void AddColorSwatch(const viskores::Vec2f_64& point0,
                              const viskores::Vec2f_64& point1,
                              const viskores::Vec2f_64& point2,
                              const viskores::Vec2f_64& point3,
                              const viskores::rendering::Color& color) const;

  VISKORES_CONT
  void AddColorSwatch(const viskores::Float64 x0,
                      const viskores::Float64 y0,
                      const viskores::Float64 x1,
                      const viskores::Float64 y1,
                      const viskores::Float64 x2,
                      const viskores::Float64 y2,
                      const viskores::Float64 x3,
                      const viskores::Float64 y3,
                      const viskores::rendering::Color& color) const;

  VISKORES_CONT
  virtual void AddLine(const viskores::Vec2f_64& point0,
                       const viskores::Vec2f_64& point1,
                       viskores::Float32 linewidth,
                       const viskores::rendering::Color& color) const;

  VISKORES_CONT
  void AddLine(viskores::Float64 x0,
               viskores::Float64 y0,
               viskores::Float64 x1,
               viskores::Float64 y1,
               viskores::Float32 linewidth,
               const viskores::rendering::Color& color) const;

  VISKORES_CONT
  virtual void AddColorBar(const viskores::Bounds& bounds,
                           const viskores::cont::ColorTable& colorTable,
                           bool horizontal) const;

  VISKORES_CONT
  void AddColorBar(viskores::Float32 x,
                   viskores::Float32 y,
                   viskores::Float32 width,
                   viskores::Float32 height,
                   const viskores::cont::ColorTable& colorTable,
                   bool horizontal) const;

  virtual void AddText(const viskores::Vec2f_32& position,
                       viskores::Float32 scale,
                       viskores::Float32 angle,
                       viskores::Float32 windowAspect,
                       const viskores::Vec2f_32& anchor,
                       const viskores::rendering::Color& color,
                       const std::string& text) const;

  VISKORES_CONT
  void AddText(viskores::Float32 x,
               viskores::Float32 y,
               viskores::Float32 scale,
               viskores::Float32 angle,
               viskores::Float32 windowAspect,
               viskores::Float32 anchorX,
               viskores::Float32 anchorY,
               const viskores::rendering::Color& color,
               const std::string& text) const;

  VISKORES_CONT
  void AddText(const viskores::Matrix<viskores::Float32, 4, 4>& transform,
               viskores::Float32 scale,
               const viskores::Vec2f_32& anchor,
               const viskores::rendering::Color& color,
               const std::string& text,
               const viskores::Float32& depth = 0) const;

  VISKORES_CONT
  void BeginTextRenderingBatch() const;

  VISKORES_CONT
  void EndTextRenderingBatch() const;

  friend class AxisAnnotation2D;
  friend class ColorBarAnnotation;
  friend class ColorLegendAnnotation;
  friend class TextAnnotationScreen;
  friend class TextRenderer;
  friend class WorldAnnotator;

private:
  bool LoadFont() const;

  bool EnsureFontLoaded() const;

  const viskores::Matrix<viskores::Float32, 4, 4>& GetModelView() const;

  const viskores::Matrix<viskores::Float32, 4, 4>& GetProjection() const;

  struct CanvasInternals;
  std::shared_ptr<CanvasInternals> Internals;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_Canvas_h
