/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePlaneWidget.h
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
// .NAME vtkImagePlaneWidget - 3D widget for reslicing image data
// .SECTION Description
// This 3D widget defines a plane that can be interactively placed in an
// image volume. A nice feature of the object is that the
// vtkImagePlaneWidget, like any 3D widget, will work with the current
// interactor style. That is, if vtkImagePlaneWidget does not handle an
// event, then all other registered observers (including the interactor
// style) have an opportunity to process the event. Otherwise, the
// vtkImagePlaneWidget will terminate the processing of the event that it
// handles.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. If the "i" key (for
// "interactor") is pressed, the vtkImagePlaneWidget will appear. (See
// superclass documentation for information about changing this behavior.)
// Selecting any part of the widget with the left or middle mouse button
// enables translation of the plane along its normal. (Once selected using
// middle mouse, moving "up" in the middle moves the plane in the direction
// of the normal; moving "down" moves it in the opposite direction.)
// Window-level is achieved by using the right mouse button.  Events that
// occur outside of the widget (i.e., no part of the widget is picked) are
// propagated to any other registered obsevers (such as the interaction
// style).  Turn off the widget by pressing the "i" key again (or invoke the
// Off() method).
//
// The vtkImagePlaneWidget has several methods that can be used in
// conjunction with other VTK objects. The GetPolyData() method can be used
// to get the polygonal representation and can be used for things like
// seeding stream lines. Typical usage of the widget is to make use of the
// StartInteractionEvent, InteractionEvent, and EndInteractionEvent
// events. The InteractionEvent is called on mouse motion; the other two
// events are called on button down and button up (either left or right
// button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of the selected and
// unselected representations of the plane's outline. In addition there are methods
// to constrain the plane so that it is aligned along the x-y-z axes.

// .SECTION Thanks
// Thanks to Dean Inglis for developing and contributing this class.
// Based on the Python SlicePlaneFactory from Atamai, Inc.

// .SECTION Caveats
// Note that handles and plane can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget  vtkPlaneWidget

#ifndef __vtkImagePlaneWidget_h
#define __vtkImagePlaneWidget_h

#include "vtk3DWidget.h"
#include "vtkPlaneSource.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkCellPicker;
class vtkTexure;
class vtkMatrix4x4;
class vtkTextureMapToPlane;
class vtkImageReslice;
class vtkDataSetMapper;
class vtkLookupTable;
class vtkImageMapToColors;
class vtkImageData;

#define VTK_NEAREST_RESLICE 0
#define VTK_LINEAR_RESLICE  1
#define VTK_CUBIC_RESLICE   2

class VTK_EXPORT vtkImagePlaneWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkImagePlaneWidget *New();

  vtkTypeRevisionMacro(vtkImagePlaneWidget,vtk3DWidget);
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
  // Set the image data and generate a default texture plane.  
  void SetInput(vtkImageData* input)
    {this->Superclass::SetInput(input);
     this->ImageData = input;
     this->GenerateTexturePlane();
    }

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
  // Get the center of the plane.
  float* GetCenter() 
    {return this->PlaneSource->GetCenter();}
  void GetCenter(float xyz[3]) 
    {this->PlaneSource->GetCenter(xyz);}

  // Description:
  // Get the normal to the plane.
  float* GetNormal() 
    {return this->PlaneSource->GetNormal();}
  void GetNormal(float xyz[3]) 
    {this->PlaneSource->GetNormal(xyz);}

  // Description:
  // Get the slice position in terms of the data extent.
  int GetSliceIndex();

  // Description:
  // Set the slice position in terms of the data extent.
  void SetSliceIndex(int index);

  // Description:
  // Get the position of the slice along its normal.
  float GetSlicePosition();

  // Description:
  // Set the position of the slice along its normal.
  void SetSlicePosition(float position);

  // Description:
  // Set the interpolation to use when texturing the plane.
  vtkGetMacro(ResliceInterpolate,int);
  void SetResliceInterpolateToNearestNeighbour();
  void SetResliceInterpolateToLinear();
  void SetResliceInterpolateToCubic();

  // Description:
  // Make sure that the plane remains within the volume.
  vtkSetMacro(RestrictPlaneToVolume,int);
  vtkGetMacro(RestrictPlaneToVolume,int);
  vtkBooleanMacro(RestrictPlaneToVolume,int);

  // Description:
  // Specify whether to interpolate the texture or not.
  vtkSetMacro(TextureInterpolate,int);
  vtkGetMacro(TextureInterpolate,int);
  vtkBooleanMacro(TextureInterpolate,int);

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
  // Get the plane properties. The properties of the plane when selected
  // and unselected can be manipulated.
  vtkSetObjectMacro(PlaneProperty,vtkProperty);
  vtkGetObjectMacro(PlaneProperty,vtkProperty);

  // Description:
  // Convenience method sets the plane orientation normal to the
  // x, y, or z axes.
  void SetPlaneOrientation(int);
  vtkGetMacro(PlaneOrientation,int);
  void SetPlaneOrientationToXAxes()
    { this->SetPlaneOrientation(0); }
  void SetPlaneOrientationToYAxes()
    { this->SetPlaneOrientation(1); }
  void SetPlaneOrientationToZAxes()
    { this->SetPlaneOrientation(2); }

  // Description:
  // Set the internal picker to one defined by the user.  In this way,
  // a set of three orthogonal planes can share the same picker so that
  // picking is performed correctly.
  void SetPicker(vtkCellPicker*);


protected:
  vtkImagePlaneWidget();
  ~vtkImagePlaneWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    WindowLevelling,
    Pushing,
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
  int   PlaneOrientation;
  int   RestrictPlaneToVolume;
  float OriginalWindow;
  float OriginalLevel;
  int   ResliceInterpolate;
  int   TextureInterpolate;
  int   UserPickerEnabled;

  // the plane
  vtkActor          *PlaneActor;
  vtkPolyDataMapper *PlaneMapper;
  vtkPlaneSource    *PlaneSource;
  vtkPolyData       *PlaneOutline;
  void HighlightPlane(int highlight);

  void PositionHandles();

  // Do the picking
  vtkCellPicker *PlanePicker;

  // Methods to manipulate the hexahedron.
  void WindowLevel(int X, int Y);
  void Push(double *p1, double *p2);

  // Plane normal, normalized
  float Normal[3];

  vtkTransform         *DummyTransform;
  vtkMatrix4x4         *ResliceAxes;
  vtkTextureMapToPlane *TexturePlaneCoords;
  vtkImageReslice      *Reslice;
  vtkDataSetMapper     *TexturePlaneMapper;
  vtkActor             *TexturePlaneActor;
  vtkLookupTable       *LookupTable;
  vtkImageMapToColors  *ColorMap;
  vtkTexture           *Texture;
  vtkImageData         *ImageData;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *PlaneProperty;
  vtkProperty *SelectedPlaneProperty;
  void CreateDefaultProperties();

  void GeneratePlane();
  void UpdateNormal();
  void UpdateOrigin();
  void GenerateTexturePlane();
  void SetRepresentation();

private:
  vtkImagePlaneWidget(const vtkImagePlaneWidget&);  //Not implemented
  void operator=(const vtkImagePlaneWidget&);  //Not implemented
};

#endif
