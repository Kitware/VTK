/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLineWidget
 * @brief   3D widget for manipulating a line
 *
 * This 3D widget defines a line that can be interactively placed in a
 * scene. The line has two handles (at its endpoints), plus the line can be
 * picked to translate it in the scene.  A nice feature of the object is that
 * the vtkLineWidget, like any 3D widget, will work with the current
 * interactor style and any other widgets present in the scene. That is, if
 * vtkLineWidget does not handle an event, then all other registered
 * observers (including the interactor style) have an opportunity to process
 * the event. Otherwise, the vtkLineWidget will terminate the processing of
 * the event that it handles.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a vtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. The interactor will act
 * normally until the "i" key (for "interactor") is pressed, at which point
 * the vtkLineWidget will appear. (See superclass documentation for
 * information about changing this behavior.) By grabbing one of the two end
 * point handles (use the left mouse button), the line can be oriented and
 * stretched (the other end point remains fixed). By grabbing the line
 * itself, or using the middle mouse button, the entire line can be
 * translated.  Scaling (about the center of the line) is achieved by using
 * the right mouse button. By moving the mouse "up" the render window the
 * line will be made bigger; by moving "down" the render window the widget
 * will be made smaller. Turn off the widget by pressing the "i" key again
 * (or invoke the Off() method). (Note: picking the line or either one of the
 * two end point handles causes a vtkPointWidget to appear.  This widget has
 * the ability to constrain motion to an axis by pressing the "shift" key
 * while moving the mouse.)
 *
 * The vtkLineWidget has several methods that can be used in conjunction with
 * other VTK objects. The Set/GetResolution() methods control the number of
 * subdivisions of the line; the GetPolyData() method can be used to get the
 * polygonal representation and can be used for things like seeding
 * streamlines. Typical usage of the widget is to make use of the
 * StartInteractionEvent, InteractionEvent, and EndInteractionEvent
 * events. The InteractionEvent is called on mouse motion; the other two
 * events are called on button down and button up (either left or right
 * button).
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You can set the properties of the selected and
 * unselected representations of the line. For example, you can set the
 * property for the handles and line. In addition there are methods to
 * constrain the line so that it is aligned along the x-y-z axes.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkPlaneWidget
*/

#ifndef vtkLineWidget_h
#define vtkLineWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtk3DWidget.h"
#include "vtkLineSource.h" // For passing calls to it

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkCellPicker;
class vtkPointWidget;
class vtkPWCallback;
class vtkPW1Callback;
class vtkPW2Callback;

class VTKINTERACTIONWIDGETS_EXPORT vtkLineWidget : public vtk3DWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkLineWidget *New();

  vtkTypeMacro(vtkLineWidget,vtk3DWidget);
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

  /**
   * Set/Get the resolution (number of subdivisions) of the line.
   */
  void SetResolution(int r)
    { this->LineSource->SetResolution(r); }
  int GetResolution()
    { return this->LineSource->GetResolution(); }

  /**
   * Set/Get the position of first end point.
   */
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double x[3])
    {this->SetPoint1(x[0], x[1], x[2]); }
  double* GetPoint1()
    {return this->LineSource->GetPoint1();}
  void GetPoint1(double xyz[3])
    {this->LineSource->GetPoint1(xyz);}

  /**
   * Set position of other end point.
   */
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double x[3])
    {this->SetPoint2(x[0], x[1], x[2]);}
  double* GetPoint2()
    {return this->LineSource->GetPoint2();}
  void GetPoint2(double xyz[3])
    {this->LineSource->GetPoint2(xyz);}

  //@{
  /**
   * Force the line widget to be aligned with one of the x-y-z axes.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the line to the axes if it is originally
   * not aligned.
   */
  vtkSetClampMacro(Align, int, XAxis, None);
  vtkGetMacro(Align, int);
  void SetAlignToXAxis() { this->SetAlign(XAxis); }
  void SetAlignToYAxis() { this->SetAlign(YAxis); }
  void SetAlignToZAxis() { this->SetAlign(ZAxis); }
  void SetAlignToNone()  { this->SetAlign(None); }
  //@}

  //@{
  /**
   * Enable/disable clamping of the point end points to the bounding box
   * of the data. The bounding box is defined from the last PlaceWidget()
   * invocation, and includes the effect of the PlaceFactor which is used
   * to gram/shrink the bounding box.
   */
  vtkSetMacro(ClampToBounds,int);
  vtkGetMacro(ClampToBounds,int);
  vtkBooleanMacro(ClampToBounds,int);
  //@}

  /**
   * Grab the polydata (including points) that defines the line.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * line. These point values are guaranteed to be up-to-date when either the
   * InteractionEvent or EndInteraction events are invoked. The user provides
   * the vtkPolyData and the points and polyline are added to it.
   */
  void GetPolyData(vtkPolyData *pd);

  //@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles when selected and normal can be
   * manipulated.
   */
  vtkGetObjectMacro(HandleProperty,vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty,vtkProperty);
  //@}

protected:
  vtkLineWidget();
  ~vtkLineWidget() VTK_OVERRIDE;

  // Manage the state of the widget
  friend class vtkPWCallback;

  int State;
  enum WidgetState
  {
    Start=0,
    MovingHandle,
    MovingLine,
    Scaling,
    Outside
  };

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
  virtual void OnMouseMove();

  // controlling ivars
  int Align;

  enum AlignmentState {
    XAxis,
    YAxis,
    ZAxis,
    None
  };

  // the line
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource     *LineSource;
  void HighlightLine(int highlight);

  // glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;

  void BuildRepresentation();
  void SizeHandles() VTK_OVERRIDE;
  void HandlesOn(double length);
  void HandlesOff();
  int HighlightHandle(vtkProp *prop); //returns cell id
  void HighlightHandles(int highlight);

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkActor *CurrentHandle;
  double LastPosition[3];
  void  SetLinePosition(double x[3]);

  // Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  // Methods to manipulate the hexahedron.
  void Scale(double *p1, double *p2, int X, int Y);

  // Initial bounds
  int   ClampToBounds;
  void  ClampPosition(double x[3]);
  int   InBounds(double x[3]);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void CreateDefaultProperties();

  void GenerateLine();

  // Methods for managing the point widgets used to control the endpoints
  vtkPointWidget *PointWidget;
  vtkPointWidget *PointWidget1;
  vtkPointWidget *PointWidget2;
  vtkPWCallback  *PWCallback;
  vtkPW1Callback *PW1Callback;
  vtkPW2Callback *PW2Callback;
  vtkPointWidget *CurrentPointWidget;
  void EnablePointWidget();
  void DisablePointWidget();
  int ForwardEvent(unsigned long event);

private:
  vtkLineWidget(const vtkLineWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLineWidget&) VTK_DELETE_FUNCTION;
};

#endif
