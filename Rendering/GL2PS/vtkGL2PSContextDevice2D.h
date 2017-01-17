/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGL2PSContextDevice2D
 * @brief   Class for drawing 2D primitives using GL2PS
 *
 *
 * This class takes care of drawing the 2D primitives for the vtkContext2D class.
 * In general this class should not be used directly, but is used internally
 * by the vtkGL2PSExporter.
 *
 * @sa
 * vtkGL2PSExporter
*/

#ifndef vtkGL2PSContextDevice2D_h
#define vtkGL2PSContextDevice2D_h

#include "vtkRenderingGL2PSModule.h" // For export macro
#include "vtkOpenGLContextDevice2D.h"

class vtkPath;

class VTKRENDERINGGL2PS_EXPORT vtkGL2PSContextDevice2D
    : public vtkOpenGLContextDevice2D
{
public:
  vtkTypeMacro(vtkGL2PSContextDevice2D, vtkOpenGLContextDevice2D);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  static vtkGL2PSContextDevice2D *New();

  /**
   * Draw a poly line using the points - fastest code path due to memory
   * layout of the coordinates. The line will be colored by colors array
   * which has nc_comps components.

   * If colors is not set and the current Pen's alpha channel is zero, no
   * OpenGL calls are emitted.
   */
  void DrawPoly(float *f, int n, unsigned char *colors = 0,
                        int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw a series of points - fastest code path due to memory layout of the
   * coordinates. The colors and nc_comps are optional - color array.
   */
  void DrawPoints(float *points, int n, unsigned char* colors = 0,
                          int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied vtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize.
   * \param colors is an optional array of colors.
   * \param nc_comps is the number of components for the color.
   */
  void DrawPointSprites(vtkImageData *sprite, float *points, int n,
                                unsigned char *colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw a quad using the specified number of points.
   */
  void DrawQuadStrip(float *, int) VTK_OVERRIDE;

  /**
   * Draw a polygon using the specified number of points.
   */
  void DrawPolygon(float *, int) VTK_OVERRIDE;

  /**
   * Draw an elliptic wedge with center at x, y, outer radii outRx, outRy,
   * inner radii inRx, inRy between angles startAngle and stopAngle
   * (expressed in degrees).
   * \pre positive_outRx: outRx>=0
   * \pre positive_outRy: outRy>=0
   * \pre positive_inRx: inRx>=0
   * \pre positive_inRy: inRy>=0
   * \pre ordered_rx: inRx<=outRx
   * \pre ordered_ry: inRy<=outRy
   */
  void DrawEllipseWedge(float x, float y, float outRx, float outRy,
                                float inRx, float inRy, float startAngle,
                                float stopAngle) VTK_OVERRIDE;

  /**
   * Draw an elliptic arc with center at x,y with radii rX and rY between
   * angles startAngle and stopAngle (expressed in degrees).
   * \pre positive_rX: rX>=0
   * \pre positive_rY: rY>=0
   */
  void DrawEllipticArc(float x, float y, float rX, float rY,
                               float startAngle, float stopAngle) VTK_OVERRIDE;

  /**
   * Draw a series of markers centered at the points supplied. The \a shape
   * argument controls the marker shape, and can be one of
   * - VTK_MARKER_CROSS
   * - VTK_MARKER_PLUS
   * - VTK_MARKER_SQUARE
   * - VTK_MARKER_CIRCLE
   * - VTK_MARKER_DIAMOND
   * \param colors is an optional array of colors.
   * \param nc_comps is the number of components for the color.
   */
  void DrawMarkers(int shape, bool highlight, float *points, int n,
                           unsigned char *colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draws a rectangle
   */
  void DrawQuad(float *points, int n) VTK_OVERRIDE;

  /**
   * Draw some text to the screen!
   */
  void DrawString(float *point, const vtkStdString &string) VTK_OVERRIDE;

  /**
   * Draw some text to the screen.
   */
  void DrawString(float *point, const vtkUnicodeString &string) VTK_OVERRIDE;

  /**
   * Draw text using MathText markup for mathematical equations. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   */
  void DrawMathTextString(float point[2], const vtkStdString &string) VTK_OVERRIDE;

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the vtkPen
   * object in the vtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyPen(vtkPen *pen) VTK_OVERRIDE;

  /**
   * Set the point size for glyphs/sprites.
   */
  void SetPointSize(float size) VTK_OVERRIDE;

  /**
   * Set the line width for glyphs/sprites.
   */
  void SetLineWidth(float width) VTK_OVERRIDE;

  /**
   * Set the line type type (using anonymous enum in vtkPen).
   */
  void SetLineType(int type) VTK_OVERRIDE;

protected:
  vtkGL2PSContextDevice2D();
  ~vtkGL2PSContextDevice2D() VTK_OVERRIDE;

private:
  vtkGL2PSContextDevice2D(const vtkGL2PSContextDevice2D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkGL2PSContextDevice2D &) VTK_DELETE_FUNCTION;

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
  void AddEllipseToPath(vtkPath *path, float x, float y, float rx, float ry,
                        bool reverse);

  // Transform the path using the current modelview matrix.
  void TransformPath(vtkPath *path) const;

  // Transform the 2D point using the current modelview matrix.
  void TransformPoint(float &x, float &y) const;

  // Transform the width and height from pixels to data units.
  void TransformSize(float &dx, float &dy) const;
};

#endif //vtkGL2PSContextDevice2D_h
