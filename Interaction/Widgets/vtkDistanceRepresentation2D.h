/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDistanceRepresentation2D
 * @brief   represent the vtkDistanceWidget
 *
 * The vtkDistanceRepresentation2D is a representation for the
 * vtkDistanceWidget. This representation consists of a measuring line (axis)
 * and two vtkHandleWidgets to place the end points of the line. Note that
 * this particular widget draws its representation in the overlay plane, and
 * the handles also operate in the 2D overlay plane. (If you desire to use
 * the distance widget for 3D measurements, use the
 * vtkDistanceRepresentation3D.)
 *
 * @sa
 * vtkDistanceWidget vtkDistanceRepresentation vtkDistanceRepresentation3D
*/

#ifndef vtkDistanceRepresentation2D_h
#define vtkDistanceRepresentation2D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkDistanceRepresentation.h"

class vtkAxisActor2D;
class vtkProperty2D;


class VTKINTERACTIONWIDGETS_EXPORT vtkDistanceRepresentation2D : public vtkDistanceRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static vtkDistanceRepresentation2D *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkDistanceRepresentation2D,vtkDistanceRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API.
   */
  double GetDistance() override
    {return this->Distance;}

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  double* GetPoint1WorldPosition() override;
  double* GetPoint2WorldPosition() override;
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  void SetPoint1WorldPosition(double pos[3]) override;
  void SetPoint2WorldPosition(double pos[3]) override;
  //@}

  void SetPoint1DisplayPosition(double pos[3]) override;
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;

  //@{
  /**
   * Retrieve the vtkAxisActor2D used to draw the measurement axis. With this
   * properties can be set and so on. There is also a convenience method to
   * get the axis property.
   */
  vtkAxisActor2D *GetAxis();
  vtkProperty2D  *GetAxisProperty();
  //@}

  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow *w) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  //@}

protected:
  vtkDistanceRepresentation2D();
  ~vtkDistanceRepresentation2D() override;

  // Add a line to the mix
  vtkAxisActor2D *AxisActor;
  vtkProperty2D  *AxisProperty;

  // The distance between the two points
  double Distance;

private:
  vtkDistanceRepresentation2D(const vtkDistanceRepresentation2D&) = delete;
  void operator=(const vtkDistanceRepresentation2D&) = delete;
};

#endif
