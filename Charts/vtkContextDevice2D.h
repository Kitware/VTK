/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextDevice2D - Abstract class for drawing 2D primitives.
//
// .SECTION Description
// This defines the interface for a vtkContextDevice2D. In this sense a
// ContextDevice is a class used to paint 2D primitives onto a device, such as
// an OpenGL context or a QGraphicsView.

#ifndef __vtkContextDevice2D_h
#define __vtkContextDevice2D_h

#include "vtkObject.h"

class vtkWindow;
class vtkViewport;
class vtkStdString;
class vtkTextProperty;
class vtkPoints2D;
class vtkImageData;
class vtkMatrix3x3;

class VTK_CHARTS_EXPORT vtkContextDevice2D : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkContextDevice2D, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Draw a poly line using the vtkPoints2D - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoly(float *points, int n) = 0;

  // Description:
  // Draw a poly line using the vtkPoints2D - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoints(float *points, int n) = 0;

  // Description:
  // Draw a quad using the specified number of points.
  virtual void DrawQuad(float *, int) { ; }

//BTX
  // Description:
  // Draw some text to the screen.
  virtual void DrawString(float *point, vtkTextProperty *tprop,
                          const vtkStdString &string) = 0;

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkStdString &string,
                                   vtkTextProperty *tprop,
                                   float bounds[4]) = 0;
//ETX

  // Description:
  // Draw the supplied image at the given x, y (p[0], p[1]) location(s) (bottom corner).
  virtual void DrawImage(float *, int , vtkImageData *) {;}

  // Description:
  // Experimentation with point sprites
  virtual unsigned int AddPointSprite(vtkImageData *) { return 0; }

  // Description:
  // Set the color for the device using unsigned char of length 4, RGBA.
  virtual void SetColor4(unsigned char *color) = 0;

  // Description:
  // Set the color for the device using unsigned char of length 3, RGB.
  virtual void SetColor(unsigned char *color) = 0;

  // Description:
  // Set the point size for glyphs/sprites.
  virtual void SetPointSize(float size) = 0;

  // Description:
  // Set the line width.
  virtual void SetLineWidth(float width) = 0;

  // Description:
  // Set the line type type (using anonymous enum in vtkPen).
  virtual void SetLineType(int type) = 0;

  // Description:
  // Get the width of the device in pixels.
  virtual int GetWidth() { return this->Geometry[0]; }

  // Description:
  // Get the width of the device in pixels.
  virtual int GetHeight() { return this->Geometry[1]; }

  // Description:
  // Set the model view matrix for the display
  virtual void SetMatrix(vtkMatrix3x3 *m) = 0;

  // Description:
  // Push the current matrix onto the stack.
  virtual void PushMatrix() = 0;

  // Description:
  // Pop the current matrix off of the stack.
  virtual void PopMatrix() = 0;

  // Description:
  // Supply a float array of length 4 with x1, y1, width, height specifying
  // clipping region for the device in pixels.
  virtual void SetClipping(int *x) = 0;

  // Description:
  // Disable clipping of the display.
  virtual void DisableClipping() = 0;

  // Description:
  // Begin drawing, pass in the viewport to set up the view.
  virtual void Begin(vtkViewport*) { }

  // Description:
  // End drawing, clean up the view.
  virtual void End() { }

//BTX
protected:
  vtkContextDevice2D();
  virtual ~vtkContextDevice2D();

  // Description:
  // Store the width and height of the device in pixels.
  int Geometry[2];

private:
  vtkContextDevice2D(const vtkContextDevice2D &); // Not implemented.
  void operator=(const vtkContextDevice2D &);   // Not implemented.

//ETX
};

#endif //__vtkContextDevice2D_h
