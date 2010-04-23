/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConstrainedPointHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConstrainedPointHandleRepresentation - point representation constrained to a 2D plane
// .SECTION Description
// This class is used to represent a vtkHandleWidget. It represents a
// position in 3D world coordinates that is constrained to a specified plane.
// The default look is to draw a white point when this widget is not selected
// or active, a thin green circle when it is highlighted, and a thicker cyan
// circle when it is active (being positioned). Defaults can be adjusted - but
// take care to define cursor geometry that makes sense for this widget.
// The geometry will be aligned on the constraining plane, with the plane
// normal aligned with the X axis of the geometry (similar behavior to
// vtkGlyph3D). 
//
// TODO: still need to work on 
// 1) translation when mouse is outside bounding planes
// 2) size of the widget
//
// .SECTION See Also
// vtkHandleRepresentation vtkHandleWidget


#ifndef __vtkConstrainedPointHandleRepresentation_h
#define __vtkConstrainedPointHandleRepresentation_h

#include "vtkHandleRepresentation.h"

class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkGlyph3D;
class vtkPoints;
class vtkPolyData;
class vtkPlane;
class vtkPlaneCollection;
class vtkPlanes;
class vtkRenderer;

class VTK_WIDGETS_EXPORT vtkConstrainedPointHandleRepresentation : public vtkHandleRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkConstrainedPointHandleRepresentation *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkConstrainedPointHandleRepresentation,vtkHandleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the cursor shape. Keep in mind that the shape will be
  // aligned with the  constraining plane by orienting it such that
  // the x axis of the geometry lies along the normal of the plane.
  void SetCursorShape(vtkPolyData *cursorShape);
  vtkPolyData *GetCursorShape();

  // Description:
  // Specify the shape of the cursor (handle) when it is active.
  // This is the geometry that will be used when the mouse is
  // close to the handle or if the user is manipulating the handle.
  void SetActiveCursorShape(vtkPolyData *activeShape);
  vtkPolyData *GetActiveCursorShape();

  // Description:
  // Set the projection normal to lie along the x, y, or z axis,
  // or to be oblique. If it is oblique, then the plane is 
  // defined in the ObliquePlane ivar.
  vtkSetClampMacro(ProjectionNormal,int,
                   vtkConstrainedPointHandleRepresentation::XAxis,
                   vtkConstrainedPointHandleRepresentation::Oblique);
  vtkGetMacro(ProjectionNormal,int);
 
  void SetProjectionNormalToXAxis()
    { this->SetProjectionNormal(vtkConstrainedPointHandleRepresentation::XAxis); }
  void SetProjectionNormalToYAxis()
    { this->SetProjectionNormal(vtkConstrainedPointHandleRepresentation::YAxis); }
  void SetProjectionNormalToZAxis()
    { this->SetProjectionNormal(vtkConstrainedPointHandleRepresentation::ZAxis); }
  void SetProjectionNormalToOblique()
    { this->SetProjectionNormal(vtkConstrainedPointHandleRepresentation::Oblique); }

  // Description:
  // If the ProjectionNormal is set to Oblique, then this is the 
  // oblique plane used to constrain the handle position
  void SetObliquePlane(vtkPlane *);
  vtkGetObjectMacro(ObliquePlane, vtkPlane);

  // Description:
  // The position of the bounding plane from the origin along the
  // normal. The origin and normal are defined in the oblique plane
  // when the ProjectionNormal is Oblique. For the X, Y, and Z
  // axes projection normals, the normal is the axis direction, and
  // the origin is (0,0,0).
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);

  // Description:
  // A collection of plane equations used to bound the position of the point.
  // This is in addition to confining the point to a plane - these contraints
  // are meant to, for example, keep a point within the extent of an image.
  // Using a set of plane equations allows for more complex bounds (such as
  // bounding a point to an oblique reliced image that has hexagonal shape)
  // than a simple extent.
  void AddBoundingPlane(vtkPlane *plane);
  void RemoveBoundingPlane(vtkPlane *plane);
  void RemoveAllBoundingPlanes();
  virtual void SetBoundingPlanes(vtkPlaneCollection*);
  vtkGetObjectMacro(BoundingPlanes,vtkPlaneCollection);
  void SetBoundingPlanes(vtkPlanes *planes);
  
  // Description:
  // Overridden from the base class. It converts the display
  // co-ordinates to world co-ordinates. It returns 1 if the point lies
  // within the constrained region, otherwise return 0
  virtual int CheckConstraint(vtkRenderer *renderer, double pos[2]);

  // Description:
  // Set/Get the position of the point in display coordinates.  These are
  // convenience methods that extend the superclasses' GetHandlePosition()
  // method. Note that only the x-y coordinate values are used
  void SetPosition(double x, double y, double z);
  void SetPosition(double xyz[3]);
  double* GetPosition();
  void GetPosition(double xyz[3]);
  
  // Description:
  // This is the property used when the handle is not active 
  // (the mouse is not near the handle)
  vtkGetObjectMacro(Property,vtkProperty);
  
  // Description:
  // This is the property used when the mouse is near the
  // handle (but the user is not yet interacting with it)
  vtkGetObjectMacro(SelectedProperty,vtkProperty);
  
  // Description:
  // This is the property used when the user is interacting
  // with the handle.
  vtkGetObjectMacro(ActiveProperty,vtkProperty);
  
  // Description:
  // Subclasses of vtkConstrainedPointHandleRepresentation must implement these methods. These
  // are the methods that the widget and its representation use to
  // communicate with each other.
  virtual void SetRenderer(vtkRenderer *ren);
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modify);

  // Description:
  // Method overridden from Superclass. computes the world
  // co-ordinates using GetIntersectionPosition()
  virtual void SetDisplayPosition(double pos[3]);

  // Description:
  // Methods to make this class behave as a vtkProp.
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();
  virtual void ShallowCopy(vtkProp* prop);

//BTX
  enum {XAxis=0,YAxis,ZAxis,Oblique};
//ETX
  
protected:
  vtkConstrainedPointHandleRepresentation();
  ~vtkConstrainedPointHandleRepresentation();

  // Render the cursor
  vtkActor             *Actor;
  vtkPolyDataMapper    *Mapper;
  vtkGlyph3D           *Glypher;
  vtkPolyData          *CursorShape;
  vtkPolyData          *ActiveCursorShape;
  vtkPolyData          *FocalData;
  vtkPoints            *FocalPoint;

  // Support picking
  double LastPickPosition[3];
  double LastEventPosition[2];
  
  // Methods to manipulate the cursor
  void Translate(double eventPos[2]);
  void Scale(double eventPos[2]);
  void Highlight(int highlight);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty   *Property;
  vtkProperty   *SelectedProperty;
  vtkProperty   *ActiveProperty;
  void           CreateDefaultProperties();
  
  // Controlling vars
  int             ProjectionNormal;
  double          ProjectionPosition;
  int             ProjectToPlane;
  vtkPlane        *ObliquePlane;

  vtkPlaneCollection *BoundingPlanes;

  // Internal method for computing 3D location from 2D screen position
  int GetIntersectionPosition( double eventPos[2],
                               double worldPos[3],
                               double tolerance = 0.0,
                               vtkRenderer *renderer=0);

  // Internal method for getting the project normal as a vector
  void GetProjectionNormal( double normal[3] );
  
  // Internal method for getting the origin of the 
  // constraining plane as a 3-tuple
  void GetProjectionOrigin( double origin[3] );
  
  
  
  // Distance between where the mouse event happens and where the
  // widget is focused - maintain this distance during interaction.
  double InteractionOffset[2];
  
private:
  vtkConstrainedPointHandleRepresentation(const vtkConstrainedPointHandleRepresentation&);  //Not implemented
  void operator=(const vtkConstrainedPointHandleRepresentation&);  //Not implemented
};

#endif
