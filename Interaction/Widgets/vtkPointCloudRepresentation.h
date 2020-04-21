/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointCloudRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointCloudRepresentation
 * @brief   represent the vtkPointCloudWidget
 *
 * This class provides support for interactively selecting a point from a
 * point cloud. It is a representation for a vtkPointCloudWidget.
 *
 * @sa
 * vtkPointCloudWidget
 */

#ifndef vtkPointCloudRepresentation_h
#define vtkPointCloudRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkOutlineFilter;
class vtkActor2D;
class vtkCoordinate;
class vtkPolyDataMapper2D;
class vtkProperty2D;
class vtkPolyData;
class vtkPicker;
class vtkPointPicker;
class vtkPointSet;
class vtkGlyphSource2D;

class VTKINTERACTIONWIDGETS_EXPORT vtkPointCloudRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPointCloudRepresentation* New();

  //@{
  /**
   * Standard VTK class methods for obtaining type information and printing.
   */
  vtkTypeMacro(vtkPointCloudRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify and place either an actor (vtkActor) or a point set
   * (vtkPointSet) that represents the point cloud. If placing with an
   * actor, then the actor must refer to a mapper which in turn refers to a
   * point set, with the actor being used to render the point cloud. If
   * placing with a vtkPointSet, then an internal vtkActor (and associated
   * vtkPointGaussianMapper) is created to render the point cloud.
   */
  void PlacePointCloud(vtkActor* a);
  void PlacePointCloud(vtkPointSet* ps);
  //@}

  //@{
  /**
   * Retrieve the associated actor and mapper used to render the point cloud.
   */
  vtkGetObjectMacro(PointCloudActor, vtkActor);
  vtkGetObjectMacro(PointCloudMapper, vtkPolyDataMapper);
  //@}

  /**
   * Retrieve the point id from the selected point. Note that this can
   * be invalid (<0) if nothing was picked.
   */
  vtkIdType GetPointId() { return this->PointId; }

  //@{
  /**
   * Retrieve the point coordinates of the selected point. Note that if the
   * point id is invalid (<0) then the coordinates are undefined.
   */
  const double* GetPointCoordinates() { return this->PointCoordinates; }
  void GetPointCoordinates(double x[3])
  {
    x[0] = this->PointCoordinates[0];
    x[1] = this->PointCoordinates[1];
    x[2] = this->PointCoordinates[2];
  }
  //@}

  //@{
  /**
   * Flag controls whether highlighting of points occurs as the mouse
   * over them. This can cause extra rendering operations.
   */
  vtkSetMacro(Highlighting, bool);
  vtkGetMacro(Highlighting, bool);
  vtkBooleanMacro(Highlighting, bool);
  //@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget. Note that ComputeInteractionState() and several other methods
  // must be implemented by subclasses.
  enum _InteractionState
  {
    Outside = 0, // no points nor outline selected
    OverOutline, // mouse is over the bounding box of the point cloud
    Over,        // mouse is over a point
    Selecting    // user has selected the point
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g., HandleWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, Selecting);
  //@}

  //@{
  /**
   * Subclasses of vtkPointHandleRepresentation2D must implement these
   * methods. These are the methods that the widget and its representation
   * use to communicate with each other.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void BuildRepresentation() override {}
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp (i.e., support rendering).
   */
  void GetActors2D(vtkPropCollection* pc) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  //@}

  //@{
  /**
   * The tolerance representing the distance to a point (as a fraction of the
   * bounding box of the point cloud). This specifies when the cursor is
   * considered near enough to the point to highlight it. Note that this is
   * a sensitive parameter - too small and it's hard to pick anything; too
   * large and points close to the eye can be picked in preference to points
   * further away which are closer to the pick ray.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 100.0);
  vtkGetMacro(Tolerance, double);
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  vtkPointCloudRepresentation();
  ~vtkPointCloudRepresentation() override;

  // The point cloud that is being operated on
  vtkActor* PointCloudActor;
  vtkPolyDataMapper* PointCloudMapper;
  vtkPointSet* PointCloud;

  // The selected point id and coordinates
  vtkIdType PointId;
  double PointCoordinates[3];

  // Data members to manage state
  bool Highlighting;
  double Tolerance;
  vtkPicker* OutlinePicker;
  vtkPointPicker* PointPicker;

  // Draw an outline around the point cloud
  vtkActor* OutlineActor;
  vtkPolyDataMapper* OutlineMapper;
  vtkOutlineFilter* OutlineFilter;

  // Highlight the selected point
  vtkActor2D* SelectionActor;
  vtkCoordinate* SelectionCoordinate;
  vtkPolyDataMapper2D* SelectionMapper;
  vtkGlyphSource2D* SelectionShape;

  vtkProperty2D* SelectionProperty;
  void CreateDefaultProperties();

private:
  vtkPointCloudRepresentation(const vtkPointCloudRepresentation&) = delete;
  void operator=(const vtkPointCloudRepresentation&) = delete;
};

#endif
