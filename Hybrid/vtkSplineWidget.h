/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSplineWidget - 3D widget for manipulating a spline
// .SECTION Description
// This 3D widget defines a spline that can be interactively placed in a
// scene. The spline has handles, the number of which can be changed, plus it
// can be picked on the spline itself to translate or rotate it in the scene.
// A nice feature of the object is that the vtkSplineWidget, like any 3D
// widget, will work with the current interactor style. That is, if
// vtkSplineWidget does not handle an event, then all other registered
// observers (including the interactor style) have an opportunity to process
// the event. Otherwise, the vtkSplineWidget will terminate the processing of
// the event that it handles.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. The interactor will act
// normally until the "i" key (for "interactor") is pressed, at which point the
// vtkSplineWidget will appear. (See superclass documentation for information
// about changing this behavior.) By grabbing one of the spherical
// handles (use the left mouse button), the spline can be oriented and
// stretched (the other handles remain fixed). By grabbing the spline
// itself (left or middle mouse button), the entire spline can be translated.
// (Translation can also be employed by using the "shift-left-mouse-button"
// combination inside of the widget.) Scaling (about the center of the spline)
// is achieved by using the right mouse button. By moving the mouse "up" the
// render window the spline will be made bigger; by moving "down" the render
// window the widget will be made smaller.  Finally, holding the ctrl key down
// and then grabbing either a handle or the spline itself with the middle mouse
// button enalbles spinning of the widget about its center. Events that occur
// outside of the widget (i.e., no part of the widget is picked) are propagated
// to any other registered obsevers (such as the interaction style).  Turn off
// the widget by pressing the "i" key again (or invoke the Off() method).
//
// The vtkSplineWidget has several methods that can be used in conjunction with
// other VTK objects. The Set/GetResolution() methods control the number of
// subdivisions of the spline; the GetPolyData() method can be used to get the
// polygonal representation and can be used for things like seeding
// streamlines or probing other data sets. Typical usage of the widget is to
// make use of the StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent events. The InteractionEvent is called on mouse motion;
// the other two events are called on button down and button up (either left or
// right button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of the selected and
// unselected representations of the spline. For example, you can set the
// property for the handles and spline. In addition there are methods to
// constrain the spline so that it is aligned with a plane.

// .SECTION Thanks
// Thanks to Dean Inglis for developing and contributing this class.

// .SECTION Caveats
// Note that handles and line can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget vtkPointWidget vtkSphereWidget
// vtkImagePlaneWidget vtkImplicitPlaneWidget vtkPlaneWidget


#ifndef __vtkSplineWidget_h
#define __vtkSplineWidget_h

#include "vtk3DWidget.h"

class vtkActor;
class vtkCellPicker;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkSpline;
class vtkTransform;

#define VTK_PROJECTION_YZ 0
#define VTK_PROJECTION_XZ 1
#define VTK_PROJECTION_XY 2
#define VTK_PROJECTION_OBLIQUE 3

class VTK_HYBRID_EXPORT vtkSplineWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkSplineWidget *New();

  vtkTypeRevisionMacro(vtkSplineWidget,vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods that satisfy the superclass' API.
  virtual void SetEnabled(int);
  virtual void PlaceWidget(float bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(float xmin, float xmax, float ymin, float ymax, 
                   float zmin, float zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  // Description:
  // Force the spline widget to be projected onto one of the orthogonal planes.
  // Remember that when the state changes, a ModifiedEvent is invoked.
  // This can be used to snap the spline to the plane if it is orginally
  // not aligned.  The normal in SetProjectionNormal is 0,1,2 for YZ,XZ,XY
  // planes respectively and 3 for arbitrary oblique planes when the widget
  // is tied to a vtkPlaneSource.
  vtkSetMacro(ProjectToPlane,int);
  vtkGetMacro(ProjectToPlane,int);
  vtkBooleanMacro(ProjectToPlane,int);

  // Description:
  // Set up a reference to a vtkPlaneSource that could be from another widget
  // object, e.g. a vtkPolyDataSourceWidget.
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

  // Description:
  // Set the position of spline handles and points in terms of a plane's
  // position. i.e., if ProjectionNormal is 0, all of the x-coordinate
  // values of the points are set to position. Any value can be passed (and is
  // ignored) to update the spline points when Projection normal is set to 3
  // for arbritrary plane orientations.
  void SetProjectionPosition(float position);
  vtkGetMacro(ProjectionPosition, float);

  // Description:
  // Grab the polydata (including points) that defines the spline.  The
  // polydata consists of the NumberOfSplinePoints points. These point values
  // are guaranteed to be up-to-date when either the InteractionEvent or
  // EndInteraction events are invoked. The user provides the vtkPolyData and
  // the points and polyline are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Get the handle properties (the little balls are the handles). The
  // properties of the handles when selected and normal can be manipulated.
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);

  // Description:
  // Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  vtkGetObjectMacro(LineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);

  // Description:
  // Set/Get the number of handles for this widget.
  void SetNumberOfHandles(int npts);
  vtkGetMacro(NumberOfHandles, int);

  // Description:
  // Set/Get the number of line segments representing the spline for
  // this widget.
  void SetResolution(int resolution);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the spline objects.  The user can supply one of currently
  // two types of spline: vtkCardinalSpline, vtkKochanekSpline.
  // WARNING: The widget does not enforce internal consistency so that all
  // three are of the same type.
  void SetXSpline(vtkSpline*);
  void SetYSpline(vtkSpline*);
  void SetZSpline(vtkSpline*);

  // Description:
  // Set/Get the position of the spline handles. Call GetNumberOfHandles
  // to determine the valid range of handle indices.
  void SetHandlePosition(int handle, float x, float y, float z);
  void SetHandlePosition(int handle, float xyz[3]);
  void GetHandlePosition(int handle, float xyz[3]);
  float* GetHandlePosition(int handle);

  // Description:
  // Control whether the spline is open or closed. A closed spline forms
  // a continuous loop: the first and last points are the same, and
  // derivatives are continuous.  This method enforces consistency with
  // user supplied subclasses of vtkSpline.
  void SetClosed(int closed);
  vtkGetMacro(Closed,int);
  void ClosedOn()
    { this->SetClosed(1); }
  void ClosedOff()
    { this->SetClosed(0); }

protected:
  vtkSplineWidget();
  ~vtkSplineWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Moving,
    Scaling,
    Spinning,
    Outside
  };
//ETX

  //handles the events
  static void ProcessEvents(vtkObject* object,
                            unsigned long event,
                            void* clientdata,
                            void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // Controlling vars
  int   ProjectionNormal;
  float ProjectionPosition;
  int   ProjectToPlane;
  vtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  // The spline
  vtkSpline *XSpline;
  vtkSpline *YSpline;
  vtkSpline *ZSpline;
  int NumberOfHandles;
  float* HandlePositions;
  vtkSpline* CreateDefaultSpline();// default is vtkCardinalSpline
  int Closed;
  float Offset;

  // The line segments
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkPolyData       *LineData;
  void HighlightLine(int highlight);
  int NumberOfSplinePoints;
  int Resolution;
  float* SplinePositions;

  // Glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;
  void Initialize();
  int  HighlightHandle(vtkProp *prop); //returns handle index or -1 on fail
  virtual void SizeHandles();
  void BuildRepresentation();

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkActor *CurrentHandle;
  int CurrentHandleIndex;

  // Methods to manipulate the spline.
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

private:
  vtkSplineWidget(const vtkSplineWidget&);  //Not implemented
  void operator=(const vtkSplineWidget&);  //Not implemented
};

#endif
