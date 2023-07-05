// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCursor2D
 * @brief   generate a 2D cursor representation
 *
 * vtkCursor2D is a class that generates a 2D cursor representation.
 * The cursor consists of two intersection axes lines that meet at the
 * cursor focus. Several optional features are available as well. An
 * optional 2D bounding box may be enabled. An inner radius, centered at
 * the focal point, can be set that erases the intersecting lines (e.g.,
 * it leaves a clear area under the focal point so you can see
 * what you are selecting). And finally, an optional point can be
 * enabled located at the focal point. All of these features can be turned
 * on and off independently.
 *
 */

#ifndef vtkCursor2D_h
#define vtkCursor2D_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkCursor2D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCursor2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with model bounds = (-10,10,-10,10), focal point = (0,0),
   * radius=2, all parts of cursor visible, and wrapping off.
   */
  static vtkCursor2D* New();

  ///@{
  /**
   * Set / get the bounding box of the 2D cursor. This defines the outline
   * of the cursor, and where the focal point should lie.
   */
  void SetModelBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  void SetModelBounds(const double bounds[6]);
  vtkGetVectorMacro(ModelBounds, double, 6);
  ///@}

  ///@{
  /**
   * Set/Get the position of cursor focus. If translation mode is on,
   * then the entire cursor (including bounding box, cursor, and shadows)
   * is translated. Otherwise, the focal point will either be clamped to the
   * bounding box, or wrapped, if Wrap is on. (Note: this behavior requires
   * that the bounding box is set prior to the focal point.) Note that the
   * method takes a 3D point but ignores the z-coordinate value.
   */
  void SetFocalPoint(double x[3]);
  void SetFocalPoint(double x, double y, double z)
  {
    double xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    this->SetFocalPoint(xyz);
  }
  vtkGetVectorMacro(FocalPoint, double, 3);
  ///@}

  ///@{
  /**
   * Turn on/off the wireframe bounding box.
   */
  vtkSetMacro(Outline, vtkTypeBool);
  vtkGetMacro(Outline, vtkTypeBool);
  vtkBooleanMacro(Outline, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the wireframe axes.
   */
  vtkSetMacro(Axes, vtkTypeBool);
  vtkGetMacro(Axes, vtkTypeBool);
  vtkBooleanMacro(Axes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify a radius for a circle. This erases the cursor
   * lines around the focal point.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Turn on/off the point located at the cursor focus.
   */
  vtkSetMacro(Point, vtkTypeBool);
  vtkGetMacro(Point, vtkTypeBool);
  vtkBooleanMacro(Point, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable the translation mode. If on, changes in cursor position
   * cause the entire widget to translate along with the cursor.
   * By default, translation mode is off.
   */
  vtkSetMacro(TranslationMode, vtkTypeBool);
  vtkGetMacro(TranslationMode, vtkTypeBool);
  vtkBooleanMacro(TranslationMode, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off cursor wrapping. If the cursor focus moves outside the
   * specified bounds, the cursor will either be restrained against the
   * nearest "wall" (Wrap=off), or it will wrap around (Wrap=on).
   */
  vtkSetMacro(Wrap, vtkTypeBool);
  vtkGetMacro(Wrap, vtkTypeBool);
  vtkBooleanMacro(Wrap, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn every part of the cursor on or off.
   */
  void AllOn();
  void AllOff();
  ///@}

protected:
  vtkCursor2D();
  ~vtkCursor2D() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double ModelBounds[6];
  double FocalPoint[3];
  vtkTypeBool Outline;
  vtkTypeBool Axes;
  vtkTypeBool Point;
  double Radius;
  vtkTypeBool TranslationMode;
  vtkTypeBool Wrap;

private:
  vtkCursor2D(const vtkCursor2D&) = delete;
  void operator=(const vtkCursor2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
