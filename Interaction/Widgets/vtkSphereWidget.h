// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSphereWidget
 * @brief   3D widget for manipulating a sphere
 *
 * This 3D widget defines a sphere that can be interactively placed in a
 * scene.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a vtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. The interactor will act
 * normally until the "i" key (for "interactor") is pressed, at which point the
 * vtkSphereWidget will appear. (See superclass documentation for information
 * about changing this behavior.)
 * Events that occur outside of the widget (i.e., no part of
 * the widget is picked) are propagated to any other registered obsevers
 * (such as the interaction style).  Turn off the widget by pressing the "i"
 * key again (or invoke the Off() method).
 *
 * The vtkSphereWidget has several methods that can be used in conjunction
 * with other VTK objects. The Set/GetThetaResolution() and
 * Set/GetPhiResolution() methods control the number of subdivisions of the
 * sphere in the theta and phi directions; the GetPolyData() method can be
 * used to get the polygonal representation and can be used for things like
 * seeding streamlines. The GetSphere() method returns a sphere implicit
 * function that can be used for cutting and clipping. Typical usage of the
 * widget is to make use of the StartInteractionEvent, InteractionEvent, and
 * EndInteractionEvent events. The InteractionEvent is called on mouse
 * motion; the other two events are called on button down and button up
 * (any mouse button).
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You can set the properties of the selected and
 * unselected representations of the sphere.
 *
 * @sa
 * vtk3DWidget vtkLineWidget vtkBoxWidget vtkPlaneWidget
 */

#ifndef vtkSphereWidget_h
#define vtkSphereWidget_h

#include "vtk3DWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkSphereSource.h"             // Needed for faster access to the sphere source

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkSphereSource;
class vtkSphere;
class vtkCellPicker;
class vtkProperty;

#define VTK_SPHERE_OFF 0
#define VTK_SPHERE_WIREFRAME 1
#define VTK_SPHERE_SURFACE 2

class VTKINTERACTIONWIDGETS_EXPORT vtkSphereWidget : public vtk3DWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkSphereWidget* New();

  vtkTypeMacro(vtkSphereWidget, vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods that satisfy the superclass' API.
   */
  void SetEnabled(int) override;
  void PlaceWidget(double bounds[6]) override;
  void PlaceWidget() override { this->Superclass::PlaceWidget(); }
  void PlaceWidget(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) override
  {
    this->Superclass::PlaceWidget(xmin, xmax, ymin, ymax, zmin, zmax);
  }
  ///@}

  ///@{
  /**
   * Set the representation of the sphere. Different representations are
   * useful depending on the application. The default is
   * VTK_SPHERE_WIREFRAME.
   */
  vtkSetClampMacro(Representation, int, VTK_SPHERE_OFF, VTK_SPHERE_SURFACE);
  vtkGetMacro(Representation, int);
  void SetRepresentationToOff() { this->SetRepresentation(VTK_SPHERE_OFF); }
  void SetRepresentationToWireframe() { this->SetRepresentation(VTK_SPHERE_WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(VTK_SPHERE_SURFACE); }
  ///@}

  /**
   * Set/Get the resolution of the sphere in the Theta direction.
   * The default is 16.
   */
  void SetThetaResolution(int r) { this->SphereSource->SetThetaResolution(r); }
  int GetThetaResolution() { return this->SphereSource->GetThetaResolution(); }

  /**
   * Set/Get the resolution of the sphere in the Phi direction.
   * The default is 8.
   */
  void SetPhiResolution(int r) { this->SphereSource->SetPhiResolution(r); }
  int GetPhiResolution() { return this->SphereSource->GetPhiResolution(); }

  ///@{
  /**
   * Set/Get the radius of sphere. Default is .5.
   */
  void SetRadius(double r)
  {
    if (r <= 0)
    {
      r = .00001;
    }
    this->SphereSource->SetRadius(r);
  }
  double GetRadius() { return this->SphereSource->GetRadius(); }
  ///@}

  ///@{
  /**
   * Set/Get the center of the sphere.
   */
  void SetCenter(double x, double y, double z) { this->SphereSource->SetCenter(x, y, z); }
  void SetCenter(double x[3]) { this->SetCenter(x[0], x[1], x[2]); }
  double* GetCenter() VTK_SIZEHINT(3) { return this->SphereSource->GetCenter(); }
  void GetCenter(double xyz[3]) { this->SphereSource->GetCenter(xyz); }
  ///@}

  ///@{
  /**
   * Enable translation and scaling of the widget. By default, the widget
   * can be translated and rotated.
   */
  vtkSetMacro(Translation, vtkTypeBool);
  vtkGetMacro(Translation, vtkTypeBool);
  vtkBooleanMacro(Translation, vtkTypeBool);
  vtkSetMacro(Scale, vtkTypeBool);
  vtkGetMacro(Scale, vtkTypeBool);
  vtkBooleanMacro(Scale, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The handle sits on the surface of the sphere and may be moved around
   * the surface by picking (left mouse) and then moving. The position
   * of the handle can be retrieved, this is useful for positioning cameras
   * and lights. By default, the handle is turned off.
   */
  vtkSetMacro(HandleVisibility, vtkTypeBool);
  vtkGetMacro(HandleVisibility, vtkTypeBool);
  vtkBooleanMacro(HandleVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the direction vector of the handle relative to the center of
   * the sphere. The direction of the handle is from the sphere center to
   * the handle position.
   */
  vtkSetVector3Macro(HandleDirection, double);
  vtkGetVector3Macro(HandleDirection, double);
  ///@}

  ///@{
  /**
   * Get the position of the handle.
   */
  vtkGetVector3Macro(HandlePosition, double);
  ///@}

  /**
   * Grab the polydata (including points) that defines the sphere.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * sphere. These point values are guaranteed to be up-to-date when either the
   * InteractionEvent or EndInteraction events are invoked. The user provides
   * the vtkPolyData and the points and polysphere are added to it.
   */
  void GetPolyData(vtkPolyData* pd);

  /**
   * Get the spherical implicit function defined by this widget.  Note that
   * vtkSphere is a subclass of vtkImplicitFunction, meaning that it can be
   * used by a variety of filters to perform clipping, cutting, and selection
   * of data.
   */
  void GetSphere(vtkSphere* sphere);

  ///@{
  /**
   * Get the sphere properties. The properties of the sphere when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(SphereProperty, vtkProperty);
  vtkGetObjectMacro(SelectedSphereProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the handle properties (the little ball on the sphere is the
   * handle). The properties of the handle when selected and unselected
   * can be manipulated.
   */
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

protected:
  vtkSphereWidget();
  ~vtkSphereWidget() override;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    Moving,
    Scaling,
    Positioning,
    Outside
  };

  // handles the events
  static void ProcessEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // the sphere
  vtkActor* SphereActor;
  vtkPolyDataMapper* SphereMapper;
  vtkSphereSource* SphereSource;
  void HighlightSphere(int highlight);
  void SelectRepresentation();

  // The representation of the sphere
  int Representation;

  // Do the picking
  vtkCellPicker* Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Methods to manipulate the sphere widget
  vtkTypeBool Translation;
  vtkTypeBool Scale;
  void Translate(double* p1, double* p2);
  void ScaleSphere(double* p1, double* p2, int X, int Y);
  void MoveHandle(double* p1, double* p2, int X, int Y);
  void PlaceHandle(double* center, double radius);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty* SphereProperty;
  vtkProperty* SelectedSphereProperty;
  vtkProperty* HandleProperty;
  vtkProperty* SelectedHandleProperty;
  void CreateDefaultProperties();

  // Managing the handle
  vtkActor* HandleActor;
  vtkPolyDataMapper* HandleMapper;
  vtkSphereSource* HandleSource;
  void HighlightHandle(int);
  vtkTypeBool HandleVisibility;
  double HandleDirection[3];
  double HandlePosition[3];
  void SizeHandles() override;

private:
  vtkSphereWidget(const vtkSphereWidget&) = delete;
  void operator=(const vtkSphereWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
