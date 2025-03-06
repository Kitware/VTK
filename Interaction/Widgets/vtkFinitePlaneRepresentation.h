// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFinitePlaneRepresentation
 * @brief   represent the vtkFinitePlaneWidget.
 *
 * This class is a concrete representation for the vtkFinitePlaneWidget. It
 * represents a plane with three handles: one on two faces, plus a
 * center handle. Through interaction with the widget, the plane
 * representation can be arbitrarily positioned and modified in the 3D space.
 *
 * To use this representation, you normally use the PlaceWidget() method
 * to position the widget at a specified region in space.
 *
 * @sa
 * vtkFinitePlaneWidget vtkImplicitPlaneWidget2
 */

#ifndef vtkFinitePlaneRepresentation_h
#define vtkFinitePlaneRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For member variable
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkCellPicker;
class vtkConeSource;
class vtkFeatureEdges;
class vtkLineSource;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;
class vtkTubeFilter;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkFinitePlaneRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkFinitePlaneRepresentation* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkFinitePlaneRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon.
   */
  void GetPolyData(vtkPolyData* pd);

  ///@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles, when selected or normal, can be
   * specified.
   */
  vtkGetObjectMacro(V1HandleProperty, vtkProperty);
  vtkGetObjectMacro(V2HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the plane properties. The
   * properties of the plane when selected and normal can be
   * set.
   */
  vtkGetObjectMacro(PlaneProperty, vtkProperty);
  vtkGetObjectMacro(SelectedPlaneProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a vtkTubeFilter.
   */
  vtkSetMacro(Tubing, bool);
  vtkGetMacro(Tubing, bool);
  vtkBooleanMacro(Tubing, bool);
  ///@}

  ///@{
  /**
   * Turn on/off enforcing a rectangular shape when moving the vectors
   * v1, and v2. Off by default.
   */
  vtkSetMacro(RectangularShape, bool);
  vtkGetMacro(RectangularShape, bool);
  vtkBooleanMacro(RectangularShape, bool);
  ///@}

  ///@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(bool plane);
  vtkGetMacro(DrawPlane, bool);
  vtkBooleanMacro(DrawPlane, bool);
  ///@}

  ///@{
  /**
   * Switches handles (the spheres) on or off by manipulating the underlying
   * actor visibility.
   */
  void SetHandles(bool handles);
  virtual void HandlesOn();
  virtual void HandlesOff();
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
  double* GetBounds() override;
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

  vtkSetClampMacro(InteractionState, int, Outside, Pushing);

  ///@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Set/Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double n[3]);
  vtkGetVector3Macro(Normal, double);
  ///@}

  ///@{
  /**
   * Set/Get the v1 vector of the plane.
   */
  void SetV1(double x, double y);
  void SetV1(double x[2]);
  vtkGetVector2Macro(V1, double);
  ///@}

  ///@{
  /**
   * Set/Get the v2 vector of the plane.
   */
  void SetV2(double x, double y);
  void SetV2(double x[2]);
  vtkGetVector2Macro(V2, double);
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
   * Get the properties on the normal (line and cone).
   */
  vtkGetObjectMacro(NormalProperty, vtkProperty);
  vtkGetObjectMacro(SelectedNormalProperty, vtkProperty);
  ///@}

  // Methods to manipulate the plane
  void TranslateOrigin(double* p1, double* p2);
  void MovePoint1(double* p1, double* p2);
  void MovePoint2(double* p1, double* p2);
  void Push(double* p1, double* p2);
  void Rotate(int X, int Y, double* p1, double* p2, double* vpn);

  enum InteractionStateType
  {
    Outside = 0,
    MoveOrigin,
    ModifyV1,
    ModifyV2,
    Moving,
    Rotating,
    Pushing
  };

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  vtkFinitePlaneRepresentation();
  ~vtkFinitePlaneRepresentation() override;

  virtual void CreateDefaultProperties();

  // Size the glyphs representing hot spots (e.g., handles)
  virtual void SizeHandles();

  void SetHighlightNormal(int highlight);
  void SetHighlightPlane(int highlight);
  void SetHighlightHandle(vtkProp* prop);

  double LastEventPosition[3];

  // the representation state
  int RepresentationState;

  // the origin
  vtkNew<vtkSphereSource> OriginGeometry;
  vtkNew<vtkPolyDataMapper> OriginMapper;
  vtkNew<vtkActor> OriginActor;
  double Origin[3];

  // the normal
  double Normal[3];

  // the previous normal
  double PreviousNormal[3];

  // the rotation transform
  vtkNew<vtkTransform> Transform;

  // the X Vector
  vtkNew<vtkSphereSource> V1Geometry;
  vtkNew<vtkPolyDataMapper> V1Mapper;
  vtkNew<vtkActor> V1Actor;
  double V1[3];

  // the Y Vector
  vtkNew<vtkSphereSource> V2Geometry;
  vtkNew<vtkPolyDataMapper> V2Mapper;
  vtkNew<vtkActor> V2Actor;
  double V2[3];

  // The + normal cone
  vtkNew<vtkConeSource> ConeSource;
  vtkNew<vtkPolyDataMapper> ConeMapper;
  vtkNew<vtkActor> ConeActor;

  // The + normal line
  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkPolyDataMapper> LineMapper;
  vtkNew<vtkActor> LineActor;

  // The - normal cone
  vtkNew<vtkConeSource> ConeSource2;
  vtkNew<vtkPolyDataMapper> ConeMapper2;
  vtkNew<vtkActor> ConeActor2;

  // The - normal line
  vtkNew<vtkLineSource> LineSource2;
  vtkNew<vtkPolyDataMapper> LineMapper2;
  vtkNew<vtkActor> LineActor2;

  // The finite plane
  vtkNew<vtkPolyData> PlanePolyData;
  vtkNew<vtkPolyDataMapper> PlaneMapper;
  vtkNew<vtkActor> PlaneActor;

  // Optional tubes are represented by extracting boundary edges
  vtkNew<vtkFeatureEdges> Edges;
  vtkNew<vtkTubeFilter> EdgesTuber;
  vtkNew<vtkPolyDataMapper> EdgesMapper;
  vtkNew<vtkActor> EdgesActor;
  bool Tubing = true;            // control whether tubing is on
  bool RectangularShape = false; // control whether rectangular shape is enforced
  bool DrawPlane = true;         // control whether plane is on

  // Picking objects
  vtkNew<vtkCellPicker> HandlePicker;
  vtkActor* CurrentHandle;

  // Transform the planes (used for rotations)
  vtkNew<vtkTransform> TransformRotation;

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> OriginHandleProperty;
  vtkNew<vtkProperty> V1HandleProperty;
  vtkNew<vtkProperty> V2HandleProperty;
  vtkNew<vtkProperty> SelectedHandleProperty;
  vtkNew<vtkProperty> PlaneProperty;
  vtkNew<vtkProperty> SelectedPlaneProperty;
  vtkNew<vtkProperty> NormalProperty;
  vtkNew<vtkProperty> SelectedNormalProperty;

private:
  vtkFinitePlaneRepresentation(const vtkFinitePlaneRepresentation&) = delete;
  void operator=(const vtkFinitePlaneRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
