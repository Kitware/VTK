// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDisplaySizedImplicitPlaneRepresentation
 * @brief   a class defining the representation for a vtkDisplaySizedImplicitPlaneWidget
 *
 * This class is a concrete representation for the
 * vtkDisplaySizedImplicitPlaneWidget. It represents an display sized disk plane defined
 * by a normal and point. Through interaction with the widget, the plane can be manipulated
 * by adjusting the plane normal, disk radius or moving/picking the origin point.
 *
 * To use this representation, you normally define a (plane) origin and (plane)
 * normal. The PlaceWidget() method is also used to initially position the
 * representation.
 *
 * @warning
 * This class, and vtkDisplaySizedImplicitPlaneWidget, are next generation VTK
 * widgets.
 *
 * @sa
 * vtkDisplaySizedImplicitPlaneWidget vtkImplicitPlaneWidget2 vtkImplicitPlaneWidget
 * vtkImplicitImageRepresentation
 */

#ifndef vtkDisplaySizedImplicitPlaneRepresentation_h
#define vtkDisplaySizedImplicitPlaneRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew command
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkCellPicker;
class vtkConeSource;
class vtkCutter;
class vtkDiskSource;
class vtkFeatureEdges;
class vtkHardwarePicker;
class vtkImageData;
class vtkLineSource;
class vtkLookupTable;
class vtkOutlineFilter;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;
class vtkTubeFilter;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkDisplaySizedImplicitPlaneRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkDisplaySizedImplicitPlaneRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkDisplaySizedImplicitPlaneRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  double* GetOrigin() VTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  ///@}

  ///@{
  /**
   * Set/Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double n[3]);
  void SetNormalToCamera();
  double* GetNormal() VTK_SIZEHINT(3);
  void GetNormal(double xyz[3]);
  ///@}

  ///@{
  /**
   * Force the plane widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the plane to the axes if it is originally
   * not aligned.
   */
  void SetNormalToXAxis(vtkTypeBool);
  vtkGetMacro(NormalToXAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToXAxis, vtkTypeBool);
  void SetNormalToYAxis(vtkTypeBool);
  vtkGetMacro(NormalToYAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToYAxis, vtkTypeBool);
  void SetNormalToZAxis(vtkTypeBool);
  vtkGetMacro(NormalToZAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToZAxis, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If enabled, and a vtkCamera is available through the renderer, then
   * LockNormalToCamera will cause the normal to follow the camera's
   * normal.
   */
  virtual void SetLockNormalToCamera(vtkTypeBool);
  vtkGetMacro(LockNormalToCamera, vtkTypeBool);
  vtkBooleanMacro(LockNormalToCamera, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the Radius Multiplier value. Default is 1.0.
   */
  virtual void SetRadiusMultiplier(double radiusMultiplier);
  virtual double GetRadiusMultiplierMinValue() { return 0.000001; }
  virtual double GetRadiusMultiplierMaxValue() { return VTK_DOUBLE_MAX; }
  vtkGetMacro(RadiusMultiplier, double);
  ///@}

  ///@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(vtkTypeBool plane);
  vtkGetMacro(DrawPlane, vtkTypeBool);
  vtkBooleanMacro(DrawPlane, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable the drawing of the outline. Default is off.
   */
  void SetDrawOutline(vtkTypeBool outline);
  vtkGetMacro(DrawOutline, vtkTypeBool);
  vtkBooleanMacro(DrawOutline, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable the drawing of the intersection edges. Default is off.
   *
   * Note: drawing the intersection edges requires DrawOutline to be on.
   */
  void SetDrawIntersectionEdges(vtkTypeBool intersectionEdges);
  vtkGetMacro(DrawIntersectionEdges, vtkTypeBool);
  vtkBooleanMacro(DrawIntersectionEdges, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the ability to translate the bounding box by grabbing it
   * with the left mouse button. Default is off.
   */
  vtkSetMacro(OutlineTranslation, vtkTypeBool);
  vtkGetMacro(OutlineTranslation, vtkTypeBool);
  vtkBooleanMacro(OutlineTranslation, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the ability to move the widget outside of the bounds
   * specified in the initial PlaceWidget() invocation.
   */
  vtkSetMacro(OutsideBounds, vtkTypeBool);
  vtkGetMacro(OutsideBounds, vtkTypeBool);
  vtkBooleanMacro(OutsideBounds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  ///@}

  ///@{
  /**
   * Returns true if ConstrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  ///@}

  ///@{
  /**
   * Set/Get the bounds of the widget representation. PlaceWidget can also be
   * used to set the bounds of the widget but it may also have other effects
   * on the internal state of the representation. Use this function when only
   * the widget bounds need to be modified.
   */
  vtkSetVector6Macro(WidgetBounds, double);
  vtkGetVector6Macro(WidgetBounds, double);
  ///@}

  ///@{
  /**
   * Turn on/off whether the plane should be constrained to the widget bounds.
   * If on, the origin will not be allowed to move outside the set widget bounds.
   * The default behaviour is off.
   * If off, the origin can be freely moved and the widget outline will change
   * accordingly.
   */
  vtkSetMacro(ConstrainToWidgetBounds, vtkTypeBool);
  vtkGetMacro(ConstrainToWidgetBounds, vtkTypeBool);
  vtkBooleanMacro(ConstrainToWidgetBounds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off whether the maximum widget size should be constrained to the widget bounds.
   * If on, the radius of the disk plane and plane normal arrow will not be allowed to be larger
   * than the half diagonal of the bounding box formed by the widget bounds.
   * If off, the radius of the disk plane and plane normal arrow can be arbitrary big
   * The default behaviour is off.
   */
  vtkSetMacro(ConstrainMaximumSizeToWidgetBounds, vtkTypeBool);
  vtkGetMacro(ConstrainMaximumSizeToWidgetBounds, vtkTypeBool);
  vtkBooleanMacro(ConstrainMaximumSizeToWidgetBounds, vtkTypeBool);

  ///@{
  /**
   * Turn on/off the ability to scale the widget with the mouse.
   */
  vtkSetMacro(ScaleEnabled, vtkTypeBool);
  vtkGetMacro(ScaleEnabled, vtkTypeBool);
  vtkBooleanMacro(ScaleEnabled, vtkTypeBool);
  ///@}

  /**
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon that is clipped by the bounding box.
   */
  void GetPolyData(vtkPolyData* pd);

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * PolyData (which represents the plane).
   */
  vtkPolyDataAlgorithm* GetPolyDataAlgorithm();

  /**
   * Get the implicit function for the plane by copying the origin and normal
   * of the cut plane into the provided vtkPlane. The user must provide the
   * instance of the class vtkPlane. Note that vtkPlane is a subclass of
   * vtkImplicitFunction, meaning that it can be used by a variety of filters
   * to perform clipping, cutting, and selection of data.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void GetPlane(vtkPlane* plane);

  /**
   * Alternative way to define the cutting plane. The normal and origin of
   * the plane provided is copied into the internal instance of the class
   * cutting vtkPlane.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetPlane(vtkPlane* plane);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement();

  ///@{
  /**
   * Get the properties on the normal (line and cone). The properties of the normal when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(NormalProperty, vtkProperty);
  vtkGetObjectMacro(SelectedNormalProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the sphere. The properties of the sphere when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(SphereProperty, vtkProperty);
  vtkGetObjectMacro(SelectedSphereProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the plane properties. The properties of the plane when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(PlaneProperty, vtkProperty);
  vtkGetObjectMacro(SelectedPlaneProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the property of the outline.
   */
  vtkGetObjectMacro(OutlineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the property of the disk edges. The properties of the edges when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(EdgesProperty, vtkProperty);
  vtkGetObjectMacro(SelectedEdgesProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the property of the intersection edges of the plane with the outline.
   */
  vtkGetObjectMacro(IntersectionEdgesProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set the color of all the widget's handles (edges, cone1, cone2, line, sphere, selected plane)
   * and their color during interaction. Foreground color applies to the outlines and unselected
   * plane.
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
   * Specify a translation distance used by the BumpPlane() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   */
  vtkSetClampMacro(BumpDistance, double, 0.000001, 1);
  vtkGetMacro(BumpDistance, double);
  ///@}

  /**
   * Translate the plane in the direction of the normal by the
   * specified BumpDistance.  The dir parameter controls which
   * direction the pushing occurs, either in the same direction
   * as the normal, or when negative, in the opposite direction.
   * The factor controls whether what percentage of the bump is
   * used.
   */
  void BumpPlane(int dir, double factor);

  /**
   * Push the plane the distance specified along the normal. Positive
   * values are in the direction of the normal; negative values are
   * in the opposite direction of the normal. The distance value is
   * expressed in world coordinates.
   */
  void PushPlane(double distance);

  ///@{
  /**
   * Enable/Disable picking camera focal info if no result is available for PickOrigin and
   * PickNormal. The default is disabled.
   */
  vtkGetMacro(PickCameraFocalInfo, bool);
  vtkSetMacro(PickCameraFocalInfo, bool);
  vtkBooleanMacro(PickCameraFocalInfo, bool);
  ///@}

  /**
   * Given the X, Y display coordinates, pick a new origin for the plane
   * from a point that is on the objects rendered by the renderer.
   *
   * Note: if a point from a rendered object is not picked, the camera focal point can optionally be
   * set.
   */
  bool PickOrigin(int X, int Y, bool snapToMeshPoint = false);

  /**
   * Given the X, Y display coordinates, pick a new normal for the plane
   * from a point that is on the objects rendered by the renderer.
   *
   * Note: if a normal from a rendered object is not picked, the camera plane normal can optionally
   * be set.
   */
  bool PickNormal(int X, int Y, bool snapToMeshPoint = false);

  ///@{
  /**
   * Methods to interface with the vtkDisplaySizedImplicitPlaneWidget.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  void StartComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  void EndComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  ///@}

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetActors(vtkPropCollection* pc) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  // Manage the state of the widget
  enum InteractionStateType
  {
    Outside = 0,
    Moving,
    MovingOutline,
    MovingOrigin,
    Rotating,
    Pushing,
    ResizeDiskRadius,
    Scaling
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkDisplaySizedImplicitPlaneWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, Scaling);
  ///@}

  ///@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  ///@}

  // Get the underlying implicit plane object used by this rep
  // that can be used as a cropping plane in vtkMapper.
  vtkPlane* GetUnderlyingPlane() { return this->Plane; }

  ///@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes.
   */
  vtkGetMacro(SnapToAxes, bool);
  vtkSetMacro(SnapToAxes, bool);
  vtkBooleanMacro(SnapToAxes, bool);
  ///@}

  ///@{
  /**
   * Forces the plane's normal to be aligned with x, y or z axis.
   * The alignment happens when calling SetNormal.
   * It defers with SnapToAxes from it is always applicable, and SnapToAxes
   * only snaps when the angle difference exceeds 16 degrees in complex interactions.
   */
  vtkGetMacro(AlwaysSnapToNearestAxis, bool);
  virtual void SetAlwaysSnapToNearestAxis(bool snap)
  {
    this->AlwaysSnapToNearestAxis = snap;
    this->SetNormal(this->GetNormal());
  }
  ///@}

protected:
  vtkDisplaySizedImplicitPlaneRepresentation();
  ~vtkDisplaySizedImplicitPlaneRepresentation() override;

  int RepresentationState;

  // Keep track of event positions
  double LastEventPosition[3];
  double LastEventOrientation[4];
  double StartEventOrientation[4];

  // Controlling ivars
  vtkTypeBool NormalToXAxis;
  vtkTypeBool NormalToYAxis;
  vtkTypeBool NormalToZAxis;

  double SnappedEventOrientation[4];
  bool SnappedOrientation;
  bool SnapToAxes;

  bool AlwaysSnapToNearestAxis;

  bool PickCameraFocalInfo;

  // Locking normal to camera
  vtkTypeBool LockNormalToCamera;

  // Controlling the push operation
  double BumpDistance;

  int TranslationAxis;

  // The bounding box is represented by a single voxel image data
  vtkNew<vtkImageData> Box;
  vtkNew<vtkOutlineFilter> Outline;
  vtkNew<vtkPolyDataMapper> OutlineMapper;
  vtkNew<vtkActor> OutlineActor;
  vtkTypeBool OutlineTranslation; // whether the outline can be moved
  vtkTypeBool ScaleEnabled;       // whether the widget can be scaled
  vtkTypeBool OutsideBounds;      // whether the widget can be moved outside input's bounds
  double WidgetBounds[6];
  vtkTypeBool ConstrainToWidgetBounds; // whether the widget can be moved outside input's bounds
  vtkTypeBool ConstrainMaximumSizeToWidgetBounds; // whether the maximum widget size is constrained
  vtkTypeBool DrawOutline;                        // whether to draw the outline
  void HighlightOutline(int highlight);

  // The plane
  double RadiusMultiplier;
  vtkNew<vtkPlane> Plane;
  vtkNew<vtkDiskSource> DiskPlaneSource;
  vtkNew<vtkPolyDataMapper> PlaneMapper;
  vtkNew<vtkActor> PlaneActor;
  vtkTypeBool DrawPlane;
  void HighlightPlane(int highlight);

  // plane boundary edges are represented as tubes
  vtkNew<vtkFeatureEdges> Edges;
  vtkNew<vtkTubeFilter> EdgesTuber;
  vtkNew<vtkPolyDataMapper> EdgesMapper;
  vtkNew<vtkActor> EdgesActor;
  void HighlightEdges(int highlight);

  ///@{
  /**
   * Set color to the edges
   */
  void SetEdgesColor(vtkLookupTable*);
  void SetEdgesColor(double, double, double);
  void SetEdgesColor(double c[3]);
  ///@}

  // The intersection edges with the outline
  vtkNew<vtkCutter> Cutter;
  vtkNew<vtkFeatureEdges> IntersectionEdges;
  vtkNew<vtkTubeFilter> IntersectionEdgesTuber;
  vtkNew<vtkPolyDataMapper> IntersectionEdgesMapper;
  vtkNew<vtkActor> IntersectionEdgesActor;
  vtkTypeBool DrawIntersectionEdges;

  ///@{
  /**
   * Set color to the intersection edges
   */
  void SetIntersectionEdgesColor(vtkLookupTable*);
  void SetIntersectionEdgesColor(double, double, double);
  void SetIntersectionEdgesColor(double c[3]);
  ///@}

  // The + normal cone
  vtkNew<vtkConeSource> ConeSource;
  vtkNew<vtkPolyDataMapper> ConeMapper;
  vtkNew<vtkActor> ConeActor;
  void HighlightNormal(int highlight);

  // The normal line
  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkPolyDataMapper> LineMapper;
  vtkNew<vtkActor> LineActor;

  // The - normal cone
  vtkNew<vtkConeSource> ConeSource2;
  vtkNew<vtkPolyDataMapper> ConeMapper2;
  vtkNew<vtkActor> ConeActor2;

  // The origin positioning handle
  vtkNew<vtkSphereSource> Sphere;
  vtkNew<vtkPolyDataMapper> SphereMapper;
  vtkNew<vtkActor> SphereActor;
  void HighlightSphere(int highlight);

  // Do the picking
  vtkNew<vtkHardwarePicker> HardwarePicker; // Used for picking rendered props (screen)
  vtkNew<vtkCellPicker> CellPicker;         // Used for picking widget props (screen and VR)
  // Compute Cell Picker tolerance
  void ComputeAdaptivePickerTolerance();

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Transform the normal (used for rotation)
  vtkNew<vtkTransform> Transform;

  // Methods to manipulate the plane
  void Rotate(double X, double Y, double* p1, double* p2, double* vpn);
  void Rotate3D(double* p1, double* p2);
  void TranslateOutline(double* p1, double* p2);
  void TranslateOrigin(double* p1, double* p2);
  void UpdatePose(double* p1, double* d1, double* p2, double* d2);
  void Push(double* p1, double* p2);
  void ResizeRadius(double* p1, double* p2, double* vpn);
  void ResizeRadius3D(double* p1, double* p2);
  void Scale(double* p1, double* p2, double X, double Y);
  void SizeHandles();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> NormalProperty;
  vtkNew<vtkProperty> SelectedNormalProperty;
  vtkNew<vtkProperty> SphereProperty;
  vtkNew<vtkProperty> SelectedSphereProperty;
  vtkNew<vtkProperty> PlaneProperty;
  vtkNew<vtkProperty> SelectedPlaneProperty;
  vtkNew<vtkProperty> OutlineProperty;
  vtkNew<vtkProperty> SelectedOutlineProperty;
  vtkNew<vtkProperty> EdgesProperty;
  vtkNew<vtkProperty> SelectedEdgesProperty;
  vtkNew<vtkProperty> IntersectionEdgesProperty;
  virtual void CreateDefaultProperties();

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

private:
  vtkDisplaySizedImplicitPlaneRepresentation(
    const vtkDisplaySizedImplicitPlaneRepresentation&) = delete;
  void operator=(const vtkDisplaySizedImplicitPlaneRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
