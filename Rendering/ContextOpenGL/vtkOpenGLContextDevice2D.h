/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLContextDevice2D
 * @brief   Class for drawing 2D primitives using OpenGL
 * 1.1+.
 *
 *
 * This class takes care of drawing the 2D primitives for the vtkContext2D class.
 * In general this class should not be used directly, but called by vtkContext2D
 * which takes care of many of the higher level details.
 *
 * @sa
 * vtkOpenGL2ContextDevice2D
*/

#ifndef vtkOpenGLContextDevice2D_h
#define vtkOpenGLContextDevice2D_h

#include "vtkRenderingContextOpenGLModule.h" // For export macro
#include "vtkContextDevice2D.h"

#include <list> // for std::list

class vtkWindow;
class vtkViewport;
class vtkRenderer;
class vtkTextRenderer;
class vtkOpenGLRenderWindow;
class vtkOpenGLExtensionManager;

class VTKRENDERINGCONTEXTOPENGL_EXPORT vtkOpenGLContextDevice2D : public vtkContextDevice2D
{
public:
  vtkTypeMacro(vtkOpenGLContextDevice2D, vtkContextDevice2D);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a 2D Painter object.
   */
  static vtkOpenGLContextDevice2D *New();

  /**
   * Draw a poly line using the points - fastest code path due to memory
   * layout of the coordinates. The line will be colored by colors array
   * which has nc_comps components.
   * \sa DrawLines()
   */
  void DrawPoly(float *f, int n, unsigned char *colors = 0,
                        int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw lines using the points - memory layout is as follows:
   * l1p1,l1p2,l2p1,l2p2... The lines will be colored by colors array
   * which has nc_comps components.
   * \sa DrawPoly()
   */
  void DrawLines(float *f, int n, unsigned char *colors = 0,
                         int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw a series of points - fastest code path due to memory
   * layout of the coordinates. Points are colored by colors array
   * which has nc_comps components.
   */
  void DrawPoints(float *points, int n, unsigned char* colors = 0,
                          int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied vtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize. Points are colored by colors
   * array which has nc_comps components - this part is optional.
   */
  void DrawPointSprites(vtkImageData *sprite, float *points, int n,
                                unsigned char* colors = 0, int nc_comps = 0) VTK_OVERRIDE;

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

  //@{
  /**
   * Adjust the size of the MarkerCache. This implementation generates point
   * sprites for each mark size/shape and uses DrawPointSprites to render them.
   * The number of cached markers can be accessed with this function.
   */
  vtkSetMacro(MaximumMarkerCacheSize, int)
  vtkGetMacro(MaximumMarkerCacheSize, int)
  //@}

  /**
   * Draws a rectangle
   */
  void DrawQuad(float *points, int n) VTK_OVERRIDE;

  /**
   * Draws a rectangle
   */
  void DrawQuadStrip(float *points, int n) VTK_OVERRIDE;

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
   * Draw some text to the screen!
   */
  void DrawString(float *point, const vtkStdString &string) VTK_OVERRIDE;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
   * empty string or string with only characters that cannot be rendered.
   * NOTE: This function does not take account of the text rotation.
   */
  void ComputeStringBounds(const vtkStdString &string,
                                   float bounds[4]) VTK_OVERRIDE;

  /**
   * Draw some text to the screen.
   */
  void DrawString(float *point, const vtkUnicodeString &string) VTK_OVERRIDE;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
   * empty string or string with only characters that cannot be rendered.
   * NOTE: This function does not take account of the text rotation.
   */
  void ComputeStringBounds(const vtkUnicodeString &string,
                                   float bounds[4]) VTK_OVERRIDE;

  /**
   * Compute the bounds of the supplied string while taking into account the
   * justification of the currently applied text property. Simple rotations
   * (0, 90, 180, 270 degrees) are also propertly taken into account.
   */
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) VTK_OVERRIDE;

  /**
   * Draw text using MathText markup for mathematical equations. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   */
  void DrawMathTextString(float point[2], const vtkStdString &string) VTK_OVERRIDE;

  /**
   * Draw the supplied image at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the image).
   */
  void DrawImage(float p[2], float scale, vtkImageData *image) VTK_OVERRIDE;

  /**
   * Draw the supplied image at the given position. The origin, width, and
   * height are specified by the supplied vtkRectf variable pos. The image
   * will be drawn scaled to that size.
   */
  void DrawImage(const vtkRectf& pos, vtkImageData *image) VTK_OVERRIDE;

  /**
   * Draw the supplied PolyData at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the actual dataset).
   *
   * @warning This functionality is not supported with the legacy OpenGL backend.
   */
  void DrawPolyData(float p[2], float scale, vtkPolyData* polyData,
    vtkUnsignedCharArray* colors, int scalarMode) VTK_OVERRIDE;

  /**
   * Set the color for the device using unsigned char of length 4, RGBA.
   */
  void SetColor4(unsigned char color[4]) VTK_OVERRIDE;

  /**
   * Set the color for the device using unsigned char of length 3, RGB.
   */
  virtual void SetColor(unsigned char color[3]);

  /**
   * Set the texture for the device, it is used to fill the polygons
   */
  void SetTexture(vtkImageData* image, int properties = 0) VTK_OVERRIDE;

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

  /**
   * Multiply the current model view matrix by the supplied one.
   */
  void MultiplyMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;

  /**
   * Set the model view matrix for the display
   */
  void SetMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;

  /**
   * Set the model view matrix for the display
   */
  void GetMatrix(vtkMatrix3x3 *m) VTK_OVERRIDE;

  /**
   * Push the current matrix onto the stack.
   */
  void PushMatrix() VTK_OVERRIDE;

  /**
   * Pop the current matrix off of the stack.
   */
  void PopMatrix() VTK_OVERRIDE;

  /**
   * Supply an int array of length 4 with x1, y1, width, height specifying
   * clipping for the display.
   */
  void SetClipping(int *x) VTK_OVERRIDE;

  /**
   * Disable clipping of the display.
   */
  void EnableClipping(bool enable) VTK_OVERRIDE;

  /**
   * Begin drawing, pass in the viewport to set up the view.
   */
  void Begin(vtkViewport* viewport) VTK_OVERRIDE;

  /**
   * End drawing, clean up the view.
   */
  void End() VTK_OVERRIDE;

  /**
   * Start BufferId creation Mode.
   * The default implementation is empty.
   * \pre not_yet: !GetBufferIdMode()
   * \pre bufferId_exists: bufferId!=0
   * \post started: GetBufferIdMode()
   */
  void BufferIdModeBegin(vtkAbstractContextBufferId *bufferId) VTK_OVERRIDE;

  /**
   * Finalize BufferId creation Mode. It makes sure that the content of the
   * bufferId passed in argument of BufferIdModeBegin() is correctly set.
   * The default implementation is empty.
   * \pre started: GetBufferIdMode()
   * \post done: !GetBufferIdMode()
   */
  void BufferIdModeEnd() VTK_OVERRIDE;

  /**
   * Force the use of the freetype based render strategy. If Qt is available
   * then freetype will be used preferentially, otherwise this has no effect.
   * Returns true on success.
   */
  bool SetStringRendererToFreeType();

  /**
   * Force the use of the Qt based string render strategy. If Qt is not available
   * then freetype will be used and this will return false.
   */
  bool SetStringRendererToQt();

  /**
   * Check whether the current context device has support for GLSL.
   */
  bool HasGLSL();

  //@{
  /**
   * Get the active RenderWindow of the device. Will return null if not active.
   */
  vtkGetObjectMacro(RenderWindow, vtkOpenGLRenderWindow);
  //@}

  /**
   * Release any graphics resources that are being consumed by this device.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkOpenGLContextDevice2D();
  ~vtkOpenGLContextDevice2D() VTK_OVERRIDE;

  /**
   * Factorized code called by DrawEllipseWedge() and DrawEllipticArc()
   * to figure out the number of iterations required to make an arc smooth.
   * \pre positive_rX: rX>=0.0f
   * \pre positive_rY: rY>=0.0f
   * \pre not_both_null: rX>0.0 || rY>0.0
   */
  int GetNumberOfArcIterations(float rX,
                               float rY,
                               float startAngle,
                               float stopAngle);

  /**
   * Store the width and height of the display devicen (in pixels).
   */
  int Geometry[2];

  /**
   * We need to store a pointer to the renderer for the text rendering
   */
  vtkRenderer *Renderer;

  /**
   * Is the device currently rendering? Prevent multiple End() calls.
   */
  bool InRender;

  //@{
  /**
   * Private data pointer of the class
   */
  class Private;
  Private *Storage;
  //@}

  /**
   * Load the OpenGL extensions we need.
   */
  virtual bool LoadExtensions(vtkOpenGLExtensionManager *m);

  /**
   * The OpenGL render window being used by the device
   */
  vtkOpenGLRenderWindow* RenderWindow;

private:
  vtkOpenGLContextDevice2D(const vtkOpenGLContextDevice2D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLContextDevice2D &) VTK_DELETE_FUNCTION;

  /**
   * Retrieve a point sprite image for a given marker shape and size. The
   * image data will be either generated or retrieved from a cache. This class
   * manages the lifetime of returned image data. Setting \a highlight to
   * true produces an alternate (usually thicker) version of the marker.
   */
  vtkImageData *GetMarker(int shape, int size, bool highlight);

  class vtkMarkerCacheObject
  {
  public:
    vtkTypeUInt64 Key;
    vtkImageData *Value;
    bool operator==(vtkTypeUInt64 key)
    {
      return this->Key == key;
    }
  };

  std::list<vtkMarkerCacheObject> MarkerCache;
  int MaximumMarkerCacheSize;

  /**
   * Generate the marker with the specified shape and size. This function should
   * not be used directly -- use GetMarker, which caches results, instead.
   */
  vtkImageData * GenerateMarker(int shape, int size, bool highlight);

  void ComputeStringBoundsInternal(const vtkUnicodeString &string,
                                   float bounds[4]);


};

#endif //vtkOpenGLContextDevice2D_h
