// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkRadialGridActor2D
 * @brief vtkRadialGridActor2D displays in overlay a list of vtkAxisActor2D
 * sharing a same origin and rotating around it.
 *
 * Rotation is defined by a start and an end angle and the origin.
 * This can be useful in a polar axes actor.
 */

#ifndef vtkRadialGridActor2D_h
#define vtkRadialGridActor2D_h

#include "vtkActor2D.h"
#include "vtkRenderingAnnotationModule.h" // For export macro

#include "vtkSetGet.h"       // for macros
#include "vtkSmartPointer.h" // for smart pointer

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkAxisActor2D;
class vtkPoints;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkRadialGridActor2D : public vtkActor2D
{
public:
  static vtkRadialGridActor2D* New();
  vtkTypeMacro(vtkRadialGridActor2D, vtkActor2D);

  /**
   * Print own members and call Superclass.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Standard render methods.
   */
  ///@{
  int RenderOverlay(vtkViewport* viewport) override;

  /**
   * Overridden as a no-op. Needed to avoid warnings/errors from Superclass.
   * Return 1.
   */
  int RenderOpaqueGeometry(vtkViewport*) override { return 1; }

  /**
   * No opaque geometry for this actor.
   * Return 0.
   */
  vtkTypeBool HasOpaqueGeometry() override { return 0; }

  /**
   * No translucent geometry for this actor.
   * Return 0.
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override { return 0; }
  ///@}

  /**
   * Append the underlying 2D actors to the collection.
   */
  void GetActors2D(vtkPropCollection* pc) override;

  ///@{
  /**
   * Set/Get a text property on the underlying axes labels.
   */
  void SetTextProperty(vtkTextProperty* property);
  vtkTextProperty* GetTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the number of axis of the grid.
   * Default is 6.
   */
  vtkSetClampMacro(NumberOfAxes, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfAxes, int);
  ///@}

  ///@{
  /**
   * Set/Get the start angle (in degrees) of the radial grid.
   * This is the orientation of the first axis, used as reference for the others.
   * Default is 0.
   *
   * @see SetEndAngle, GetEndAngle, SetNumberOfAxes, GetNumberOfAxes
   */
  vtkSetMacro(StartAngle, double);
  vtkGetMacro(StartAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the end angle (in degrees) of the radial grid.
   * This is the orientation of the last axis.
   * Default is 90.
   *
   * @see SetStartAngle, SetEndAngle, SetNumberOfAxes, GetNumberOfAxes.
   */
  vtkSetMacro(EndAngle, double);
  vtkGetMacro(EndAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the origin (in normalized viewport coordinates) of the radial grid.
   * Default is [0.5, 0.5]
   */
  vtkSetVector2Macro(Origin, double);
  vtkGetVector2Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Set/Get the number of ticks for each axis.
   * Default is 6.
   */
  vtkSetClampMacro(NumberOfTicks, int, 2, VTK_INT_MAX);
  vtkGetMacro(NumberOfTicks, int);
  ///@}

  ///@{
  /**
   * Set/Get the length of each axis, in viewport coordinates.
   * Default is 100.
   */
  vtkSetClampMacro(AxesViewportLength, double, 2, VTK_DOUBLE_MAX);
  vtkGetMacro(AxesViewportLength, double);
  ///@}

  ///@{
  /**
   * Get the first/last axes points.
   */
  vtkPoints* GetFirstAxesPoints();
  vtkPoints* GetLastAxesPoints();
  ///@}

protected:
  vtkRadialGridActor2D();
  ~vtkRadialGridActor2D() override;

private:
  vtkRadialGridActor2D(const vtkRadialGridActor2D&) = delete;
  void operator=(const vtkRadialGridActor2D&) = delete;

  /**
   * Compute axis ending position in viewport coordinates.
   */
  void ComputeAxisRelativeEndPosition(int index, double position[2]);

  /**
   * Get the angle for the given axes, in degree.
   */
  double GetAxisAngle(int index);

  /**
   * Compute axis range in world coordinates.
   * Min is always 0. Max is computed from AxesViewportLength
   */
  void ComputeAxisWorldRange(vtkViewport* viewport, vtkAxisActor2D* axis, double range[2]);

  /**
   * Set axis title string do display angle measurement.
   */
  void UpdateAxisTitle(vtkAxisActor2D* axis, double angle);

  /**
   * Create and set up the internal axes depending on current configuration
   * This includes (not exhaustive): number of Axes, display range, position
   */
  void SetupAxes(vtkViewport* viewport);

  std::vector<vtkSmartPointer<vtkAxisActor2D>> Axes;

  int NumberOfAxes = 6;
  int NumberOfTicks = 6;
  double StartAngle = 0.;
  double EndAngle = 90.;
  // relative display coordinates
  double Origin[2] = { 0.5, 0.5 };
  // display coordinantes
  double AxesViewportLength = 100;

  vtkSmartPointer<vtkTextProperty> TextProperty;
};

VTK_ABI_NAMESPACE_END

#endif
