// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkContextDevice3D
 * @brief   Abstract class for drawing 3D primitives.
 *
 *
 * This defines the interface for a vtkContextDevice3D. In this sense a
 * ContextDevice is a class used to paint 3D primitives onto a device, such as
 * an OpenGL context.
 *
 * This is private API, and should not be used outside of the vtkContext3D.
 */

#ifndef vtkContextDevice3D_h
#define vtkContextDevice3D_h

#include "vtkObject.h"
#include "vtkRect.h"                     // For the rectangles..
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkVector.h"                   // For the vector coordinates.

#include <cstdint> // For std::uintptr_t

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;
class vtkViewport;
class vtkPen;
class vtkBrush;
class vtkPoints;
class vtkUnsignedCharArray;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextDevice3D : public vtkObject
{
public:
  vtkTypeMacro(vtkContextDevice3D, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkContextDevice3D* New();

  /**
   * Draw a polyline between the specified points.
   * \sa DrawLines()
   */
  virtual void DrawPoly(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;

  /**
   * Draw lines defined by specified pair of points.
   * \sa DrawPoly()
   */
  virtual void DrawLines(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;

  /**
   * Draw points at the vertex positions specified.
   */
  virtual void DrawPoints(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;
  virtual void DrawPoints(vtkDataArray* positions, vtkUnsignedCharArray* colors,
    std::uintptr_t vtkNotUsed(cacheIdentifier));

  /**
   * Draw triangles to generate the specified mesh.
   */
  virtual void DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc) = 0;
  virtual void DrawTriangleMesh(vtkDataArray* positions, vtkUnsignedCharArray* colors,
    std::uintptr_t vtkNotUsed(cacheIdentifier));

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the vtkPen
   * object in the vtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyPen(vtkPen* pen) = 0;

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the vtkBrush
   * object in the vtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyBrush(vtkBrush* brush) = 0;

  /**
   * Set the model view matrix for the display
   */
  virtual void SetMatrix(vtkMatrix4x4* m) = 0;

  /**
   * Set the model view matrix for the display
   */
  virtual void GetMatrix(vtkMatrix4x4* m) = 0;

  /**
   * Multiply the current model view matrix by the supplied one
   */
  virtual void MultiplyMatrix(vtkMatrix4x4* m) = 0;

  /**
   * Push the current matrix onto the stack.
   */
  virtual void PushMatrix() = 0;

  /**
   * Pop the current matrix off of the stack.
   */
  virtual void PopMatrix() = 0;

  /**
   * Supply a float array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  virtual void SetClipping(const vtkRecti& rect) = 0;

  /**
   * Disable clipping of the display.
   * Remove in a future release - retained for API compatibility.
   */
  virtual void DisableClipping() { this->EnableClipping(false); }

  /**
   * Enable or disable the clipping of the scene.
   */
  virtual void EnableClipping(bool enable) = 0;

  ///@{
  /**
   * Enable/Disable the specified clipping plane.
   */
  virtual void EnableClippingPlane(int i, double* planeEquation) = 0;
  virtual void DisableClippingPlane(int i) = 0;
  ///@}

  /**
   * Concrete graphics implementations maintain a cache of heavy-weight buffer objects
   * to achieve higher interactive framerates.
   * This method requests the devices to release the cached objects for a given cache identifier.
   */
  virtual void ReleaseCache(std::uintptr_t vtkNotUsed(cacheIdentifier)) {}

protected:
  vtkContextDevice3D();
  ~vtkContextDevice3D() override;

private:
  vtkContextDevice3D(const vtkContextDevice3D&) = delete;
  void operator=(const vtkContextDevice3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
