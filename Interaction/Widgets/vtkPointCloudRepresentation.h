// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointCloudRepresentation
 * @brief   represent the vtkPointCloudWidget
 *
 * This class provides support for interactively querying and selecting
 * points from a point cloud. It is a representation for the
 * vtkPointCloudWidget.
 *
 * @sa
 * vtkPointCloudWidget vtkHardwareSelection vtkPointPicker
 */

#ifndef vtkPointCloudRepresentation_h
#define vtkPointCloudRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
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
struct vtkPointCloudPicker;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkPointCloudRepresentation
  : public vtkWidgetRepresentation
{
  friend struct vtkPointCloudPicker;

public:
  /**
   * Instantiate this class.
   */
  static vtkPointCloudRepresentation* New();

  ///@{
  /**
   * Standard VTK class methods for obtaining type information and printing.
   */
  vtkTypeMacro(vtkPointCloudRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify and place either an actor (vtkActor) or a point set
   * (vtkPointSet) that represents the point cloud. If placing with an
   * actor, then the actor must refer to a mapper which in turn refers to a
   * vtkPointSet, with the actor being used to render the point cloud. If
   * placing with a vtkPointSet, then an internal vtkActor (and associated
   * vtkPointGaussianMapper) is created to render the point cloud.
   */
  void PlacePointCloud(vtkActor* a);
  void PlacePointCloud(vtkPointSet* ps);
  ///@}

  ///@{
  /**
   * Retrieve the associated actor and mapper used to render the point cloud.
   */
  vtkGetObjectMacro(PointCloudActor, vtkActor);
  vtkGetObjectMacro(PointCloudMapper, vtkPolyDataMapper);
  ///@}

  /**
   * Retrieve the point id from the selected point. Note that this can
   * be invalid (<0) if nothing was picked.
   */
  vtkIdType GetPointId() { return this->PointId; }

  ///@{
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
  ///@}

  ///@{
  /**
   * Flag controls whether highlighting of points occurs as the mouse
   * moves over them. This can cause extra rendering operations.
   */
  vtkSetMacro(Highlighting, bool);
  vtkGetMacro(Highlighting, bool);
  vtkBooleanMacro(Highlighting, bool);
  ///@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget.
  enum InteractionStateType
  {
    Outside = 0, // no points nor outline selected
    OverOutline, // mouse is over the bounding box of the point cloud
    Over,        // mouse is over a point
    Selecting    // user has selected the point
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g., PointCloudWidget)
   * or other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking process
   * with the widget: First ComputeInteractionState() is invoked that returns
   * a state based on geometric considerations (i.e., cursor near a widget
   * feature), then based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, Selecting);
  ///@}

  ///@{
  /**
   * Some methods required to satisfy the vtkWidgetRepresentation API.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void BuildRepresentation() override {}
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  ///@}

  ///@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp (i.e., support rendering).
   */
  void GetActors(vtkPropCollection* pc) override;
  void GetActors2D(vtkPropCollection* pc) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  ///@}

  ///@{
  /**
   * Because point clouds can be very large, alternative point picking
   * approaches can be used to select points: either hardware picking (via
   * rendering) or software rendering (via CPU ray cast). In summary,
   * hardware picking (via vtkHardwareSelector) is preferred, with an
   * optional software picker (via vtkPointPicker) available. Each approach
   * has potential advantages and disadvantages - mainly, vtkHardwareSelector
   * is faster but only selects opaque geometry and what is visible on the
   * screen, does not work with anti-aliasing, cannot handle assemblies, and
   * may not work on some systems. vtkPointPicker avoids extra renders, and
   * can handle translucent geometry, can select points "behind" other
   * objects, will work on all systems, but is scalable to only a few tens of
   * thousands of points. (See vtkHardwareSelector and vtkPointPicker for
   * further information.) The choice of picker also has implications on the
   * type of tolerancing used (as described in the following documentation).
   * (Note also that the pickers may return slightly different results, this
   * is expected due to the different way tolerancing works.)
   */
  enum PickingModeType
  {
    HARDWARE_PICKING = 0,
    SOFTWARE_PICKING
  };
  vtkSetClampMacro(PickingMode, int, HARDWARE_PICKING, SOFTWARE_PICKING);
  vtkGetMacro(PickingMode, int);
  void SetPickingModeToHardware() { this->SetPickingMode(HARDWARE_PICKING); }
  void SetPickingModeToSoftware() { this->SetPickingMode(SOFTWARE_PICKING); }
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to a point expressed in pixels.
   * A tolerance of 0 selects from the pixel precisely under the cursor. A
   * tolerance of 1 results in a 3x3 pixel square under the cursor (padded
   * out by 1 in each direction); a tolerance of N results in a (2N+1)**2
   * selection rectangle. The point in the selection rectangle which is
   * closest in z-buffer to the pick position is selected. Note that this can
   * sometimes return points further away from the cursor (which can be
   * unexpected - use the tolerance carefully).
   */
  vtkSetMacro(HardwarePickingTolerance, unsigned int);
  vtkGetMacro(HardwarePickingTolerance, unsigned int);
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to a point (as a fraction of the
   * bounding box of the point cloud). This specifies when the cursor is
   * considered near enough to the point to highlight it. Note that this is
   * a sensitive parameter - too small and it's hard to pick anything; too
   * large and points close to the eye can be picked in preference to points
   * further away which are closer to the pick ray.
   */
  vtkSetClampMacro(SoftwarePickingTolerance, double, 0.0, 100.0);
  vtkGetMacro(SoftwarePickingTolerance, double);
  ///@}

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
  int PickingMode;
  unsigned int HardwarePickingTolerance;
  double SoftwarePickingTolerance;
  vtkPicker* OutlinePicker;
  vtkPointCloudPicker* PointCloudPicker;

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

VTK_ABI_NAMESPACE_END
#endif
