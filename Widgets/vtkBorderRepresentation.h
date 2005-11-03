/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBorderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBorderRepresentation - represent a vtkBorderWidget
// .SECTION Description
// This class is used to represent and render a vtBorderWidget. To
// use this class, you need to specify the two corners of a rectangular
// region.
//
// The class may be subclassed so that alternative representations can
// be created.  The class defines an API and a default implementation that
// the vtkBorderRepresentation interacts with to render itself in the scene.

// .SECTION Caveats
// The separation of the widget event handling and representation enables
// users and developers to create new appearances for the widget. It also
// facilitates parallel processing, where the client application handles
// events, and remote representations of the widget are slaves to the 
// client (and do not handle events).

// .SECTION See Also
// vtkBorderWidget


#ifndef __vtkBorderRepresentation_h
#define __vtkBorderRepresentation_h

#include "vtkWidgetRepresentation.h"
#include "vtkCoordinate.h" //Because of the viewport coordinate macro

class vtkPoints;
class vtkPolyData;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkProperty2D;


class VTK_WIDGETS_EXPORT vtkBorderRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkBorderRepresentation *New();

  // Description:
  // Define standard methods.
  vtkTypeRevisionMacro(vtkBorderRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify opposite corners of the box defining the boundary of the
  // widget. By default, these coordinates are in the normalized viewport
  // coordinate system, with Position the lower left of the outline, and
  // Position2 relative to Position.
  vtkViewportCoordinateMacro(Position);
  vtkViewportCoordinateMacro(Position2);

//BTX
  enum {BORDER_OFF=0,BORDER_ON,BORDER_ACTIVE};
//ETX
  // Description:
  // Specify when and if the border should appear. If ShowBorder is "on",
  // then the border will always appear. If ShowBorder is "off" then the
  // border will never appear.  If ShowBorder is "active" then the border
  // will appear when the mouse pointer enters the region bounded by the
  // border widget.
  vtkSetClampMacro(ShowBorder,int,BORDER_OFF,BORDER_ACTIVE);
  vtkGetMacro(ShowBorder,int);
  void SetShowBorderToOff() {this->SetShowBorder(BORDER_OFF);}
  void SetShowBorderToOn() {this->SetShowBorder(BORDER_ON);}
  void SetShowBorderToActive() {this->SetShowBorder(BORDER_ACTIVE);}
  
  // Description:
  // Specify the properties of the border.
  vtkGetObjectMacro(BorderProperty,vtkProperty2D);

  // Description:
  // Indicate whether resizing operations should keep the x-y directions
  // proportional to one another.
  vtkSetMacro(ProportionalResize,int);
  vtkGetMacro(ProportionalResize,int);
  vtkBooleanMacro(ProportionalResize,int);

  // Description:
  // The tolerance representing the distance to the widget (in pixels)
  // in which the cursor is considered to be on the widget, or on a
  // widget feature (e.g., a corner point or edge).
  vtkSetClampMacro(Tolerance,int,1,10);
  vtkGetMacro(Tolerance,int);

  // Description:
  // After a selection event within the region interior to the border; the
  // normalized selection coordinates may be obtained.
  vtkGetVectorMacro(SelectionPoint,double,2);

  // Description:
  // This is a modifier of the interaction state. When set, widget interaction
  // allows the border (and stuff inside of it) to be translated with mouse
  // motion.
  vtkSetMacro(Moving,int);
  vtkGetMacro(Moving,int);
  vtkBooleanMacro(Moving,int);

//BTX
  // Description:
  // Define the various states that the representation can be in.
  enum _InteractionState
  {
    Outside=0,
    Inside,
    AdjustingP0,
    AdjustingP1,
    AdjustingP2,
    AdjustingP3,
    AdjustingE0,
    AdjustingE1,
    AdjustingE2,
    AdjustingE3
  };
//ETX

  // Description:
  // Subclasses should implement these methods. See the superclasses'
  // documentation for more information.
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual void GetSize(double size[2])
    {size[0]=1.0; size[1]=1.0;}
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  
  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentGeometry(vtkViewport*);

protected:
  vtkBorderRepresentation();
  ~vtkBorderRepresentation();

  // Ivars
  int           ShowBorder;
  vtkProperty2D *BorderProperty;
  int           ProportionalResize;
  int           Tolerance;
  int           Moving;
  double        SelectionPoint[2];
  
  // Layout
  vtkCoordinate *PositionCoordinate;
  vtkCoordinate *Position2Coordinate;

  // Sometimes subclasses must negotiate with their superclasses
  // to achieve the correct layout.
  int Negotiated;
  virtual void NegotiateLayout();

  // Keep track of start position when moving border
  double StartPosition[2]; 

  // Border representation. Subclasses may use the BWTransform class
  // to transform their geometry into the region surrounded by the border.
  vtkPoints                  *BWPoints;
  vtkPolyData                *BWPolyData;
  vtkTransform               *BWTransform;
  vtkTransformPolyDataFilter *BWTransformFilter;
  vtkPolyDataMapper2D        *BWMapper;
  vtkActor2D                 *BWActor;

private:
  vtkBorderRepresentation(const vtkBorderRepresentation&);  //Not implemented
  void operator=(const vtkBorderRepresentation&);  //Not implemented
};

#endif
