/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTracerWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageTracerWidget - 3D widget for tracing on planar props.
// .SECTION Description
// vtkImageTracerWidget is different from other widgets in three distinct ways:
// 1) any sub-class of vtkProp can be input rather than just vtkProp3D, so that
// vtkImageActor can be set as the prop and then traced over, 2) the widget fires
// pick events at the input prop to decide where to move its handles, 3) the
// widget has 2D glyphs for handles instead of 3D spheres as is done in other
// sub-classes of vtk3DWidget. This widget is primarily designed for manually
// tracing over image data.
// The button actions and key modifiers are as follows for controlling the
// widget:
// 1) left button click over the image, hold and drag draws a free hand line.
// 2) left button click and release erases the widget line,
// if it exists, and repositions the first handle.
// 3) middle button click starts a snap drawn line.  The line is terminated by
// clicking the middle button while depressing the ctrl key.
// 4) when tracing a continuous or snap drawn line, if the last cursor position
// is within a specified tolerance to the first handle, the widget line will form
// a closed loop.
// 5) right button clicking and holding on any handle that is part of a snap
// drawn line allows handle dragging: existing line segments are updated
// accordingly.  If the path is open and AutoClose is set to On, the path can
// be closed by repositioning the first and last points over one another.
// 6) ctrl key + right button down on any handle will erase it: existing
// snap drawn line segments are updated accordingly.  If the line was formed by
// continuous tracing, the line is deleted leaving one handle.
// 7) shift key + right button down on any snap drawn line segment will insert
// a handle at the cursor position.  The line segment is split accordingly.

// .SECTION Caveats
// the input vtkDataSet should be vtkImageData.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget vtkPointWidget vtkSphereWidget
// vtkImagePlaneWidget vtkImplicitPlaneWidget vtkPlaneWidget

#ifndef vtkImageTracerWidget_h
#define vtkImageTracerWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtk3DWidget.h"

class vtkAbstractPropPicker;
class vtkActor;
class vtkCellArray;
class vtkCellPicker;
class vtkFloatArray;
class vtkGlyphSource2D;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkPropPicker;
class vtkTransform;
class vtkTransformPolyDataFilter;

#define VTK_ITW_PROJECTION_YZ 0
#define VTK_ITW_PROJECTION_XZ 1
#define VTK_ITW_PROJECTION_XY 2
#define VTK_ITW_SNAP_CELLS    0
#define VTK_ITW_SNAP_POINTS   1

class VTKINTERACTIONWIDGETS_EXPORT vtkImageTracerWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkImageTracerWidget *New();

  vtkTypeMacro(vtkImageTracerWidget,vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods that satisfy the superclass' API.
  virtual void SetEnabled(int);
  virtual void PlaceWidget(double bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  // Description:
  // Set/Get the handle properties (the 2D glyphs are the handles). The
  // properties of the handles when selected and normal can be manipulated.
  virtual void SetHandleProperty(vtkProperty*);
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  virtual void SetSelectedHandleProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);

  // Description:
  // Set/Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  virtual void SetLineProperty(vtkProperty*);
  vtkGetObjectMacro(LineProperty, vtkProperty);
  virtual void SetSelectedLineProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);

  // Description:
  // Set the prop, usually a vtkImageActor, to trace over.
  void SetViewProp(vtkProp* prop);

  // Description:
  // Force handles to be on a specific ortho plane. Default is Off.
  vtkSetMacro(ProjectToPlane,int);
  vtkGetMacro(ProjectToPlane,int);
  vtkBooleanMacro(ProjectToPlane,int);

  // Description:
  // Set the projection normal.  The normal in SetProjectionNormal is 0,1,2
  // for YZ,XZ,XY planes respectively.  Since the handles are 2D glyphs, it is
  // necessary to specify a plane on which to generate them, even though
  // ProjectToPlane may be turned off.
  vtkSetClampMacro(ProjectionNormal,int,VTK_ITW_PROJECTION_YZ,VTK_ITW_PROJECTION_XY);
  vtkGetMacro(ProjectionNormal,int);
  void SetProjectionNormalToXAxes()
    { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes()
    { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes()
    { this->SetProjectionNormal(2); }

  // Description:
  // Set the position of the widgets' handles in terms of a plane's position.
  // e.g., if ProjectionNormal is 0, all of the x-coordinate values of the
  // handles are set to ProjectionPosition.  No attempt is made to ensure that
  // the position is within the bounds of either the underlying image data or
  // the prop on which tracing is performed.
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition,double);

   // Description:
  // Force snapping to image data while tracing. Default is Off.
  void SetSnapToImage(int snap);
  vtkGetMacro(SnapToImage,int);
  vtkBooleanMacro(SnapToImage,int);

  // Description:
  // In concert with a CaptureRadius value, automatically
  // form a closed path by connecting first to last path points.
  // Default is Off.
  vtkSetMacro(AutoClose,int);
  vtkGetMacro(AutoClose,int);
  vtkBooleanMacro(AutoClose,int);

  // Description:
  // Set/Get the capture radius for automatic path closing.  For image
  // data, capture radius should be half the distance between voxel/pixel
  // centers.
  // Default is 1.0
  vtkSetMacro(CaptureRadius,double);
  vtkGetMacro(CaptureRadius,double);

  // Description:
  // Grab the points and lines that define the traced path. These point values
  // are guaranteed to be up-to-date when either the InteractionEvent or
  // EndInteraction events are invoked. The user provides the vtkPolyData and
  // the points and cells representing the line are added to it.
  void GetPath(vtkPolyData *pd);

  // Description:
  // Get the handles' geometric representation via vtkGlyphSource2D.
  vtkGlyphSource2D* GetGlyphSource() { return this->HandleGenerator; }

  // Description:
  // Set/Get the type of snapping to image data: center of a pixel/voxel or
  // nearest point defining a pixel/voxel.
  vtkSetClampMacro(ImageSnapType,int,VTK_ITW_SNAP_CELLS,VTK_ITW_SNAP_POINTS);
  vtkGetMacro(ImageSnapType,int);

  // Description:
  // Set/Get the handle position in terms of a zero-based array of handles.
  void SetHandlePosition(int handle, double xyz[3]);
  void SetHandlePosition(int handle, double x, double y, double z);
  void GetHandlePosition(int handle, double xyz[3]);
  double* GetHandlePosition(int handle);

  // Description:
  // Get the number of handles.
  vtkGetMacro(NumberOfHandles,int);

  // Description:
  // Enable/disable mouse interaction when the widget is visible.
  void SetInteraction(int interact);
  vtkGetMacro(Interaction,int);
  vtkBooleanMacro(Interaction,int);

  // Description:
  // Initialize the widget with a set of points and generate
  // lines between them.  If AutoClose is on it will handle the
  // case wherein the first and last points are congruent.
  void InitializeHandles(vtkPoints*);

  // Description:
  // Is the path closed or open?
  int IsClosed();

  // Description:
  // Enable/Disable mouse button events
  vtkSetMacro(HandleLeftMouseButton,int);
  vtkGetMacro(HandleLeftMouseButton,int);
  vtkBooleanMacro(HandleLeftMouseButton,int);
  vtkSetMacro(HandleMiddleMouseButton,int);
  vtkGetMacro(HandleMiddleMouseButton,int);
  vtkBooleanMacro(HandleMiddleMouseButton,int);
  vtkSetMacro(HandleRightMouseButton,int);
  vtkGetMacro(HandleRightMouseButton,int);
  vtkBooleanMacro(HandleRightMouseButton,int);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# define SetPropA SetProp
# define SetPropW SetProp
#endif

  // Description:
  // @deprecated Replaced by vtkImageTracerWidget::SetViewProp() as of VTK 5.0.
  VTK_LEGACY(void SetProp(vtkProp* prop));

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef SetPropA
# undef SetPropW
  //BTX
  VTK_LEGACY(void SetPropA(vtkProp*));
  VTK_LEGACY(void SetPropW(vtkProp*));
  //ETX
#endif

protected:
  vtkImageTracerWidget();
  ~vtkImageTracerWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Tracing,
    Snapping,
    Erasing,
    Inserting,
    Moving,
    Translating,
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

  void AddObservers();

  // Controlling ivars
  int    Interaction;
  int    ProjectionNormal;
  double ProjectionPosition;
  int    ProjectToPlane;
  int    ImageSnapType;
  int    SnapToImage;
  double CaptureRadius; // tolerance for auto path close
  int    AutoClose;
  int    IsSnapping;
  int    LastX;
  int    LastY;

  void  Trace(int , int );
  void  Snap(double* );
  void  MovePoint(const double* , const double* );
  void  Translate(const double* , const double* );
  void  ClosePath();

  // 2D glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyData       **HandleGeometry;
  vtkGlyphSource2D   *HandleGenerator;

  // Transforms required as 2D glyphs are generated in the x-y plane
  vtkTransformPolyDataFilter *TransformFilter;
  vtkTransform               *Transform;
  vtkFloatArray              *TemporaryHandlePoints;

  void AppendHandles(double*);
  void ResetHandles();
  void AllocateHandles(const int& );
  void AdjustHandlePosition(const int& , double*);
  int  HighlightHandle(vtkProp* ); // returns handle index or -1 on fail
  void EraseHandle(const int& );
  virtual void SizeHandles();
  void InsertHandleOnLine(double* );

  int NumberOfHandles;
  vtkActor *CurrentHandle;
  int CurrentHandleIndex;

  vtkProp       *ViewProp;    // the prop we want to pick on
  vtkPropPicker *PropPicker;  // the prop's picker

  // Representation of the line
  vtkPoints         *LinePoints;
  vtkCellArray      *LineCells;
  vtkActor          *LineActor;
  vtkPolyData       *LineData;
  vtkIdType          CurrentPoints[2];

  void HighlightLine(const int& );
  void BuildLinesFromHandles();
  void ResetLine(double* );
  void AppendLine(double* );
  int  PickCount;

  // Do the picking of the handles and the lines
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkAbstractPropPicker* CurrentPicker;

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void CreateDefaultProperties();

  // Enable/Disable mouse button events
  int HandleLeftMouseButton;
  int HandleMiddleMouseButton;
  int HandleRightMouseButton;

private:
  vtkImageTracerWidget(const vtkImageTracerWidget&);  //Not implemented
  void operator=(const vtkImageTracerWidget&);  //Not implemented
};

#endif
