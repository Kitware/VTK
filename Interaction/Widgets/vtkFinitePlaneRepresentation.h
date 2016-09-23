/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFinitePlaneRepresentation.h

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkWidgetRepresentation.h"

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

class VTKINTERACTIONWIDGETS_EXPORT vtkFinitePlaneRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkFinitePlaneRepresentation *New();

  //@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkFinitePlaneRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon.
   */
  void GetPolyData(vtkPolyData *pd);

  //@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles, when selected or normal, can be
   * specified.
   */
  vtkGetObjectMacro(V1HandleProperty, vtkProperty);
  vtkGetObjectMacro(V2HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  //@}

  //@{
  /**
   * Get the plane properties. The
   * properties of the plane when selected and normal can be
   * set.
   */
  vtkGetObjectMacro(PlaneProperty, vtkProperty);
  vtkGetObjectMacro(SelectedPlaneProperty, vtkProperty);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a vtkTubeFilter.
   */
  vtkSetMacro(Tubing, bool);
  vtkGetMacro(Tubing, bool);
  vtkBooleanMacro(Tubing, bool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(bool plane);
  vtkGetMacro(DrawPlane, bool);
  vtkBooleanMacro(DrawPlane, bool);
  //@}

  //@{
  /**
   * Switches handles (the spheres) on or off by manipulating the underlying
   * actor visibility.
   */
  void SetHandles(bool handles);
  virtual void HandlesOn();
  virtual void HandlesOff();
  //@}

  //@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual int  ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();
  //@}

  //@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int  RenderOpaqueGeometry(vtkViewport*);
  virtual int  RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int  HasTranslucentPolygonalGeometry();
  //@}

  vtkSetClampMacro(InteractionState, int, Outside, Pushing);

  //@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  vtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set/Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  vtkGetVector3Macro(Normal, double);
  //@}

  //@{
  /**
   * Set/Get the v1 vector of the plane.
   */
  void SetV1(double x, double y);
  void SetV1(double x[2]);
  vtkGetVector2Macro(V1, double);
  //@}

  //@{
  /**
   * Set/Get the v2 vector of the plane.
   */
  void SetV2(double x, double y);
  void SetV2(double x[2]);
  vtkGetVector2Macro(V2, double);
  //@}

  //@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  //@}

  //@{
  /**
   * Get the properties on the normal (line and cone).
   */
  vtkGetObjectMacro(NormalProperty, vtkProperty);
  vtkGetObjectMacro(SelectedNormalProperty, vtkProperty);
  //@}

  // Methods to manipulate the plane
  void TranslateOrigin(double *p1, double *p2);
  void MovePoint1(double *p1, double *p2);
  void MovePoint2(double *p1, double *p2);
  void Push(double *p1, double *p2);
  void Rotate(int X, int Y, double *p1, double *p2, double *vpn);

  enum _InteractionState
  {
    Outside = 0,
    MoveOrigin,
    ModifyV1,
    ModifyV2,
    Moving,
    Rotating,
    Pushing
  };

protected:
  vtkFinitePlaneRepresentation();
  ~vtkFinitePlaneRepresentation();

  virtual void CreateDefaultProperties();

  // Size the glyphs representing hot spots (e.g., handles)
  virtual void SizeHandles();

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  void SetHighlightNormal(int highlight);
  void SetHighlightPlane(int highlight);
  void SetHighlightHandle(vtkProp *prop);

  double LastEventPosition[3];

  // the representation state
  int RepresentationState;

  // the origin
  vtkSphereSource   *OriginGeometry;
  vtkPolyDataMapper *OriginMapper;
  vtkActor          *OriginActor;
  double            Origin[3];

  // the normal
  double Normal[3];

  // the previous normal
  double PreviousNormal[3];

  // the rotation transform
  vtkTransform *Transform;

  // the X Vector
  vtkSphereSource   *V1Geometry;
  vtkPolyDataMapper *V1Mapper;
  vtkActor          *V1Actor;
  double            V1[3];

  // the Y Vector
  vtkSphereSource   *V2Geometry;
  vtkPolyDataMapper *V2Mapper;
  vtkActor          *V2Actor;
  double            V2[3];

  // The + normal cone
  vtkConeSource     *ConeSource;
  vtkPolyDataMapper *ConeMapper;
  vtkActor          *ConeActor;

  // The + normal line
  vtkLineSource     *LineSource;
  vtkPolyDataMapper *LineMapper;
  vtkActor          *LineActor;

  // The - normal cone
  vtkConeSource     *ConeSource2;
  vtkPolyDataMapper *ConeMapper2;
  vtkActor          *ConeActor2;

  // The - normal line
  vtkLineSource     *LineSource2;
  vtkPolyDataMapper *LineMapper2;
  vtkActor          *LineActor2;

  // The finite plane
  vtkPolyData       *PlanePolyData;
  vtkPolyDataMapper *PlaneMapper;
  vtkActor          *PlaneActor;

  // Optional tubes are represented by extracting boundary edges
  vtkFeatureEdges   *Edges;
  vtkTubeFilter     *EdgesTuber;
  vtkPolyDataMapper *EdgesMapper;
  vtkActor          *EdgesActor;
  bool              Tubing; //control whether tubing is on
  bool              DrawPlane; //control whether plane is on

  // Picking objects
  vtkCellPicker *HandlePicker;
  vtkActor      *CurrentHandle;

  // Transform the planes (used for rotations)
  vtkTransform *TransformRotation;

  // Support GetBounds() method
  vtkBox *BoundingBox;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *OriginHandleProperty;
  vtkProperty *V1HandleProperty;
  vtkProperty *V2HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *PlaneProperty;
  vtkProperty *SelectedPlaneProperty;
  vtkProperty *NormalProperty;
  vtkProperty *SelectedNormalProperty;

private:
  vtkFinitePlaneRepresentation(const vtkFinitePlaneRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFinitePlaneRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
