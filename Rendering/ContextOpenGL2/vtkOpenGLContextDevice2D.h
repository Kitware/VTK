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

// .NAME vtkOpenGLContextDevice2D - Class for drawing 2D primitives using OpenGL
// 1.1+.
//
// .SECTION Description
// This class takes care of drawing the 2D primitives for the vtkContext2D class.
// In general this class should not be used directly, but called by vtkContext2D
// which takes care of many of the higher level details.
//
// .SECTION See Also
// vtkOpenGL2ContextDevice2D

#ifndef vtkOpenGLContextDevice2D_h
#define vtkOpenGLContextDevice2D_h

#include "vtkRenderingContextOpenGL2Module.h" // For export macro
#include "vtkContextDevice2D.h"

#include <list> // for std::list

class vtkWindow;
class vtkViewport;
class vtkRenderer;
class vtkStringToImage;
class vtkOpenGLRenderWindow;
class vtkOpenGLExtensionManager;

class VTKRENDERINGCONTEXTOPENGL2_EXPORT vtkOpenGLContextDevice2D : public vtkContextDevice2D
{
public:
  vtkTypeMacro(vtkOpenGLContextDevice2D, vtkContextDevice2D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkOpenGLContextDevice2D *New();

  // Description:
  // Draw a poly line using the points - fastest code path due to memory
  // layout of the coordinates. The line will be colored by colors array
  // which has nc_comps components
  virtual void DrawPoly(float *f, int n, unsigned char *colors = 0,
                        int nc_comps = 0);

  // Description:
  // Draw a series of points - fastest code path due to memory
  // layout of the coordinates. Points are colored by colors array
  // which has nc_comps components
  virtual void DrawPoints(float *points, int n, unsigned char* colors = 0,
                          int nc_comps = 0);

  // Description:
  // Draw a series of point sprites, images centred at the points supplied.
  // The supplied vtkImageData is the sprite to be drawn, only squares will be
  // drawn and the size is set using SetPointSize. Points are colored by colors
  // array which has nc_comps components - this part is optional.
  virtual void DrawPointSprites(vtkImageData *sprite, float *points, int n,
                                unsigned char* colors = 0, int nc_comps = 0);

  // Description:
  // Draw a series of markers centered at the points supplied. The \a shape
  // argument controls the marker shape, and can be one of
  //   - VTK_MARKER_CROSS
  //   - VTK_MARKER_PLUS
  //   - VTK_MARKER_SQUARE
  //   - VTK_MARKER_CIRCLE
  //   - VTK_MARKER_DIAMOND
  // \param colors is an optional array of colors.
  // \param nc_comps is the number of components for the color.
  virtual void DrawMarkers(int shape, bool highlight, float *points, int n,
                           unsigned char *colors = 0, int nc_comps = 0);

  // Description:
  // Adjust the size of the MarkerCache. This implementation generates point
  // sprites for each mark size/shape and uses DrawPointSprites to render them.
  // The number of cached markers can be accessed with this function.
  vtkSetMacro(MaximumMarkerCacheSize, int)
  vtkGetMacro(MaximumMarkerCacheSize, int)

  // Description:
  // Draws a rectangle
  virtual void DrawQuad(float *points, int n);

  // Description:
  // Draws a rectangle
  virtual void DrawQuadStrip(float *points, int n);

  // Description:
  // Draw a polygon using the specified number of points.
  virtual void DrawPolygon(float *, int);

  // Description:
  // Draw an elliptic wedge with center at x, y, outer radii outRx, outRy,
  // inner radii inRx, inRy between angles startAngle and stopAngle
  // (expressed in degrees).
  // \pre positive_outRx: outRx>=0
  // \pre positive_outRy: outRy>=0
  // \pre positive_inRx: inRx>=0
  // \pre positive_inRy: inRy>=0
  // \pre ordered_rx: inRx<=outRx
  // \pre ordered_ry: inRy<=outRy
  virtual void DrawEllipseWedge(float x, float y, float outRx, float outRy,
                                float inRx, float inRy, float startAngle,
                                float stopAngle);

  // Description:
  // Draw an elliptic arc with center at x,y with radii rX and rY between
  // angles startAngle and stopAngle (expressed in degrees).
  // \pre positive_rX: rX>=0
  // \pre positive_rY: rY>=0
  virtual void DrawEllipticArc(float x, float y, float rX, float rY,
                               float startAngle, float stopAngle);


  // Description:
  // Draw some text to the screen!
  virtual void DrawString(float *point, const vtkStdString &string);

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
  // empty string or string with only characters that cannot be rendered.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkStdString &string,
                                   float bounds[4]);

  // Description:
  // Draw some text to the screen.
  virtual void DrawString(float *point, const vtkUnicodeString &string);

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
  // empty string or string with only characters that cannot be rendered.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkUnicodeString &string,
                                   float bounds[4]);

  // Description:
  // Draw text using MathText markup for mathematical equations. See
  // http://matplotlib.sourceforge.net/users/mathtext.html for more information.
  virtual void DrawMathTextString(float point[2], const vtkStdString &string);

  // Description:
  // Draw the supplied image at the given x, y (p[0], p[1]) (bottom corner),
  // scaled by scale (1.0 would match the image).
  virtual void DrawImage(float p[2], float scale, vtkImageData *image);

  // Description:
  // Draw the supplied image at the given position. The origin, width, and
  // height are specified by the supplied vtkRectf variable pos. The image
  // will be drawn scaled to that size.
  void DrawImage(const vtkRectf& pos, vtkImageData *image);

  // Description:
  // Set the color for the device using unsigned char of length 4, RGBA.
  virtual void SetColor4(unsigned char color[4]);

  // Description:
  // Set the color for the device using unsigned char of length 3, RGB.
  virtual void SetColor(unsigned char color[3]);

  // Description:
  // Set the texture for the device, it is used to fill the polygons
  virtual void SetTexture(vtkImageData* image, int properties = 0);

  // Description:
  // Set the point size for glyphs/sprites.
  virtual void SetPointSize(float size);

  // Description:
  // Set the line width for glyphs/sprites.
  virtual void SetLineWidth(float width);

  // Description:
  // Set the line type type (using anonymous enum in vtkPen).
  virtual void SetLineType(int type);

  // Description:
  // Multiply the current model view matrix by the supplied one.
  virtual void MultiplyMatrix(vtkMatrix3x3 *m);

  // Description:
  // Set the model view matrix for the display
  virtual void SetMatrix(vtkMatrix3x3 *m);

  // Description:
  // Set the model view matrix for the display
  virtual void GetMatrix(vtkMatrix3x3 *m);

  // Description:
  // Push the current matrix onto the stack.
  virtual void PushMatrix();

  // Description:
  // Pop the current matrix off of the stack.
  virtual void PopMatrix();

  // Description:
  // Supply an int array of length 4 with x1, y1, x2, y2 specifying clipping
  // for the display.
  virtual void SetClipping(int *x);

  // Description:
  // Disable clipping of the display.
  virtual void EnableClipping(bool enable);

  // Description:
  // Begin drawing, pass in the viewport to set up the view.
  virtual void Begin(vtkViewport* viewport);

  // Description:
  // End drawing, clean up the view.
  virtual void End();

  // Description:
  // Start BufferId creation Mode.
  // The default implementation is empty.
  // \pre not_yet: !GetBufferIdMode()
  // \pre bufferId_exists: bufferId!=0
  // \post started: GetBufferIdMode()
  virtual void BufferIdModeBegin(vtkAbstractContextBufferId *bufferId);

  // Description:
  // Finalize BufferId creation Mode. It makes sure that the content of the
  // bufferId passed in argument of BufferIdModeBegin() is correctly set.
  // The default implementation is empty.
  // \pre started: GetBufferIdMode()
  // \post done: !GetBufferIdMode()
  virtual void BufferIdModeEnd();

  // Description:
  // Force the use of the freetype based render strategy. If Qt is available
  // then freetype will be used preferentially, otherwise this has no effect.
  // Returns true on success.
  bool SetStringRendererToFreeType();

  // Description:
  // Force the use of the Qt based string render strategy. If Qt is not available
  // then freetype will be used and this will return false.
  bool SetStringRendererToQt();

  // Description:
  // Check whether the current context device has support for GLSL.
  bool HasGLSL();

  // Description:
  // Get the active RenderWindow of the device. Will return null if not active.
  vtkGetObjectMacro(RenderWindow, vtkOpenGLRenderWindow);

  // Description:
  // Release any graphics resources that are being consumed by this device.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

//BTX
protected:
  vtkOpenGLContextDevice2D();
  virtual ~vtkOpenGLContextDevice2D();

  // Description:
  // Factorized code called by DrawEllipseWedge() and DrawEllipticArc()
  // to figure out the number of iterations required to make an arc smooth.
  // \pre positive_rX: rX>=0.0f
  // \pre positive_rY: rY>=0.0f
  // \pre not_both_null: rX>0.0 || rY>0.0
  int GetNumberOfArcIterations(float rX,
                               float rY,
                               float startAngle,
                               float stopAngle);

  // Description:
  // Store the width and height of the display devicen (in pixels).
  int Geometry[2];

  // Description:
  // We need to store a pointer to the renderer for the text rendering
  vtkRenderer *Renderer;

  // Description:
  // We also need a label render strategy
  vtkStringToImage *TextRenderer;

  // Description:
  // Is the device currently rendering? Prevent multiple End() calls.
  bool InRender;

  // Description:
  // Private data pointer of the class
  class Private;
  Private *Storage;

  // Description:
  // The OpenGL render window being used by the device
  vtkOpenGLRenderWindow* RenderWindow;

private:
  vtkOpenGLContextDevice2D(const vtkOpenGLContextDevice2D &); // Not implemented.
  void operator=(const vtkOpenGLContextDevice2D &);   // Not implemented.

  void AlignText(double orientation, float width, float height, float *p);

  // Description:
  // Retrieve a point sprite image for a given marker shape and size. The
  // image data will be either generated or retrieved from a cache. This class
  // manages the lifetime of returned image data. Setting \a highlight to
  // true produces an alternate (usually thicker) version of the marker.
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

  // Description:
  // Generate the marker with the specified shape and size. This function should
  // not be used directly -- use GetMarker, which caches results, instead.
  vtkImageData * GenerateMarker(int shape, int size, bool highlight);

//ETX
};

#endif //vtkOpenGLContextDevice2D_h
