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
// vtkImgeActor can be set as the prop and then traced over, 2) the widget fires
// picks at the input prop to decide where to move its handles, 3) the
// widget has 2D glyphs for handles instead of 3D spheres as is done on other
// sub-classes of vtk3DWidget. This widget is primarily designed for manually
// tracing over image data.

// .SECTION Thanks
// Thanks to Dean Inglis for developing and contributing this class.

// .SECTION Caveats
// the input vtkDataSet should be vtkImageData.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget vtkPointWidget vtkSphereWidget
// vtkImagePlaneWidget vtkImplicitPlaneWidget vtkPlaneWidget


#ifndef __vtkImageTracerWidget_h
#define __vtkImageTracerWidget_h

#include "vtk3DWidget.h"

class vtkAbstractPropPicker;
class vtkActor;
class vtkCellArray;
class vtkCellPicker;
class vtkFloatArray;
class vtkGlyphSource2D;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
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

class VTK_HYBRID_EXPORT vtkImageTracerWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkImageTracerWidget *New();

  vtkTypeRevisionMacro(vtkImageTracerWidget,vtk3DWidget);
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
  void SetProp(vtkProp* prop);

  // Description:
  // Force handles to be on a specific ortho plane.
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
  vtkSetMacro(ProjectionPosition,double);
  vtkGetMacro(ProjectionPosition,double);

   // Description:
  // Force snapping to image data while tracing.
  void SetSnapToImage(int snap);
  vtkGetMacro(SnapToImage,int);
  vtkBooleanMacro(SnapToImage,int);

  // Description:
  // In concert with a CaptureRadius value, automatically
  // form a closed path by connecting first to last path points.
  vtkSetMacro(AutoClose,int);
  vtkGetMacro(AutoClose,int);
  vtkBooleanMacro(AutoClose,int);

  // Description:
  // Set/Get the initial orientation of the glyph/handle when generated.
  vtkSetMacro(GlyphAngle,double);
  vtkGetMacro(GlyphAngle,double);

  // Description:
  // Set/Get the capture radius for automatic path closing.  For image
  // data, capture radius should be half the distance between voxel/pixel
  // centers.
  vtkSetMacro(CaptureRadius,double);
  vtkGetMacro(CaptureRadius,double);

  // Description:
  // Grab the the points that define the traced path. These point values
  // are guaranteed to be up-to-date when either the InteractionEvent or
  // EndInteraction events are invoked. The user provides the vtkPolyData and
  // the points and polyline are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Get the handles' geometric representation via vtkGlyphSource2D.
  vtkGlyphSource2D* GetGlyphSource() { return this->HandleGeometryGenerator; }

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

  // Controlling vars
  int    Interaction;
  int    ProjectionNormal;
  double ProjectionPosition;
  int    ProjectToPlane;
  int    ImageSnapType;
  int    SnapToImage;
  double CaptureRadius; // how close begin and end points are for auto path close
  double GlyphAngle;  // pre-rotation of a glyph eg., a cross can be pre-rotated 45 degrees
  int    AutoClose;

  int   IsSnapping;
  int   MouseMoved;
  void  Trace(int X, int Y);
  void  Snap(double*);
  void  MovePoint(double *p1, double *p2);
  void  ClosePath();

  // 2D glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkPolyData       **HandleGeometry;
  vtkGlyphSource2D   *HandleGeometryGenerator;

  // Transforms required as 2D glyphs are generated in the x-y plane
  vtkTransformPolyDataFilter *TransformFilter;
  vtkTransform               *Transform;
  vtkFloatArray              *TemporaryHandlePoints;

  void AppendHandles(double*);
  void ResetHandles();
  void AllocateHandles(int nhandles);
  void AdjustHandlePosition(int, double*);
  int  HighlightHandle(vtkProp *prop); // returns handle index or -1 on fail
  void EraseHandle(int);
  virtual void SizeHandles();
  void InsertHandleOnLine(double*);

  int NumberOfHandles;
  vtkActor *CurrentHandle;
  int CurrentHandleIndex;

  vtkProp       *Prop;              // the prop we want to pick on
  vtkPropPicker *PropPicker;  // the prop's picker

  // Representation of the line
  vtkPoints         *LinePoints;
  vtkCellArray      *LineCells;
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkPolyData       *LineData;
  vtkIdType          CurrentPoints[2];

  void HighlightLine(int);
  void BuildLinesFromHandles();
  void ResetLine(double*);
  void AppendLine(double*);
  int  PickCount;

  // Do the picking of the handles and the lines
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkAbstractPropPicker* CurrentPicker;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void CreateDefaultProperties();

private:
  vtkImageTracerWidget(const vtkImageTracerWidget&);  //Not implemented
  void operator=(const vtkImageTracerWidget&);  //Not implemented
};

#endif
