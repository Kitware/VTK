/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSphereRepresentation - a class defining the representation for the vtkSphereWidget2
// .SECTION Description
// This class is a concrete representation for the vtkSphereWidget2. It
// represents a sphere with an optional handle.  Through interaction with the
// widget, the sphere can be arbitrarily positioned and scaled in 3D space;
// and the handle can be moved on the surface of the sphere. Typically the
// vtkSphereWidget2/vtkSphereRepresentation are used to position a sphere for
// the purpose of extracting, cutting or clipping data; or the handle is
// moved on the sphere to position a light or camera.
//
// To use this representation, you normally use the PlaceWidget() method
// to position the widget at a specified region in space. It is also possible
// to set the center of the sphere, a radius, and/or a handle position.
//
// .SECTION Caveats
// Note that the representation is overconstrained in that the center and radius
// of the sphere can be defined, this information plus the handle direction defines
// the geometry of the representation. Alternatively, the user may specify the center
// of the sphere plus the handle position.
//
// This class, and vtkSphereWidget2, are second generation VTK widgets. An
// earlier version of this functionality was defined in the class
// vtkSphereWidget.

// .SECTION See Also
// vtkSphereWidget2 vtkSphereWidget


#ifndef __vtkSphereRepresentation_h
#define __vtkSphereRepresentation_h

#include "vtkWidgetRepresentation.h"
#include "vtkSphereSource.h" // Needed for fast access to the sphere source

class vtkActor;
class vtkPolyDataMapper;
class vtkSphere;
class vtkSphereSource;
class vtkCellPicker;
class vtkProperty;
class vtkPolyData;
class vtkPoints;
class vtkPolyDataAlgorithm;
class vtkTransform;
class vtkDoubleArray;
class vtkMatrix4x4;
class vtkTextMapper;
class vtkActor2D;
class vtkTextProperty;
class vtkLineSource;

#define VTK_SPHERE_OFF 0
#define VTK_SPHERE_WIREFRAME 1
#define VTK_SPHERE_SURFACE 2

class VTK_WIDGETS_EXPORT vtkSphereRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkSphereRepresentation *New();

  // Description:
  // Standard methods for type information and to print out the contents of the class.
  vtkTypeMacro(vtkSphereRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX - used to manage the state of the widget
  enum {Outside=0,MovingHandle,OnSphere,Translating,Scaling};
//ETX

  // Description:
  // Set the representation (i.e., appearance) of the sphere. Different
  // representations are useful depending on the application. 
  vtkSetClampMacro(Representation,int,VTK_SPHERE_OFF,VTK_SPHERE_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToOff()
    { this->SetRepresentation(VTK_SPHERE_OFF);}
  void SetRepresentationToWireframe()
    { this->SetRepresentation(VTK_SPHERE_WIREFRAME);}
  void SetRepresentationToSurface()
    { this->SetRepresentation(VTK_SPHERE_SURFACE);}

  // Description:
  // Set/Get the resolution of the sphere in the theta direction.
  void SetThetaResolution(int r)
    { this->SphereSource->SetThetaResolution(r); }
  int GetThetaResolution()
    { return this->SphereSource->GetThetaResolution(); }

  // Description:
  // Set/Get the resolution of the sphere in the phi direction.
  void SetPhiResolution(int r)
    { this->SphereSource->SetPhiResolution(r); }
  int GetPhiResolution()
    { return this->SphereSource->GetPhiResolution(); }

  // Description:
  // Set/Get the center position of the sphere. Note that this may
  // adjust the direction from the handle to the center, as well as
  // the radius of the sphere.
  void SetCenter(double c[3]);
  void SetCenter(double x, double y, double z)
    {double c[3]; c[0]=x; c[1]=y; c[2]=z; this->SetCenter(c);}
  double* GetCenter() 
    {return this->SphereSource->GetCenter();}
  void GetCenter(double xyz[3]) 
    {this->SphereSource->GetCenter(xyz);}

  // Description:
  // Set/Get the radius of sphere. Default is 0.5. Note that this may
  // modify the position of the handle based on the handle direction.
  void SetRadius(double r);
  double GetRadius()
    { return this->SphereSource->GetRadius(); }

  // Description:
  // The handle sits on the surface of the sphere and may be moved around
  // the surface by picking (left mouse) and then moving. The position
  // of the handle can be retrieved, this is useful for positioning cameras
  // and lights. By default, the handle is turned off.
  vtkSetMacro(HandleVisibility,int);
  vtkGetMacro(HandleVisibility,int);
  vtkBooleanMacro(HandleVisibility,int);

  // Description:
  // Set/Get the position of the handle. Note that this may adjust the radius
  // of the sphere and the handle direction.
  void SetHandlePosition(double handle[3]);
  void SetHandlePosition(double x, double y, double z)
    {double p[3]; p[0]=x; p[1]=y; p[2]=z; this->SetHandlePosition(p);}
  vtkGetVector3Macro(HandlePosition,double);
  
  // Description:
  // Set/Get the direction vector of the handle relative to the center of
  // the sphere. This may affect the position of the handle and the radius
  // of the sphere.
  void SetHandleDirection(double dir[3]);
  void SetHandleDirection(double dx, double dy, double dz)
    {double d[3]; d[0]=dx; d[1]=dy; d[2]=dz; this->SetHandleDirection(d);}
  vtkGetVector3Macro(HandleDirection,double);

  // Description:
  // Enable/disable a label that displays the location of the handle in
  // spherical coordinates (radius,theta,phi). The two angles, theta and
  // phi, are displayed in degrees. Note that phi is measured from the
  // north pole down towards the equator; and theta is the angle around 
  // the north/south axis.
  vtkSetMacro(HandleText,int);
  vtkGetMacro(HandleText,int);
  vtkBooleanMacro(HandleText,int);

  // Description:
  // Enable/disable a radial line segment that joins the center of the
  // outer sphere and the handle.
  vtkSetMacro(RadialLine,int);
  vtkGetMacro(RadialLine,int);
  vtkBooleanMacro(RadialLine,int);

  // Description:
  // Grab the polydata (including points) that defines the sphere.  The
  // polydata consists of n+1 points, where n is the resolution of the
  // sphere. These point values are guaranteed to be up-to-date when either the
  // InteractionEvent or EndInteraction events are invoked. The user provides
  // the vtkPolyData and the points and polysphere are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Get the spherical implicit function defined by this widget.  Note that
  // vtkSphere is a subclass of vtkImplicitFunction, meaning that it can be
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
  
  // Description:
  // Get the handle text property. This can be used to control the appearance 
  // of the handle text.
  vtkGetObjectMacro(HandleTextProperty,vtkTextProperty);
  
  // Description:
  // Get the property of the radial line. This can be used to control the 
  // appearance of the optional line connecting the center to the handle.
  vtkGetObjectMacro(RadialLineProperty,vtkProperty);
  
  // Description:
  // The interaction state may be set from a widget (e.g., vtkSphereWidget2) or
  // other object. This controls how the interaction with the widget
  // proceeds. Normally this method is used as part of a handshaking
  // process with the widget: First ComputeInteractionState() is invoked that
  // returns a state based on geometric considerations (i.e., cursor near a
  // widget feature), then based on events, the widget may modify this
  // further.
  void SetInteractionState(int state);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API. Note that a 
  // version of place widget is available where the center and handle position
  // are specified.
  virtual void PlaceWidget(double bounds[6]);
  virtual void PlaceWidget(double center[3], double handlePosition[3]);
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();
  
  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();
  
protected:
  vtkSphereRepresentation();
  ~vtkSphereRepresentation();

  // Manage how the representation appears
  double LastEventPosition[3];
  
  // the sphere
  vtkActor            *SphereActor;
  vtkPolyDataMapper   *SphereMapper;
  vtkSphereSource     *SphereSource;
  void HighlightSphere(int highlight);

  // The representation of the sphere
  int Representation;

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *SpherePicker;
  double LastPickPosition[3];
  
  // Methods to manipulate the sphere widget
  void Translate(double *p1, double *p2);
  void Scale(double *p1, double *p2, int X, int Y);
  void PlaceHandle(double *center, double radius);
  virtual void SizeHandles();
  
  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *SphereProperty;
  vtkProperty *SelectedSphereProperty;
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  void CreateDefaultProperties();

  // Managing the handle
  vtkActor          *HandleActor;
  vtkPolyDataMapper *HandleMapper;
  vtkSphereSource   *HandleSource;
  void HighlightHandle(int);
  int HandleVisibility;
  double HandleDirection[3];
  double HandlePosition[3];

  // Manage the handle label
  int              HandleText;
  vtkTextProperty *HandleTextProperty;
  vtkTextMapper   *HandleTextMapper;
  vtkActor2D      *HandleTextActor;

  // Manage the radial line segment
  int RadialLine;
  vtkProperty       *RadialLineProperty;
  vtkLineSource     *RadialLineSource;
  vtkPolyDataMapper *RadialLineMapper;
  vtkActor          *RadialLineActor;
  
private:
  vtkSphereRepresentation(const vtkSphereRepresentation&);  //Not implemented
  void operator=(const vtkSphereRepresentation&);  //Not implemented
};

#endif
