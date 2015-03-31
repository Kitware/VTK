/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurveRepresentation

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCurveRepresentation - vtkWidgetRepresentation
// base class for a widget that represents an curve that connects control
// points.
// .SECTION Description
// Base class for widgets used to define curves from points, such as
// vtkPolyLineRepresentation and vtkSplineRepresentation.  This class
// uses handles, the number of which can be changed, to represent the
// points that define the curve. The handles can be picked can be
// picked on the curve itself to translate or rotate it in the scene.

#ifndef vtkCurveRepresentation_h
#define vtkCurveRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkCellPicker;
class vtkDoubleArray;
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
class VTKINTERACTIONWIDGETS_EXPORT vtkCurveRepresentation : public vtkWidgetRepresentation
{
public:
  vtkTypeMacro(vtkCurveRepresentation, vtkWidgetRepresentation);
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

  // Description:
  // Set the interaction state
  vtkSetMacro(InteractionState, int);

  // Description:
  // Force the widget to be projected onto one of the orthogonal
  // planes.  Remember that when the InteractionState changes, a
  // ModifiedEvent is invoked.  This can be used to snap the curve to
  // the plane if it is originally not aligned.  The normal in
  // SetProjectionNormal is 0,1,2 for YZ,XZ,XY planes respectively and
  // 3 for arbitrary oblique planes when the widget is tied to a
  // vtkPlaneSource.
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
  // Set the position of poly line handles and points in terms of a plane's
  // position. i.e., if ProjectionNormal is 0, all of the x-coordinate
  // values of the points are set to position. Any value can be passed (and is
  // ignored) to update the poly line points when Projection normal is set to 3
  // for arbritrary plane orientations.
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);

  // Description:
  // Grab the polydata (including points) that defines the
  // interpolating curve. Points are guaranteed to be up-to-date when
  // either the InteractionEvent or EndInteraction events are
  // invoked. The user provides the vtkPolyData and the points and
  // polyline are added to it.
  virtual void GetPolyData(vtkPolyData *pd) = 0;

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
  virtual void SetNumberOfHandles(int npts) = 0;
  vtkGetMacro(NumberOfHandles, int);

  // Description:
  // Set/Get the position of the handles. Call GetNumberOfHandles
  // to determine the valid range of handle indices.
  virtual void SetHandlePosition(int handle, double x, double y, double z);
  virtual void SetHandlePosition(int handle, double xyz[3]);
  virtual void GetHandlePosition(int handle, double xyz[3]);
  virtual double* GetHandlePosition(int handle);
  virtual vtkDoubleArray* GetHandlePositions() = 0;

  // Description:
  // Control whether the curve is open or closed. A closed forms a
  // continuous loop: the first and last points are the same.  A
  // minimum of 3 handles are required to form a closed loop.
  void SetClosed(int closed);
  vtkGetMacro(Closed,int);
  vtkBooleanMacro(Closed,int);

  // Description:
  // Convenience method to determine whether the curve is
  // closed in a geometric sense.  The widget may be set "closed" but still
  // be geometrically open (e.g., a straight line).
  int IsClosed();

  // Description:
  // Get the approximate vs. the true arc length of the curve. Calculated as
  // the summed lengths of the individual straight line segments. Use
  // SetResolution to control the accuracy.
  virtual double GetSummedLength() = 0;

  // Description:
  // Convenience method to allocate and set the handles from a
  // vtkPoints instance.  If the first and last points are the same,
  // the curve sets Closed to the on InteractionState and disregards
  // the last point, otherwise Closed remains unchanged.
  virtual void InitializeHandles(vtkPoints* points) = 0;

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's
  // API. Note that a version of place widget is available where the
  // center and handle position are specified.
  virtual void BuildRepresentation() = 0;
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
  vtkCurveRepresentation();
  ~vtkCurveRepresentation();

  double LastEventPosition[3];
  double Bounds[6];

  // Controlling vars
  int             ProjectionNormal;
  double          ProjectionPosition;
  int             ProjectToPlane;
  vtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  int NumberOfHandles;
  int Closed;

  // The line segments
  vtkActor           *LineActor;
  void HighlightLine(int highlight);

  // Glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkSphereSource   **HandleGeometry;
  void Initialize();
  int  HighlightHandle(vtkProp *prop); //returns handle index or -1 on fail
  virtual void SizeHandles();
  virtual void InsertHandleOnLine(double* pos) = 0;
  void EraseHandle(const int&);

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  double LastPickPosition[3];
  vtkActor *CurrentHandle;
  int CurrentHandleIndex;

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  // Methods to manipulate the curve.
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
  vtkCurveRepresentation(const vtkCurveRepresentation&); // Not implemented.
  void operator=(const vtkCurveRepresentation&); // Not implemented.
//ETX
};


#endif
