/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrokenLineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBrokenLineWidget
 * @brief   3D widget for manipulating a broken line
 *
 * This 3D widget defines a broken line that can be interactively placed in a
 * scene. The broken line has handles, the number of which can be changed, plus it
 * can be picked on the broken line itself to translate or rotate it in the scene.
 * A nice feature of the object is that the vtkBrokenLineWidget, like any 3D
 * widget, will work with the current interactor style. That is, if
 * vtkBrokenLineWidget does not handle an event, then all other registered
 * observers (including the interactor style) have an opportunity to process
 * the event. Otherwise, the vtkBrokenLineWidget will terminate the processing of
 * the event that it handles.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a vtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. The interactor will act
 * normally until the "i" key (for "interactor") is pressed, at which point the
 * vtkBrokenLineWidget will appear. (See superclass documentation for information
 * about changing this behavior.) Events that occur outside of the widget
 * (i.e., no part of the widget is picked) are propagated to any other
 * registered obsevers (such as the interaction style).  Turn off the widget
 * by pressing the "i" key again (or invoke the Off() method).
 *
 * The button actions and key modifiers are as follows for controlling the
 * widget:
 * 1) left button down on and drag one of the spherical handles to change the
 * shape of the broken line: the handles act as "control points".
 * 2) left button or middle button down on a line segment forming the broken line
 * allows uniform translation of the widget.
 * 3) ctrl + middle button down on the widget enables spinning of the widget
 * about its center.
 * 4) right button down on the widget enables scaling of the widget. By moving
 * the mouse "up" the render window the broken line will be made bigger; by moving
 * "down" the render window the widget will be made smaller.
 * 5) ctrl key + right button down on any handle will erase it providing there
 * will be two or more points remaining to form a broken line.
 * 6) shift key + right button down on any line segment will insert a handle
 * onto the broken line at the cursor position.
 *
 * The vtkBrokenLineWidget has several methods that can be used in conjunction with
 * other VTK objects. The GetPolyData() method can be used to get the
 * polygonal representation and can be used for things like seeding
 * streamlines or probing other data sets. Typical usage of the widget is to
 * make use of the StartInteractionEvent, InteractionEvent, and
 * EndInteractionEvent events. The InteractionEvent is called on mouse motion;
 * the other two events are called on button down and button up (either left or
 * right button).
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You can set the properties of the selected and
 * unselected representations of the broken line. For example, you can set the
 * property for the handles and broken line. In addition there are methods to
 * constrain the broken line so that it is aligned with a plane.  Note that a simple
 * ruler widget can be derived by setting the resolution to 1, the number of
 * handles to 2, and calling the GetSummedLength method!
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012
 * This work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkLineWidget vtkPointWidget vtkSphereWidget
 * vtkImagePlaneWidget vtkImplicitPlaneWidget vtkPlaneWidget
*/

#ifndef vtkBrokenLineWidget_h
#define vtkBrokenLineWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtk3DWidget.h"

class vtkActor;
class vtkCellPicker;
class vtkLineSource;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;

#define VTK_PROJECTION_YZ 0
#define VTK_PROJECTION_XZ 1
#define VTK_PROJECTION_XY 2
#define VTK_PROJECTION_OBLIQUE 3

class VTKINTERACTIONWIDGETS_EXPORT vtkBrokenLineWidget : public vtk3DWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkBrokenLineWidget *New();

  vtkTypeMacro(vtkBrokenLineWidget,vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Methods that satisfy the superclass' API.
   */
  virtual void SetEnabled(int);
  virtual void PlaceWidget(double bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}
  //@}

  //@{
  /**
   * Force the broken line widget to be projected onto one of the orthogonal planes.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the broken line to the plane if it is originally
   * not aligned.  The normal in SetProjectionNormal is 0,1,2 for YZ,XZ,XY
   * planes respectively and 3 for arbitrary oblique planes when the widget
   * is tied to a vtkPlaneSource.
   */
  vtkSetMacro(ProjectToPlane,int);
  vtkGetMacro(ProjectToPlane,int);
  vtkBooleanMacro(ProjectToPlane,int);
  //@}

  /**
   * Set up a reference to a vtkPlaneSource that could be from another widget
   * object, e.g. a vtkPolyDataSourceWidget.
   */
  void SetPlaneSource(vtkPlaneSource* plane);

  vtkSetClampMacro(ProjectionNormal,int,VTK_PROJECTION_YZ,VTK_PROJECTION_OBLIQUE);
  vtkGetMacro(ProjectionNormal,int);
  void SetProjectionNormalToXAxes()
    { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes()
    { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes()
    { this->SetProjectionNormal(2); }
  void SetProjectionNormalToOblique()
    { this->SetProjectionNormal(3); }

  //@{
  /**
   * Set the position of broken line handles and points in terms of a plane's
   * position. i.e., if ProjectionNormal is 0, all of the x-coordinate
   * values of the points are set to position. Any value can be passed (and is
   * ignored) to update the broken line points when Projection normal is set to 3
   * for arbritrary plane orientations.
   */
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);
  //@}

  /**
   * Grab the polydata (including points) that defines the broken line.  The
   * polydata consists of points and line segments numbering nHandles
   * and nHandles - 1, respectively. Points are guaranteed to be up-to-date when
   * either the InteractionEvent or  EndInteraction events are invoked. The
   * user provides the vtkPolyData and the points and polyline are added to it.
   */
  void GetPolyData(vtkPolyData *pd);

  //@{
  /**
   * Set/Get the handle properties (the spheres are the handles). The
   * properties of the handles when selected and unselected can be manipulated.
   */
  virtual void SetHandleProperty(vtkProperty*);
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  virtual void SetSelectedHandleProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  //@}

  //@{
  /**
   * Set/Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  virtual void SetLineProperty(vtkProperty*);
  vtkGetObjectMacro(LineProperty, vtkProperty);
  virtual void SetSelectedLineProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);
  //@}

  //@{
  /**
   * Set/Get the number of handles for this widget.
   */
  virtual void SetNumberOfHandles(int npts);
  vtkGetMacro(NumberOfHandles, int);
  //@}

  //@{
  /**
   * Set/Get the position of the broken line handles. Call GetNumberOfHandles
   * to determine the valid range of handle indices.
   */
  void SetHandlePosition(int handle, double x, double y, double z);
  void SetHandlePosition(int handle, double xyz[3]);
  void GetHandlePosition(int handle, double xyz[3]);
  double* GetHandlePosition(int handle);
  //@}

  /**
   * Get the summed lengths of the individual straight line segments.
   */
  double GetSummedLength();

  /**
   * Convenience method to allocate and set the handles from a vtkPoints
   * instance.
   */
  void InitializeHandles(vtkPoints* points);

  //@{
  /**
   * Turn on / off event processing for this widget. If off, the widget will
   * not respond to user interaction
   */
  vtkSetClampMacro(ProcessEvents, int, 0, 1);
  vtkGetMacro(ProcessEvents, int);
  vtkBooleanMacro( ProcessEvents, int );
  //@}

  //@{
  /**
   * Set/Get the size factor to be applied to the handle radii.
   * Default: 1.
   */
  vtkSetClampMacro(HandleSizeFactor, double, 0., 100.);
  vtkGetMacro(HandleSizeFactor, double);
  //@}

protected:
  vtkBrokenLineWidget();
  ~vtkBrokenLineWidget();

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Moving,
    Scaling,
    Spinning,
    Inserting,
    Erasing,
    Outside
  };

  //handles the events
  static void ProcessEventsHandler(vtkObject* object,
                                   unsigned long event,
                                   void* clientdata,
                                   void* calldata);

  // ProcessEventsHandler() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // Controlling vars
  int   ProjectionNormal;
  double ProjectionPosition;
  int   ProjectToPlane;
  vtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  // The broken line
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource     *LineSource;
  void HighlightLine(int highlight);
  int NumberOfHandles;
  void BuildRepresentation();

  // Glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkSphereSource   **HandleGeometry;
  void Initialize();
  int  HighlightHandle(vtkProp *prop); //returns handle index or -1 on fail
  virtual void SizeHandles();
  void InsertHandleOnLine(double* pos);
  void EraseHandle(const int&);

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkActor *CurrentHandle;
  int CurrentHandleIndex;

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  // Methods to manipulate the broken line.
  void MovePoint(double *p1, double *p2);
  void Scale(double *p1, double *p2, int X, int Y);
  void Translate(double *p1, double *p2);
  void Spin(double *p1, double *p2, double *vpn);

  // Transform the control points (used for spinning)
  vtkTransform *Transform;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void CreateDefaultProperties();

  // For efficient spinning
  double Centroid[3];
  void CalculateCentroid();
  int  ProcessEvents;

  // Handle sizing factor
  double HandleSizeFactor;

private:
  vtkBrokenLineWidget(const vtkBrokenLineWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBrokenLineWidget&) VTK_DELETE_FUNCTION;
};

#endif
