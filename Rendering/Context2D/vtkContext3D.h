/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContext3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContext3D - Class for drawing 3D primitives to a graphical context.
//
// .SECTION Description
// This defines the interface for drawing onto a 3D context. The context must
// be set up with a vtkContextDevice3D derived class that provides the functions
// to facilitate the low level calls to the context. Currently only an OpenGL
// based device is provided.

#ifndef __vtkContext3D_h
#define __vtkContext3D_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVector.h" // For the vector coordinates.
#include "vtkSmartPointer.h" // For SP ivars.

class vtkContextDevice3D;
class vtkPen;
class vtkBrush;
class vtkTransform;
class vtkPoints2D;
class vtkUnicodeString;
class vtkTextProperty;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContext3D : public vtkObject
{
public:
  vtkTypeMacro(vtkContext3D, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 3D context object.
  static vtkContext3D *New();

  // Description:
  // Begin painting on a vtkContextDevice3D, no painting can occur before this
  // call has been made. Only one painter is allowed at a time on any given
  // paint device. Returns true if successful, otherwise false.
  bool Begin(vtkContextDevice3D *device);

  // Description:
  // Get access to the underlying 3D context.
  vtkContextDevice3D * GetDevice();

  // Description:
  // Ends painting on the device, you would not usually need to call this as it
  // should be called by the destructor. Returns true if the painter is no
  // longer active, otherwise false.
  bool End();

  // Description:
  // Draw a line between the specified points.
  void DrawLine(const vtkVector3f &start, const vtkVector3f &end);

  // Description:
  // Draw a point at the point in 3D space.
  void DrawPoint(const vtkVector3f &point);

  // Description:
  // Draw a sequence of points at the specified locations.
  void DrawPoints(const float *points, int n);
  
  // Description:
  // Draw a sequence of points at the specified locations.  The points will be
  // colored by the colors array, which must have nc_comps components
  // (defining a single color).
  void DrawPoints(const float *points, int n,
                  unsigned char *colors, int nc_comps);

  // Description:
  // Apply the supplied pen which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkPen
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  void ApplyPen(vtkPen *pen);

  // Description:
  // Apply the supplied brush which controls the outlines of shapes, as well as
  // lines, points and related primitives. This makes a deep copy of the vtkBrush
  // object in the vtkContext2D, it does not hold a pointer to the supplied object.
  void ApplyBrush(vtkBrush *brush);

  // Description:
  // Set the transform for the context, the underlying device will use the
  // matrix of the transform. Note, this is set immediately, later changes to
  // the matrix will have no effect until it is set again.
  void SetTransform(vtkTransform *transform);

  // Description:
  // Compute the current transform applied to the context.
  vtkTransform* GetTransform();

  // Description:
  // Append the transform for the context, the underlying device will use the
  // matrix of the transform. Note, this is set immediately, later changes to
  // the matrix will have no effect until it is set again. The matrix of the
  // transform will multiply the current context transform.
  void AppendTransform(vtkTransform *transform);

  // Description:
  // Push/pop the transformation matrix for the painter (sets the underlying
  // matrix for the device when available).
  void PushMatrix();
  void PopMatrix();
  
  // Description:
  // Draw some text to the screen in a bounding rectangle with the alignment
  // of the text properties respecting the rectangle. The points should be
  // supplied as bottom corner (x, y), width, height.
  void DrawStringRect(vtkPoints2D *rect, const vtkStdString &string);
  void DrawStringRect(vtkPoints2D *rect, const vtkUnicodeString &string);
  void DrawStringRect(vtkPoints2D *rect, const char* string);

  // Description:
  // Draw some text to the screen.
  void DrawString(vtkPoints2D *point, const vtkStdString &string);
  void DrawString(float x, float y, const vtkStdString &string);
  void DrawString(vtkPoints2D *point, const vtkUnicodeString &string);
  void DrawString(float x, float y, const vtkUnicodeString &string);
  void DrawString(vtkPoints2D *point, const char* string);
  void DrawString(float x, float y, const char* string);

  // Description:
  // Compute the bounds of the supplied string. The bounds will be copied to the
  // supplied bounds variable, the first two elements are the bottom corner of
  // the string, and the second two elements are the width and height of the
  // bounding box.
  // NOTE: This function does not take account of the text rotation.
  void ComputeStringBounds(const vtkStdString &string, vtkPoints2D *bounds);
  void ComputeStringBounds(const vtkStdString &string, float bounds[4]);
  void ComputeStringBounds(const vtkUnicodeString &string, vtkPoints2D *bounds);
  void ComputeStringBounds(const vtkUnicodeString &string, float bounds[4]);
  void ComputeStringBounds(const char* string, vtkPoints2D *bounds);
  void ComputeStringBounds(const char* string, float bounds[4]);

  // Description:
  // Draw a MathText formatted equation to the screen. See
  // http://matplotlib.sourceforge.net/users/mathtext.html for more information.
  // MathText requires matplotlib and python, and the vtkMatplotlib module must
  // be enabled manually during build configuration. This method will do nothing
  // but print a warning if vtkMathTextUtilities::GetInstance() returns NULL.
  void DrawMathTextString(vtkPoints2D *point, const vtkStdString &string);
  void DrawMathTextString(float x, float y, const vtkStdString &string);
  void DrawMathTextString(vtkPoints2D *point, const char *string);
  void DrawMathTextString(float x, float y, const char *string);

  // Description:
  // Draw a MathText formatted equation to the screen. See
  // http://matplotlib.sourceforge.net/users/mathtext.html for more information.
  // MathText requires matplotlib and python, and the vtkMatplotlib module must
  // be enabled manually during build configuration.
  // If MathText is not available on the target device the non-MathText string
  // in "fallback" is rendered using DrawString.
  void DrawMathTextString(vtkPoints2D *point, const vtkStdString &string,
                          const vtkStdString &fallback);
  void DrawMathTextString(float x, float y, const vtkStdString &string,
                          const vtkStdString &fallback);
  void DrawMathTextString(vtkPoints2D *point, const char *string,
                          const char *fallback);
  void DrawMathTextString(float x, float y, const char *string,
                          const char *fallback);


  // Description:
  // Return true if MathText rendering available on the current device.
  bool MathTextIsSupported();
  
  // Description:
  // Apply the supplied text property which controls how text is rendered.
  // This makes a deep copy of the vtkTextProperty object in the vtkContext3D,
  // it does not hold a pointer to the supplied object.
  void ApplyTextProp(vtkTextProperty *prop);

protected:
  vtkContext3D();
  ~vtkContext3D();

  vtkSmartPointer<vtkContextDevice3D> Device; // The underlying device
  vtkSmartPointer<vtkTransform> Transform;    // Current transform

private:
  // Description:
  // Calculate position of text for rendering in a rectangle.
  vtkVector2f CalculateTextPosition(vtkPoints2D* rect);

  vtkContext3D(const vtkContext3D &);   // Not implemented.
  void operator=(const vtkContext3D &); // Not implemented.
};

#endif // VTKCONTEXT3D_H
