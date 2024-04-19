// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrientationRepresentation
 * @brief   a class defining the representation for the vtkOrientationWidget
 *
 * This class is a concrete representation for the vtkOrientationWidget.
 * The widget is represented by three flat tori of different colors in
 * each base direction (X/Y/Z). Additional arrows can be added to it for
 * each direction in order to simplify grabbing and understanding. Their
 * default look is a diamond shape, but they can be customized to look
 * like real double arrows.
 *
 * To use this representation, you can use the PlaceWidget() method to
 * position the widget around an actor and scale it properly.
 * You can retrieve orientation values with component-wise getters or
 * through a vtkTransform.
 *
 * @sa
 * vtkOrientationWidget
 */

#ifndef vtkOrientationRepresentation_h
#define vtkOrientationRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <map>

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkArrowSource;
class vtkBox;
class vtkCellPicker;
class vtkOrientationWidget;
class vtkPolyDataNormals;
class vtkProperty;
class vtkSuperquadricSource;
class vtkTransform;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkOrientationRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOrientationRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkOrientationRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Get the orientation transform.
   */
  vtkTransform* GetTransform();
  ///@}

  ///@{
  /**
   * Set/Get the orientation values.
   * Angles are in interval [-180, 180] degrees.
   */
  virtual void SetOrientation(double values[3]);
  virtual void SetOrientationX(double value);
  virtual void SetOrientationY(double value);
  virtual void SetOrientationZ(double value);
  double* GetOrientation();
  double GetOrientationX();
  double GetOrientationY();
  double GetOrientationZ();
  ///@}

  ///@{
  /**
   * Set/Get the properties values.
   * Axis is clamped to axis values.
   * If selected is true, applies to selected properties (on hover or click).
   */
  void SetProperty(int axis, bool selected, vtkProperty* property);
  void SetPropertyX(bool selected, vtkProperty* property)
  {
    this->SetProperty(Axis::X_AXIS, selected, property);
  }
  void SetPropertyY(bool selected, vtkProperty* property)
  {
    this->SetProperty(Axis::Y_AXIS, selected, property);
  }
  void SetPropertyZ(bool selected, vtkProperty* property)
  {
    this->SetProperty(Axis::Z_AXIS, selected, property);
  }
  vtkProperty* GetProperty(int axis, bool selected);
  vtkProperty* GetPropertyX(bool selected) { return this->GetProperty(Axis::X_AXIS, selected); }
  vtkProperty* GetPropertyY(bool selected) { return this->GetProperty(Axis::Y_AXIS, selected); }
  vtkProperty* GetPropertyZ(bool selected) { return this->GetProperty(Axis::Z_AXIS, selected); }
  ///@}

  ///@{
  /**
   * Set/Get the length (Z scale) of the torus.
   * This is a factor of Thickness parameter.
   * Clamped between [0.01, 100.0].
   * Default: 7.5.
   */
  vtkSetClampMacro(TorusLength, double, MINIMUM_TORUS_LENGTH, MAXIMUM_TORUS_LENGTH);
  vtkGetMacro(TorusLength, double);
  ///@}

  ///@{
  /**
   * Set/Get the thickness of the torus.
   * Thickness handles width in every axes.
   * This means Length depends on it.
   * Clamped between [0.001, 0.1].
   * Default: 0.005.
   */
  vtkSetClampMacro(TorusThickness, double, MINIMUM_TORUS_THICKNESS, MAXIMUM_TORUS_THICKNESS);
  vtkGetMacro(TorusThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get whether to show arrows.
   * Default: False.
   */
  vtkSetMacro(ShowArrows, bool);
  vtkGetMacro(ShowArrows, bool);
  vtkBooleanMacro(ShowArrows, bool);
  ///@}

  ///@{
  /**
   * Set/Get the distance between arrows and torus.
   * Clamped between [0.0, 0.5].
   * Default: 0.0.
   */
  vtkSetClampMacro(ArrowDistance, double, MINIMUM_ARROW_DISTANCE, MAXIMUM_ARROW_DISTANCE);
  vtkGetMacro(ArrowDistance, double);
  ///@}

  ///@{
  /**
   * Set/Get the arrow length.
   * This includes shaft+tip.
   * Note that double arrows are two arrows
   * next to each other.
   * Clamped between [0.01, 0.5].
   * Default: 0.05.
   */
  vtkSetClampMacro(ArrowLength, double, MINIMUM_ARROW_LENGTH, MAXIMUM_ARROW_LENGTH);
  vtkGetMacro(ArrowLength, double);
  ///@}

  ///@{
  /**
   * Set/Get the length of the arrow tip.
   * Factor of arrow length, equals if set to 1.
   * Note that double arrows are two arrows
   * next to each other.
   * Clamped between [0.0, 1.0].
   * Default: 1.0.
   */
  vtkSetMacro(ArrowTipLength, double);
  vtkGetMacro(ArrowTipLength, double);
  ///@}

  ///@{
  /**
   * Set/Get the radius of the arrow tip.
   * Clamped between [0.001, 0.5].
   * Default: 0.03.
   */
  vtkSetClampMacro(ArrowTipRadius, double, MINIMUM_ARROW_TIP_RADIUS, MAXIMUM_ARROW_TIP_RADIUS);
  vtkGetMacro(ArrowTipRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the radius of the arrow shaft.
   * Clamped between [0.001, 0.5].
   * Default: 0.001.
   */
  vtkSetClampMacro(
    ArrowShaftRadius, double, MINIMUM_ARROW_SHAFT_RADIUS, MAXIMUM_ARROW_SHAFT_RADIUS);
  vtkGetMacro(ArrowShaftRadius, double);
  ///@}

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  double* GetBounds() VTK_SIZEHINT(6) override;
  ///@}

  ///@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * The interaction state may be set from a widget (e.g., vtkOrientationWidget)
   * or other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  void SetInteractionState(int state);

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp (i.e., support rendering).
   * GetActors adds all the internal props used by this representation to the supplied collection.
   */
  void GetActors(vtkPropCollection*) override;

  // Used to manage the state of the widget
  enum
  {
    Outside = 0,
    RotatingX,
    RotatingY,
    RotatingZ
  };

  // Used to select properties axis dependent
  enum Axis : int
  {
    X_AXIS = 0,
    Y_AXIS,
    Z_AXIS
  };

protected:
  vtkOrientationRepresentation();
  ~vtkOrientationRepresentation() override;

  virtual void CreateDefaultProperties();
  void UpdateGeometry();
  void HighlightHandle();

private:
  vtkOrientationRepresentation(const vtkOrientationRepresentation&) = delete;
  void operator=(const vtkOrientationRepresentation&) = delete;

  /**
   * Method to initialize (instantiate) geometric sources (tori and arrows).
   */
  void InitSources();
  /**
   * Method to initialize transform handling position and scaling of tori.
   * Can be used to recompute them.
   */
  void InitTransforms();

  /**
   * Helper method to rotate the orientation transform around a base vector
   * from the angle formed by two input positions.
   */
  void Rotate(const double p1[4], const double p2[4], const double baseVector[3]);

  /**
   * Helper to create a source made of 4 arrows rotated depending on axis.
   */
  vtkSmartPointer<vtkPolyDataNormals> GetArrowsOutput(int axisIndex);

  // Manage how the representation appears
  double LastEventPosition[3] = { 0.0 };

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

  // Do the picking
  vtkNew<vtkCellPicker> HandlePicker;
  vtkSmartPointer<vtkProp> CurrentHandle;
  vtkSmartPointer<vtkProp> LastHandle;

  // Transform information
  vtkNew<vtkTransform> BaseTransform;
  vtkNew<vtkTransform> OrientationTransform;

  // Actors and geometry
  vtkNew<vtkTransform> ArrowPosTransform;
  vtkNew<vtkTransform> ArrowPosInvTransform;
  vtkNew<vtkTransform> ArrowScaleTransform;
  std::vector<vtkSmartPointer<vtkArrowSource>> ArrowSources;
  std::vector<vtkSmartPointer<vtkSuperquadricSource>> TorusSources;
  std::map<Axis, vtkNew<vtkActor>> TorusActors;
  std::map<Axis, vtkNew<vtkActor>> ArrowsActors;
  // Parameters used to control the appearance of selected objects and
  // the manipulator in general.
  std::map<Axis, vtkSmartPointer<vtkProperty>> Properties;
  std::map<Axis, vtkSmartPointer<vtkProperty>> SelectedProperties;
  // ... torus specific
  double TorusLength = 7.5;
  double TorusThickness = 0.005;
  // ... arrow specific
  bool ShowArrows = false;
  double ArrowDistance = 0.0;
  double ArrowLength = 0.05;
  double ArrowTipLength = 1.0;
  double ArrowTipRadius = 0.03;
  double ArrowShaftRadius = 0.001;

  // Minima/maxima to clamp values
  static constexpr double MINIMUM_TORUS_THICKNESS = 0.001;
  static constexpr double MAXIMUM_TORUS_THICKNESS = 0.1;
  static constexpr double MINIMUM_TORUS_LENGTH = 0.01;
  static constexpr double MAXIMUM_TORUS_LENGTH = 100.0;

  static constexpr double MINIMUM_ARROW_DISTANCE = 0.0;
  static constexpr double MAXIMUM_ARROW_DISTANCE = 0.5;
  static constexpr double MINIMUM_ARROW_LENGTH = 0.01;
  static constexpr double MAXIMUM_ARROW_LENGTH = 0.5;
  static constexpr double MINIMUM_ARROW_TIP_RADIUS = 0.001;
  static constexpr double MAXIMUM_ARROW_TIP_RADIUS = 0.5;
  static constexpr double MINIMUM_ARROW_SHAFT_RADIUS = 0.001;
  static constexpr double MAXIMUM_ARROW_SHAFT_RADIUS = 0.5;
};

VTK_ABI_NAMESPACE_END
#endif
