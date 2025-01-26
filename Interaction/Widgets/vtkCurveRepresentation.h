// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCurveRepresentation
 * @brief   base class for a widget that represents a curve that connects control
 * points.
 *
 * Base class for widgets used to define curves from points, such as
 * vtkPolyLineRepresentation and vtkSplineRepresentation.  This class
 * uses handles, the number of which can be changed, to represent the
 * points that define the curve. The handles can be picked can be
 * picked on the curve itself to translate or rotate it in the scene.
 *
 * @sa
 * vtkPolyLineRepresentation vtkSplineRepresentation
 */

#ifndef vtkCurveRepresentation_h
#define vtkCurveRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"        // needed for vtkPolyDataAlgorithm
#include "vtkWidgetRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCellPicker;
class vtkDoubleArray;
class vtkHandleSource;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkTransform;

#define VTK_PROJECTION_YZ 0
#define VTK_PROJECTION_XZ 1
#define VTK_PROJECTION_XY 2
#define VTK_PROJECTION_OBLIQUE 3
class VTKINTERACTIONWIDGETS_EXPORT vtkCurveRepresentation : public vtkWidgetRepresentation
{
public:
  vtkTypeMacro(vtkCurveRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Used to manage the InteractionState of the widget
  enum InteractionStateType
  {
    Outside = 0,
    OnHandle,
    OnLine,
    Moving,
    Scaling,
    Spinning,
    Inserting,
    Erasing,
    Pushing
  };

  ///@{
  /**
   * Set the interaction state
   */
  vtkSetMacro(InteractionState, int);
  ///@}

  ///@{
  /**
   * Force the widget to be projected onto one of the orthogonal
   * planes.  Remember that when the InteractionState changes, a
   * ModifiedEvent is invoked.  This can be used to snap the curve to
   * the plane if it is originally not aligned.  The normal in
   * SetProjectionNormal is 0,1,2 for YZ,XZ,XY planes respectively and
   * 3 for arbitrary oblique planes when the widget is tied to a
   * vtkPlaneSource.
   */
  vtkSetMacro(ProjectToPlane, vtkTypeBool);
  vtkGetMacro(ProjectToPlane, vtkTypeBool);
  vtkBooleanMacro(ProjectToPlane, vtkTypeBool);
  ///@}

  /**
   * Set up a reference to a vtkPlaneSource that could be from another widget
   * object, e.g. a vtkPolyDataSourceWidget.
   */
  void SetPlaneSource(vtkPlaneSource* plane);

  vtkSetClampMacro(ProjectionNormal, int, VTK_PROJECTION_YZ, VTK_PROJECTION_OBLIQUE);
  vtkGetMacro(ProjectionNormal, int);
  void SetProjectionNormalToXAxes() { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes() { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes() { this->SetProjectionNormal(2); }
  void SetProjectionNormalToOblique() { this->SetProjectionNormal(3); }

  ///@{
  /**
   * Set the position of poly line handles and points in terms of a plane's
   * position. i.e., if ProjectionNormal is 0, all of the x-coordinate
   * values of the points are set to position. Any value can be passed (and is
   * ignored) to update the poly line points when Projection normal is set to 3
   * for arbitrary plane orientations.
   */
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);
  ///@}

  /**
   * Grab the polydata (including points) that defines the
   * interpolating curve. Points are guaranteed to be up-to-date when
   * either the InteractionEvent or EndInteraction events are
   * invoked. The user provides the vtkPolyData and the points and
   * polyline are added to it.
   */
  virtual void GetPolyData(vtkPolyData* pd) = 0;

  ///@{
  /**
   * Set/Get the handle properties (the spheres are the handles). The
   * properties of the handles when selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set/Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set/Get the number of handles for this widget.
   */
  virtual void SetNumberOfHandles(int npts) = 0;
  vtkGetMacro(NumberOfHandles, int);
  ///@}

  ///@{
  /**
   * Set the representation to be directional or not.
   * The meaning of being directional depends on the representation and
   * its handles implementations in the subclasses.
   */
  virtual void SetDirectional(bool val);
  vtkGetMacro(Directional, bool);
  vtkBooleanMacro(Directional, bool);
  ///@}

  ///@{
  /**
   * Set/Get the position of the handles. Call GetNumberOfHandles
   * to determine the valid range of handle indices.
   */
  virtual void SetHandlePosition(int handle, double x, double y, double z);
  virtual void SetHandlePosition(int handle, double xyz[3]);
  virtual void GetHandlePosition(int handle, double xyz[3]);
  virtual double* GetHandlePosition(int handle);
  virtual vtkDoubleArray* GetHandlePositions() = 0;
  ///@}

  ///@{
  /**
   * Control whether the curve is open or closed. A closed forms a
   * continuous loop: the first and last points are the same.  A
   * minimum of 3 handles are required to form a closed loop.
   */
  void SetClosed(vtkTypeBool closed);
  vtkGetMacro(Closed, vtkTypeBool);
  vtkBooleanMacro(Closed, vtkTypeBool);
  ///@}

  /**
   * Convenience method to determine whether the curve is
   * closed in a geometric sense.  The widget may be set "closed" but still
   * be geometrically open (e.g., a straight line).
   */
  vtkTypeBool IsClosed();

  /**
   * Get the approximate vs. the true arc length of the curve. Calculated as
   * the summed lengths of the individual straight line segments. Use
   * SetResolution to control the accuracy.
   */
  virtual double GetSummedLength() = 0;

  /**
   * Convenience method to allocate and set the handles from a
   * vtkPoints instance.  If the first and last points are the same,
   * the curve sets Closed to the on InteractionState and disregards
   * the last point, otherwise Closed remains unchanged.
   */
  virtual void InitializeHandles(vtkPoints* points) = 0;

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation
   * API. Note that a version of place widget is available where the
   * center and handle position are specified.
   */
  void BuildRepresentation() override = 0;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  void EndWidgetInteraction(double e[2]) override;
  double* GetBounds() override;
  ///@}

  ///@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  int RenderOverlay(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLineProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  ///@{
  /**
   * Set the color when unselected and selected.
   */
  void SetInteractionColor(double, double, double);
  void SetInteractionColor(double c[3]) { this->SetInteractionColor(c[0], c[1], c[2]); }
  void SetForegroundColor(double, double, double);
  void SetForegroundColor(double c[3]) { this->SetForegroundColor(c[0], c[1], c[2]); }
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  ///@{
  /**
   * Get/Set the current handle index. Setting the current handle index will
   * also result in the handle being highlighted. Set to `-1` to remove the
   * highlight.
   */
  void SetCurrentHandleIndex(int index);
  vtkGetMacro(CurrentHandleIndex, int);
  ///@}

  ///@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  vtkGetMacro(TranslationAxis, int);
  vtkSetClampMacro(TranslationAxis, int, -1, 2);
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
   * Methods to make this class behave as a vtkProp. They are repeated here (from the
   * vtkProp superclass) as a reminder to the widget implementer. Failure to implement
   * these methods properly may result in the representation not appearing in the scene
   * (i.e., not implementing the Render() methods properly) or leaking graphics resources
   * (i.e., not implementing ReleaseGraphicsResources() properly).
   */
  void GetActors(vtkPropCollection*) override;

protected:
  vtkCurveRepresentation();
  ~vtkCurveRepresentation() override;

  double LastEventPosition[3];
  double Bounds[6];

  // Controlling vars
  int ProjectionNormal;
  double ProjectionPosition;
  vtkTypeBool ProjectToPlane;
  vtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  int NumberOfHandles = 0;
  vtkTypeBool Closed;

  // The line segments
  vtkActor* LineActor;
  void HighlightLine(int highlight);
  int HighlightHandle(vtkProp* prop); // returns handle index or -1 on fail

  // accessors to glyphs representing hot spots (e.g., handles)
  virtual vtkActor* GetHandleActor(int index) = 0;
  virtual vtkHandleSource* GetHandleSource(int index) = 0;

  /**
   * returns handle index or -1 on fail
   */
  virtual int GetHandleIndex(vtkProp* prop) = 0;
  virtual void SizeHandles();

  /**
   * Returns the position of insertion or -1 on fail.
   */
  virtual int InsertHandleOnLine(double* pos) = 0;

  virtual void PushHandle(double* pos);
  virtual void EraseHandle(const int&);

  // Do the picking
  vtkCellPicker* HandlePicker;
  vtkCellPicker* LinePicker;
  double LastPickPosition[3];
  vtkActor* CurrentHandle;
  int CurrentHandleIndex;
  bool FirstSelected;

  // Methods to manipulate the curve.
  void MovePoint(double* p1, double* p2);
  void Scale(double* p1, double* p2, int X, int Y);
  void Translate(double* p1, double* p2);
  void Spin(double* p1, double* p2, double* vpn);

  // Transform the control points (used for spinning)
  vtkTransform* Transform;

  // Manage how the representation appears
  bool Directional = false;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty* HandleProperty;
  vtkProperty* SelectedHandleProperty;
  vtkProperty* LineProperty;
  vtkProperty* SelectedLineProperty;
  void CreateDefaultProperties();

  // For efficient spinning
  double Centroid[3];
  void CalculateCentroid();

  int TranslationAxis;

private:
  vtkCurveRepresentation(const vtkCurveRepresentation&) = delete;
  void operator=(const vtkCurveRepresentation&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
