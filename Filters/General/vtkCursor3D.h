// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCursor3D
 * @brief   generate a 3D cursor representation
 *
 * vtkCursor3D is an object that generates a 3D representation of a cursor.
 * The cursor consists of a wireframe bounding box, three intersecting
 * axes lines that meet at the cursor focus, and "shadows" or projections
 * of the axes against the sides of the bounding box. Each of these
 * components can be turned on/off.
 *
 * This filter generates two output datasets. The first (Output) is just the
 * geometric representation of the cursor. The second (Focus) is a single
 * point at the focal point.
 */

#ifndef vtkCursor3D_h
#define vtkCursor3D_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkCursor3D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCursor3D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with model bounds = (-1,1,-1,1,-1,1), focal point = (0,0,0),
   * all parts of cursor visible, and wrapping off.
   */
  static vtkCursor3D* New();

  ///@{
  /**
   * Set / get the boundary of the 3D cursor.
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
   * that the bounding box is set prior to the focal point.)
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
   * Turn on/off the wireframe x-shadows.
   */
  vtkSetMacro(XShadows, vtkTypeBool);
  vtkGetMacro(XShadows, vtkTypeBool);
  vtkBooleanMacro(XShadows, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the wireframe y-shadows.
   */
  vtkSetMacro(YShadows, vtkTypeBool);
  vtkGetMacro(YShadows, vtkTypeBool);
  vtkBooleanMacro(YShadows, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the wireframe z-shadows.
   */
  vtkSetMacro(ZShadows, vtkTypeBool);
  vtkGetMacro(ZShadows, vtkTypeBool);
  vtkBooleanMacro(ZShadows, vtkTypeBool);
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

  /**
   * Get the focus for this filter.
   */
  vtkPolyData* GetFocus() { return this->Focus; }

  ///@{
  /**
   * Turn every part of the 3D cursor on or off.
   */
  void AllOn();
  void AllOff();
  ///@}

protected:
  vtkCursor3D();
  ~vtkCursor3D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkPolyData* Focus;
  double ModelBounds[6];
  double FocalPoint[3];
  vtkTypeBool Outline;
  vtkTypeBool Axes;
  vtkTypeBool XShadows;
  vtkTypeBool YShadows;
  vtkTypeBool ZShadows;
  vtkTypeBool TranslationMode;
  vtkTypeBool Wrap;

private:
  vtkCursor3D(const vtkCursor3D&) = delete;
  void operator=(const vtkCursor3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
