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
class vtkAbstractContextBufferId;

class VTK_CHARTS_EXPORT vtkContext2D : public vtkObject
{
public:
  vtkTypeMacro(vtkContext2D, vtkObject);
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
  // Tell if the context is in BufferId creation mode. Initial value is false.
  bool GetBufferIdMode() const;

  // Description:
  // Start BufferId creation Mode.
  // \pre not_yet: !GetBufferIdMode()
  // \pre bufferId_exists: bufferId!=0
  // \post started: GetBufferIdMode()
  void BufferIdModeBegin(vtkAbstractContextBufferId *bufferId);

  // Description:
  // Finalize BufferId creation Mode. It makes sure that the content of the
  // bufferId passed in argument of BufferIdModeBegin() is correctly set.
  // \pre started: GetBufferIdMode()
  // \post done: !GetBufferIdMode()
  void BufferIdModeEnd();

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
  // Draw a series of point sprites, images centred at the points supplied.
  // The supplied vtkImageData is the sprite to be drawn, only squares will be
  // drawn and the size is set using SetPointSize.
  void DrawPointSprites(vtkImageData *sprite, vtkPoints2D *points);

  // Description:
  // Draw a series of point sprites, images centred at the points supplied.
  // The supplied vtkImageData is the sprite to be drawn, only squares will be
  // drawn and the size is set using SetPointSize.
  void DrawPointSprites(vtkImageData *sprite, float *points, int n);

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
  // \pre positive_rx: rx>=0
  // \pre positive_ry: ry>=0
  void DrawEllipse(float x, float y, float rx, float ry);

  // Description:
  // Draw a circular wedge with center at x, y, outer radius outRadius,
  // inner radius inRadius between angles startAngle and stopAngle
  // (expressed in degrees).
  // \pre positive_outRadius: outRadius>=0
  // \pre positive_inRadius: inRadius>=0
  // \pre ordered_radii: inRadius<=outRadius
  void DrawWedge(float x, float y, float outRadius,
                 float inRadius,float startAngle,
                 float stopAngle);

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
  void DrawEllipseWedge(float x, float y, float outRx, float outRy,
                        float inRx, float inRy, float startAngle,
                        float stopAngle);


  // Description:
  // Draw a circular arc with center at x,y with radius r between angles
  // startAngle and stopAngle (expressed in degrees).
  // \pre positive_radius: r>=0
  void DrawArc(float x, float y, float r, float startAngle,
               float stopAngle);

  // Description:
  // Draw an elliptic arc with center at x,y with radii rX and rY between
  // angles startAngle and stopAngle (expressed in degrees).
  // \pre positive_rX: rX>=0
  // \pre positive_rY: rY>=0
  void DrawEllipticArc(float x, float y, float rX, float rY, float startAngle,
                       float stopAngle);


  // Description:
  // Draw the supplied image at the given x, y location (bottom corner).
  void DrawImage(float x, float y, vtkImageData *image);

//BTX
  // Description:
  // Draw some text to the screen in a bounding rectangle with the alignment
  // of the text properties respecting the rectangle. The points should be
  // supplied as bottom corner (x, y), width, height.
  void DrawStringRect(vtkPoints2D *rect, const vtkStdString &string);

  // Description:
  // Draw some text to the screen.
  void DrawString(vtkPoints2D *point, const vtkStdString &string);
  void DrawString(float x, float y, const vtkStdString &string);
//ETX
  void DrawString(vtkPoints2D *point, const char *string);
  void DrawString(float x, float y, const char *string);

//BTX
  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  void ComputeStringBounds(const vtkStdString &string, vtkPoints2D *bounds);
  void ComputeStringBounds(const vtkStdString &string, float bounds[4]);
//ETX
  void ComputeStringBounds(const char *string, float bounds[4]);

  // Description:
  // Apply the supplied pen which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkPen
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  void ApplyPen(vtkPen *pen);

  // Description:
  // Get the pen which controls the outlines of shapes, as well as lines,
  // points and related primitives. This object can be modified and the changes
  // will be reflected in subsequent drawing operations.
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Apply the supplied brush which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkBrush
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  void ApplyBrush(vtkBrush *brush);

  // Description:
  // Get the pen which controls the outlines of shapes as well as lines, points
  // and related primitives.
  vtkGetObjectMacro(Brush, vtkBrush);

  // Description:
  // Apply the supplied text property which controls how text is rendered.
  // This makes a deep copy of the vtkTextProperty object in the vtkContext2D,
  // it does not hold a pointer to the supplied object.
  void ApplyTextProp(vtkTextProperty *prop);

  // Description:
  // Get the text properties object for the vtkContext2D.
  vtkGetObjectMacro(TextProp, vtkTextProperty);

  // Description:
  // Set the transform for the context, the underlying device will use the
  // matrix of the transform. Note, this is set immediately, later changes to
  // the matrix will have no effect until it is set again.
  void SetTransform(vtkTransform2D *transform);
  vtkGetObjectMacro(Transform, vtkTransform2D);

  // Description:
  // Append the transform for the context, the underlying device will use the
  // matrix of the transform. Note, this is set immediately, later changes to
  // the matrix will have no effect until it is set again. The matrix of the
  // transform will multiply the current context transform.
  void AppendTransform(vtkTransform2D *transform);

  // Description:
  // Push/pop the transformation matrix for the painter (sets the underlying
  // matrix for the device when available).
  void PushMatrix();
  void PopMatrix();

  // Description:
  // Apply id as a color.
  void ApplyId(vtkIdType id);

//BTX
protected:
  vtkContext2D();
  ~vtkContext2D();

  vtkContextDevice2D *Device; // The underlying device
  vtkPen *Pen;                // Outlining
  vtkBrush *Brush;            // Fills
  vtkTextProperty *TextProp;  // Text property
  vtkTransform2D *Transform;  // The painter transform

  vtkAbstractContextBufferId *BufferId;

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
