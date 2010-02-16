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

// .NAME vtkOpenGLContextDevice2D - Class for drawing 2D primitives using OpenGL.
//
// .SECTION Description
// This class takes care of drawing the 2D primitives for the vtkContext2D class.
// In general this class should not be used directly, but called by vtkContext2D
// which takes care of many of the higher level details.

#ifndef __vtkOpenGLContextDevice2D_h
#define __vtkOpenGLContextDevice2D_h

#include "vtkContextDevice2D.h"

class vtkWindow;
class vtkViewport;
class vtkRenderer;
class vtkLabelRenderStrategy;
class vtkOpenGLRenderWindow;
class vtkOpenGLExtensionManager;

class VTK_CHARTS_EXPORT vtkOpenGLContextDevice2D : public vtkContextDevice2D
{
public:
  vtkTypeRevisionMacro(vtkOpenGLContextDevice2D, vtkContextDevice2D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkOpenGLContextDevice2D *New();

  // Description:
  // Draw a poly line using the vtkPoints2D - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoly(float *points, int n);

  // Description:
  // Draw a poly line using the vtkPoints2D - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoints(float *points, int n);

  // Description:
  // Draws a rectangle
  virtual void DrawQuad(float *points, int n);
  
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
  
//BTX
  // Description:
  // Draw some text to the screen!
  virtual void DrawString(float *point, vtkTextProperty *tprop,
                          const vtkStdString &string);

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkStdString &string,
                                   vtkTextProperty *tprop,
                                   float bounds[4]);
//ETX

  // Description:
  // Draw the supplied image at the given x, y (p[0], p[1]) location(s) (bottom corner).
  virtual void DrawImage(float *p, int n, vtkImageData *image);

  // Description:
  // Experimentation with point sprites
  virtual unsigned int AddPointSprite(vtkImageData *image);

  // Description:
  // Set the color for the device using unsigned char of length 4, RGBA.
  virtual void SetColor4(unsigned char *color);

  // Description:
  // Set the color for the device using unsigned char of length 3, RGB.
  virtual void SetColor(unsigned char *color);

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
  // Set the model view matrix for the display
  virtual void SetMatrix(vtkMatrix3x3 *m);

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
  virtual void DisableClipping();

  // Description:
  // Begin drawing, pass in the viewport to set up the view.
  virtual void Begin(vtkViewport* viewport);

  // Description:
  // End drawing, clean up the view.
  virtual void End();

  // Description:
  // Force the use of the freetype based render strategy. If Qt is available
  // then freetype will be used preferentially, otherwise this has no effect.
  // Returns true on success.
  bool SetStringRendererToFreeType();

  // Description:
  // Force the use of the Qt based string render strategy. If Qt is not available
  // then freetype will be used and this will return false.
  bool SetStringRendererToQt();

//BTX
protected:
  vtkOpenGLContextDevice2D();
  virtual ~vtkOpenGLContextDevice2D();

  // Description:
  // Store the width and height of the display devicen (in pixels).
  int Geometry[2];

  // Description:
  // We need to store a pointer to the renderer for the text rendering
  vtkRenderer *Renderer;

  // Description:
  // We also need a label render strategy
  vtkLabelRenderStrategy *TextRenderer;

  // Description:
  // Store whether any text has been drawn to control Start frame end frame
  bool IsTextDrawn;

  // Description:
  // Is the device currently rendering? Prevent multiple End() calls.
  bool InRender;

  // Description:
  // Private data pointer of the class
  class Private;
  Private *Storage;

  // Description:
  // Load the OpenGL extensions we need.
  bool LoadExtensions(vtkOpenGLExtensionManager *m);

private:
  vtkOpenGLContextDevice2D(const vtkOpenGLContextDevice2D &); // Not implemented.
  void operator=(const vtkOpenGLContextDevice2D &);   // Not implemented.

//ETX
};

#endif //__vtkOpenGLContextDevice2D_h
