/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHandleRepresentation - abstract class for representing widget handles
// .SECTION Description
// This class defines an API for widget handle representations. These
// representations interact with vtkHandleWidget. Various representations
// can be used depending on the nature of the handle. The basic functionality 
// of the handle representation is to maintain a position. The position is
// represented via a vtkCoordinate, meaning that the position can be easily
// obtained in a variety of coordinate systems.
//
// Optional features for this representation include an active mode (the widget
// appears only when the mouse pointer is close to it). The active distance is
// expressed in pixels and represents a circle in display space.
//
// The class may be subclassed so that alternative representations can
// be created.  The class defines an API and a default implementation that
// the vtkHandleWidget interacts with to render itself in the scene.

// .SECTION Caveats
// The separation of the widget event handling and representation enables
// users and developers to create new appearances for the widget. It also
// facilitates parallel processing, where the client application handles
// events, and remote representations of the widget are slaves to the 
// client (and do not handle events).

// .SECTION See Also
// vtkRectilinearWipeWidget vtkWidgetRepresentation vtkAbstractWidget


#ifndef __vtkHandleRepresentation_h
#define __vtkHandleRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkCoordinate;

class VTK_WIDGETS_EXPORT vtkHandleRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkHandleRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Handles usually have their coordinates set in display coordinates
  // (generally by an associated widget) and internally maintain the position
  // in world coordinates. (Using world coordinates insures that handles are
  // rendered in the right position when the camera view changes.) These
  // methods are often subclassed because special constraint operations can
  // be used to control the actual positioning.
  virtual void SetDisplayPosition(double pos[3]);
  virtual void GetDisplayPosition(double pos[3]);
  virtual void SetWorldPosition(double pos[3]);
  virtual void GetWorldPosition(double pos[3]);

  // Description:
  // The tolerance representing the distance to the widget (in pixels)
  // in which the cursor is considered near enough to the widget to
  // be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // Flag controls whether the widget becomes visible when the mouse pointer
  // moves close to it (i.e., the widget becomes active). By default,
  // ActiveRepresentation is off and the representation is always visible.
  vtkSetMacro(ActiveRepresentation,int);
  vtkGetMacro(ActiveRepresentation,int);
  vtkBooleanMacro(ActiveRepresentation,int);

//BTX
  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget. Note that ComputeInteractionState() and several other methods
  // must be implemented by subclasses.
  enum _InteractionState { Outside=0, Nearby, Selecting, Translating, Scaling };
//ETX
  
  // Description:
  // The interaction state may be set from a widget (e.g., HandleWidget) 
  // or other object. This controls how the interaction with the
  // widget proceeds.
  vtkSetMacro(InteractionState,int);

  // Description:
  // Specify whether any motions (such as scale, translate, etc.) are
  // constrained in some way (along an axis, etc.) Widgets can use this
  // to control the resulting motion.
  vtkSetMacro(Constrained,int);
  vtkGetMacro(Constrained,int);
  vtkBooleanMacro(Constrained,int);

  // Description:
  // Methods to make this class properly act like a vtkWidgetRepresentation.
  virtual void ShallowCopy(vtkProp *prop);
  virtual void SetRenderer(vtkRenderer *ren);

protected:
  vtkHandleRepresentation();
  ~vtkHandleRepresentation();

  int Tolerance;
  int ActiveRepresentation;
  int Constrained;
  
  // Two vtkCoordinates are available to subclasses, one in display
  // coordinates and the other in world coordinates. These facilitate
  // the conversion between these two systems. Note that the WorldPosition
  // is the ultimate maintainer of position.
  vtkCoordinate *DisplayPosition;
  vtkCoordinate *WorldPosition;
  
  // Keep track of when coordinates were changed
  vtkTimeStamp DisplayPositionTime;
  vtkTimeStamp WorldPositionTime;

private:
  vtkHandleRepresentation(const vtkHandleRepresentation&);  //Not implemented
  void operator=(const vtkHandleRepresentation&);  //Not implemented
};

#endif
