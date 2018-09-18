/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAngleRepresentation3D
 * @brief   represent the vtkAngleWidget
 *
 * The vtkAngleRepresentation3D is a representation for the
 * vtkAngleWidget. This representation consists of two rays and three
 * vtkHandleRepresentations to place and manipulate the three points defining
 * the angle representation. (Note: the three points are referred to as Point1,
 * Center, and Point2, at the two end points (Point1 and Point2) and Center
 * (around which the angle is measured). This particular implementation is a
 * 3D representation, meaning that it draws in the overlay plane.
 *
 * @sa
 * vtkAngleWidget vtkHandleRepresentation
*/

#ifndef vtkAngleRepresentation3D_h
#define vtkAngleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAngleRepresentation.h"

class vtkActor;
class vtkProperty;
class vtkPolyDataMapper;
class vtkLineSource;
class vtkArcSource;
class vtkFollower;
class vtkVectorText;
class vtkPolyDataMapper;
class vtkTextProperty;

class VTKINTERACTIONWIDGETS_EXPORT vtkAngleRepresentation3D : public vtkAngleRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static vtkAngleRepresentation3D *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkAngleRepresentation3D,vtkAngleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API. Angle returned is in radians.
   */
  double GetAngle() override;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetCenterWorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  virtual void SetPoint1WorldPosition(double pos[3]);
  void SetPoint1DisplayPosition(double pos[3]) override;
  virtual void SetCenterWorldPosition(double pos[3]);
  void SetCenterDisplayPosition(double pos[3]) override;
  virtual void SetPoint2WorldPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetCenterDisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;
  //@}

  //@{
  /**
   * Set/Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  vtkGetObjectMacro(Ray1,vtkActor);
  vtkGetObjectMacro(Ray2,vtkActor);
  vtkGetObjectMacro(Arc,vtkActor);
  vtkGetObjectMacro(TextActor,vtkFollower);
  //@}

  //@{
  /**
   * Scale text.
   */
  virtual void SetTextActorScale( double scale[3] );
  virtual double * GetTextActorScale();
  //@}

  /**
   * Method defined by vtkWidgetRepresentation superclass and
   * needed here.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow *w) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  vtkAngleRepresentation3D();
  ~vtkAngleRepresentation3D() override;

  // The pieces that make up the angle representations
  vtkLineSource     *Line1Source;
  vtkLineSource     *Line2Source;
  vtkArcSource      *ArcSource;
  vtkPolyDataMapper *Line1Mapper;
  vtkPolyDataMapper *Line2Mapper;
  vtkPolyDataMapper *ArcMapper;
  vtkActor          *Ray1;
  vtkActor          *Ray2;
  vtkActor          *Arc;
  vtkFollower       *TextActor;
  vtkPolyDataMapper *TextMapper;
  vtkVectorText     *TextInput;
  double             Angle;
  bool               ScaleInitialized;
  double             TextPosition[3];

private:
  vtkAngleRepresentation3D(const vtkAngleRepresentation3D&) = delete;
  void operator=(const vtkAngleRepresentation3D&) = delete;
};

#endif
