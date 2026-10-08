// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkWebGPUContextDevice2D
 * @brief   2D context device implementation using WebGPU graphics backend.
 *
 * vtkWebGPUContextDevice2D provides hardware-accelerated 2D rendering primitives
 * (lines, points, polygons, images, and text) for VTK context actors using WebGPU.
 * Features a high-performance ring-buffer sub-allocator and persistent marker caching.
 */

#ifndef vtkWebGPUContextDevice2D_h
#define vtkWebGPUContextDevice2D_h

#include "vtkContextDevice2D.h"
#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtk_wgpu.h"                 // for webgpu type
#include <cstdint>                    // For std::uintptr_t

VTK_ABI_NAMESPACE_BEGIN

class vtkImageData;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkPolyData;
class vtkTransform;
class vtkUnsignedCharArray;
class vtkWebGPURenderWindow;
class vtkRenderer;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUContextDevice2D : public vtkContextDevice2D
{
public:
  vtkTypeMacro(vtkWebGPUContextDevice2D, vtkContextDevice2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Painter object.
   */
  static vtkWebGPUContextDevice2D* New();

  /**
   * @brief Begins the 2D rendering pass for the given viewport.
   * Resets the vertex ring allocator and configures initial matrices.
   */
  void Begin(vtkViewport* viewport) override;

  /**
   * @brief Ends the 2D rendering pass and cleans up temporary render state.
   */
  void End() override;

  /**
   * @brief Flushes the internal batched vertex buffer to the WebGPU rendering pipeline.
   */
  void Flush();

  ///@{
  /**
   * @brief Initialize Device Resources: Uniforms, BindGroups, Pipelines, and Textures.
   */
  bool InitializeDeviceResources();
  void InitializeTexture();
  ///@}

  ///@{
  /**
   * Noop method
   */
  void BufferIdModeBegin(vtkAbstractContextBufferId* /*bufferId*/) override { /*noop*/ }
  void BufferIdModeEnd() override { /*noop*/ }
  void ReleaseCache(std::uintptr_t /*cacheIdentifier*/) override { /*noop*/ }
  ///@}

  /**
   * Draw a poly line using the points - fastest code path due to memory
   * layout of the coordinates. The line will be colored by colors array
   * which has nc_comps components.
   */
  void DrawPoly(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw lines using the points - memory layout is as follows:
   * l1p1,l1p2,l2p1,l2p2... The lines will be colored by colors array
   * which has nc_comps components.
   * @sa DrawPoly()
   */
  void DrawLines(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw a series of points - fastest code path due to memory
   * layout of the coordinates. Points are colored by colors array
   * which has nc_comps components.
   */
  void DrawPoints(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawPoints(
    vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t cacheIdentifier) override;

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied vtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize. Points are colored by colors
   * array which has nc_comps components - this part is optional.
   */
  void DrawPointSprites(vtkImageData* sprite, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawPointSprites(vtkImageData* sprite, vtkDataArray* positions, vtkUnsignedCharArray* colors,
    std::uintptr_t cacheIdentifier) override;

  /**
   * Draw markers of a specified shape at multiple 2D positions.
   *
   * The size of the markers is determined by the current Pen's width. If the pen
   * width is less than or equal to 0, a default size of 7 pixels is used. The shape
   * is generated into a vtkImageData using vtkMarkerUtilities and rendered via
   * DrawPointSprites.
   */
  void DrawMarkers(int shape, bool highlight, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawMarkers(int shape, bool highlight, vtkDataArray* positions, vtkUnsignedCharArray* colors,
    std::uintptr_t cacheIdentifier) override;

  /**
   * Draw a quad using the 4 specified points.
   */
  void DrawQuad(float* points, int n) override;

  /**
   * Draw a quad strip using a sequence of points.
   */
  void DrawQuadStrip(float* points, int n) override;

  /**
   * Draw a filled convex polygon defined by points.
   */
  void DrawPolygon(float* points, int n) override;

  /**
   * Draw a filled polygon with optional per-vertex or per-polygon colors.
   */
  void DrawColoredPolygon(
    float* points, int numPoints, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw an elliptical wedge (pie slice) defined by radii and angles.
   */
  void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle) override;

  /**
   * Draw an open elliptical arc line segment.
   */
  void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) override;

  /**
   * Render a text string at the specified world coordinate point.
   */
  void DrawString(float* point, const vtkStdString& string) override;

  /**
   * Compute the bounding box dimensions for a given text string.
   */
  void ComputeStringBounds(const vtkStdString& string, float bounds[4]) override;

  /**
   * Compute justified bounding box dimensions for a C-style string.
   */
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) override;

  /**
   * Render a math equation string (MathText format) at the given point.
   */
  void DrawMathTextString(float point[2], const vtkStdString& string) override;

  /**
   * Render an image data object scaled and positioned at a given point.
   */
  void DrawImage(float p[2], float scale, vtkImageData* image) override;

  /**
   * Render an image data object within a specified 2D rectangle.
   */
  void DrawImage(const vtkRectf& pos, vtkImageData* image) override;

  /**
   * Render a vtkPolyData geometry mesh (lines and polygons) with offset and scale.
   */
  void DrawPolyData(float p[2], float scale, vtkPolyData* polyData, vtkUnsignedCharArray* colors,
    int scalarMode) override;

  /**
   * Set the color for the device using unsigned char of length 4, RGBA.
   */
  void SetColor4(unsigned char color[4]) override;

  /**
   * Set the color for the device using unsigned char of length 3, RGB.
   */
  virtual void SetColor(unsigned char color[3]);

  /**
   * Set the texture for the device, it is used to fill the polygons
   */
  void SetTexture(vtkImageData* image, int properties = 0) override;

  /**
   * Set the point size for glyphs/sprites.
   */
  void SetPointSize(float size) override;

  /**
   * Set the line width for glyphs/sprites.
   */
  void SetLineWidth(float width) override;

  /**
   * Set the line type type (using anonymous enum in vtkPen).
   */
  void SetLineType(int type) override;

  /**
   * Set the clipping rectangle for the 2D context.
   *
   * This function defines the boundaries for a scissor/clipping box. Any rendering
   * performed while clipping is enabled will be restricted to this rectangle.
   */
  void SetClipping(int* x) override;

  /**
   * Enable or disable clipping.
   *
   * When enabled, subsequent 2D rendering primitives will be clipped to the
   * bounds specified by `SetClipping()`.
   */
  void EnableClipping(bool enable) override;

  /**
   * Multiply the current model view matrix by the supplied one.
   */
  void MultiplyMatrix(vtkMatrix3x3* m) override;

  /**
   * Set the model view matrix for the display
   */
  void SetMatrix(vtkMatrix3x3* m) override;

  /**
   * Set the model view matrix for the display
   */
  void GetMatrix(vtkMatrix3x3* m) override;

  /**
   * Push the current matrix onto the stack.
   */
  void PushMatrix() override;

  /**
   * Pop the current matrix off of the stack.
   */
  void PopMatrix() override;

  /**
   * Release any graphics resources that are being consumed by this device.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow* /*window*/);

protected:
  vtkWebGPUContextDevice2D();
  ~vtkWebGPUContextDevice2D() override;

  vtkRenderer* Renderer = nullptr;
  vtkWebGPURenderWindow* RenderWindow = nullptr;

  class Private;
  Private* Storage;

  vtkTransform* ProjectionMatrix;
  vtkTransform* ModelMatrix;

  bool InRender = false;

private:
  vtkWebGPUContextDevice2D(const vtkWebGPUContextDevice2D&) = delete;
  void operator=(const vtkWebGPUContextDevice2D&) = delete;

  void UpdateCameraUniforms();
  void UpdatePaintUniforms(const unsigned char color[4]);

  /**
   * @brief Create Pipelines and inject constant and primitive
   */
  WGPURenderPipeline CreatePipelineWithConstants(float haveTCoordsVal = 0.0f,
    float haveLinesVal = 0.0f, WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList);

  /**
   * @brief Generates or retrieves a cached anti-aliased marker image for point sprites.
   */
  vtkImageData* GenerateMarker(int shape, int width, bool highlight);

  /**
   * @brief Evaluates clipping rectangles and applies the WebGPU scissor region.
   * Clamps coordinates dynamically against active render window dimensions.
   */
  void ApplyScissorRect(WGPURenderPassEncoder passEncoder);

  ///@{
  /**
   * WebGPU natively only supports 1-pixel wide lines (LineList topology).
   * To support thick lines, DrawThickLines triangulates the line segments
   * into quads and renders them using the Triangle pipeline.
   */
  void DrawThickLines(
    float* points, int n, unsigned char* colors, int nc, const std::vector<float>& distances);

  /**
   * Fast path for 1-pixel wide lines using native WebGPU LineList topology.
   */
  void DrawThinLines(
    float* points, int n, unsigned char* colors, int nc, const std::vector<float>& distances);
  ///@}
};

VTK_ABI_NAMESPACE_END

#endif // vtkWebGPUContextDevice2D_h
