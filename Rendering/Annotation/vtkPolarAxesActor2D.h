// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkPolarAxesActor2D
 * @brief Display polar axes in Viewport 2D space.
 *
 * The polar axes actor is drawn on overlay. It displays polar coordinates.
 * It is made of concentric axes linked with arcs.
 *
 * @warning
 * Please be aware that the axes coordinate values are subject to perspective
 * effects. With perspective projection, the computed distances may look wrong.
 * These effects are not present when parallel projection is enabled.
 * (@sa vtkCamera::ParallelProjectionOn())
 *
 * It is the polar counter part of vtkLegendScaleActor.
 * @sa vtkRadialGridActor2D
 */

#ifndef vtkPolarAxesActor2D_h
#define vtkPolarAxesActor2D_h

#include "vtkActor2D.h"
#include "vtkRenderingAnnotationModule.h" // For export macro

#include "vtkNew.h"    // for vtkNew
#include "vtkSetGet.h" // for macros

VTK_ABI_NAMESPACE_BEGIN
class vtkArcGridActorInternal;
class vtkRadialGridActor2D;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkPolarAxesActor2D : public vtkActor2D
{
public:
  static vtkPolarAxesActor2D* New();
  vtkTypeMacro(vtkPolarAxesActor2D, vtkActor2D);

  /**
   * Print own members and call Superclass.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Standard vtkProp render methods.
   */
  ///@{
  /**
   * Render the actor as overlay.
   */
  int RenderOverlay(vtkViewport* viewport) override;

  /**
   * Overridden as a no-op. Needed to avoid warnings/errors from Superclass
   * that expects a Mapper to be defined. It is not the case here as
   * everything is delegated to other internal actors.
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
   * Set/Get the text property for the axes.
   */
  void SetAxesTextProperty(vtkTextProperty* property);
  vtkTextProperty* GetAxesTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the number of Axes to use.
   * Default is 6.
   */
  void SetNumberOfAxes(int number);
  int GetNumberOfAxes();
  ///@}

  ///@{
  /**
   * Set/Get the number of ticks for each axis.
   * Default is 6.
   */
  void SetNumberOfAxesTicks(int number);
  int GetNumberOfAxesTicks();
  ///@}

  ///@{
  /**
   * Set/Get the length of each axis in viewport coordinates.
   * Default is 100.
   */
  void SetAxesLength(double length);
  double GetAxesLength();
  ///@}

  ///@{
  /**
   * Set the angle for the main radial axis.
   * Default is 0.
   */
  void SetStartAngle(double angle);
  double GetStartAngle();
  ///@}

  ///@{
  /**
   * Set the angle for the last radial axis.
   * Default is 90.
   */
  void SetEndAngle(double angle);
  double GetEndAngle();
  ///@}

  ///@{
  /**
   * Set the origin of the radial measurement, in normalized viewport coordinates.
   * Default is [0.5, 0.5].
   */
  void SetOrigin(double x, double y);
  void SetOrigin(double origin[2]) { this->SetOrigin(origin[0], origin[1]); }
  void GetOrigin(double origin[2]);
  ///@}

protected:
  vtkPolarAxesActor2D();
  ~vtkPolarAxesActor2D() override = default;

private:
  vtkPolarAxesActor2D(const vtkPolarAxesActor2D&) = delete;
  void operator=(const vtkPolarAxesActor2D&) = delete;

  vtkNew<vtkRadialGridActor2D> RadialGrid;
  vtkNew<vtkArcGridActorInternal> ArcGrid;
};

VTK_ABI_NAMESPACE_END

#endif
