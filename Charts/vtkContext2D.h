/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContext2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContext2D - Class for drawing 2D primitives to a graphical context.
//
// .SECTION Description
// This defines the interface for drawing onto a 2D context. The context must
// be set up with a vtkContextDevice2D derived class that provides the functions
// to facilitate the low level calls to the context. Currently only an OpenGL
// based device is provided, but this could be extended in the future.

#ifndef __vtkContext2D_h
#define __vtkContext2D_h

#include "vtkObject.h"

class vtkWindow;

class vtkStdString;
class vtkTextProperty;

class vtkPoints2D;
class vtkContextDevice2D;
class vtkPen;
class vtkBrush;
class vtkImageData;
class vtkTransform2D;

class VTK_CHARTS_EXPORT vtkContext2D : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkContext2D, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkContext2D *New();

  // Description:
  // Begin painting on a vtkContextDevice2D, no painting can occur before this call
  // has been made. Only one painter is allowed at a time on any given paint
  // device. Returns true if successful, otherwise false.
  bool Begin(vtkContextDevice2D *device);

  vtkGetObjectMacro(Device, vtkContextDevice2D);

  // Description:
  // Ends painting on the device, you would not usually need to call this as it
  // should be called by the destructor. Returns true if the painter is no
  // longer active, otherwise false.
  bool End();

  // Description:
  // Draw a line between the specified points.
  void DrawLine(float x1, float y1, float x2, float y2);

  // Description:
  // Draw a line between the specified points.
  void DrawLine(float p[4]);

  // Description:
  // Draw a line between the specified points.
  // Note: Fastest path - points packed in x and y.
  void DrawLine(vtkPoints2D *points);

  // Description:
  // Draw a poly line between the specified points.
  void DrawPoly(float *x, float *y, int n);

  // Description:
  // Draw a poly line between the specified points - fastest code path due to
  // memory layout of the coordinates.
  void DrawPoly(vtkPoints2D *points);

  // Description:
  // Draw a poly line between the specified points, where the float array is of
  // size 2*n and the points are packed x1, y1, x2, y2 etc.
  // Note: Fastest code path - points packed in x and y.
  void DrawPoly(float *points, int n);

  // Description:
  // Draw a point at the supplied x and y coordinate
  void DrawPoint(float x, float y);

  // Description:
  // Draw the specified number of points using the x and y arrays supplied
  void DrawPoints(float *x, float *y, int n);

  // Description:
  // Draw a poly line between the specified points - fastest code path due to
  // memory layout of the coordinates.
  void DrawPoints(vtkPoints2D *points);

  // Description:
  // Draw a poly line between the specified points, where the float array is of
  // size 2*n and the points are packed x1, y1, x2, y2 etc.
  // Note: Fastest code path - points packed in x and y.
  void DrawPoints(float *points, int n);

  // Description:
  // Draw a rectangle with origin at x, y and width w, height h
  void DrawRect(float x, float y, float w, float h);

  // Description:
  // Draw a quadrilateral at the specified points (4 points, 8 floats in x, y).
  void DrawQuad(float x1, float y1, float x2, float y2,
                float x3, float y3, float x4, float y4);
  void DrawQuad(float *p);

  // Description:
  // Draw an ellipse with center at x, y and radii rx, ry.
  void DrawEllipse(float x, float y, float rx, float ry);

  // Description:
  // Draw the supplied image at the given x, y location (bottom corner).
  void DrawImage(float x, float y, vtkImageData *image);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define DrawTextA DrawText
# define DrawTextW DrawText
#endif

//BTX
  // Description:
  // Draw some text to the screen.
  void DrawString(vtkPoints2D *point, const vtkStdString &string);
  void DrawString(float x, float y, const vtkStdString &string);
//ETX
  void DrawString(vtkPoints2D *point, const char *string);
  void DrawString(float x, float y, const char *string);

  // Description:
  // Get/Set the pen which controls the outlines of shapes as well as lines,
  // points and related primitives.
  void SetPen(vtkPen *pen);
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Get/Set the pen which controls the outlines of shapes as well as lines,
  // points and related primitives.
  void SetBrush(vtkBrush *brush);
  vtkGetObjectMacro(Brush, vtkBrush);

  // Description:
  // Get/set the text properties.
  void SetTextProp(vtkTextProperty *prop);
  vtkGetObjectMacro(TextProp, vtkTextProperty);

  // Description:
  // Experimentation with point sprites
  unsigned int AddPointSprite(vtkImageData *image);

  // Description:
  // Set the transform for the context.
  void SetTransform(vtkTransform2D *transform);
  vtkGetObjectMacro(Transform, vtkTransform2D);

//BTX
protected:
  vtkContext2D();
  ~vtkContext2D();

  vtkContextDevice2D *Device; // The underlying device
  vtkPen *Pen;                // Outlining
  vtkBrush *Brush;            // Fills
  vtkTextProperty *TextProp;  // Text property
  vtkTransform2D *Transform;  // The painter transform

private:
  vtkContext2D(const vtkContext2D &); // Not implemented.
  void operator=(const vtkContext2D &);   // Not implemented.

  // Apply the pen settings to the context
  void ApplyPen();
  // Apply the brush settings to the context
  void ApplyBrush();

//ETX
};

#endif //__vtkContext2D_h
