/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientationRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOrientationRepresentation
 * @brief   a class defining the representation for the vtkOrientationWidget
 *
 * This class is a concrete representation for the vtkOrientationWidget.
 * The camera is represented by a box and a cone. The first one allows
 * camera movement, the second allows view angle update. There are three
 * more handles to rotate the view up, and move the target position.
 * It also has a frustum representation.
 *
 * To use this representation, you can use the PlaceWidget() method
 * to position the widget looking at a specified region in space. This
 * is optional as you may want to not move the camera at setup.
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

#include <array>

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkOrientationWidget;
class vtkCellPicker;
class vtkProperty;
class vtkSuperquadricSource;
class vtkTransform;

class VTKINTERACTIONWIDGETS_EXPORT vtkOrientationRepresentation : public vtkWidgetRepresentation
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
  vtkProperty* GetPropertyY(bool selected) { return this->GetProperty(Axis::X_AXIS, selected); }
  vtkProperty* GetPropertyZ(bool selected) { return this->GetProperty(Axis::X_AXIS, selected); }
  ///@}

  ///@{
  /**
   * Set/Get the length (Z scale) of the torus.
   * This is a factor of Thickness parameter.
   * Clamped between [0.01, 100.0].
   * Default: 7.5.
   */
  void SetLength(double length);
  vtkGetMacro(Length, double);
  ///@}

  ///@{
  /**
   * Set/Get the thickness of the torus.
   * Thickness handles width in every axes.
   * This means Length depends on it.
   * Clamped between [0.001, 0.001].
   * Default: 0.005.
   */
  void SetThickness(double thickness);
  vtkGetMacro(Thickness, double);
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

  // Used to select properties axis dependant
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
  void HighlightHandle(vtkProp* prop);

  void Rotate(const double p1[4], const double p2[4], const double baseVector[3]);

  // Manage how the representation appears
  double LastEventPosition[3] = { 0.0 };

  // The torus actors and geometry
  std::map<Axis, vtkNew<vtkSuperquadricSource>> TorusSources;
  std::map<Axis, vtkNew<vtkActor>> TorusActors;
  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  std::map<Axis, vtkSmartPointer<vtkProperty>> TorusProperties;
  std::map<Axis, vtkSmartPointer<vtkProperty>> SelectedTorusProperties;
  double Thickness = 0.005;
  double Length = 7.5;

  // Do the picking
  vtkNew<vtkCellPicker> HandlePicker;
  vtkProp* CurrentHandle = nullptr;
  vtkProp* LastHandle = nullptr;

  // Transform informations
  vtkSmartPointer<vtkTransform> BaseTransform;
  vtkSmartPointer<vtkTransform> OrientationTransform;

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

private:
  vtkOrientationRepresentation(const vtkOrientationRepresentation&) = delete;
  void operator=(const vtkOrientationRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
