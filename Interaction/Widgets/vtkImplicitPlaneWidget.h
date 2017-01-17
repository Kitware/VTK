/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitPlaneWidget
 * @brief   3D widget for manipulating an infinite plane
 *
 * This 3D widget defines an infinite plane that can be interactively placed
 * in a scene. The widget is represented by a plane with a normal vector; the
 * plane is contained by a bounding box, and where the plane intersects the
 * bounding box the edges are shown (possibly tubed). The normal can be
 * selected and moved to rotate the plane; the plane itself can be selected
 * and translated in various directions. As the plane is moved, the implicit
 * plane function and polygon (representing the plane cut against the bounding
 * box) is updated.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a vtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. If the "i" key (for
 * "interactor") is pressed, the vtkImplicitPlaneWidget will appear. (See
 * superclass documentation for information about changing this behavior.)
 * If you select the normal vector, the plane can be arbitrarily rotated. The
 * plane can be translated along the normal by selecting the plane and moving
 * it. The plane (the plane origin) can also be arbitrary moved by selecting
 * the plane with the middle mouse button. The right mouse button can be used
 * to uniformly scale the bounding box (moving "up" the box scales larger;
 * moving "down" the box scales smaller). Events that occur outside of the
 * widget (i.e., no part of the widget is picked) are propagated to any other
 * registered obsevers (such as the interaction style).  Turn off the widget
 * by pressing the "i" key again (or invoke the Off() method).
 *
 * The vtkImplicitPlaneWidget has several methods that can be used in
 * conjunction with other VTK objects.  The GetPolyData() method can be used
 * to get a polygonal representation (the single polygon clipped by the
 * bounding box).  Typical usage of the widget is to make use of the
 * StartInteractionEvent, InteractionEvent, and EndInteractionEvent
 * events. The InteractionEvent is called on mouse motion; the other two
 * events are called on button down and button up (either left or right
 * button). (Note: there is also a PlaceWidgetEvent that is invoked when
 * the widget is placed with PlaceWidget().)
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You do this by setting property values on the
 * normal vector (selected and unselected properties); the plane (selected
 * and unselected properties); the outline (selected and unselected
 * properties); and the edges. The edges may also be tubed or not.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkPlaneWidget vtkLineWidget vtkPointWidget
 * vtkSphereWidget vtkImagePlaneWidget
*/

#ifndef vtkImplicitPlaneWidget_h
#define vtkImplicitPlaneWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataSourceWidget.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;
class vtkConeSource;
class vtkLineSource;
class vtkSphereSource;
class vtkTubeFilter;
class vtkPlane;
class vtkCutter;
class vtkProperty;
class vtkImageData;
class vtkOutlineFilter;
class vtkFeatureEdges;
class vtkPolyData;
class vtkTransform;

class VTKINTERACTIONWIDGETS_EXPORT vtkImplicitPlaneWidget : public vtkPolyDataSourceWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkImplicitPlaneWidget *New();

  vtkTypeMacro(vtkImplicitPlaneWidget,vtkPolyDataSourceWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Methods that satisfy the superclass' API.
   */
  void SetEnabled(int) VTK_OVERRIDE;
  void PlaceWidget(double bounds[6]) VTK_OVERRIDE;
  void PlaceWidget() VTK_OVERRIDE
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax) VTK_OVERRIDE
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}
  //@}

  //@{
  /**
   * Get the origin of the plane.
   */
  virtual void SetOrigin(double x, double y, double z);
  virtual void SetOrigin(double x[3]);
  double* GetOrigin();
  void GetOrigin(double xyz[3]);
  //@}

  //@{
  /**
   * Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  double* GetNormal();
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
  void SetNormalToXAxis(int);
  vtkGetMacro(NormalToXAxis,int);
  vtkBooleanMacro(NormalToXAxis,int);
  void SetNormalToYAxis(int);
  vtkGetMacro(NormalToYAxis,int);
  vtkBooleanMacro(NormalToYAxis,int);
  void SetNormalToZAxis(int);
  vtkGetMacro(NormalToZAxis,int);
  vtkBooleanMacro(NormalToZAxis,int);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a vtkTubeFilter.
   */
  vtkSetMacro(Tubing,int);
  vtkGetMacro(Tubing,int);
  vtkBooleanMacro(Tubing,int);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(int plane);
  vtkGetMacro(DrawPlane,int);
  vtkBooleanMacro(DrawPlane,int);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the bounding box by grabbing it
   * with the left mouse button.
   */
  vtkSetMacro(OutlineTranslation,int);
  vtkGetMacro(OutlineTranslation,int);
  vtkBooleanMacro(OutlineTranslation,int);
  //@}

  //@{
  /**
   * Turn on/off the ability to move the widget outside of the input's bound
   */
  vtkSetMacro(OutsideBounds,int);
  vtkGetMacro(OutsideBounds,int);
  vtkBooleanMacro(OutsideBounds,int);
  //@}

  //@{
  /**
   * Turn on/off the ability to scale with the mouse
   */
  vtkSetMacro(ScaleEnabled,int);
  vtkGetMacro(ScaleEnabled,int);
  vtkBooleanMacro(ScaleEnabled,int);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the origin (sphere)
   * with the left mouse button.
   */
  vtkSetMacro(OriginTranslation,int);
  vtkGetMacro(OriginTranslation,int);
  vtkBooleanMacro(OriginTranslation,int);
  //@}

  //@{
  /**
   * By default the arrow is 30% of the diagonal length. DiagonalRatio control
   * this ratio in the interval [0-2]
   */
  vtkSetClampMacro(DiagonalRatio,double,0,2);
  vtkGetMacro(DiagonalRatio,double);
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
  vtkPolyDataAlgorithm* GetPolyDataAlgorithm() VTK_OVERRIDE;

  /**
   * Get the implicit function for the plane. The user must provide the
   * instance of the class vtkPlane. Note that vtkPlane is a subclass of
   * vtkImplicitFunction, meaning that it can be used by a variety of filters
   * to perform clipping, cutting, and selection of data.
   */
  void GetPlane(vtkPlane *plane);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement() VTK_OVERRIDE;

  /**
   * Control widget appearance
   */
  void SizeHandles() VTK_OVERRIDE;

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

protected:
  vtkImplicitPlaneWidget();
  ~vtkImplicitPlaneWidget() VTK_OVERRIDE;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    MovingPlane,
    MovingOutline,
    MovingOrigin,
    Scaling,
    Pushing,
    Rotating,
    Outside
  };

  //handles the events
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // Controlling ivars
  int NormalToXAxis;
  int NormalToYAxis;
  int NormalToZAxis;
  void UpdateRepresentation();

  // The actual plane which is being manipulated
  vtkPlane *Plane;

  // The bounding box is represented by a single voxel image data
  vtkImageData      *Box;
  vtkOutlineFilter  *Outline;
  vtkPolyDataMapper *OutlineMapper;
  vtkActor          *OutlineActor;
  void HighlightOutline(int highlight);
  int OutlineTranslation; //whether the outline can be moved
  int ScaleEnabled; //whether the widget can be scaled
  int OutsideBounds; //whether the widget can be moved outside input's bounds

  // The cut plane is produced with a vtkCutter
  vtkCutter         *Cutter;
  vtkPolyDataMapper *CutMapper;
  vtkActor          *CutActor;
  int               DrawPlane;
  virtual void HighlightPlane(int highlight);

  // Optional tubes are represented by extracting boundary edges and tubing
  vtkFeatureEdges   *Edges;
  vtkTubeFilter     *EdgesTuber;
  vtkPolyDataMapper *EdgesMapper;
  vtkActor          *EdgesActor;
  int               Tubing; //control whether tubing is on

  // Control final length of the arrow:
  double DiagonalRatio;

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
  int OriginTranslation; //whether the origin (sphere) can be moved

  // Do the picking
  vtkCellPicker *Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  // Transform the normal (used for rotation)
  vtkTransform *Transform;

  // Methods to manipulate the plane
  void ConstrainOrigin(double x[3]);
  void Rotate(int X, int Y, double *p1, double *p2, double *vpn);
  void TranslatePlane(double *p1, double *p2);
  void TranslateOutline(double *p1, double *p2);
  void TranslateOrigin(double *p1, double *p2);
  void Push(double *p1, double *p2);
  void Scale(double *p1, double *p2, int X, int Y);

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

private:
  vtkImplicitPlaneWidget(const vtkImplicitPlaneWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImplicitPlaneWidget&) VTK_DELETE_FUNCTION;
};

#endif
