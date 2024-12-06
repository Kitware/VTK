// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkImplicitAnnulusRepresentation
 * @brief defining the representation for a vtkImplicitAnnulusWidget
 *
 * This class is a concrete representation for the
 * vtkImplicitAnnulusWidget. It represents an infinite annulus
 * defined by its inner/outer radiuses, its center, and its axis. The annulus is placed
 * within its associated bounding box and the intersection of the
 * annulus with the bounding box is shown to visually indicate the
 * orientation and position of the representation. This annulus
 * representation can be manipulated by using the
 * vtkImplicitAnnulusWidget to adjust the annulus angle, axis,
 * and/or center point. (Note that the bounding box is defined during
 * invocation of the superclass' PlaceWidget() method.)
 *
 * To use this representation, you normally specify inner and outer radii, center,
 * and axis, and a resolution for the annulus. Finally, place the widget and
 * its representation in the scene using PlaceWidget().
 *
 * @sa
 * vtkImplicitAnnulusWidget vtkAnnulus
 */

#ifndef vtkImplicitAnnulusRepresentation_h
#define vtkImplicitAnnulusRepresentation_h

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
class vtkAnnulus;
class vtkProperty;
class vtkPolyData;
class vtkBox;
class vtkCellPicker;

#define VTK_MAX_ANNULUS_RESOLUTION 2048

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitAnnulusRepresentation
  : public vtkBoundedWidgetRepresentation
{
public:
  // Manage the state of the widget
  enum InteractionStateType
  {
    Outside = 0,
    Moving, // Generic state set by the widget
    MovingOutline,
    MovingCenter,
    RotatingAxis,
    AdjustingInnerRadius,
    AdjustingOuterRadius,
    Scaling,
    TranslatingCenter
  };

  static vtkImplicitAnnulusRepresentation* New();
  vtkTypeMacro(vtkImplicitAnnulusRepresentation, vtkBoundedWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the center of the annulus representation. The center is located along the
   * annulus axis.
   * Default is (0, 0, 0)
   */
  void SetCenter(double x, double y, double z);
  void SetCenter(double x[3]);
  double* GetCenter() const VTK_SIZEHINT(3);
  void GetCenter(double xyz[3]) const;
  ///@}

  ///@{
  /**
   * Set/Get the axis of rotation for the annulus. If the axis is not
   * specified as a unit vector, it will be normalized.
   * Default is the Y-Axis (0, 1, 0)
   */
  void SetAxis(double x, double y, double z);
  void SetAxis(double a[3]);
  double* GetAxis() const VTK_SIZEHINT(3);
  void GetAxis(double a[3]) const;
  ///@}

  ///@{
  /**
   * Set/Get the annulus inner radius. Should be lower than OuterRadius.
   * Default is 0.25.

   * @see GetOuterRadius
   */
  void SetInnerRadius(double r);
  double GetInnerRadius() const;
  ///@}

  ///@{
  /**
   * Set/Get the annulus outer radius. Should be greater than InnerRadius.
   * Default is 0.5.
   *
   * @see GetInnerRadius.
   */
  void SetOuterRadius(double r);
  double GetOuterRadius() const;
  ///@}

  ///@{
  /**
   * Force the annulus widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the annulus to the axes if it is centerally
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
   * Enable/disable the drawing of the annulus. In some cases the annulus
   * interferes with the object that it is operating on (e.g., the
   * annulus interferes with the cut surface it produces resulting in
   * z-buffer artifacts.) By default it is off.
   */
  void SetDrawAnnulus(bool draw);
  vtkGetMacro(DrawAnnulus, bool);
  vtkBooleanMacro(DrawAnnulus, bool);
  ///@}

  ///@{
  /**
   * Set/Get the resolution of the annulus. This is the number of
   * polygonal facets used to approximate the
   * surface (for rendering purposes). A vtkAnnulus is used under
   * the hood to provide an exact surface representation.
   * Defaults to 128.
   */
  vtkSetClampMacro(Resolution, int, 8, VTK_MAX_ANNULUS_RESOLUTION);
  vtkGetMacro(Resolution, int);
  ///@}

  ///@{
  /**
   * Turn on/off tubing of the wire outline of the annulus
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
   * Grab the polydata that defines the annulus. The polydata contains
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
   * Get the properties on the axis (line and annulus).
   */
  vtkGetObjectMacro(AxisProperty, vtkProperty);
  vtkGetObjectMacro(SelectedAxisProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the annulus properties. The properties of the annulus when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(AnnulusProperty, vtkProperty);
  vtkGetObjectMacro(SelectedAnnulusProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the annulus radii properties. The properties of the annulus inner and outer radii when
   * selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(RadiusHandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedRadiusHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set the color of all the widgets handles (edges, axis, selected annulus)
   * and their color during interaction. Foreground color applies to the outlines and unselected
   * annulus.
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
   * Methods to interface with the vtkImplicitAnnulusWidget.
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
   * Specify a translation distance used by the BumpAnnulus() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   * Defaults to 0.01.
   */
  vtkSetClampMacro(BumpDistance, double, 0.000001, 1);
  vtkGetMacro(BumpDistance, double);
  ///@}

  /**
   * Translate the annulus in the direction of the view vector by the
   * specified BumpDistance. The dir parameter controls which
   * direction the pushing occurs, either in the same direction as the
   * view vector, or when negative, in the opposite direction. The factor
   * controls what percentage of the bump is used.
   */
  void BumpAnnulus(int dir, double factor);

  /**
   * Push the annulus the distance specified along the view
   * vector. Positive values are in the direction of the view vector;
   * negative values are in the opposite direction. The distance value
   * is expressed in world coordinates.
   */
  void PushAnnulus(double distance);

  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkImplicitAnnulusWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, InteractionStateType, InteractionStateType::Outside,
    InteractionStateType::TranslatingCenter);

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

  void GetAnnulus(vtkAnnulus* annulus) const;

protected:
  vtkImplicitAnnulusRepresentation();
  ~vtkImplicitAnnulusRepresentation() override;

private:
  struct AxisHandleRepresentation
  {
    vtkNew<vtkLineSource> LineSource;
    vtkNew<vtkPolyDataMapper> LineMapper;
    vtkNew<vtkActor> LineActor;

    vtkNew<vtkConeSource> ArrowSource;
    vtkNew<vtkPolyDataMapper> ArrowMapper;
    vtkNew<vtkActor> ArrowActor;
  };

  struct RadiusHandleRepresentation
  {
    vtkNew<vtkPolyData> PolyData;
    vtkNew<vtkTubeFilter> Tuber;
    vtkNew<vtkPolyDataMapper> Mapper;
    vtkNew<vtkActor> Actor;
  };

  vtkImplicitAnnulusRepresentation(const vtkImplicitAnnulusRepresentation&) = delete;
  void operator=(const vtkImplicitAnnulusRepresentation&) = delete;

  void HighlightAnnulus(bool highlight);
  void HighlightCenterHandle(bool highlight);
  void HighlightAxis(bool highlight);
  void HighlightInnerRadiusHandle(bool highlight);
  void HighlightOuterRadiusHandle(bool highlight);

  // Methods to manipulate the annulus
  void Rotate(
    double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2, const vtkVector3d& vpn);
  void TranslateAnnulus(const vtkVector3d& p1, const vtkVector3d& p2);
  void TranslateRepresentation(const vtkVector3d& motion) override;
  void TranslateCenter(const vtkVector3d& p1, const vtkVector3d& p2);
  void TranslateCenterOnAxis(const vtkVector3d& p1, const vtkVector3d& p2);
  void ScaleRadii(const vtkVector3d& p1, const vtkVector3d& p2);
  void AdjustOuterRadius(double x, double y, const vtkVector3d& point);
  void AdjustInnerRadius(double x, double y, const vtkVector3d& point);
  void Scale(const vtkVector3d& p1, const vtkVector3d& p2, double X, double Y);
  void SizeHandles();

  // Generate the annulus polydata, cropped by the bounding box
  void BuildAnnulus();

  // The actual annulus we're manipulating
  vtkNew<vtkAnnulus> Annulus;

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

  vtkNew<vtkPolyData> AnnulusPD;
  vtkNew<vtkPolyDataMapper> AnnulusMapper;
  vtkNew<vtkActor> AnnulusActor;
  bool DrawAnnulus = true;

  // Optional tubes are represented by extracting boundary edges and tubing
  RadiusHandleRepresentation InnerRadiusRepresentation;
  RadiusHandleRepresentation OuterRadiusRepresentation;
  bool Tubing = true; // control whether tubing is on

  // Axis representations
  AxisHandleRepresentation LowerAxisRepresentation;
  AxisHandleRepresentation UpperAxisRepresentation;

  // Center positioning handle
  vtkNew<vtkSphereSource> CenterHandleSource;
  vtkNew<vtkPolyDataMapper> CenterHandleMapper;
  vtkNew<vtkActor> CenterHandleActor;

  // Do the picking
  vtkNew<vtkCellPicker> Picker;
  vtkNew<vtkCellPicker> AnnulusPicker;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> AxisProperty;
  vtkNew<vtkProperty> SelectedAxisProperty;
  vtkNew<vtkProperty> AnnulusProperty;
  vtkNew<vtkProperty> SelectedAnnulusProperty;
  vtkNew<vtkProperty> RadiusHandleProperty;
  vtkNew<vtkProperty> SelectedRadiusHandleProperty;
  vtkNew<vtkProperty> CenterHandleProperty;
  vtkNew<vtkProperty> SelectedCenterHandleProperty;

  vtkNew<vtkBox> BoundingBox;
};

VTK_ABI_NAMESPACE_END
#endif
