/*=========================================================================

  Program:   Visualization Library
  Module:    Actor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlActor - an entity in a rendered image
// .SECTION Description
// vlActor is used to represent an entity in a rendering scene.  It handles
// functions related to the actors position, orientation and scaling. It
// combines these instance variables into one matrix as follows: [x y z 1]
// = [x y z 1] Translate(-origin) Scale(scale) Rot(y) Rot(x) Rot (z) 
// Trans(origin) Trans(position).
//
// The actor also maintains a reference to the defining geometry (i.e., the 
// mapper), and rendering properties. 

#ifndef __vlActor_hh
#define __vlActor_hh

#include "Object.hh"
#include "Property.hh"
#include "Mapper.hh"
#include "Trans.hh"

class vlRenderer;

class vlActor : public vlObject
{
 public:
  vlActor();
  ~vlActor();
  char *GetClassName() {return "vlActor";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Render(vlRenderer *ren);

  // Description:
  // Specify the property object to control rendering surface properties.
  vlSetObjectMacro(Property,vlProperty);
  // Description:
  // Get the property object that controls rendering surface properties.
  vlGetObjectMacro(Property,vlProperty);

  // Description:
  // This is the method that is used to connect an actor to the end of a
  // visualization pipeline, i.e. the Mapper.  
  vlSetObjectMacro(Mapper,vlMapper);
  // Description:
  // Returns the Mapper that this actor is getting it's data from.
  vlGetObjectMacro(Mapper,vlMapper);

  // Description:
  // Get the position of the actor.
  vlGetVectorMacro(Position,float,3);
  // Description:
  // Sets the posiiton of the actor.
  vlSetVector3Macro(Position,float);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float delatX, float deltaY, float deltaZ);

  // Description:
  // Get the origin of the actor. This is the point about which all 
  // rotations take place.
  vlGetVectorMacro(Origin,float,3);
  // Description:
  // Set the origin of the actor. This is the point about which all 
  // rotations take place.
  vlSetVector3Macro(Origin,float);

  // Description:
  // Get the scale of the actor. Scaling in performed independently on the
  // X,Y and Z axis.
  vlGetVectorMacro(Scale,float,3);
  // Description:
  // Set the scale of the actor. Scaling in performed independently on the
  // X,Y and Z axis.
  vlSetVector3Macro(Scale,float);

  // Description:
  // Get the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vlGetMacro(Visibility,int);
  // Description:
  // Set the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vlSetMacro(Visibility,int);
  // Description:
  // Set the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vlBooleanMacro(Visibility,int);

  // Description:
  // Get the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vlGetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vlSetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vlBooleanMacro(Pickable,int);

  // Description:
  // Get the value of the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlGetMacro(Dragable,int);
  // Description:
  // Set the value of the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlSetMacro(Dragable,int);
  // Description:
  // Set the value of the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlBooleanMacro(Dragable,int);

  vlMatrix4x4 GetMatrix();

  float *GetBounds();
  float *GetXRange();
  float *GetYRange();
  float *GetZRange();

  void RotateX(float);
  void RotateY(float);
  void RotateZ(float);
  void RotateWXYZ(float,float,float,float);

  void SetOrientation(float,float,float);
  void SetOrientation(float a[3]);
  float *GetOrientation();
  void AddOrientation(float,float,float);
  void AddOrientation(float a[3]);

protected:
  vlProperty *Property; 
  vlMapper *Mapper;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
  vlTransform Transform;
  float Bounds[6];
};

#endif

