// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitConeRepresentation
 * @brief   defining the representation for a vtkImplicitConeWidget
 *
 * This class is a concrete representation for the
 * vtkImplicitConeWidget. It represents an infinite cone
 * defined by an angle, a origin, and an axis. The cone is placed
 * within its associated bounding box and the intersection of the
 * cone with the bounding box is shown to visually indicate the
 * orientation and position of the representation. This cone
 * representation can be manipulated by using the
 * vtkImplicitConeWidget to adjust the cone angle, axis,
 * and/or origin point. (Note that the bounding box is defined during
 * invocation of the superclass' PlaceWidget() method.)
 *
 * To use this representation, you normally specify a angle, origin,
 * and axis. Optionally you can specify a minimum and maximum angle,
 * and a resolution for the cone. Finally, place the widget and
 * its representation in the scene using PlaceWidget().
 *
 * @sa
 * vtkImplicitConeWidget vtkCone
 */

#ifndef vtkImplicitConeRepresentation_h
#define vtkImplicitConeRepresentation_h

#include "vtkBoundedWidgetRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkVector.h"                   // For vtkVector3d
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkPolyDataMapper;
class vtkConeSource;
class vtkLineSource;
class vtkSphereSource;
class vtkTubeFilter;
class vtkCone;
class vtkProperty;
class vtkImageData;
class vtkPolyData;
class vtkBox;
class vtkCellPicker;

#define VTK_MAX_CONE_RESOLUTION 2048

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitConeRepresentation
  : public vtkBoundedWidgetRepresentation
{
public:
  // Manage the state of the widget
  enum InteractionStateType
  {
    Outside = 0,
    Moving, // Generic state set by the widget
    MovingOutline,
    MovingOrigin,
    RotatingAxis,
    AdjustingAngle,
    Scaling,
    TranslatingOrigin
  };

  static vtkImplicitConeRepresentation* New();
  vtkTypeMacro(vtkImplicitConeRepresentation, vtkBoundedWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the origin of the cone representation. The origin is located along the
   * cone axis.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  double* GetOrigin() VTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  ///@}

  ///@{
  /**
   * Set/Get the axis of rotation for the cone. If the axis is not
   * specified as a unit vector, it will be normalized.
   */
  void SetAxis(double x, double y, double z);
  void SetAxis(double a[3]);
  double* GetAxis() VTK_SIZEHINT(3);
  void GetAxis(double a[3]);
  ///@}

  ///@{
  /**
   * Set/Get the cone angle (expressed in degrees).
   */
  void SetAngle(double r);
  double GetAngle();
  ///@}

  ///@{
  /**
   * Force the cone widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the cone to the axes if it is originally
   * not aligned.
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
   * Enable/disable the drawing of the cone. In some cases the cone
   * interferes with the object that it is operating on (e.g., the
   * cone interferes with the cut surface it produces resulting in
   * z-buffer artifacts.) By default it is off.
   */
  void SetDrawCone(bool draw);
  vtkGetMacro(DrawCone, bool);
  vtkBooleanMacro(DrawCone, bool);
  ///@}

  ///@{
  /**
   * Set/Get the resolution of the cone. This is the number of
   * polygonal facets used to approximate the
   * surface (for rendering purposes). A vtkCone is used under
   * the hood to provide an exact surface representation.
   * Defaults to 128.
   */
  vtkSetClampMacro(Resolution, int, 8, VTK_MAX_CONE_RESOLUTION);
  vtkGetMacro(Resolution, int);
  ///@}

  ///@{
  /**
   * Turn on/off tubing of the wire outline of the cone
   * intersection (against the bounding box). The tube thickens the
   * line by wrapping with a vtkTubeFilter.
   * Defaults to true.
   */
  vtkSetMacro(Tubing, bool);
  vtkGetMacro(Tubing, bool);
  vtkBooleanMacro(Tubing, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the ability to scale the widget with the mouse.
   * Defaults to true.
   */
  vtkSetMacro(ScaleEnabled, bool);
  vtkGetMacro(ScaleEnabled, bool);
  vtkBooleanMacro(ScaleEnabled, bool);
  ///@}

  /**
   * Grab the polydata that defines the cone. The polydata contains
   * polygons that are clipped by the bounding box.
   */
  void GetPolyData(vtkPolyData* pd);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource.
   */
  void UpdatePlacement();

  ///@{
  /**
   * Get the properties on the axis (line and cone).
   */
  vtkGetObjectMacro(AxisProperty, vtkProperty);
  vtkGetObjectMacro(SelectedAxisProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the cone properties. The properties of the cone when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(ConeProperty, vtkProperty);
  vtkGetObjectMacro(SelectedConeProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the property of the intersection edges. (This property also
   * applies to the edges when tubed.)
   */
  vtkGetObjectMacro(EdgesProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set the color of all the widgets handles (edges, axis, selected cone)
   * and their color during interaction. Foreground color applies to the outlines and unselected
   * cone.
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
   * Methods to interface with the vtkImplicitConeWidget.
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

  ///@{
  /**
   * Specify a translation distance used by the BumpCone() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   * Defaults to 0.01.
   */
  vtkSetClampMacro(BumpDistance, double, 0.000001, 1);
  vtkGetMacro(BumpDistance, double);
  ///@}

  /**
   * Translate the cone in the direction of the view vector by the
   * specified BumpDistance. The dir parameter controls which
   * direction the pushing occurs, either in the same direction as the
   * view vector, or when negative, in the opposite direction. The factor
   * controls what percentage of the bump is used.
   */
  void BumpCone(int dir, double factor);

  /**
   * Push the cone the distance specified along the view
   * vector. Positive values are in the direction of the view vector;
   * negative values are in the opposite direction. The distance value
   * is expressed in world coordinates.
   */
  void PushCone(double distance);

  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkImplicitConeWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, InteractionStateType, InteractionStateType::Outside,
    InteractionStateType::TranslatingOrigin);

  ///@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(InteractionStateType);
  vtkGetMacro(RepresentationState, InteractionStateType);
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  void GetCone(vtkCone* cone) const;

protected:
  vtkImplicitConeRepresentation();
  ~vtkImplicitConeRepresentation() override;

  // The actual cone we're manipulating
  vtkNew<vtkCone> Cone;

  InteractionStateType RepresentationState = InteractionStateType::Outside;

  // Keep track of event positions
  vtkVector3d LastEventPosition;

  // Controlling the push operation
  double BumpDistance = 0.01;

  // Controlling ivars
  bool AlongXAxis = false;
  bool AlongYAxis = false;
  bool AlongZAxis = false;

  // The facet resolution for rendering purposes.
  int Resolution = 128;

  bool ScaleEnabled = true; // whether the widget can be scaled

  vtkNew<vtkPolyData> ConePD;
  vtkNew<vtkPolyDataMapper> ConePDMapper;
  vtkNew<vtkActor> ConePDActor;
  bool DrawCone = true;

  // Optional tubes are represented by extracting boundary edges and tubing
  vtkNew<vtkPolyData> EdgesPD;
  vtkNew<vtkTubeFilter> EdgesTuber;
  vtkNew<vtkPolyDataMapper> EdgesMapper;
  vtkNew<vtkActor> EdgesActor;
  bool Tubing = true; // control whether tubing is on

  // The axis line
  vtkNew<vtkLineSource> AxisLineSource;
  vtkNew<vtkPolyDataMapper> AxisLineMapper;
  vtkNew<vtkActor> AxisLineActor;

  // Axis line arrow
  vtkNew<vtkConeSource> AxisArrowSource;
  vtkNew<vtkPolyDataMapper> AxisArrowMapper;
  vtkNew<vtkActor> AxisArrowActor;

  // The origin positioning handle
  vtkNew<vtkSphereSource> OriginHandleSource;
  vtkNew<vtkPolyDataMapper> OriginHandleMapper;
  vtkNew<vtkActor> OriginHandleActor;

  // Do the picking
  vtkNew<vtkCellPicker> Picker;
  vtkNew<vtkCellPicker> ConePicker;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> AxisProperty;
  vtkNew<vtkProperty> SelectedAxisProperty;
  vtkNew<vtkProperty> ConeProperty;
  vtkNew<vtkProperty> SelectedConeProperty;
  vtkNew<vtkProperty> EdgesProperty;
  vtkNew<vtkProperty> OriginHandleProperty;
  vtkNew<vtkProperty> SelectedOriginHandleProperty;

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

  void HighlightCone(bool highlight);
  void HighlightOriginHandle(bool highlight);
  void HighlightAxis(bool highlight);

  // Methods to manipulate the cone
  void Rotate(
    double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2, const vtkVector3d& vpn);
  void TranslateCone(const vtkVector3d& p1, const vtkVector3d& p2);
  void TranslateRepresentation(const vtkVector3d& motion) override;
  void TranslateOrigin(const vtkVector3d& p1, const vtkVector3d& p2);
  void TranslateOriginOnAxis(const vtkVector3d& p1, const vtkVector3d& p2);
  void ScaleAngle(const vtkVector3d& p1, const vtkVector3d& p2);
  void AdjustAngle(double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2);
  void Scale(const vtkVector3d& p1, const vtkVector3d& p2, double X, double Y);
  void SizeHandles();

  // Intersect oriented infinite cone against bounding box
  void BuildCone();

  vtkImplicitConeRepresentation(const vtkImplicitConeRepresentation&) = delete;
  void operator=(const vtkImplicitConeRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
