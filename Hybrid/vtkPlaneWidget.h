/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneWidget.h
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
// .NAME vtkPlaneWidget - 3D widget for manipulating a plane
// .SECTION Description


// .SECTION Caveats
// Note that handles and line can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget


#ifndef __vtkPlaneWidget_h
#define __vtkPlaneWidget_h

#include "vtk3DWidget.h"
#include "vtkPlaneSource.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkLineSource;
class vtkConeSource;
class vtkSphereSource;
class vtkCellPicker;

class VTK_EXPORT vtkPlaneWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkPlaneWidget *New();

  vtkTypeRevisionMacro(vtkPlaneWidget,vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods that satisfy the superclass' API.
  virtual void SetEnabled(int);
  virtual void PlaceWidget(float bounds[6]);

  // Description:
  // Set/Get the resolution (number of subdivisions) of the plane.
  void SetResolution(int r)
    { this->PlaneSource->SetXResolution(r); this->PlaneSource->SetYResolution(r); }
  int GetResolution()
    { return this->PlaneSource->GetXResolution(); }

  // Description:
  // Set/Get the origin of the plane.
  void SetOrigin(float x, float y, float z) 
    {this->PlaneSource->SetOrigin(x,y,z);}
  void SetOrigin(float x[3]) 
    {this->PlaneSource->SetOrigin(x);}
  float* GetOrigin() 
    {return this->PlaneSource->GetOrigin();}
  void GetOrigin(float xyz[3]) 
    {this->PlaneSource->GetOrigin(xyz);}

  // Description:
  // Set/Get the position of the point defining the first axis of the plane.
  void SetPoint1(float x, float y, float z) 
    {this->PlaneSource->SetPoint1(x,y,z);}
  void SetPoint1(float x[3]) 
    {this->PlaneSource->SetPoint1(x);}
  float* GetPoint1() 
    {return this->PlaneSource->GetPoint1();}
  void GetPoint1(float xyz[3]) 
    {this->PlaneSource->GetPoint1(xyz);}

  // Description:
  // Set/Get the position of the point defining the second axis of the plane.
  void SetPoint2(float x, float y, float z) 
    {this->PlaneSource->SetPoint2(x,y,z);}
  void SetPoint2(float x[3]) 
    {this->PlaneSource->SetPoint2(x);}
  float* GetPoint2() 
    {return this->PlaneSource->GetPoint2();}
  void GetPoint2(float xyz[3]) 
    {this->PlaneSource->GetPoint2(xyz);}

  // Description:
  // Force the plane widget to be aligned with one of the x-y-z axes.
  // Remember that when the state changes, a ModifiedEvent is invoked.
  // This can be used to snap the plane to the axes if it is orginally
  // not aligned.
  vtkSetMacro(NormalToXAxis,int);
  vtkGetMacro(NormalToXAxis,int);
  vtkBooleanMacro(NormalToXAxis,int);
  vtkSetMacro(NormalToYAxis,int);
  vtkGetMacro(NormalToYAxis,int);
  vtkBooleanMacro(NormalToYAxis,int);
  vtkSetMacro(NormalToZAxis,int);
  vtkGetMacro(NormalToZAxis,int);
  vtkBooleanMacro(NormalToZAxis,int);

  // Description:
  // Grab the polydata (including points) that defines the plane.  The
  // polydata consists of (res+1)*(res+1) points, and res*res quadrilateral
  // polygons, where res is the resolution of the plane. These point values
  // are guaranteed to be up-to-date when either the InteractionEvent or
  // EndInteraction events are invoked. The user provides the vtkPolyData and
  // the points and polyplane are added to it.
  void GetPolyData(vtkPolyData *pd)
    { pd->ShallowCopy(this->PlaneSource->GetOutput()); }

  // Description:
  // Get the handle properties (the little balls are the handles). The 
  // properties of the handles when selected and normal can be 
  // manipulated.
  vtkGetObjectMacro(HandleProperty,vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);
  
  // Description:
  // Get the plane properties. The properties of the plane when selected 
  // and unselected can be manipulated.
  vtkSetObjectMacro(PlaneProperty,vtkProperty);
  vtkGetObjectMacro(PlaneProperty,vtkProperty);
  
protected:
  vtkPlaneWidget();
  ~vtkPlaneWidget();

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
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  void OnLeftButtonUp(int ctrl, int shift, int X, int Y);
  void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  void OnMiddleButtonUp(int ctrl, int shift, int X, int Y);
  void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  void OnRightButtonUp(int ctrl, int shift, int X, int Y);
  void OnMouseMove(int ctrl, int shift, int X, int Y);

  // controlling ivars
  int NormalToXAxis;
  int NormalToYAxis;
  int NormalToZAxis;

  // the plane
  vtkActor          *PlaneActor;
  vtkPolyDataMapper *PlaneMapper;
  vtkPlaneSource     *PlaneSource;
  void HighlightPlane(int highlight);

  // glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;
  void PositionHandles();
  void HandlesOn(double length);
  void HandlesOff();
  int HighlightHandle(vtkProp *prop); //returns cell id
  
  // the normal cone
  vtkActor *ConeActor;
  vtkPolyDataMapper *ConeMapper;
  vtkConeSource *ConeSource;
  void HighlightNormal(int highlight);

  // the normal line
  vtkActor *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource *LineSource;

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *PlanePicker;
  vtkActor *CurrentHandle;
  
  // Methods to manipulate the hexahedron.
  void MovePoint1(double *p1, double *p2);
  void MovePoint2(double *p1, double *p2);
  void MovePoint3(double *p1, double *p2);
  void MovePoint4(double *p1, double *p2);
  void Rotate(int X, int Y, double *p1, double *p2, double *vpn);
  void Scale(double *p1, double *p2, int X, int Y);
  void Translate(double *p1, double *p2);
  
  // Initial bounds
  float InitialBounds[6];
  float InitialLength;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *PlaneProperty;
  vtkProperty *SelectedPlaneProperty;
  void CreateDefaultProperties();
  
  void GeneratePlane();
  
private:
  vtkPlaneWidget(const vtkPlaneWidget&);  //Not implemented
  void operator=(const vtkPlaneWidget&);  //Not implemented
};

#endif
