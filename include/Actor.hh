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
// .NAME vlActor - an entity in a rendering scene
//
// .SECTION Description
// vlActor is used to represent an entity in a rendering scene.  It handles
// functions related to the actors position, orientation and scaling. It
// combines these instance variables into one matrix as follows: [x y z 1]
// = [x y z 1] Translate(-origin) Scale(scale) Rot(y) Rot(x) Rot (z) 
// Trans(origin) Trans(position).
//
// The actor also maintains the front and back facing properties.  

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

  void Render(vlRenderer *ren);


  // Description:
  // Get the position of the actor.
  vlGetVectorMacro(Position,float);
  // Description:
  // Sets the posiiton of the actor.
  vlSetVector3Macro(Position,float);

  // Description:
  // Get the origin of the actor. This is the point about which all 
  // rotations take place.
  vlGetVectorMacro(Origin,float);
  // Description:
  // Set the origin of the actor. This is the point about which all 
  // rotations take place.
  vlSetVector3Macro(Origin,float);

  // Description:
  // Get the scale of the actor. Scaling in performed independently on the
  // X,Y and Z axis.
  vlGetVectorMacro(Scale,float);
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
  void SetMapper(vlMapper *m);
  vlMapper *GetMapper();
  vlProperty *Property; 
  char *GetClassName() {return "vlActor";};
  void PrintSelf(ostream& os, vlIndent indent);

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

