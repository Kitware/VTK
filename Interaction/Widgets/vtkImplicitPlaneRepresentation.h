/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitPlaneRepresentation
 * @brief   a class defining the representation for a vtkImplicitPlaneWidget2
 *
 * This class is a concrete representation for the
 * vtkImplicitPlaneWidget2. It represents an infinite plane defined by a
 * normal and point in the context of a bounding box. Through interaction
 * with the widget, the plane can be manipulated by adjusting the plane
 * normal or moving the origin point.
 *
 * To use this representation, you normally define a (plane) origin and (plane)
 * normal. The PlaceWidget() method is also used to initially position the
 * representation.
 *
 * @warning
 * This class, and vtkImplicitPlaneWidget2, are next generation VTK
 * widgets. An earlier version of this functionality was defined in the
 * class vtkImplicitPlaneWidget.
 *
 * @sa
 * vtkImplicitPlaneWidget2 vtkImplicitPlaneWidget
*/

#ifndef vtkImplicitPlaneRepresentation_h
#define vtkImplicitPlaneRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;
class vtkConeSource;
class vtkLineSource;
class vtkSphereSource;
class vtkTubeFilter;
class vtkPlane;
class vtkPlaneSource;
class vtkCutter;
class vtkProperty;
class vtkImageData;
class vtkOutlineFilter;
class vtkFeatureEdges;
class vtkPolyData;
class vtkPolyDataAlgorithm;
class vtkTransform;
class vtkBox;
class vtkLookupTable;

class VTKINTERACTIONWIDGETS_EXPORT vtkImplicitPlaneRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkImplicitPlaneRepresentation *New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkImplicitPlaneRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  double* GetOrigin() VTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  //@}

  //@{
  /**
   * Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  void SetNormalToCamera();
  double* GetNormal() VTK_SIZEHINT(3);
  void GetNormal(double xyz[3]);
  //@}

  //@{
  /**
   * Force the plane widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the plane to the axes if it is originally
   * not aligned.
   */
  void SetNormalToXAxis(vtkTypeBool);
  vtkGetMacro(NormalToXAxis,vtkTypeBool);
  vtkBooleanMacro(NormalToXAxis,vtkTypeBool);
  void SetNormalToYAxis(vtkTypeBool);
  vtkGetMacro(NormalToYAxis,vtkTypeBool);
  vtkBooleanMacro(NormalToYAxis,vtkTypeBool);
  void SetNormalToZAxis(vtkTypeBool);
  vtkGetMacro(NormalToZAxis,vtkTypeBool);
  vtkBooleanMacro(NormalToZAxis,vtkTypeBool);
  //@}

  //@{
  /**
   * If enabled, and a vtkCamera is available through the renderer, then
   * LockNormalToCamera will cause the normal to follow the camera's
   * normal.
   */
  virtual void SetLockNormalToCamera(vtkTypeBool);
  vtkGetMacro(LockNormalToCamera,vtkTypeBool);
  vtkBooleanMacro(LockNormalToCamera,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a vtkTubeFilter.
   */
  vtkSetMacro(Tubing,vtkTypeBool);
  vtkGetMacro(Tubing,vtkTypeBool);
  vtkBooleanMacro(Tubing,vtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(vtkTypeBool plane);
  vtkGetMacro(DrawPlane,vtkTypeBool);
  vtkBooleanMacro(DrawPlane,vtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the outline.
   */
  void SetDrawOutline(vtkTypeBool plane);
  vtkGetMacro(DrawOutline,vtkTypeBool);
  vtkBooleanMacro(DrawOutline,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the bounding box by grabbing it
   * with the left mouse button.
   */
  vtkSetMacro(OutlineTranslation,vtkTypeBool);
  vtkGetMacro(OutlineTranslation,vtkTypeBool);
  vtkBooleanMacro(OutlineTranslation,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to move the widget outside of the bounds
   * specified in the initial PlaceWidget() invocation.
   */
  vtkSetMacro(OutsideBounds,vtkTypeBool);
  vtkGetMacro(OutsideBounds,vtkTypeBool);
  vtkBooleanMacro(OutsideBounds,vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the bounds of the widget representation. PlaceWidget can also be
   * used to set the bounds of the widget but it may also have other effects
   * on the internal state of the representation. Use this function when only
   * the widget bounds are needs to be modified.
   */
  vtkSetVector6Macro(WidgetBounds, double);
  vtkGetVector6Macro(WidgetBounds, double);
  //@}

  //@{
  /**
   * Turn on/off whether the plane should be constrained to the widget bounds.
   * If on, the origin will not be allowed to move outside the set widget bounds.
   * This is the default behaviour.
   * If off, the origin can be freely moved and the widget outline will change
   * accordingly.
   */
  vtkSetMacro(ConstrainToWidgetBounds,vtkTypeBool);
  vtkGetMacro(ConstrainToWidgetBounds,vtkTypeBool);
  vtkBooleanMacro(ConstrainToWidgetBounds,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to scale the widget with the mouse.
   */
  vtkSetMacro(ScaleEnabled,vtkTypeBool);
  vtkGetMacro(ScaleEnabled,vtkTypeBool);
  vtkBooleanMacro(ScaleEnabled,vtkTypeBool);
  //@}

  /**
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon that is clipped by the bounding box.
   */
  void GetPolyData(vtkPolyData *pd);

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
  void GetPlane(vtkPlane *plane);

  /**
   * Alternative way to define the cutting plane. The normal and origin of
   * the plane provided is copied into the internal instance of the class
   * cutting vtkPlane.
   */
  void SetPlane(vtkPlane *plane);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement(void);

  //@{
  /**
   * Get the properties on the normal (line and cone).
   */
  vtkGetObjectMacro(NormalProperty,vtkProperty);
  vtkGetObjectMacro(SelectedNormalProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the plane properties. The properties of the plane when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(PlaneProperty,vtkProperty);
  vtkGetObjectMacro(SelectedPlaneProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the property of the outline.
   */
  vtkGetObjectMacro(OutlineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the property of the intersection edges. (This property also
   * applies to the edges when tubed.)
   */
  vtkGetObjectMacro(EdgesProperty,vtkProperty);
  //@}
  //@{
  /**
   * Set color to the edge
   */
  void SetEdgeColor(vtkLookupTable*);
  void SetEdgeColor(double, double, double);
  void SetEdgeColor(double x[3]);
  //@}

  //@{
  /**
   * Specify a translation distance used by the BumpPlane() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   */
  vtkSetClampMacro(BumpDistance,double,0.000001,1);
  vtkGetMacro(BumpDistance,double);
  //@}

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

  //@{
  /**
   * Methods to interface with the vtkImplicitPlaneWidget2.
   */
  int ComputeInteractionState(int X, int Y, int modify=0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  void StartComplexInteraction(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata) override;
  void ComplexInteraction(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata) override;
  int ComputeComplexInteractionState(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata, int modify = 0) override;
  void EndComplexInteraction(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double *GetBounds() VTK_SIZEHINT(6) override;
  void GetActors(vtkPropCollection *pc) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside=0,
    Moving,
    MovingOutline,
    MovingOrigin,
    Rotating,
    Pushing,
    Scaling
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkImplicitPlaneWidget2) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState,int,Outside,Scaling);
  //@}

  //@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  //@}

  // Get the underlying plane object used by this rep
  // this can be used as a cropping plane in vtkMapper
  vtkPlane *GetUnderlyingPlane() {
    return this->Plane; }

  //@{
  /**
   * Control if the plane should be drawn cropped by the bounding box
   * or without cropping. Defaults to on.
   */
  virtual void SetCropPlaneToBoundingBox(bool);
  vtkGetMacro(CropPlaneToBoundingBox,bool);
  vtkBooleanMacro(CropPlaneToBoundingBox,bool);
  //@}

  //@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  vtkGetMacro(SnapToAxes, bool);
  vtkSetMacro(SnapToAxes, bool);
  //@}

protected:
  vtkImplicitPlaneRepresentation();
  ~vtkImplicitPlaneRepresentation() override;

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

  // Locking normal to camera
  vtkTypeBool LockNormalToCamera;

  // Controlling the push operation
  double BumpDistance;

  // The actual plane which is being manipulated
  vtkPlane *Plane;

  // The bounding box is represented by a single voxel image data
  vtkImageData      *Box;
  vtkOutlineFilter  *Outline;
  vtkPolyDataMapper *OutlineMapper;
  vtkActor          *OutlineActor;
  void HighlightOutline(int highlight);
  vtkTypeBool  OutlineTranslation; //whether the outline can be moved
  vtkTypeBool  ScaleEnabled; //whether the widget can be scaled
  vtkTypeBool  OutsideBounds; //whether the widget can be moved outside input's bounds
  double WidgetBounds[6];
  vtkTypeBool ConstrainToWidgetBounds;

  // The cut plane is produced with a vtkCutter
  vtkCutter         *Cutter;
  vtkPlaneSource    *PlaneSource;
  vtkPolyDataMapper *CutMapper;
  vtkActor          *CutActor;
  vtkTypeBool                DrawPlane;
  vtkTypeBool                DrawOutline;
  void HighlightPlane(int highlight);

  // Optional tubes are represented by extracting boundary edges and tubing
  vtkFeatureEdges   *Edges;
  vtkTubeFilter     *EdgesTuber;
  vtkPolyDataMapper *EdgesMapper;
  vtkActor          *EdgesActor;
  vtkTypeBool                Tubing; //control whether tubing is on

  // The + normal cone
  vtkConeSource     *ConeSource;
  vtkPolyDataMapper *ConeMapper;
  vtkActor          *ConeActor;
  void HighlightNormal(int highlight);

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

  // The origin positioning handle
  vtkSphereSource   *Sphere;
  vtkPolyDataMapper *SphereMapper;
  vtkActor          *SphereActor;

  // Do the picking
  vtkCellPicker *Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Transform the normal (used for rotation)
  vtkTransform *Transform;

  // Methods to manipulate the plane
  void Rotate(double X, double Y, double *p1, double *p2, double *vpn);
  void Rotate3D(double *p1, double *p2);
  void TranslatePlane(double *p1, double *p2);
  void TranslateOutline(double *p1, double *p2);
  void TranslateOrigin(double *p1, double *p2);
  void UpdatePose(double *p1, double *d1, double *p2, double *d2);
  void Push(double *p1, double *p2);
  void Scale(double *p1, double *p2, double X, double Y);
  void SizeHandles();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *NormalProperty;
  vtkProperty *SelectedNormalProperty;
  vtkProperty *PlaneProperty;
  vtkProperty *SelectedPlaneProperty;
  vtkProperty *OutlineProperty;
  vtkProperty *SelectedOutlineProperty;
  vtkProperty *EdgesProperty;
  void CreateDefaultProperties();

  void GeneratePlane();

  bool CropPlaneToBoundingBox;

  // Support GetBounds() method
  vtkBox *BoundingBox;

private:
  vtkImplicitPlaneRepresentation(const vtkImplicitPlaneRepresentation&) = delete;
  void operator=(const vtkImplicitPlaneRepresentation&) = delete;
};

#endif
