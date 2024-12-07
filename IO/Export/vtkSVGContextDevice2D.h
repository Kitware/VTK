// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSVGContextDevice2D
 * @brief vtkContextDevice2D implementation for use with vtkSVGExporter.
 *
 * Limitations:
 * - The Nearest/Linear texture properties are ignored, since SVG doesn't
 *   provide any reliable control over interpolation.
 * - Embedded fonts are experimental and poorly tested. Viewer support is
 *   lacking at the time of writing, hence the feature is largely useless. By
 *   default, fonts are not embedded since they're basically useless bloat.
 * - TextAsPath is enabled by default, since viewers differ wildly in how they
 *   handle text objects (eg. Inkscape renders at expected size, but webkit is
 *   way too big).
 * - Pattern fills and markers are not shown on some viewers, e.g. KDE's okular
 *   (Webkit seems to work, though).
 * - Clipping seems to be broken in most viewers. Webkit is buggy and forces the
 *   clip coordinates to objectBoundingBox, even when explicitly set to
 *   userSpaceOnUse.
 * - Many viewers anti-alias the output, leaving thin outlines around the
 *   triangles that make up larger polygons. This is a viewer issue and there
 *   not much we can do about it from the VTK side of things (and most viewers
 *   don't seem to have an antialiasing toggle, either...).
 */

#ifndef vtkSVGContextDevice2D_h
#define vtkSVGContextDevice2D_h

#include "vtkContextDevice2D.h"
#include "vtkIOExportModule.h" // For export macro
#include "vtkNew.h"            // For vtkNew!

#include <array> // For std::array!

VTK_ABI_NAMESPACE_BEGIN
class vtkColor3ub;
class vtkColor4ub;
class vtkPath;
class vtkRenderer;
class vtkTransform;
class vtkVector3f;
class vtkXMLDataElement;

class VTKIOEXPORT_EXPORT vtkSVGContextDevice2D : public vtkContextDevice2D
{
public:
  static vtkSVGContextDevice2D* New();
  vtkTypeMacro(vtkSVGContextDevice2D, vtkContextDevice2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /** The svg container element to draw into, and the global definitions
   *  element. */
  void SetSVGContext(vtkXMLDataElement* context, vtkXMLDataElement* defs);

  /**
   * EXPERIMENTAL: If true, the font glyph information will be embedded in the
   * output. Default is false.
   *
   * @note This feature is experimental and not well tested, as most browsers
   * and SVG viewers do not support rendering embedded fonts. As such, enabling
   * this option typically just increases file size for no real benefit.
   *
   * @{
   */
  vtkSetMacro(EmbedFonts, bool);
  vtkGetMacro(EmbedFonts, bool);
  vtkBooleanMacro(EmbedFonts, bool);
  /**@}*/

  /**
   * If true, draw all text as path objects rather than text objects. Enabling
   * this option will:
   *
   * - Improve portability (text will look exactly the same everywhere).
   * - Increase file size (text objects are much more compact than paths).
   * - Prevent text from being easily edited (text metadata is lost).
   *
   * Note that some text (e.g. MathText) is always rendered as a path.
   *
   * The default is true, as many browsers and SVG viewers render text objects
   * inconsistently.
   *
   * @{
   */
  vtkSetMacro(TextAsPath, bool);
  vtkGetMacro(TextAsPath, bool);
  vtkBooleanMacro(TextAsPath, bool);
  /**@}*/

  /**
   * Set the threshold for subdividing gradient-shaded polygons/line. Default
   * value is 1, and lower values yield higher quality and larger files. Larger
   * values will reduce the number of primitives, but will decrease quality.
   *
   * A triangle / line will not be subdivided further if all of it's vertices
   * satisfy the equation:
   *
   * |v1 - v2|^2 < thresh
   *
   * e.g. the squared norm of the vector between any verts must be greater than
   * the threshold for subdivision to occur.
   *
   * @{
   */
  vtkSetMacro(SubdivisionThreshold, float);
  vtkGetMacro(SubdivisionThreshold, float);
  /**@}*/

  /**
   * Write any definition information (fonts, images, etc) that are accumulated
   * between actors.
   */
  void GenerateDefinitions();

  void Begin(vtkViewport*) override;
  void End() override;

  using vtkContextDevice2D::DrawMarkers;
  using vtkContextDevice2D::DrawPoints;
  using vtkContextDevice2D::DrawPointSprites;
  void DrawPoly(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawLines(float* f, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawPoints(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawPointSprites(vtkImageData* sprite, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawMarkers(int shape, bool highlight, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawQuad(float*, int) override;
  void DrawQuadStrip(float*, int) override;
  void DrawPolygon(float*, int) override;
  void DrawColoredPolygon(
    float* points, int numPoints, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle) override;
  void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) override;
  void DrawString(float* point, const vtkStdString& string) override;
  void ComputeStringBounds(const vtkStdString& string, float bounds[4]) override;
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) override;
  void DrawMathTextString(float* point, const vtkStdString& str) override;
  void DrawImage(float p[2], float scale, vtkImageData* image) override;
  void DrawImage(const vtkRectf& pos, vtkImageData* image) override;
  void SetColor4(unsigned char color[4]) override;
  void SetTexture(vtkImageData* image, int properties) override;
  void SetPointSize(float size) override;
  void SetLineWidth(float width) override;

  void SetLineType(int type) override;
  void SetMatrix(vtkMatrix3x3* m) override;
  void GetMatrix(vtkMatrix3x3* m) override;
  void MultiplyMatrix(vtkMatrix3x3* m) override;
  void PushMatrix() override;
  void PopMatrix() override;
  void SetClipping(int* x) override;
  void EnableClipping(bool enable) override;

protected:
  vtkSVGContextDevice2D();
  ~vtkSVGContextDevice2D() override;

  void SetViewport(vtkViewport*);

  void PushGraphicsState();
  void PopGraphicsState();

  // Apply clipping and transform information current active node.
  void SetupClippingAndTransform();

  // pen -> stroke state
  void ApplyPenStateToNode(vtkXMLDataElement* node);
  void ApplyPenColorToNode(vtkXMLDataElement* node);
  void ApplyPenOpacityToNode(vtkXMLDataElement* node);
  void ApplyPenWidthToNode(vtkXMLDataElement* node);
  void ApplyPenStippleToNode(vtkXMLDataElement* node);

  // pen -> fill state
  void ApplyPenAsFillColorToNode(vtkXMLDataElement* node);
  void ApplyPenAsFillOpacityToNode(vtkXMLDataElement* node);

  // brush -> fill state
  void ApplyBrushStateToNode(vtkXMLDataElement* node);
  void ApplyBrushColorToNode(vtkXMLDataElement* node);
  void ApplyBrushOpacityToNode(vtkXMLDataElement* node);
  void ApplyBrushTextureToNode(vtkXMLDataElement* node);

  // tprop --> text state
  void ApplyTextPropertyStateToNode(vtkXMLDataElement* node, float x, float y);
  void ApplyTextPropertyStateToNodeForPath(vtkXMLDataElement* node, float x, float y);

  void ApplyTransform();

  // Add marker symbols to defs, return symbol id.
  std::string AddCrossSymbol(bool highlight);
  std::string AddPlusSymbol(bool highlight);
  std::string AddSquareSymbol(bool highlight);
  std::string AddCircleSymbol(bool highlight);
  std::string AddDiamondSymbol(bool highlight);

  void DrawPath(vtkPath* path, std::ostream& out);

  void DrawLineGradient(const vtkVector2f& p1, const vtkColor4ub& c1, const vtkVector2f& p2,
    const vtkColor4ub& c2, bool useAlpha);
  void DrawTriangleGradient(const vtkVector2f& p1, const vtkColor4ub& c1, const vtkVector2f& p2,
    const vtkColor4ub& c2, const vtkVector2f& p3, const vtkColor4ub& c3, bool useAlpha);

  // Used by the Draw*Gradient methods to prevent subdividing triangles / lines
  // that are already really small.
  bool AreaLessThanTolerance(const vtkVector2f& p1, const vtkVector2f& p2, const vtkVector2f& p3);
  bool LengthLessThanTolerance(const vtkVector2f& p1, const vtkVector2f& p2);

  bool ColorsAreClose(const vtkColor4ub& c1, const vtkColor4ub& c2, bool useAlpha);
  bool ColorsAreClose(
    const vtkColor4ub& c1, const vtkColor4ub& c2, const vtkColor4ub& c3, bool useAlpha);

  void WriteFonts();
  void WriteImages();
  void WritePatterns();
  void WriteClipRects();

  void AdjustMatrixForSVG(const double in[9], double out[9]);
  void GetSVGMatrix(double svg[9]);
  static bool Transform2DEqual(const double mat3[9], const double mat4[16]);
  static void Matrix3ToMatrix4(const double mat3[9], double mat4[16]);
  static void Matrix4ToMatrix3(const double mat4[16], double mat3[9]);

  float GetScaledPenWidth();
  void GetScaledPenWidth(float& x, float& y);
  void TransformSize(float& x, float& y);

  vtkImageData* PreparePointSprite(vtkImageData* in);

  struct Details;
  Details* Impl;

  vtkViewport* Viewport;
  vtkXMLDataElement* ContextNode;
  vtkXMLDataElement* ActiveNode;
  vtkXMLDataElement* DefinitionNode;

  // This is a 3D transform, the 2D version doesn't support push/pop.
  vtkNew<vtkTransform> Matrix;
  std::array<double, 9> ActiveNodeTransform;

  std::array<int, 4> ClipRect;           // x, y, w, h
  std::array<int, 4> ActiveNodeClipRect; // x, y, w, h

  float CanvasHeight; // Used in y coordinate conversions.
  float SubdivisionThreshold;
  bool IsClipping;
  bool ActiveNodeIsClipping;
  bool EmbedFonts;
  bool TextAsPath;

private:
  vtkSVGContextDevice2D(const vtkSVGContextDevice2D&) = delete;
  void operator=(const vtkSVGContextDevice2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSVGContextDevice2D_h
