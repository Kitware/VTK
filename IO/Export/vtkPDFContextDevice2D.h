/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDFContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPDFContextDevice2D
 * @brief vtkContextDevice2D implementation for use with vtkPDFExporter.
 */

#ifndef vtkPDFContextDevice2D_h
#define vtkPDFContextDevice2D_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkContextDevice2D.h"
#include "vtkNew.h" // For vtkNew!

class vtkColor3ub;
class vtkPath;
class vtkRenderer;
class vtkTransform;

class VTKIOEXPORT_EXPORT vtkPDFContextDevice2D: public vtkContextDevice2D
{
public:
  static vtkPDFContextDevice2D* New();
  vtkTypeMacro(vtkPDFContextDevice2D, vtkContextDevice2D)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the HPDF_Doc and HPDF_Page to use while exporting the scene. The
   * type is void* to keep the libharu opaque types from leaking into headers.
   * This function expects HPDF_Document* and HPDF_Page* as the arguments.
   */
  void SetHaruObjects(void *doc, void *page);

  void SetRenderer(vtkRenderer*);

  void DrawPoly(float *points, int n, unsigned char *colors = 0,
                int nc_comps = 0) VTK_OVERRIDE;
  void DrawLines(float *f, int n, unsigned char *colors = 0,
                 int nc_comps = 0) VTK_OVERRIDE;
  void DrawPoints(float *points, int n, unsigned char* colors = 0,
                  int nc_comps = 0) VTK_OVERRIDE;
  void DrawPointSprites(vtkImageData *sprite, float *points, int n,
                        unsigned char *colors = 0,
                        int nc_comps = 0) VTK_OVERRIDE;
  void DrawMarkers(int shape, bool highlight, float *points, int n,
                   unsigned char *colors = 0, int nc_comps = 0) VTK_OVERRIDE;
  void DrawQuad(float *, int) VTK_OVERRIDE;
  void DrawQuadStrip(float *, int) VTK_OVERRIDE;
  void DrawPolygon(float *, int) VTK_OVERRIDE;
  void DrawEllipseWedge(float x, float y, float outRx, float outRy,
                        float inRx, float inRy, float startAngle,
                        float stopAngle) VTK_OVERRIDE;
  void DrawEllipticArc(float x, float y, float rX, float rY,
                       float startAngle, float stopAngle) VTK_OVERRIDE;
  void DrawString(float *point, const vtkStdString &string) VTK_OVERRIDE;
  void ComputeStringBounds(const vtkStdString &string,
                           float bounds[4]) VTK_OVERRIDE;
  void DrawString(float *point, const vtkUnicodeString &string) VTK_OVERRIDE;
  void ComputeStringBounds(const vtkUnicodeString &string,
                           float bounds[4]) VTK_OVERRIDE;
  void ComputeJustifiedStringBounds(const char* string,
                                    float bounds[4]) VTK_OVERRIDE;
  void DrawMathTextString(float *point, const vtkStdString &str) VTK_OVERRIDE;
  void DrawImage(float p[2], float scale, vtkImageData *image) VTK_OVERRIDE;
  void DrawImage(const vtkRectf& pos, vtkImageData *image) VTK_OVERRIDE;
  void DrawPolyData(float p[2], float scale, vtkPolyData* polyData,
                    vtkUnsignedCharArray* colors, int scalarMode) VTK_OVERRIDE;
  void SetColor4(unsigned char color[4]) VTK_OVERRIDE;
  void SetTexture(vtkImageData* image, int properties) VTK_OVERRIDE;
  void SetPointSize(float size) VTK_OVERRIDE;
  void SetLineWidth(float width) VTK_OVERRIDE;

  void SetLineType(int type) VTK_OVERRIDE;
  void SetMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;
  void GetMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;
  void MultiplyMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;
  void PushMatrix() VTK_OVERRIDE;
  void PopMatrix() VTK_OVERRIDE;
  void SetClipping(int *x) VTK_OVERRIDE;
  void EnableClipping(bool enable) VTK_OVERRIDE;

protected:
  vtkPDFContextDevice2D();
  ~vtkPDFContextDevice2D() VTK_OVERRIDE;

  void PushGraphicsState();
  void PopGraphicsState();

  void ApplyPenState();
  void ApplyStrokeColor(unsigned char *color, int numComps);
  void ApplyLineWidth(float width);
  void ApplyLineType(int type);
  void Stroke();

  void ApplyPenStateAsFill();
  void ApplyBrushState();
  void ApplyTextPropertyState();
  void ApplyFillColor(unsigned char *color, int numComps);
  void ApplyFillAlpha(unsigned char alpha);
  void Fill(bool stroke = false);
  void FillEvenOdd(bool stroke = false);

  void BeginClipPathForTexture();
  void RegisterTexturePoints(float *data, int numPoints);
  void FillTexture();

  // converts input to RGB if needed. Call Delete() on the returned object when
  // finished with it.
  vtkImageData* PrepareImageData(vtkImageData *in);

  void DrawEllipticArcSegments(float x, float y, float rX, float rY,
                               float startAngle, float stopAngle,
                               bool startPath);
  int GetNumberOfArcIterations(float rX, float rY,
                               float startAngle, float stopAngle);

  void DrawCrossMarkers(bool highlight, float *points, int n,
                        unsigned char *colors, int nc_comps);
  void DrawPlusMarkers(bool highlight, float *points, int n,
                       unsigned char *colors, int nc_comps);
  void DrawSquareMarkers(bool highlight, float *points, int n,
                         unsigned char *colors, int nc_comps);
  void DrawCircleMarkers(bool highlight, float *points, int n,
                         unsigned char *colors, int nc_comps);
  void DrawDiamondMarkers(bool highlight, float *points, int n,
                          unsigned char *colors, int nc_comps);

  void DrawPath(vtkPath *path, float x, float y);

  void BeginText();
  float ComputeTextWidth(const vtkStdString &str);
  // Transforms pos form the VTK anchor point to the PDF anchor point, and
  // returns a guess at the height of the rendered string.
  // realWidth is the width computed by ComputTextWidth.
  float ComputeTextPosition(float pos[2], const vtkStdString &str,
                            float realWidth);
  void AlignText(double orientation, float width, float height, float *p);
  void EndText();

  void ApplyTransform();

  // Converts a 2D transform matrix into a 3D transform matrix, or vice versa
  static void Matrix3ToMatrix4(vtkMatrix3x3 *mat3, double mat4[16]);
  static void Matrix4ToMatrix3(double mat4[16], vtkMatrix3x3 *mat3);

  // Convert a 3D transform matrix to an HPDF transformation.
  // trans = {a, b, c, d, x, y}, which define the transform:
  // | a b x |
  // | c d y |
  // | 0 0 1 |
  static void Matrix4ToHPDFTransform(const double mat4[16], float hpdfMat[6]);
  static void Matrix3ToHPDFTransform(const double mat4[9], float hpdfMat[6]);
  static void HPDFTransformToMatrix3(float a, float b, float c, float d,
                                     float x, float y, double mat3[9]);

  struct Details;
  Details *Impl;

  // This is a 3D transform, the 2D version doesn't support push/pop.
  vtkNew<vtkTransform> Matrix;

  vtkRenderer *Renderer;
  float PointSize;
  bool IsClipping;
  float ClipBox[4]; // x, y, w, h

  bool IsInTexturedFill;
  float TextureBounds[4]; // xmin, xmax, ymin, ymax; used for placing textures

private:
  vtkPDFContextDevice2D(const vtkPDFContextDevice2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPDFContextDevice2D&) VTK_DELETE_FUNCTION;
};

#endif // vtkPDFContextDevice2D_h
