/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSplineRepresentation - vtkWidgetRepresentation for a spline.
// .SECTION Description
// vtkSplineRepresentation is a vtkWidgetRepresentation for a spline.
// This 3D widget defines a spline that can be interactively placed in a
// scene. The spline has handles, the number of which can be changed, plus it
// can be picked on the spline itself to translate or rotate it in the scene.
// This is based on vtkSplineWidget.
// .SECTION See Also
// vtkSplineWidget, vtkSplineWidget2

#ifndef __vtkSplineRepresentation_h
#define __vtkSplineRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkCellPicker;
class vtkDoubleArray;
class vtkParametricFunctionSource;
class vtkParametricSpline;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;

#define VTK_PROJECTION_YZ 0
#define VTK_PROJECTION_XZ 1
#define VTK_PROJECTION_XY 2
#define VTK_PROJECTION_OBLIQUE 3
class VTK_WIDGETS_EXPORT vtkSplineRepresentation : public vtkWidgetRepresentation
{
public:
  static vtkSplineRepresentation* New();
  vtkTypeMacro(vtkSplineRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // Used to manage the InteractionState of the widget
  enum _InteractionState {
    Outside=0,
    OnHandle,
    OnLine,
    Moving,
    Scaling,
    Spinning,
    Inserting,
    Erasing
  };
//ETX
  
  vtkSetMacro(InteractionState, int);

  // Description:
  // Force the spline widget to be projected onto one of the orthogonal planes.
  // Remember that when the InteractionState changes, a ModifiedEvent is invoked.
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
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);

  // Description:
  // Grab the polydata (including points) that defines the spline.  The
  // polydata consists of points and line segments numbering Resolution + 1
  // and Resoltuion, respectively. Points are guaranteed to be up-to-date when
  // either the InteractionEvent or  EndInteraction events are invoked. The
  // user provides the vtkPolyData and the points and polyline are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Set/Get the handle properties (the spheres are the handles). The
  // properties of the handles when selected and unselected can be manipulated.
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);

  // Description:
  // Set/Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  vtkGetObjectMacro(LineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);

  // Description:
  // Set/Get the number of handles for this widget.
  virtual void SetNumberOfHandles(int npts);
  vtkGetMacro(NumberOfHandles, int);

  // Description:
  // Set/Get the number of line segments representing the spline for
  // this widget.
  void SetResolution(int resolution);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the parametric spline object. Through vtkParametricSpline's API, the
  // user can supply and configure one of currently two types of spline:
  // vtkCardinalSpline, vtkKochanekSpline. The widget controls the open
  // or closed configuration of the spline.
  // WARNING: The widget does not enforce internal consistency so that all
  // three are of the same type.
  virtual void SetParametricSpline(vtkParametricSpline*);
  vtkGetObjectMacro(ParametricSpline,vtkParametricSpline);

  // Description:
  // Set/Get the position of the spline handles. Call GetNumberOfHandles
  // to determine the valid range of handle indices.
  void SetHandlePosition(int handle, double x, double y, double z);
  void SetHandlePosition(int handle, double xyz[3]);
  void GetHandlePosition(int handle, double xyz[3]);
  double* GetHandlePosition(int handle);
  vtkDoubleArray* GetHandlePositions();

  // Description:
  // Control whether the spline is open or closed. A closed spline forms
  // a continuous loop: the first and last points are the same, and
  // derivatives are continuous.  A minimum of 3 handles are required to
  // form a closed loop.  This method enforces consistency with
  // user supplied subclasses of vtkSpline.
  void SetClosed(int closed);
  vtkGetMacro(Closed,int);
  vtkBooleanMacro(Closed,int);

  // Description:
  // Convenience method to determine whether the spline is
  // closed in a geometric sense.  The widget may be set "closed" but still
  // be geometrically open (e.g., a straight line).
  int IsClosed();

  // Description:
  // Get the approximate vs. the true arc length of the spline. Calculated as
  // the summed lengths of the individual straight line segments. Use
  // SetResolution to control the accuracy.
  double GetSummedLength();

  // Description:
  // Convenience method to allocate and set the handles from a vtkPoints
  // instance.  If the first and last points are the same, the spline sets
  // Closed to the on InteractionState and disregards the last point, otherwise Closed
  // remains unchanged.
  void InitializeHandles(vtkPoints* points);

 // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API. Note that a 
  // version of place widget is available where the center and handle position
  // are specified.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual void EndWidgetInteraction(double e[2]);
  virtual double *GetBounds();
  
  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Convenience method to set the line color.
  // Ideally one should use GetLineProperty()->SetColor().
  void SetLineColor(double r, double g, double b);

//BTX
protected:
  vtkSplineRepresentation();
  ~vtkSplineRepresentation();
  
  double LastEventPosition[3];
  double Bounds[6];

  // Controlling vars
  int   ProjectionNormal;
  double ProjectionPosition;
  int   ProjectToPlane;
  vtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  // The spline
  vtkParametricSpline *ParametricSpline;
  vtkParametricFunctionSource *ParametricFunctionSource;
  int NumberOfHandles;
  int Closed;
  
  // The line segments
  vtkActor           *LineActor;
  void HighlightLine(int highlight);
  int Resolution;

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
  double LastPickPosition[3];
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

  // For efficient spinning
  double Centroid[3];
  void CalculateCentroid();

private:
  vtkSplineRepresentation(const vtkSplineRepresentation&); // Not implemented.
  void operator=(const vtkSplineRepresentation&); // Not implemented.
//ETX
};

#endif


