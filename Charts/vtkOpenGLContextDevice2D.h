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
  vtkTypeRevisionMacro(vtkOpenGLContextDevice2D, vtkObject);
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

//BTX
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define DrawTextA DrawText
# define DrawTextW DrawText
#endif
  // Description:
  // Draw some text to the screen!
  virtual void DrawText(float *point, vtkTextProperty *tprop,
                        const vtkStdString &string);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef DrawTextA
# undef DrawTextW
  //BTX
  // Define possible mangled names.
  virtual void DrawTextA(float *point, vtkTextProperty *tprop,
                         const vtkStdString &string);
  virtual void DrawTextW(float *point, vtkTextProperty *tprop,
                         const vtkStdString &string);
  //ETX
#endif

//ETX

  // Description:
  // Draw the supplied image at the given x, y (p[0], p[1]) location(s) (bottom corner).
  void DrawImage(float *p, int n, vtkImageData *image);

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
  // Supply a float array of length 4 with x1, y1, x2, y2 specifying the extents
  // of the display
  virtual void SetViewExtents(float *x);

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
