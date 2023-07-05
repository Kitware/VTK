// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractSplineRepresentation
 * @brief   abstract representation for a spline.
 *
 * vtkAbstractSplineRepresentation is a vtkWidgetRepresentation for an abstract spline.
 * This 3D widget defines a spline that can be accessed, set and configured.
 * Deriving classes are then able to combine their own handles using this class.
 */

#ifndef vtkAbstractSplineRepresentation_h
#define vtkAbstractSplineRepresentation_h

#include "vtkCurveRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkParametricFunctionSource;
class vtkParametricSpline;
class vtkPolyDataMapper;

class VTKINTERACTIONWIDGETS_EXPORT vtkAbstractSplineRepresentation : public vtkCurveRepresentation
{
public:
  vtkTypeMacro(vtkAbstractSplineRepresentation, vtkCurveRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Grab the polydata (including points) that defines the spline.  The
   * polydata consists of points and line segments numbering Resolution + 1
   * and Resolution, respectively. Points are guaranteed to be up-to-date when
   * either the InteractionEvent or EndInteraction events are invoked. The
   * user provides the vtkPolyData and the points and polyline are added to it.
   */
  void GetPolyData(vtkPolyData* pd) override;

  ///@{
  /**
   * Set/Get the number of line segments representing the spline for
   * this widget.
   * The default value is 499.
   */
  void SetResolution(int resolution);
  vtkGetMacro(Resolution, int);
  ///@}

  ///@{
  /**
   * Set the parametric spline object.
   * Can be redefined in the child classes for further updates
   * when a spline is set.
   * The default value is nullptr.
   */
  vtkGetObjectMacro(ParametricSpline, vtkParametricSpline);
  virtual void SetParametricSpline(vtkParametricSpline* spline);
  ///@}

  /**
   * Get the position of the spline handles.
   */
  vtkDoubleArray* GetHandlePositions() override;

  /**
   * Get the approximate vs. the true arc length of the spline. Calculated as
   * the summed lengths of the individual straight line segments. Use
   * SetResolution to control the accuracy.
   */
  double GetSummedLength() override;

protected:
  vtkAbstractSplineRepresentation();
  ~vtkAbstractSplineRepresentation() override;

  void CleanRepresentation();

  void SetParametricSplineInternal(vtkParametricSpline* spline);

  // The spline
  vtkParametricSpline* ParametricSpline = nullptr;
  vtkNew<vtkParametricFunctionSource> ParametricFunctionSource;

  // The number of line segments used to represent the spline.
  int Resolution = 499;

  // the mapper supposed to display the spline
  vtkNew<vtkPolyDataMapper> LineMapper;

private:
  vtkAbstractSplineRepresentation(const vtkAbstractSplineRepresentation&) = delete;
  void operator=(const vtkAbstractSplineRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
