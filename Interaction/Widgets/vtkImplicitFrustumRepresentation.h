// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkImplicitFrustumRepresentation_h
#define vtkImplicitFrustumRepresentation_h

#include "vtkBoundedWidgetRepresentation.h"
#include "vtkEllipseArcSource.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkVector.h"                   // For vtkVector3d
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkConeSource;
class vtkCellPicker;
class vtkEllipseArcSource;
class vtkFrustum;
class vtkLineSource;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkSphereSource;
class vtkTransform;
class vtkTubeFilter;

/**
 * @class vtkImplicitFrustumRepresentation
 * @brief The representation for a vtkImplicitFrustumWidget
 *
 * This class is a concrete representation for the vtkImplicitFrustumWidget. It represents an
 * infinite frustum defined by its origin, its orientation, the two angles between its forward axis
 * and its horizontal and vertical planes, and the distance between its origin and near plane. This
 * frustum representation can be manipulated by using the vtkImplicitFrustumWidget.
 *
 * @sa vtkImplicitFrustumWidget vtkFrustum
 */
class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitFrustumRepresentation
  : public vtkBoundedWidgetRepresentation
{
public:
  // Manage the state of the widget
  enum InteractionStateType
  {
    Outside = 0,
    Moving, // Generic state set by the widget
    MovingOrigin,
    AdjustingHorizontalAngle,
    AdjustingVerticalAngle,
    AdjustingNearPlaneDistance,
    AdjustingYaw,
    AdjustingPitch,
    AdjustingRoll,
    TranslatingOriginOnAxis
  };

  static vtkImplicitFrustumRepresentation* New();
  vtkTypeMacro(vtkImplicitFrustumRepresentation, vtkBoundedWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the origin of the frustum representation. The origin is located along the frustum axis.
   * Default is (0, 0, 0)
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  void SetOrigin(const vtkVector3d& xyz);
  double* GetOrigin() VTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]) const;
  ///@}

  ///@{
  /**
   * Get/Set the orientation of the frustum. Defaults to (0,0,0)
   */
  void SetOrientation(double x, double y, double z);
  void SetOrientation(const double xyz[3]);
  void SetOrientation(const vtkVector3d& xyz);
  double* GetOrientation() VTK_SIZEHINT(3);
  void GetOrientation(double& x, double& y, double& z);
  void GetOrientation(double xyz[3]);
  ///@}

  ///@{
  /**
   * Get/Set the horizontal angle of the frustum in degrees. It represents the angle between its
   * forward axis and its right and left planes. Defaults to 30.
   */
  double GetHorizontalAngle() const;
  void SetHorizontalAngle(double angle);
  ///@}

  ///@{
  /**
   * Get/Set the vertical angle of the frustum in degrees. It represents the angle between its
   * forward axis and its top and bottom planes. Defaults to 30.
   */
  double GetVerticalAngle() const;
  void SetVerticalAngle(double angle);
  ///@}

  ///@{
  /**
   * Get/Set the near plane distance of the frustum, i.e. the distance between its origin and near
   * plane along the forward axis. Defaults to 0.5.
   */
  double GetNearPlaneDistance() const;
  void SetNearPlaneDistance(double angle);
  ///@}

  ///@{
  /**
   * Force the frustum widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the frustum to the axes if it is originally not aligned.
   * Default to false.
   */
  void SetAlongXAxis(bool);
  vtkGetMacro(AlongXAxis, bool);
  vtkBooleanMacro(AlongXAxis, bool);
  void SetAlongYAxis(bool);
  vtkGetMacro(AlongYAxis, bool);
  vtkBooleanMacro(AlongYAxis, bool);
  void SetAlongZAxis(bool);
  vtkGetMacro(AlongZAxis, bool);
  vtkBooleanMacro(AlongZAxis, bool);
  ///@}

  ///@{
  /**
   * Enable/disable the drawing of the frustum. In some cases the frustum interferes with the object
   * that it is operating on (e.g., the frustum interferes with the cut surface it produces
   * resulting in z-buffer artifacts.) By default it is on.
   */
  void SetDrawFrustum(bool draw);
  vtkGetMacro(DrawFrustum, bool);
  vtkBooleanMacro(DrawFrustum, bool);
  ///@}

  /**
   * Grab the polydata that defines the frustum. The polydata contains polygons that are clipped by
   * the bounding box.
   */
  void GetPolyData(vtkPolyData* pd);

  /**
   * Satisfies the superclass API.  This will change the state of the widget to match changes that
   * have been made to the underlying PolyDataSource.
   */
  void UpdatePlacement();

  /**
   * Get properties of the frustum actor.
   */
  vtkGetObjectMacro(FrustumProperty, vtkProperty);

  ///@{
  /**
   * Get the properties of the edge handles actors. i.e. the properties of the near plane and angles
   * handles when selected or not.
   */
  vtkGetObjectMacro(EdgeHandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedEdgeHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set the color of all the widgets handles (origin, orientations, near plane and angles) and
   * their color during interaction. Foreground color applies to the frustum itself
   */
  void SetInteractionColor(double, double, double);
  void SetInteractionColor(double c[3]) { this->SetInteractionColor(c[0], c[1], c[2]); }
  void SetHandleColor(double, double, double);
  void SetHandleColor(double c[3]) { this->SetHandleColor(c[0], c[1], c[2]); }
  void SetForegroundColor(double, double, double);
  void SetForegroundColor(double c[3]) { this->SetForegroundColor(c[0], c[1], c[2]); }
  ///@}

  ///@{
  /**
   * Methods to interface with the vtkImplicitFrustumWidget.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  ///@}

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() override;
  void GetActors(vtkPropCollection* pc) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * The interaction state may be set from a widget (e.g., vtkImplicitFrustumWidget) or other
   * object. This controls how the interaction with the widget proceeds. Normally this method is
   * used as part of a handshaking process with the widget: First ComputeInteractionState() is
   * invoked and returns a state based on geometric considerations (i.e., cursor near a widget
   * feature), then based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, InteractionStateType, InteractionStateType::Outside,
    InteractionStateType::TranslatingOriginOnAxis);

  ///@{
  /**
   * Sets the visual appearance of the representation based on the state it is in. This state is
   * usually the same as InteractionState.
   */
  virtual void SetRepresentationState(InteractionStateType);
  vtkGetMacro(RepresentationState, InteractionStateType);
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  /**
   * Get the concrete represented frustum
   **/
  void GetFrustum(vtkFrustum* frustum) const;

protected:
  vtkImplicitFrustumRepresentation();
  ~vtkImplicitFrustumRepresentation() override;

private:
  enum class FrustumFace
  {
    None = -1,
    Right = 0,
    Left,
    Top,
    Bottom,
    Near,
  };

  struct SphereHandle
  {
    vtkNew<vtkSphereSource> Source;
    vtkNew<vtkPolyDataMapper> Mapper;
    vtkNew<vtkActor> Actor;

    SphereHandle();
  };

  struct EdgeHandle
  {
    vtkNew<vtkPolyData> PolyData;
    vtkNew<vtkTubeFilter> Tuber;
    vtkNew<vtkPolyDataMapper> Mapper;
    vtkNew<vtkActor> Actor;

    EdgeHandle();
  };

  struct EllipseHandle
  {
    vtkNew<vtkEllipseArcSource> Source;
    vtkNew<vtkTubeFilter> Tuber;
    vtkNew<vtkPolyDataMapper> Mapper;
    vtkNew<vtkActor> Actor;

    EllipseHandle();
  };

  vtkImplicitFrustumRepresentation(const vtkImplicitFrustumRepresentation&) = delete;
  void operator=(const vtkImplicitFrustumRepresentation&) = delete;

  // Helpers to get the frustum basis
  vtkVector3d GetForwardAxis();
  vtkVector3d GetUpAxis();
  vtkVector3d GetRightAxis();

  void HighlightOriginHandle(bool highlight);
  void HighlightFarPlaneVerticalHandle(bool highlight);
  void HighlightFarPlaneHorizontalHandle(bool highlight);
  void HighlightNearPlaneHandle(bool highlight);
  void HighlightRollHandle(bool highlight);
  void HighlightYawHandle(bool highlight);
  void HighlightPitchHandle(bool highlight);

  // Methods to manipulate the frustum
  void TranslateOrigin(const vtkVector3d& p1, const vtkVector3d& p2);
  void TranslateOriginOnAxis(const vtkVector3d& p1, const vtkVector3d& p2);
  void AdjustHorizontalAngle(const vtkVector3d& p1, const vtkVector3d& p2);
  void AdjustVerticalAngle(const vtkVector3d& p1, const vtkVector3d& p2);
  void AdjustNearPlaneDistance(
    const vtkVector2d& eventPosition, const vtkVector3d& p1, const vtkVector3d& p2);
  void Rotate(
    const vtkVector3d& prevPickPoint, const vtkVector3d& pickPoint, const vtkVector3d& axis);

  // Set the frustum transform according to the representation's orientation and position
  void UpdateFrustumTransform();

  // Re-compute the widget handles' sizes
  void SizeHandles();

  // Generate the frustum polydata, cropped by the bounding box
  void BuildFrustum();

  // The actual frustum we're manipulating
  vtkNew<vtkFrustum> Frustum;

  InteractionStateType RepresentationState = InteractionStateType::Outside;

  // Keep track of event positions
  vtkVector3d LastEventPosition = { 0., 0., 0. };

  bool AlongXAxis = false;
  bool AlongYAxis = false;
  bool AlongZAxis = false;

  double Length = 1;

  vtkVector3d Origin = { 0, 0, 0 };
  vtkNew<vtkTransform> OrientationTransform;

  vtkNew<vtkPolyData> FrustumPD;
  vtkNew<vtkPolyDataMapper> FrustumMapper;
  vtkNew<vtkActor> FrustumActor;
  bool DrawFrustum = true;

  std::array<EdgeHandle, 4> FarPlaneHandles;
  EdgeHandle NearPlaneEdgesHandle;
  SphereHandle NearPlaneCenterHandle;
  EllipseHandle RollHandle;
  EllipseHandle YawHandle;
  EllipseHandle PitchHandle;
  SphereHandle OriginHandle;

  FrustumFace ActiveEdgeHandle = FrustumFace::None;

  vtkNew<vtkCellPicker> Picker;
  vtkNew<vtkCellPicker> FrustumPicker;

  // Properties used to control the appearance of selected objects and the manipulator in general.
  vtkNew<vtkProperty> FrustumProperty;
  vtkNew<vtkProperty> EdgeHandleProperty;
  vtkNew<vtkProperty> SelectedEdgeHandleProperty;
  vtkNew<vtkProperty> OriginHandleProperty;
  vtkNew<vtkProperty> SelectedOriginHandleProperty;

  vtkNew<vtkBox> BoundingBox;
};

VTK_ABI_NAMESPACE_END
#endif
