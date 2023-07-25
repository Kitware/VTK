// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlaneWidget
 * @brief   3D widget for manipulating a finite plane
 *
 * This 3D widget defines a finite (bounded) plane that can be interactively
 * placed in a scene. The plane has four handles (at its corner vertices), a
 * normal vector, and the plane itself. The handles are used to resize the
 * plane; the normal vector to rotate it, and the plane can be picked and
 * translated. Selecting the plane while pressing CTRL makes it spin around
 * the normal. A nice feature of the object is that the vtkPlaneWidget, like
 * any 3D widget, will work with the current interactor style. That is, if
 * vtkPlaneWidget does not handle an event, then all other registered
 * observers (including the interactor style) have an opportunity to process
 * the event. Otherwise, the vtkPlaneWidget will terminate the processing of
 * the event that it handles.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a vtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. If the "i" key (for
 * "interactor") is pressed, the vtkPlaneWidget will appear. (See superclass
 * documentation for information about changing this behavior.) By grabbing
 * the one of the four handles (use the left mouse button), the plane can be
 * resized.  By grabbing the plane itself, the entire plane can be
 * arbitrarily translated. Pressing CTRL while grabbing the plane will spin
 * the plane around the normal. If you select the normal vector, the plane can be
 * arbitrarily rotated. Selecting any part of the widget with the middle
 * mouse button enables translation of the plane along its normal. (Once
 * selected using middle mouse, moving the mouse in the direction of the
 * normal translates the plane in the direction of the normal; moving in the
 * direction opposite the normal translates the plane in the direction
 * opposite the normal.) Scaling (about the center of the plane) is achieved
 * by using the right mouse button. By moving the mouse "up" the render
 * window the plane will be made bigger; by moving "down" the render window
 * the widget will be made smaller. Events that occur outside of the widget
 * (i.e., no part of the widget is picked) are propagated to any other
 * registered obsevers (such as the interaction style).  Turn off the widget
 * by pressing the "i" key again (or invoke the Off() method).
 *
 * The vtkPlaneWidget has several methods that can be used in conjunction
 * with other VTK objects. The Set/GetResolution() methods control the number
 * of subdivisions of the plane; the GetPolyData() method can be used to get
 * the polygonal representation and can be used for things like seeding
 * stream lines. GetPlane() can be used to update a vtkPlane implicit
 * function. Typical usage of the widget is to make use of the
 * StartInteractionEvent, InteractionEvent, and EndInteractionEvent
 * events. The InteractionEvent is called on mouse motion; the other two
 * events are called on button down and button up (either left or right
 * button).
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You can set the properties of the selected and
 * unselected representations of the plane. For example, you can set the
 * property for the handles and plane. In addition there are methods to
 * constrain the plane so that it is perpendicular to the x-y-z axes.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkLineWidget vtkSphereWidget
 * vtkImplicitPlaneWidget
 */

#ifndef vtkPlaneWidget_h
#define vtkPlaneWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataSourceWidget.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCellPicker;
class vtkConeSource;
class vtkLineSource;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;
class vtkPlane;

#define VTK_PLANE_OFF 0
#define VTK_PLANE_OUTLINE 1
#define VTK_PLANE_WIREFRAME 2
#define VTK_PLANE_SURFACE 3

#define VTK_PLANE_ZERO_THRESHOLD (std::numeric_limits<double>::min() * 1000)

class VTKINTERACTIONWIDGETS_EXPORT vtkPlaneWidget : public vtkPolyDataSourceWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkPlaneWidget* New();

  vtkTypeMacro(vtkPlaneWidget, vtkPolyDataSourceWidget);
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
   * Set/Get the resolution (number of subdivisions) of the plane.
   */
  void SetResolution(int r);
  int GetResolution();
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
   * Set/Get the position of the point defining the first axis of the plane.
   */
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double x[3]);
  double* GetPoint1() VTK_SIZEHINT(3);
  void GetPoint1(double xyz[3]);
  ///@}

  ///@{
  /**
   * Set/Get the position of the point defining the second axis of the plane.
   */
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double x[3]);
  double* GetPoint2() VTK_SIZEHINT(3);
  void GetPoint2(double xyz[3]);
  ///@}

  ///@{
  /**
   * Get the center of the plane.
   */
  void SetCenter(double x, double y, double z);
  void SetCenter(double x[3]);
  double* GetCenter() VTK_SIZEHINT(3);
  void GetCenter(double xyz[3]);
  ///@}

  ///@{
  /**
   * Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  double* GetNormal() VTK_SIZEHINT(3);
  void GetNormal(double xyz[3]);
  ///@}

  ///@{
  /**
   * Control how the plane appears when GetPolyData() is invoked.
   * If the mode is "outline", then just the outline of the plane
   * is shown. If the mode is "wireframe" then the plane is drawn
   * with the outline plus the interior mesh (corresponding to the
   * resolution specified). If the mode is "surface" then the plane
   * is drawn as a surface.
   */
  vtkSetClampMacro(Representation, int, VTK_PLANE_OFF, VTK_PLANE_SURFACE);
  vtkGetMacro(Representation, int);
  void SetRepresentationToOff() { this->SetRepresentation(VTK_PLANE_OFF); }
  void SetRepresentationToOutline() { this->SetRepresentation(VTK_PLANE_OUTLINE); }
  void SetRepresentationToWireframe() { this->SetRepresentation(VTK_PLANE_WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(VTK_PLANE_SURFACE); }
  ///@}

  ///@{
  /**
   * Force the plane widget to be aligned with one of the x-y-z axes.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the plane to the axes if it is originally
   * not aligned.
   */
  vtkSetMacro(NormalToXAxis, vtkTypeBool);
  vtkGetMacro(NormalToXAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToXAxis, vtkTypeBool);
  vtkSetMacro(NormalToYAxis, vtkTypeBool);
  vtkGetMacro(NormalToYAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToYAxis, vtkTypeBool);
  vtkSetMacro(NormalToZAxis, vtkTypeBool);
  vtkGetMacro(NormalToZAxis, vtkTypeBool);
  vtkBooleanMacro(NormalToZAxis, vtkTypeBool);
  ///@}

  /**
   * Grab the polydata (including points) that defines the plane.  The
   * polydata consists of (res+1)*(res+1) points, and res*res quadrilateral
   * polygons, where res is the resolution of the plane. These point values
   * are guaranteed to be up-to-date when either the InteractionEvent or
   * EndInteraction events are invoked. The user provides the vtkPolyData and
   * the points and polyplane are added to it.
   */
  void GetPolyData(vtkPolyData* pd);

  /**
   * Get the planes describing the implicit function defined by the plane
   * widget. The user must provide the instance of the class vtkPlane. Note
   * that vtkPlane is a subclass of vtkImplicitFunction, meaning that it can
   * be used by a variety of filters to perform clipping, cutting, and
   * selection of data.
   */
  void GetPlane(vtkPlane* plane);

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * PolyData.  Make changes to this before calling the initial PlaceWidget()
   * to have the initial placement follow suit.  Or, make changes after the
   * widget has been initialised and call UpdatePlacement() to realise.
   */
  vtkPolyDataAlgorithm* GetPolyDataAlgorithm() override;

  /**
   * Satisfies superclass API.  This will change the state of the widget to
   * match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement() override;

  ///@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles when selected and normal can be
   * manipulated.
   */
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the plane properties. The properties of the plane when selected
   * and unselected can be manipulated.
   */
  virtual void SetPlaneProperty(vtkProperty*);
  vtkGetObjectMacro(PlaneProperty, vtkProperty);
  vtkGetObjectMacro(SelectedPlaneProperty, vtkProperty);
  ///@}

protected:
  vtkPlaneWidget();
  ~vtkPlaneWidget() override;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    Moving,
    Scaling,
    Pushing,
    Rotating,
    Spinning,
    Outside,
    Pinching
  };

  // handles the events
  static void ProcessEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();
  void OnStartPinch();
  void OnPinch();
  void OnEndPinch();

  // controlling ivars
  vtkTypeBool NormalToXAxis;
  vtkTypeBool NormalToYAxis;
  vtkTypeBool NormalToZAxis;
  int Representation;
  void SelectRepresentation();

  // the plane
  vtkActor* PlaneActor;
  vtkPolyDataMapper* PlaneMapper;
  vtkPlaneSource* PlaneSource;
  vtkPolyData* PlaneOutline;
  void HighlightPlane(int highlight);

  // glyphs representing hot spots (e.g., handles)
  vtkActor** Handle;
  vtkPolyDataMapper** HandleMapper;
  vtkSphereSource** HandleGeometry;
  void PositionHandles();
  void HandlesOn(double length);
  void HandlesOff();
  int HighlightHandle(vtkProp* prop); // returns cell id
  void SizeHandles() override;

  // the normal cone
  vtkActor* ConeActor;
  vtkPolyDataMapper* ConeMapper;
  vtkConeSource* ConeSource;
  void HighlightNormal(int highlight);

  // the normal line
  vtkActor* LineActor;
  vtkPolyDataMapper* LineMapper;
  vtkLineSource* LineSource;

  // the normal cone
  vtkActor* ConeActor2;
  vtkPolyDataMapper* ConeMapper2;
  vtkConeSource* ConeSource2;

  // the normal line
  vtkActor* LineActor2;
  vtkPolyDataMapper* LineMapper2;
  vtkLineSource* LineSource2;

  // Do the picking
  vtkCellPicker* HandlePicker;
  vtkCellPicker* PlanePicker;
  vtkActor* CurrentHandle;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Methods to manipulate the hexahedron.
  void MoveOrigin(double* p1, double* p2);
  void MovePoint1(double* p1, double* p2);
  void MovePoint2(double* p1, double* p2);
  void MovePoint3(double* p1, double* p2);
  void Rotate(int X, int Y, double* p1, double* p2, double* vpn);
  void Spin(double* p1, double* p2);
  void Scale(double* p1, double* p2, int X, int Y);
  void Translate(double* p1, double* p2);
  void Push(double* p1, double* p2);

  // Plane normal, normalized
  double Normal[3];

  // Transform the hexahedral points (used for rotations)
  vtkTransform* Transform;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty* HandleProperty;
  vtkProperty* SelectedHandleProperty;
  vtkProperty* PlaneProperty;
  vtkProperty* SelectedPlaneProperty;
  void CreateDefaultProperties();

  void GeneratePlane();

  int LastPickValid;
  double HandleSizeFactor;

private:
  vtkPlaneWidget(const vtkPlaneWidget&) = delete;
  void operator=(const vtkPlaneWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
