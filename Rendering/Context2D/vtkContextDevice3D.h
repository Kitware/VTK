/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextDevice3D - Abstract class for drawing 3D primitives.
//
// .SECTION Description
// This defines the interface for a vtkContextDevice3D. In this sense a
// ContextDevice is a class used to paint 3D primitives onto a device, such as
// an OpenGL context.
//
// This is private API, and should not be used outside of the vtkContext3D.

#ifndef __vtkContextDevice3D_h
#define __vtkContextDevice3D_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVector.h"       // For the vector coordinates.
#include "vtkRect.h"         // For the rectangles..

class vtkMatrix4x4;
class vtkViewport;
class vtkPen;
class vtkBrush;
class vtkTextProperty;
class vtkUnicodeString;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextDevice3D : public vtkObject
{
public:
  vtkTypeMacro(vtkContextDevice3D, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Draw a polyline between the specified points.
  virtual void DrawPoly(const float *verts, int n,
                        const unsigned char *colors = 0, int nc = 0) = 0;

  // Description:
  // Draw points at the vertex positions specified.
  virtual void DrawPoints(const float *verts, int n,
                          const unsigned char *colors = 0, int nc = 0) = 0;

  // Description:
  // Apply the supplied pen which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkPen
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  virtual void ApplyPen(vtkPen *pen) = 0;

  // Description:
  // Apply the supplied brush which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkBrush
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  virtual void ApplyBrush(vtkBrush *brush) = 0;

  // Description:
  // Set the model view matrix for the display
  virtual void SetMatrix(vtkMatrix4x4 *m) = 0;

  // Description:
  // Set the model view matrix for the display
  virtual void GetMatrix(vtkMatrix4x4 *m) = 0;

  // Description:
  // Multiply the current model view matrix by the supplied one
  virtual void MultiplyMatrix(vtkMatrix4x4 *m) = 0;

  // Description:
  // Push the current matrix onto the stack.
  virtual void PushMatrix() = 0;

  // Description:
  // Pop the current matrix off of the stack.
  virtual void PopMatrix() = 0;

  // Description:
  // Supply a float array of length 4 with x1, y1, width, height specifying
  // clipping region for the device in pixels.
  virtual void SetClipping(const vtkRecti &rect) = 0;

  // Description:
  // Disable clipping of the display.
  // Remove in a future release - retained for API compatibility.
  virtual void DisableClipping() { this->EnableClipping(false); }

  // Description:
  // Enable or disable the clipping of the scene.
  virtual void EnableClipping(bool enable) = 0;

  // Description:
  // Begin drawing, pass in the viewport to set up the view.
  virtual void Begin(vtkViewport*) { }

  // Description:
  // End drawing, clean up the view.
  virtual void End() { }

  // Description:
  // Draw some text to the screen.
  virtual void DrawString(float *point, const vtkStdString &string) = 0;

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkStdString &string,
                                   float bounds[4]) = 0;

  // Description:
  // Draw some text to the screen.
  virtual void DrawString(float *point, const vtkUnicodeString &string) = 0;
  virtual void DrawZAxisLabel(float *point, const vtkStdString &string) = 0;

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  virtual void ComputeStringBounds(const vtkUnicodeString &string,
                                   float bounds[4]) = 0;

  // Description:
  // Draw text using MathText markup for mathematical equations. See
  // http://matplotlib.sourceforge.net/users/mathtext.html for more information.
  virtual void DrawMathTextString(float *point, const vtkStdString &string) = 0;

  // Description:
  // Return true if MathText rendering available on this device.
  virtual bool MathTextIsSupported();

  // Description:
  // Get the text properties object for the vtkContext2D.
  vtkGetObjectMacro(TextProp, vtkTextProperty);

  // Description:
  // Apply the supplied text property which controls how text is rendered.
  // This makes a deep copy of the vtkTextProperty object in the vtkContext2D,
  // it does not hold a pointer to the supplied object.
  void ApplyTextProp(vtkTextProperty *prop);

  enum TextureProperty {
    Nearest = 0x01,
    Linear  = 0x02,
    Stretch = 0x04,
    Repeat  = 0x08
  };


protected:
  vtkContextDevice3D();
  ~vtkContextDevice3D();
  vtkTextProperty *TextProp;  // Text property


private:
  vtkContextDevice3D(const vtkContextDevice3D &); // Not implemented.
  void operator=(const vtkContextDevice3D &);   // Not implemented.
};

#endif
