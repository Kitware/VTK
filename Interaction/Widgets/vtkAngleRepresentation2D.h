// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAngleRepresentation2D
 * @brief   represent the vtkAngleWidget
 *
 * The vtkAngleRepresentation2D is a representation for the
 * vtkAngleWidget. This representation consists of two rays and three
 * vtkHandleRepresentations to place and manipulate the three points defining
 * the angle representation. (Note: the three points are referred to as Point1,
 * Center, and Point2, at the two end points (Point1 and Point2) and Center
 * (around which the angle is measured). This particular implementation is a
 * 2D representation, meaning that it draws in the overlay plane.
 *
 * @sa
 * vtkAngleWidget vtkHandleRepresentation
 */

#ifndef vtkAngleRepresentation2D_h
#define vtkAngleRepresentation2D_h

#include "vtkAngleRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkLeaderActor2D;
class vtkProperty2D;

class VTKINTERACTIONWIDGETS_EXPORT vtkAngleRepresentation2D : public vtkAngleRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static vtkAngleRepresentation2D* New();

  ///@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkAngleRepresentation2D, vtkAngleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Satisfy the superclasses API.
   */
  double GetAngle() override;

  ///@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]) VTK_FUTURE_CONST override;
  void GetCenterWorldPosition(double pos[3]) VTK_FUTURE_CONST override;
  void GetPoint2WorldPosition(double pos[3]) VTK_FUTURE_CONST override;
  void SetPoint1DisplayPosition(double pos[3]) override;
  void SetPoint1WorldPosition(double pos[3]);
  void SetCenterDisplayPosition(double pos[3]) override;
  void SetCenterWorldPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]) override;
  void SetPoint2WorldPosition(double pos[3]);
  void GetPoint1DisplayPosition(double pos[3]) VTK_FUTURE_CONST override;
  void GetCenterDisplayPosition(double pos[3]) VTK_FUTURE_CONST override;
  void GetPoint2DisplayPosition(double pos[3]) VTK_FUTURE_CONST override;
  ///@}

  ///@{
  /**
   * Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  vtkGetObjectMacro(Ray1, vtkLeaderActor2D);
  vtkGetObjectMacro(Ray2, vtkLeaderActor2D);
  vtkGetObjectMacro(Arc, vtkLeaderActor2D);
  ///@}

  ///@{
  /**
   * Set/Get if the widget should use screen space or world space coordinates
   * when trying to place the arc. Screen space may produce nicer results but
   * breaks easily when interacting with the camera.
   *
   * Default is false (screen space)
   */
  vtkSetMacro(Force3DArcPlacement, bool);
  vtkGetMacro(Force3DArcPlacement, bool);
  ///@}

  /**
   * Method defined by vtkWidgetRepresentation superclass and
   * needed here.
   */
  void BuildRepresentation() override;

  ///@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;
  int RenderOverlay(vtkViewport* viewport) override;
  ///@}

protected:
  vtkAngleRepresentation2D();
  ~vtkAngleRepresentation2D() override;

  // The pieces that make up the angle representations
  vtkNew<vtkLeaderActor2D> Ray1;
  vtkNew<vtkLeaderActor2D> Ray2;
  vtkNew<vtkLeaderActor2D> Arc;

  bool Force3DArcPlacement = false;

private:
  vtkAngleRepresentation2D(const vtkAngleRepresentation2D&) = delete;
  void operator=(const vtkAngleRepresentation2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
