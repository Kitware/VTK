/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereWidget.h
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
// .NAME vtkSphereWidget - 3D widget for manipulating a sphere
// .SECTION Description
// This 3D widget defines a sphere that can be interactively placed in a
// scene. The widget can be used to transform a vtkProp3D, produce a
// polygonal representation of a sphere, and/or generate a vtkSphere implicit
// function (used for clipping, cutting, extracting, etc.)  To use this
// object, just invoke SetInteractor() with the argument of the method a
// vtkRenderWindowInteractor.  You may also wish to invoke "PlaceWidget()" to
// initially position the widget. The interactor will act normally until the
// "i" key (for "interactor") is pressed, at which point the vtkSphereWidget
// will appear. (See superclass documentation for information about changing
// this behavior.)  Events that occur outside of the widget (i.e., no part of
// the widget is picked) are propagated to any other registered obsevers
// (such as the interaction style).  Turn off the widget by pressing the "i"
// key again (or invoke the Off() method).
//
// vtkSphereWidget consists of a sphere, which may be shown in wireframe,
// surface, or even not shown at all. In addition, there is an optional
// handle (a small sphere) that can be moved around on the surface of the
// sphere. (The sphere handle can be turned off. Often the handle is enabled
// when you want to position something like a light; the handle is typically
// turned off when you want to use the vtkSpherePosition strictly perform
// transformation of an underlying object.)
// 
// vtkSphereWidget responds to left, middle, and right mouse button events.
// Selecting the sphere with the left button produces rotations.
// Shift-left-button enables translation, as does the middle mouse button.
// The right mouse button allows you to scale the sphere (moving the mouse
// pointer "up" scales the sphere up; moving down scales the sphere
// down). Selecting the handle with the left mouse button allows you to move
// the handle across the surface of the sphere. (Note: there are instance
// variables that can be set to disable/enable the handle, rotation, scaling,
// and translation.)
//
// The vtkSphereWidget has several methods that can be used in conjunction
// with other VTK objects. The Set/GetThetaResolution() and
// Set/GetPhiResolution() methods control the number of subdivisions of the
// sphere in the theta and phi directions; the GetPolyData() method can be
// used to get the polygonal representation and can be used for things like
// seeding streamlines. The GetSphere() method returns a sphere implicit
// function that can be used for cutting and clipping. GetTransform()
// produces a vtkSphere implicit function. Typical usage of the widget is to
// make use of the StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent events. The InteractionEvent is called on mouse
// motion; the other two events are called on button down and button up (any
// mouse button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of the selected and
// unselected representations of the sphere and handle.

// .SECTION Caveats
// Note that the sphere can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkPointWidget vtkLineWidget vtkPlaneWidget vtkBoxWidget 
// vtkImagePlaneWidget vtkImplicitPlaneWidget


#ifndef __vtkSphereWidget_h
#define __vtkSphereWidget_h

#include "vtk3DWidget.h"
#include "vtkSphereSource.h" // Needed for faster access to the sphere source

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkSphereSource;
class vtkSphere;
class vtkCellPicker;
class vtkProperty;
class vtkTransform;
class vtkTransformPolyDataFilter;

#define VTK_SPHERE_OFF 0
#define VTK_SPHERE_WIREFRAME 1
#define VTK_SPHERE_SURFACE 2

class VTK_HYBRID_EXPORT vtkSphereWidget : public vtk3DWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkSphereWidget *New();

  vtkTypeRevisionMacro(vtkSphereWidget,vtk3DWidget);
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
  // Set the representation of the sphere. Different representations are
  // useful depending on the application.
  vtkSetClampMacro(Representation,int,VTK_SPHERE_OFF,VTK_SPHERE_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToOff()
    { this->SetRepresentation(VTK_SPHERE_OFF);}
  void SetRepresentationToWireframe()
    { this->SetRepresentation(VTK_SPHERE_WIREFRAME);}
  void SetRepresentationToSurface()
    { this->SetRepresentation(VTK_SPHERE_SURFACE);}

  // Description:
  // Set/Get the resolution of the sphere in the Theta direction.
  void SetThetaResolution(int r)
    { this->SphereSource->SetThetaResolution(r); }
  int GetThetaResolution()
    { return this->SphereSource->GetThetaResolution(); }

  // Description:
  // Set/Get the resolution of the sphere in the Phi direction.
  void SetPhiResolution(int r)
    { this->SphereSource->SetPhiResolution(r); }
  int GetPhiResolution()
    { return this->SphereSource->GetPhiResolution(); }

  // Description:
  // Set/Get the radius of sphere. Default is .5.
  void SetRadius(float r);
  float GetRadius();

  // Description:
  // Set/Get the center of the sphere.
  void SetCenter(float x, float y, float z);
  void SetCenter(float x[3]) 
    { this->SetCenter(x[0], x[1], x[2]); }
  float *GetCenter()
    { return this->SphereCenter; }
  void GetCenter(float xyz[3]);

  // Description:
  // Enable translation, scaling, and or rotation of the widget. By default, 
  // the widget can be translated, scaled and rotated.
  vtkSetMacro(Translation,int);
  vtkGetMacro(Translation,int);
  vtkBooleanMacro(Translation,int);
  vtkSetMacro(Scale,int);
  vtkGetMacro(Scale,int);
  vtkBooleanMacro(Scale,int);
  vtkSetMacro(Rotation,int);
  vtkGetMacro(Rotation,int);
  vtkBooleanMacro(Rotation,int);

  // Description:
  // The handle sits on the surface of the sphere and may be moved around
  // the surface by picking (left mouse) and then moving. The position
  // of the handle can be retrieved, this is useful for positioning cameras
  // and lights. By default, the handle is turned off.
  vtkSetMacro(HandleVisibility,int);
  vtkGetMacro(HandleVisibility,int);
  vtkBooleanMacro(HandleVisibility,int);

  // Description:
  // Set/Get the direction vector of the handle relative to the center of
  // the sphere.
  vtkSetVector3Macro(HandleDirection,float);
  vtkGetVector3Macro(HandleDirection,float);

  // Description:
  // Get the position of the handle.
  vtkGetVector3Macro(HandlePosition,float);
  
  // Description:
  // Retrieve a linear transform characterizing the transformation of the
  // sphere. Note that the transformation is relative to where PlaceWidget
  // was initially called. This method modifies the transform provided. The
  // transform can be used to control the position of vtkProp3D's, as well as
  // other transformation operations (e.g., vtkTranformPolyData).
  void GetTransform(vtkTransform *t);

  // Description:
  // Grab the polydata (including points) that defines the sphere.  The
  // polydata consists of n+1 points, where n is the resolution of the
  // sphere. These point values are guaranteed to be up-to-date when either
  // the InteractionEvent or EndInteraction events are invoked. The user
  // provides the vtkPolyData and the points and polysphere are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Get the spherical implicit function defined by this widget.  Note that
  // vtkPlanes is a subclass of vtkImplicitFunction, meaning that it can be
  // used by a variety of filters to perform clipping, cutting, and selection
  // of data.
  void GetSphere(vtkSphere *sphere);

  // Description:
  // Get the sphere properties. The properties of the sphere when selected 
  // and unselected can be manipulated.
  vtkGetObjectMacro(SphereProperty,vtkProperty);
  vtkGetObjectMacro(SelectedSphereProperty,vtkProperty);
  
  // Description:
  // Get the handle properties (the little ball on the sphere is the
  // handle). The properties of the handle when selected and unselected
  // can be  manipulated.
  vtkGetObjectMacro(HandleProperty,vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);
  
protected:
  vtkSphereWidget();
  ~vtkSphereWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Translating,
    Scaling,
    Rotating,
    Positioning,
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

  // The transformation used to position the sphere and handle
  vtkTransform               *SphereTransform;
  vtkTransformPolyDataFilter *TransformSphereFilter;
  vtkTransform               *HandleTransform;
  vtkTransformPolyDataFilter *TransformHandleFilter;

  // the sphere
  vtkSphereSource     *SphereSource;
  vtkPolyDataMapper   *SphereMapper;
  vtkActor            *SphereActor;
  void HighlightSphere(int highlight);

  // Managing the handle
  vtkSphereSource   *HandleSource;
  vtkPolyDataMapper *HandleMapper;
  vtkActor          *HandleActor;
  void HighlightHandle(int);
  float HandleDirection[3];
  float HandlePosition[3];

  // The representation of the sphere
  int Representation;

  // Do the picking
  vtkCellPicker *SpherePicker;
  
  // Booleans to control the behavior of the widget
  int HandleVisibility;
  int Translation;
  int Scale;
  int Rotation;

  // Methods to manipulate the sphere widget
  void Rotate(int X, int Y, double *p1, double *p2, double *vpn);
  void Translate(double *p1, double *p2);
  void ScaleSphere(double *p1, double *p2, int X, int Y);
  void MoveHandle(double *p1, double *p2, int X, int Y);
  void PlaceHandle(float *center, float radius);
  
  // Methods to control the transformation of the sphere
  float SphereScale[3];
  float SphereCenter[3];
  float SphereOrientation[3];
  float HandleScale[3];
  float HandleCenter[3];
  void  BuildRepresentation();
    
  // Initial bounds
  float InitialBounds[6];
  float InitialLength;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *SphereProperty;
  vtkProperty *SelectedSphereProperty;
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  void CreateDefaultProperties();

private:
  vtkSphereWidget(const vtkSphereWidget&);  //Not implemented
  void operator=(const vtkSphereWidget&);  //Not implemented
};

#endif
