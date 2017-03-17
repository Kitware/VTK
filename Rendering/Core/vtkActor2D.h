/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkActor2D
 * @brief   a actor that draws 2D data
 *
 * vtkActor2D is similar to vtkActor, but it is made to be used with two
 * dimensional images and annotation.  vtkActor2D has a position but does not
 * use a transformation matrix like vtkActor (see the superclass vtkProp
 * for information on positioning vtkActor2D).  vtkActor2D has a reference to
 * a vtkMapper2D object which does the rendering.
 *
 * @sa
 * vtkProp  vtkMapper2D vtkProperty2D
*/

#ifndef vtkActor2D_h
#define vtkActor2D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkMapper2D;
class vtkProperty2D;

class VTKRENDERINGCORE_EXPORT vtkActor2D : public vtkProp
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  vtkTypeMacro(vtkActor2D,vtkProp);

  /**
   * Creates an actor2D with the following defaults:
   * position (0,0) (coordinate system is viewport);
   * at layer 0.
   */
  static vtkActor2D* New();

  //@{
  /**
   * Support the standard render methods.
   */
  virtual int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  virtual int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  virtual int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the vtkMapper2D which defines the data to be drawn.
   */
  virtual void SetMapper(vtkMapper2D *mapper);
  vtkGetObjectMacro(Mapper, vtkMapper2D);
  //@}

  //@{
  /**
   * Set/Get the layer number in the overlay planes into which to render.
   */
  vtkSetMacro(LayerNumber, int);
  vtkGetMacro(LayerNumber, int);
  //@}

  /**
   * Returns this actor's vtkProperty2D.  Creates a property if one
   * doesn't already exist.
   */
  vtkProperty2D* GetProperty();

  /**
   * Set this vtkProp's vtkProperty2D.
   */
  virtual void SetProperty(vtkProperty2D*);

  //@{
  /**
   * Get the PositionCoordinate instance of vtkCoordinate.
   * This is used for for complicated or relative positioning.
   * The position variable controls the lower left corner of the Actor2D
   */
  vtkViewportCoordinateMacro(Position);
  //@}

  /**
   * Set the Prop2D's position in display coordinates.
   */
  void SetDisplayPosition(int,int);

  //@{
  /**
   * Access the Position2 instance variable. This variable controls
   * the upper right corner of the Actor2D. It is by default
   * relative to Position and in normalized viewport coordinates.
   * Some 2D actor subclasses ignore the position2 variable
   */
  vtkViewportCoordinateMacro(Position2);
  //@}

  //@{
  /**
   * Set/Get the height and width of the Actor2D. The value is expressed
   * as a fraction of the viewport. This really is just another way of
   * setting the Position2 instance variable.
   */
  void SetWidth(double w);
  double GetWidth();
  void SetHeight(double h);
  double GetHeight();
  //@}

  /**
   * Return this objects MTime.
   */
  virtual vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  virtual void GetActors2D(vtkPropCollection *pc) VTK_OVERRIDE;

  /**
   * Shallow copy of this vtkActor2D. Overloads the virtual vtkProp method.
   */
  virtual void ShallowCopy(vtkProp *prop) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Return the actual vtkCoordinate reference that the mapper should use
   * to position the actor. This is used internally by the mappers and should
   * be overridden in specialized subclasses and otherwise ignored.
   */
  virtual vtkCoordinate *GetActualPositionCoordinate(void)
    { return this->PositionCoordinate; }

  /**
   * Return the actual vtkCoordinate reference that the mapper should use
   * to position the actor. This is used internally by the mappers and should
   * be overridden in specialized subclasses and otherwise ignored.
   */
  virtual vtkCoordinate *GetActualPosition2Coordinate(void)
    { return this->Position2Coordinate; }

protected:
  vtkActor2D();
  ~vtkActor2D();

  vtkMapper2D *Mapper;
  int LayerNumber;
  vtkProperty2D *Property;
  vtkCoordinate *PositionCoordinate;
  vtkCoordinate *Position2Coordinate;

private:
  vtkActor2D(const vtkActor2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkActor2D&) VTK_DELETE_FUNCTION;
};

#endif



