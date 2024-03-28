// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCoordinateFrameRepresentation
 * @brief   a class defining the representation for a vtkCoordinateFrameWidget
 *
 * This class is a concrete representation for the
 * vtkCoordinateFrameWidget. It represents a coordinate frame with an origin, 3 axis and
 * 3 axis lockers. Through interaction with the widget, the coordinate frame can be manipulated
 * by adjusting the axis normals, locking them, or moving/picking the origin point.
 *
 * The PlaceWidget() method is also used to initially position the
 * representation.
 *
 * @warning
 * This class, and vtkCoordinateFrameWidget, are next generation VTK
 * widgets.
 */

#ifndef vtkCoordinateFrameRepresentation_h
#define vtkCoordinateFrameRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew command
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkCellPicker;
class vtkConeSource;
class vtkFeatureEdges;
class vtkGenericCell;
class vtkHardwarePicker;
class vtkLineSource;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCoordinateFrameRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCoordinateFrameRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkCoordinateFrameRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the origin of the coordinate frame.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Set/Get the normal of one of the axes of the coordinate frame.
   *
   * 1) If 1 arrow tip is constrained, the corresponding normal vector is set to the picked normal.
   * 2) Otherwise, the axis closest to the picked normal (i.e., with the largest dot product) is
   * reset to the picked normal.
   *
   * In both cases, the remaining normals are re-orthogonalized using the
   * <a href="https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process">
   * Gram-Schmidt procedure</a>.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double n[3]);
  void SetNormalToCamera();
  vtkGetVector3Macro(XVectorNormal, double);
  vtkGetVector3Macro(YVectorNormal, double);
  vtkGetVector3Macro(ZVectorNormal, double);
  ///@}

  ///@{
  /**
   * Set the direction of the locked (or absent a locked axis, the nearest
   * axis) to point from the frame's origin toward the given (x,y,z) location.
   */
  void SetDirection(double x, double y, double z);
  void SetDirection(double d[3]);
  ///@}

  ///@{
  /**
   * Force an axis to be aligned with the vector \a v, regardless of whether any axis is locked.
   *
   * This will normalize \a v and re-orthogonalize the remaining axes using the Gram-Schmidt
   * procedure. Passing in a degenerate (zero-length) vector will be ignored.
   */
  void SetXAxisVector(const double v[3]);
  void SetXAxisVector(double x, double y, double z);
  void SetYAxisVector(const double v[3]);
  void SetYAxisVector(double x, double y, double z);
  void SetZAxisVector(const double v[3]);
  void SetZAxisVector(double x, double y, double z);
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

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement();

  /**
   * Reset the origin (by calling update placement) and the axes (to be aligned
   * with the world coordinate X, Y, and Z axes).
   */
  void Reset();

  /**
   * Reset only the axis orientations (not the origin).
   */
  void ResetAxes();

  ///@{
  /**
   * Get the properties of the origin. The properties of the origin when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(OriginProperty, vtkProperty);
  vtkGetObjectMacro(SelectedOriginProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the XVector. The properties of the XVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(XVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedXVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the LockedXVector. The properties of the LockedXVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LockedXVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLockedXVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the UnlockedXVector. The properties of the UnlockedXVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(UnlockedXVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedUnlockedXVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the YVector. The properties of the YVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(YVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedYVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the LockedYVector. The properties of the LockedYVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LockedYVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLockedYVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the UnlockedYVector. The properties of the UnlockedYVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(UnlockedYVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedUnlockedYVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the ZVector. The properties of the ZVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(ZVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedZVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the LockedZVector. The properties of the LockedZVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LockedZVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLockedZVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the properties on the UnlockedZVector. The properties of the UnlockedZVector when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(UnlockedZVectorProperty, vtkProperty);
  vtkGetObjectMacro(SelectedUnlockedZVectorProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Enable/Disable picking camera focal info if no result is available for PickOrigin, PickNormal
   * and PickDirectionPoint. The default is disabled.
   */
  vtkGetMacro(PickCameraFocalInfo, bool);
  vtkSetMacro(PickCameraFocalInfo, bool);
  vtkBooleanMacro(PickCameraFocalInfo, bool);
  ///@}

  /**
   * Given the X, Y display coordinates, pick a new origin for the coordinate frame
   * from a point that is on the objects rendered by the renderer.
   *
   * Note: if a point from a rendered object is not picked, the camera focal point can optionally be
   * set.
   */
  bool PickOrigin(int X, int Y, bool snapToMeshPoint = false);

  /**
   * Given the X, Y display coordinates, pick a new normal for the coordinate frame
   * from a point that is on the objects rendered by the renderer.
   *
   * Note: if a normal from a rendered object is not picked, the camera plane normal can optionally
   * be set.
   */
  bool PickNormal(int X, int Y, bool snapToMeshPoint = false);

  /**
   * Given the X, Y display coordinates, pick a point and using the origin define normal for the
   * coordinate frame from a point that is on the objects rendered by the renderer.
   *
   * Note: if a point from a rendered object is not picked, the camera focal point can optionally be
   * set.
   */
  bool PickDirectionPoint(int X, int Y, bool snapToMeshPoint = false);

  /**
   * Get/set which axis (if any) is locked.
   *
   * At most, a single axis can be locked at a time.
   *
   * The axis must be one of the following values: { -1, 0, 1, 2 }.
   * -1 indicates that no axis is locked; 0 corresponds to the X axis; 1 to Y; and 2 to Z.
   *
   * In terms of mouse interactions, locking an axis prevents its direction from being
   * modified by rotation (so only rotations about that axis are possible) and
   * prevents the origin from translating along it (so all translations must be in the
   * plane using it as a normal).
   *
   * In terms of picking interactions, locking an axis selects it as the target axis
   * to be modified (i.e., the locked axis will be overwritten with a normal vector
   * or direction vector).
   */
  int GetLockedAxis() const;
  void SetLockedAxis(int axis);

  ///@{
  /**
   * Methods to interface with the vtkCoordinateFrameWidget.
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
    MovingOrigin,
    RotatingXVector,
    RotatingYVector,
    RotatingZVector,
    ModifyingLockerXVector,
    ModifyingLockerYVector,
    ModifyingLockerZVector
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkCoordinateFrameWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, ModifyingLockerZVector);
  ///@}

  ///@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  ///@}

  ///@{
  /**
   * Set/get the length of the axis glyphs relative to screen size.
   * The default is 0.04.
   */
  vtkSetClampMacro(LengthFactor, double, 0, 1);
  vtkGetMacro(LengthFactor, double);
  ///@}

protected:
  vtkCoordinateFrameRepresentation();
  ~vtkCoordinateFrameRepresentation() override;

  int RepresentationState = Outside;

  // Keep track of event positions
  double LastEventPosition[3];

  bool PickCameraFocalInfo = false;

  // Locking normal to camera
  vtkTypeBool LockNormalToCamera = false;

  int TranslationAxis = Axis::NONE;

  double Origin[3] = { 0, 0, 0 };
  double XVectorNormal[3] = { 1, 0, 0 };
  double YVectorNormal[3] = { 0, 1, 0 };
  double ZVectorNormal[3] = { 0, 0, 1 };
  vtkSetVector3Macro(XVectorNormal, double);
  vtkSetVector3Macro(YVectorNormal, double);
  vtkSetVector3Macro(ZVectorNormal, double);

  // The origin positioning handle
  vtkNew<vtkSphereSource> OriginSphereSource;
  vtkNew<vtkPolyDataMapper> OriginSphereMapper;
  vtkNew<vtkActor> OriginSphereActor;
  void HighlightOrigin(int highlight);

  // The XVector line source
  vtkNew<vtkLineSource> XVectorLineSource;
  vtkNew<vtkPolyDataMapper> XVectorLineMapper;
  vtkNew<vtkActor> XVectorLineActor;
  // The XVector cone source
  vtkNew<vtkConeSource> XVectorConeSource;
  vtkNew<vtkPolyDataMapper> XVectorConeMapper;
  vtkNew<vtkActor> XVectorConeActor;
  void HighlightXVector(int highlight);
  // The lock XVector cone source
  bool XVectorIsLocked = false;
  vtkNew<vtkConeSource> LockerXVectorConeSource;
  vtkNew<vtkPolyDataMapper> LockerXVectorConeMapper;
  vtkNew<vtkActor> LockerXVectorConeActor;
  void HighlightLockerXVector(int highlight);

  // The YVector line source
  vtkNew<vtkLineSource> YVectorLineSource;
  vtkNew<vtkPolyDataMapper> YVectorLineMapper;
  vtkNew<vtkActor> YVectorLineActor;
  // The YVector cone source
  vtkNew<vtkConeSource> YVectorConeSource;
  vtkNew<vtkPolyDataMapper> YVectorConeMapper;
  vtkNew<vtkActor> YVectorConeActor;
  void HighlightYVector(int highlight);
  // The lock YVector cone source
  bool YVectorIsLocked = false;
  vtkNew<vtkConeSource> LockerYVectorConeSource;
  vtkNew<vtkPolyDataMapper> LockerYVectorConeMapper;
  vtkNew<vtkActor> LockerYVectorConeActor;
  void HighlightLockerYVector(int highlight);

  // The Vector Z line source
  vtkNew<vtkLineSource> ZVectorLineSource;
  vtkNew<vtkPolyDataMapper> ZVectorLineMapper;
  vtkNew<vtkActor> ZVectorLineActor;
  // The Vector Z cone source
  vtkNew<vtkConeSource> ZVectorConeSource;
  vtkNew<vtkPolyDataMapper> ZVectorConeMapper;
  vtkNew<vtkActor> ZVectorConeActor;
  void HighlightZVector(int highlight);
  // The lock Vector Z cone source
  bool ZVectorIsLocked = false;
  vtkNew<vtkConeSource> LockerZVectorConeSource;
  vtkNew<vtkPolyDataMapper> LockerZVectorConeMapper;
  vtkNew<vtkActor> LockerZVectorConeActor;
  void HighlightLockerZVector(int highlight);

  // Do the picking
  vtkNew<vtkHardwarePicker> HardwarePicker; // Used for picking rendered props
  vtkNew<vtkCellPicker> CellPicker;         // Used for picking widget props
  // Compute Picker tolerance
  void ComputeAdaptivePickerTolerance();

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Transform the normal (used for rotation)
  vtkNew<vtkTransform> Transform;

  // Methods to manipulate the plane
  void Rotate(double X, double Y, double* p1, double* p2, double* vpn);
  void ModifyingLocker(int axis);
  void TranslateOrigin(double* p1, double* p2);
  void SizeHandles();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> OriginProperty;
  vtkNew<vtkProperty> SelectedOriginProperty;
  vtkNew<vtkProperty> XVectorProperty;
  vtkNew<vtkProperty> SelectedXVectorProperty;
  vtkNew<vtkProperty> LockedXVectorProperty;
  vtkNew<vtkProperty> SelectedLockedXVectorProperty;
  vtkNew<vtkProperty> UnlockedXVectorProperty;
  vtkNew<vtkProperty> SelectedUnlockedXVectorProperty;
  vtkNew<vtkProperty> YVectorProperty;
  vtkNew<vtkProperty> SelectedYVectorProperty;
  vtkNew<vtkProperty> LockedYVectorProperty;
  vtkNew<vtkProperty> SelectedLockedYVectorProperty;
  vtkNew<vtkProperty> UnlockedYVectorProperty;
  vtkNew<vtkProperty> SelectedUnlockedYVectorProperty;
  vtkNew<vtkProperty> ZVectorProperty;
  vtkNew<vtkProperty> SelectedZVectorProperty;
  vtkNew<vtkProperty> LockedZVectorProperty;
  vtkNew<vtkProperty> SelectedLockedZVectorProperty;
  vtkNew<vtkProperty> UnlockedZVectorProperty;
  vtkNew<vtkProperty> SelectedUnlockedZVectorProperty;
  virtual void CreateDefaultProperties();

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;
  vtkNew<vtkGenericCell> Cell;

  double LengthFactor = 0.04;

private:
  vtkCoordinateFrameRepresentation(const vtkCoordinateFrameRepresentation&) = delete;
  void operator=(const vtkCoordinateFrameRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
