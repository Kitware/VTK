/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLContextDevice3D
 * @brief   OpenGL class drawing 3D primitives.
 *
 *
 * This defines the implementation of a 3D context device for drawing simple
 * primitives.
*/

#ifndef vtkOpenGLContextDevice3D_h
#define vtkOpenGLContextDevice3D_h

#include "vtkRenderingContextOpenGLModule.h" // For export macro
#include "vtkContextDevice3D.h"
#include "vtkNew.h"             // For ivars.

class vtkBrush;
class vtkPen;

class VTKRENDERINGCONTEXTOPENGL_EXPORT vtkOpenGLContextDevice3D : public vtkContextDevice3D
{
public:
  vtkTypeMacro(vtkOpenGLContextDevice3D, vtkContextDevice3D);
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkOpenGLContextDevice3D * New();

  /**
   * Draw a polyline between the specified points.
   * \sa DrawLines()
   */
  void DrawPoly(const float *verts, int n, const unsigned char *colors, int nc);

  /**
   * Draw lines defined by specified pair of points.
   * \sa DrawPoly()
   */
  void DrawLines(const float *verts, int n, const unsigned char *colors, int nc);

  /**
   * Draw points at the vertex positions specified.
   */
  void DrawPoints(const float *verts, int n,
                  const unsigned char *colors, int nc);

  /**
   * Draw triangles to generate the specified mesh.
   */
  void DrawTriangleMesh(const float *mesh, int n,
                        const unsigned char *colors, int nc);

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the vtkPen
   * object in the vtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyPen(vtkPen *pen);

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the vtkBrush
   * object in the vtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyBrush(vtkBrush *brush);

  /**
   * Set the model view matrix for the display
   */
  void SetMatrix(vtkMatrix4x4 *m);

  /**
   * Set the model view matrix for the display
   */
  void GetMatrix(vtkMatrix4x4 *m);

  /**
   * Multiply the current model view matrix by the supplied one
   */
  void MultiplyMatrix(vtkMatrix4x4 *m);

  /**
   * Push the current matrix onto the stack.
   */
  void PushMatrix();

  /**
   * Pop the current matrix off of the stack.
   */
  void PopMatrix();

  /**
   * Supply a float array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  void SetClipping(const vtkRecti &rect);

  /**
   * Enable or disable the clipping of the scene.
   */
  void EnableClipping(bool enable);

  //@{
  /**
   * Enable/Disable the specified clipping plane.
   * i is the index of the clipping plane being enabled or disabled (0 - 5).
   * planeEquation points to the four coefficients of the equation for the
   * clipping plane: Ax + By + Cz + D = 0.  This is the equation format
   * expected by glClipPlane.
   */
  void EnableClippingPlane(int i, double *planeEquation);
  void DisableClippingPlane(int i);
  //@}

protected:
  vtkOpenGLContextDevice3D();
  ~vtkOpenGLContextDevice3D();

  /**
   * Begin drawing, turn on the depth buffer.
   */
  virtual void EnableDepthBuffer();

  /**
   * End drawing, turn off the depth buffer.
   */
  virtual void DisableDepthBuffer();

private:
  vtkOpenGLContextDevice3D(const vtkOpenGLContextDevice3D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLContextDevice3D &) VTK_DELETE_FUNCTION;

  //@{
  /**
   * Private data pointer of the class
   */
  class Private;
  Private *Storage;
  //@}

  vtkNew<vtkBrush> Brush;
  vtkNew<vtkPen>   Pen;
};

#endif
