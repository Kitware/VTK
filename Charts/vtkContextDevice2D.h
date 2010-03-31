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
class vtkAbstractContextBufferId;

class VTK_CHARTS_EXPORT vtkContextDevice2D : public vtkObject
{
public:
  vtkTypeMacro(vtkContextDevice2D, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Draw a poly line using the points - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoly(float *points, int n) = 0;

  // Description:
  // Draw a series of points - fastest code path due to memory
  // layout of the coordinates.
  virtual void DrawPoints(float *points, int n) = 0;

  // Description:
  // Draw a series of point sprites, images centred at the points supplied.
  // The supplied vtkImageData is the sprite to be drawn, only squares will be
  // drawn and the size is set using SetPointSize.
  virtual void DrawPointSprites(vtkImageData *sprite, float *points, int n) = 0;

  // Description:
  // Draw a quad using the specified number of points.
  virtual void DrawQuad(float *, int) { ; }

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
                                float stopAngle)=0;

  // Description:
  // Draw an elliptic arc with center at x,y with radii rX and rY between
  // angles startAngle and stopAngle (expressed in degrees).
  // \pre positive_rX: rX>=0
  // \pre positive_rY: rY>=0
  virtual void DrawEllipticArc(float x, float y, float rX, float rY,
                               float startAngle, float stopAngle)=0;

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
  // Multiply the current model view matrix by the supplied one
  virtual void MultiplyMatrix(vtkMatrix3x3 *m) = 0;

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

  // Description:
  // Tell if the device context is in BufferId creation mode.
  // Initial value is false.
  virtual bool GetBufferIdMode() const;

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

//BTX
protected:
  vtkContextDevice2D();
  virtual ~vtkContextDevice2D();

  // Description:
  // Store the width and height of the device in pixels.
  int Geometry[2];

  vtkAbstractContextBufferId *BufferId;

private:
  vtkContextDevice2D(const vtkContextDevice2D &); // Not implemented.
  void operator=(const vtkContextDevice2D &);   // Not implemented.

//ETX
};

#endif //__vtkContextDevice2D_h
