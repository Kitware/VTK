/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineWidget.h
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
// .NAME vtkLineWidget - 3D widget for manipulating a line
// .SECTION Description
// This 3D widget defines a line that can be interactively placed in a
// scene. The line has two handles (at its endpoints), plus it can be picked
// on the line itself to translate it in the scene.  A nice feature of the
// object is that the vtkLineWidget, like any 3D widget, will work with the
// current interactor style. That is, if vtkLineWidget does not handle an
// event, then all other registered observers (including the interactor
// style) have an opportunity to process the event. Otherwise, the
// vtkLineWidget will terminate the processing of the event that it handles.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. The interactor will act
// normally until the "i" key (for "interactor") is pressed, at which point the
// vtkLineWidget will appear. (See superclass documentation for information
// about changing this behavior.) By grabbing the one of the two end point
// handles (use the left mouse button), the line can be oriented and
// stretched (the other end point remains fixed). By grabbing the line
// itself, the entire line can be translated. (Translation can also be
// employed by using the "shift-left-mouse-button" combination inside of the
// widget.) Scaling (about the center of the line) is achieved by using the
// right mouse button. By moving the mouse "up" the render window the line
// will be made bigger; by moving "down" the render window the widget will be
// made smaller. Events that occur outside of the widget (i.e., no part of
// the widget is picked) are propagated to any other registered obsevers
// (such as the interaction style).  Turn off the widget by pressing the "i"
// key again (or invoke the Off() method).
//
// The vtkLineWidget has several methods that can be used in conjunction with
// other VTK objects. The Set/GetResolution() methods control the number of
// subdivisions of the line; the GetPolyData() method can be used to get the
// polygonal representation and can be used for things like seeding
// streamlines. Typical usage of the widget is to make use of the
// StartInteractionEvent, InteractionEvent, and EndInteractionEvent
// events. The InteractionEvent is called on mouse motion; the other two
// events are called on button down and button up (either left or right
// button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of the selected and
// unselected representations of the line. For example, you can set the
// property for the handles and line. In addition there are methods to
// constrain the line so that it is aligned along the x-y-z axes.

// .SECTION Caveats
// Note that handles and line can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkPlaneWidget


#ifndef __vtkLineWidget_h
#define __vtkLineWidget_h

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

class VTK_HYBRID_EXPORT vtkLineWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkLineWidget *New();

  vtkTypeRevisionMacro(vtkLineWidget,vtk3DWidget);
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
  // Set/Get the resolution (number of subdivisions) of the line.
  void SetResolution(int r)
    { this->LineSource->SetResolution(r); }
  int GetResolution()
    { return this->LineSource->GetResolution(); }

  // Description:
  // Set/Get the position of first end point.
  void SetPoint1(float x, float y, float z) 
    {this->LineSource->SetPoint1(x,y,z); this->PositionHandles();}
  void SetPoint1(float x[3]) 
    {this->SetPoint1(x[0], x[1], x[2]); }
  float* GetPoint1() 
    {return this->LineSource->GetPoint1();}
  void GetPoint1(float xyz[3]) 
    {this->LineSource->GetPoint1(xyz);}

  // Description:
  // Set position of other end point.
  void SetPoint2(float x, float y, float z) 
    {this->LineSource->SetPoint2(x,y,z); this->PositionHandles();}
  void SetPoint2(float x[3]) 
    {this->SetPoint2(x[0], x[1], x[2]);}
  float* GetPoint2() 
    {return this->LineSource->GetPoint2();}
  void GetPoint2(float xyz[3]) 
    {this->LineSource->GetPoint2(xyz);}

  // Description:
  // Force the line widget to be aligned with one of the x-y-z axes.
  // Remember that when the state changes, a ModifiedEvent is invoked.
  // This can be used to snap the line to the axes if it is orginally
  // not aligned.
  vtkSetMacro(AlignWithXAxis,int);
  vtkGetMacro(AlignWithXAxis,int);
  vtkBooleanMacro(AlignWithXAxis,int);
  vtkSetMacro(AlignWithYAxis,int);
  vtkGetMacro(AlignWithYAxis,int);
  vtkBooleanMacro(AlignWithYAxis,int);
  vtkSetMacro(AlignWithZAxis,int);
  vtkGetMacro(AlignWithZAxis,int);
  vtkBooleanMacro(AlignWithZAxis,int);

  // Description:
  // Grab the polydata (including points) that defines the line.  The
  // polydata consists of n+1 points, where n is the resolution of the
  // line. These point values are guaranteed to be up-to-date when either the
  // InteractionEvent or EndInteraction events are invoked. The user provides
  // the vtkPolyData and the points and polyline are added to it.
  void GetPolyData(vtkPolyData *pd)
    { pd->ShallowCopy(this->LineSource->GetOutput()); }

  // Description:
  // Get the handle properties (the little balls are the handles). The 
  // properties of the handles when selected and normal can be 
  // manipulated.
  vtkGetObjectMacro(HandleProperty,vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);
  
  // Description:
  // Get the line properties. The properties of the line when selected 
  // and unselected can be manipulated.
  vtkGetObjectMacro(LineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty,vtkProperty);
  
protected:
  vtkLineWidget();
  ~vtkLineWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Moving,
    Scaling,
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

  // controlling ivars
  int AlignWithXAxis;
  int AlignWithYAxis;
  int AlignWithZAxis;

  // the line
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource     *LineSource;
  void HighlightLine(int highlight);

  // glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;
  void PositionHandles();
  void HandlesOn(double length);
  void HandlesOff();
  int HighlightHandle(vtkProp *prop); //returns cell id
  
  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *LinePicker;
  vtkActor *CurrentHandle;
  
  // Methods to manipulate the hexahedron.
  void MovePoint1(double *p1, double *p2);
  void MovePoint2(double *p1, double *p2);
  void Scale(double *p1, double *p2, int X, int Y);
  void Translate(double *p1, double *p2);
  
  // Initial bounds
  float InitialBounds[6];
  float InitialLength;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void CreateDefaultProperties();
  
  void GenerateLine();
  
private:
  vtkLineWidget(const vtkLineWidget&);  //Not implemented
  void operator=(const vtkLineWidget&);  //Not implemented
};

#endif
