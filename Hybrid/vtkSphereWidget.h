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
// scene. 
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. The interactor will act
// normally until the "i" key (for "interactor") is pressed, at which point the
// vtkSphereWidget will appear. (See superclass documentation for information
// about changing this behavior.) 
// Events that occur outside of the widget (i.e., no part of
// the widget is picked) are propagated to any other registered obsevers
// (such as the interaction style).  Turn off the widget by pressing the "i"
// key again (or invoke the Off() method).
//
// The vtkSphereWidget has several methods that can be used in conjunction
// with other VTK objects. The Set/GetThetaResolution() and
// Set/GetPhiResolution() methods control the number of subdivisions of the
// sphere in the theta and phi directions; the GetPolyData() method can be
// used to get the polygonal representation and can be used for things like
// seeding streamlines. The GetSphere() method returns a sphere implicit
// function that can be used for cutting and clipping. Typical usage of the
// widget is to make use of the StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent events. The InteractionEvent is called on mouse
// motion; the other two events are called on button down and button up
// (any mouse button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of the selected and
// unselected representations of the sphere. 

// .SECTION Caveats
// Note that the sphere can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkLineWidget vtkBoxWidget vtkPlaneWidget


#ifndef __vtkSphereWidget_h
#define __vtkSphereWidget_h

#include "vtk3DWidget.h"
#include "vtkSphereSource.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkSphereSource;
class vtkSphere;
class vtkCellPicker;

#define VTK_SPHERE_OFF 0
#define VTK_SPHERE_WIREFRAME 1
#define VTK_SPHERE_SURFACE 2

class VTK_EXPORT vtkSphereWidget : public vtk3DWidget
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
  void SetRadius(float r)
    { this->SphereSource->SetRadius(r); }
  float GetRadius()
    { return this->SphereSource->GetRadius(); }

  // Description:
  // Set/Get the center of the sphere.
  void SetCenter(float x, float y, float z) 
    {this->SphereSource->SetCenter(x,y,z);}
  void SetCenter(float x[3]) 
    {this->SphereSource->SetCenter(x);}
  float* GetCenter() 
    {return this->SphereSource->GetCenter();}
  void GetCenter(float xyz[3]) 
    {this->SphereSource->GetCenter(xyz);}

  // Description:
  // Grab the polydata (including points) that defines the sphere.  The
  // polydata consists of n+1 points, where n is the resolution of the
  // sphere. These point values are guaranteed to be up-to-date when either the
  // InteractionEvent or EndInteraction events are invoked. The user provides
  // the vtkPolyData and the points and polysphere are added to it.
  void GetPolyData(vtkPolyData *pd)
    { pd->ShallowCopy(this->SphereSource->GetOutput()); }

  // Description:
  // Get the spherical implicit function defined by this widget.  Note that
  // vtkPlanes is a subclass of vtkImplicitFunction, meaning that it can be
  // used by a variety of filters to perform clipping, cutting, and selection
  // of data.
  void GetSphere(vtkSphere *sphere);

  // Description:
  // Get the sphere properties. The properties of the sphere when selected 
  // and unselected can be manipulated.
  vtkSetObjectMacro(SphereProperty,vtkProperty);
  vtkGetObjectMacro(SphereProperty,vtkProperty);
  
protected:
  vtkSphereWidget();
  ~vtkSphereWidget();

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
  void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  void OnRightButtonUp(int ctrl, int shift, int X, int Y);
  void OnMouseMove(int ctrl, int shift, int X, int Y);

  // the sphere
  vtkActor          *SphereActor;
  vtkPolyDataMapper *SphereMapper;
  vtkSphereSource     *SphereSource;
  void HighlightSphere(int highlight);
  void SelectRepresentation();

  // The representation of the sphere
  int Representation;

  // Do the picking
  vtkCellPicker *SpherePicker;
  
  // Methods to manipulate the hexahedron.
  void Translate(double *p1, double *p2);
  void Scale(double *p1, double *p2, int X, int Y);
  
  // Initial bounds
  float InitialBounds[6];
  float InitialLength;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *SphereProperty;
  vtkProperty *SelectedSphereProperty;
  void CreateDefaultProperties();

  void GenerateSphere();
  
private:
  vtkSphereWidget(const vtkSphereWidget&);  //Not implemented
  void operator=(const vtkSphereWidget&);  //Not implemented
};

#endif
