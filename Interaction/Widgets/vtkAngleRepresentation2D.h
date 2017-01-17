/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAngleRepresentation.h"

class vtkLeaderActor2D;
class vtkProperty2D;


class VTKINTERACTIONWIDGETS_EXPORT vtkAngleRepresentation2D : public vtkAngleRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static vtkAngleRepresentation2D *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkAngleRepresentation2D,vtkAngleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Satisfy the superclasses API.
   */
  double GetAngle() VTK_OVERRIDE;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]) VTK_OVERRIDE;
  void GetCenterWorldPosition(double pos[3]) VTK_OVERRIDE;
  void GetPoint2WorldPosition(double pos[3]) VTK_OVERRIDE;
  void SetPoint1DisplayPosition(double pos[3]) VTK_OVERRIDE;
  void SetCenterDisplayPosition(double pos[3]) VTK_OVERRIDE;
  void SetPoint2DisplayPosition(double pos[3]) VTK_OVERRIDE;
  void GetPoint1DisplayPosition(double pos[3]) VTK_OVERRIDE;
  void GetCenterDisplayPosition(double pos[3]) VTK_OVERRIDE;
  void GetPoint2DisplayPosition(double pos[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  vtkGetObjectMacro(Ray1,vtkLeaderActor2D);
  vtkGetObjectMacro(Ray2,vtkLeaderActor2D);
  vtkGetObjectMacro(Arc,vtkLeaderActor2D);
  //@}

  /**
   * Method defined by vtkWidgetRepresentation superclass and
   * needed here.
   */
  void BuildRepresentation() VTK_OVERRIDE;

  //@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  //@}

protected:
  vtkAngleRepresentation2D();
  ~vtkAngleRepresentation2D() VTK_OVERRIDE;

  // The pieces that make up the angle representations
  vtkLeaderActor2D *Ray1;
  vtkLeaderActor2D *Ray2;
  vtkLeaderActor2D *Arc;

private:
  vtkAngleRepresentation2D(const vtkAngleRepresentation2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAngleRepresentation2D&) VTK_DELETE_FUNCTION;
};

#endif
