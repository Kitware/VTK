// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageTracerWidget
 * @brief   3D widget for tracing on planar props.
 *
 * vtkImageTracerWidget is different from other widgets in three distinct ways:
 * 1) any sub-class of vtkProp can be input rather than just vtkProp3D, so that
 * vtkImageActor can be set as the prop and then traced over, 2) the widget fires
 * pick events at the input prop to decide where to move its handles, 3) the
 * widget has 2D glyphs for handles instead of 3D spheres as is done in other
 * sub-classes of vtk3DWidget. This widget is primarily designed for manually
 * tracing over image data.
 * The button actions and key modifiers are as follows for controlling the
 * widget:
 * 1) left button click over the image, hold and drag draws a free hand line.
 * 2) left button click and release erases the widget line,
 * if it exists, and repositions the first handle.
 * 3) middle button click starts a snap drawn line.  The line is terminated by
 * clicking the middle button while depressing the ctrl key.
 * 4) when tracing a continuous or snap drawn line, if the last cursor position
 * is within a specified tolerance to the first handle, the widget line will form
 * a closed loop.
 * 5) right button clicking and holding on any handle that is part of a snap
 * drawn line allows handle dragging: existing line segments are updated
 * accordingly.  If the path is open and AutoClose is set to On, the path can
 * be closed by repositioning the first and last points over one another.
 * 6) ctrl key + right button down on any handle will erase it: existing
 * snap drawn line segments are updated accordingly.  If the line was formed by
 * continuous tracing, the line is deleted leaving one handle.
 * 7) shift key + right button down on any snap drawn line segment will insert
 * a handle at the cursor position.  The line segment is split accordingly.
 *
 * @warning
 * the input vtkDataSet should be vtkImageData.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkLineWidget vtkPointWidget vtkSphereWidget
 * vtkImagePlaneWidget vtkImplicitPlaneWidget vtkPlaneWidget
 */

#ifndef vtkImageTracerWidget_h
#define vtkImageTracerWidget_h

#include "vtk3DWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
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
#define VTK_ITW_SNAP_CELLS 0
#define VTK_ITW_SNAP_POINTS 1

class VTKINTERACTIONWIDGETS_EXPORT vtkImageTracerWidget : public vtk3DWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkImageTracerWidget* New();

  vtkTypeMacro(vtkImageTracerWidget, vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods that satisfy the superclass' API.
   */
  void SetEnabled(int) override;
  void PlaceWidget(double bounds[6]) override;
  void PlaceWidget() override { this->Superclass::PlaceWidget(); }
  void PlaceWidget(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) override
  {
    this->Superclass::PlaceWidget(xmin, xmax, ymin, ymax, zmin, zmax);
  }
  ///@}

  ///@{
  /**
   * Set/Get the handle properties (the 2D glyphs are the handles). The
   * properties of the handles when selected and normal can be manipulated.
   */
  virtual void SetHandleProperty(vtkProperty*);
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  virtual void SetSelectedHandleProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set/Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  virtual void SetLineProperty(vtkProperty*);
  vtkGetObjectMacro(LineProperty, vtkProperty);
  virtual void SetSelectedLineProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);
  ///@}

  /**
   * Set the prop, usually a vtkImageActor, to trace over.
   */
  void SetViewProp(vtkProp* prop);

  ///@{
  /**
   * Force handles to be on a specific ortho plane. Default is Off.
   */
  vtkSetMacro(ProjectToPlane, vtkTypeBool);
  vtkGetMacro(ProjectToPlane, vtkTypeBool);
  vtkBooleanMacro(ProjectToPlane, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the projection normal.  The normal in SetProjectionNormal is 0,1,2
   * for YZ,XZ,XY planes respectively.  Since the handles are 2D glyphs, it is
   * necessary to specify a plane on which to generate them, even though
   * ProjectToPlane may be turned off.
   */
  vtkSetClampMacro(ProjectionNormal, int, VTK_ITW_PROJECTION_YZ, VTK_ITW_PROJECTION_XY);
  vtkGetMacro(ProjectionNormal, int);
  void SetProjectionNormalToXAxes() { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes() { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes() { this->SetProjectionNormal(2); }
  ///@}

  ///@{
  /**
   * Set the position of the widgets' handles in terms of a plane's position.
   * e.g., if ProjectionNormal is 0, all of the x-coordinate values of the
   * handles are set to ProjectionPosition.  No attempt is made to ensure that
   * the position is within the bounds of either the underlying image data or
   * the prop on which tracing is performed.
   */
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);
  ///@}

  ///@{
  /**
   * Force snapping to image data while tracing. Default is Off.
   */
  void SetSnapToImage(vtkTypeBool snap);
  vtkGetMacro(SnapToImage, vtkTypeBool);
  vtkBooleanMacro(SnapToImage, vtkTypeBool);
  ///@}

  ///@{
  /**
   * In concert with a CaptureRadius value, automatically
   * form a closed path by connecting first to last path points.
   * Default is Off.
   */
  vtkSetMacro(AutoClose, vtkTypeBool);
  vtkGetMacro(AutoClose, vtkTypeBool);
  vtkBooleanMacro(AutoClose, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the capture radius for automatic path closing.  For image
   * data, capture radius should be half the distance between voxel/pixel
   * centers.
   * Default is 1.0
   */
  vtkSetMacro(CaptureRadius, double);
  vtkGetMacro(CaptureRadius, double);
  ///@}

  /**
   * Grab the points and lines that define the traced path. These point values
   * are guaranteed to be up-to-date when either the InteractionEvent or
   * EndInteraction events are invoked. The user provides the vtkPolyData and
   * the points and cells representing the line are added to it.
   */
  void GetPath(vtkPolyData* pd);

  /**
   * Get the handles' geometric representation via vtkGlyphSource2D.
   */
  vtkGlyphSource2D* GetGlyphSource() { return this->HandleGenerator; }

  ///@{
  /**
   * Set/Get the type of snapping to image data: center of a pixel/voxel or
   * nearest point defining a pixel/voxel.
   */
  vtkSetClampMacro(ImageSnapType, int, VTK_ITW_SNAP_CELLS, VTK_ITW_SNAP_POINTS);
  vtkGetMacro(ImageSnapType, int);
  ///@}

  ///@{
  /**
   * Set/Get the handle position in terms of a zero-based array of handles.
   */
  void SetHandlePosition(int handle, double xyz[3]);
  void SetHandlePosition(int handle, double x, double y, double z);
  void GetHandlePosition(int handle, double xyz[3]);
  double* GetHandlePosition(int handle) VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * Get the number of handles.
   */
  vtkGetMacro(NumberOfHandles, int);
  ///@}

  ///@{
  /**
   * Enable/disable mouse interaction when the widget is visible.
   */
  void SetInteraction(vtkTypeBool interact);
  vtkGetMacro(Interaction, vtkTypeBool);
  vtkBooleanMacro(Interaction, vtkTypeBool);
  ///@}

  /**
   * Initialize the widget with a set of points and generate
   * lines between them.  If AutoClose is on it will handle the
   * case wherein the first and last points are congruent.
   */
  void InitializeHandles(vtkPoints*);

  /**
   * Is the path closed or open?
   */
  int IsClosed();

  ///@{
  /**
   * Enable/Disable mouse button events
   */
  vtkSetMacro(HandleLeftMouseButton, vtkTypeBool);
  vtkGetMacro(HandleLeftMouseButton, vtkTypeBool);
  vtkBooleanMacro(HandleLeftMouseButton, vtkTypeBool);
  vtkSetMacro(HandleMiddleMouseButton, vtkTypeBool);
  vtkGetMacro(HandleMiddleMouseButton, vtkTypeBool);
  vtkBooleanMacro(HandleMiddleMouseButton, vtkTypeBool);
  vtkSetMacro(HandleRightMouseButton, vtkTypeBool);
  vtkGetMacro(HandleRightMouseButton, vtkTypeBool);
  vtkBooleanMacro(HandleRightMouseButton, vtkTypeBool);
  ///@}

protected:
  vtkImageTracerWidget();
  ~vtkImageTracerWidget() override;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    Tracing,
    Snapping,
    Erasing,
    Inserting,
    Moving,
    Translating,
    Outside
  };

  // handles the events
  static void ProcessEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

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
  vtkTypeBool Interaction;
  int ProjectionNormal;
  double ProjectionPosition;
  vtkTypeBool ProjectToPlane;
  int ImageSnapType;
  vtkTypeBool SnapToImage;
  double CaptureRadius; // tolerance for auto path close
  vtkTypeBool AutoClose;
  int IsSnapping;
  int LastX;
  int LastY;

  void Trace(int, int);
  void Snap(double*);
  void MovePoint(const double*, const double*);
  void Translate(const double*, const double*);
  void ClosePath();

  // 2D glyphs representing hot spots (e.g., handles)
  vtkActor** Handle;
  vtkPolyData** HandleGeometry;
  vtkGlyphSource2D* HandleGenerator;

  // Transforms required as 2D glyphs are generated in the x-y plane
  vtkTransformPolyDataFilter* TransformFilter;
  vtkTransform* Transform;
  vtkFloatArray* TemporaryHandlePoints;

  void AppendHandles(double*);
  void ResetHandles();
  void AllocateHandles(const int&);
  void AdjustHandlePosition(const int&, double*);
  int HighlightHandle(vtkProp*); // returns handle index or -1 on fail
  void EraseHandle(const int&);
  void SizeHandles() override;
  void InsertHandleOnLine(double*);

  int NumberOfHandles;
  vtkActor* CurrentHandle;
  int CurrentHandleIndex;

  vtkProp* ViewProp;         // the prop we want to pick on
  vtkPropPicker* PropPicker; // the prop's picker

  // Representation of the line
  vtkPoints* LinePoints;
  vtkCellArray* LineCells;
  vtkActor* LineActor;
  vtkPolyData* LineData;
  vtkIdType CurrentPoints[2];

  void HighlightLine(const int&);
  void BuildLinesFromHandles();
  void ResetLine(double*);
  void AppendLine(double*);
  int PickCount;

  // Do the picking of the handles and the lines
  vtkCellPicker* HandlePicker;
  vtkCellPicker* LinePicker;
  vtkAbstractPropPicker* CurrentPicker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty* HandleProperty;
  vtkProperty* SelectedHandleProperty;
  vtkProperty* LineProperty;
  vtkProperty* SelectedLineProperty;
  void CreateDefaultProperties();

  // Enable/Disable mouse button events
  vtkTypeBool HandleLeftMouseButton;
  vtkTypeBool HandleMiddleMouseButton;
  vtkTypeBool HandleRightMouseButton;

private:
  vtkImageTracerWidget(const vtkImageTracerWidget&) = delete;
  void operator=(const vtkImageTracerWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
