/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Actor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkActor - an entity in a rendered image
// .SECTION Description
// vtkActor is used to represent an entity in a rendering scene.  It handles
// functions related to the actors position, orientation and scaling. It
// combines these instance variables into one matrix as follows: [x y z 1]
// = [x y z 1] Translate(-origin) Scale(scale) Rot(y) Rot(x) Rot (z) 
// Trans(origin) Trans(position).
//
// The actor also maintains a reference to the defining geometry (i.e., the 
// mapper), and rendering properties. 

#ifndef __vtkActor_hh
#define __vtkActor_hh

#include "Object.hh"
#include "Property.hh"
#include "Texture.hh"
#include "Mapper.hh"
#include "Trans.hh"

class vtkRenderer;

class vtkActor : public vtkObject
{
 public:
  vtkActor();
  ~vtkActor();
  char *GetClassName() {return "vtkActor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Render(vtkRenderer *ren);

  // Description:
  // Specify the property object to control rendering surface properties.
  void SetProperty(vtkProperty *lut);
  void SetProperty(vtkProperty& lut) {this->SetProperty(&lut);};
  // Description:
  // Get the property object that controls rendering surface properties.
  vtkProperty *GetProperty();

  // Description:
  // Specify the Texture object to control rendering texture.
  vtkSetObjectMacro(Texture,vtkTexture);
  // Description:
  // Get the Texture object that controls rendering texture.
  vtkGetObjectMacro(Texture,vtkTexture);

  // Description:
  // This is the method that is used to connect an actor to the end of a
  // visualization pipeline, i.e. the Mapper.  
  vtkSetObjectMacro(Mapper,vtkMapper);
  // Description:
  // Returns the Mapper that this actor is getting it's data from.
  vtkGetObjectMacro(Mapper,vtkMapper);

  // Description:
  // Set a user defined matrix to concatenate with.  
  vtkSetObjectMacro(UserMatrix,vtkMatrix4x4);
  // Description:
  // Returns the user defined transformation matrix.
  vtkGetObjectMacro(UserMatrix,vtkMatrix4x4);

  // Description:
  // Get the position of the actor.
  vtkGetVectorMacro(Position,float,3);
  // Description:
  // Sets the posiiton of the actor.
  vtkSetVector3Macro(Position,float);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Get the origin of the actor. This is the point about which all 
  // rotations take place.
  vtkGetVectorMacro(Origin,float,3);
  // Description:
  // Set the origin of the actor. This is the point about which all 
  // rotations take place.
  vtkSetVector3Macro(Origin,float);

  // Description:
  // Get the scale of the actor. Scaling in performed independently on the
  // X,Y and Z axis.
  vtkGetVectorMacro(Scale,float,3);
  // Description:
  // Set the scale of the actor. Scaling in performed independently on the
  // X,Y and Z axis.
  vtkSetVector3Macro(Scale,float);

  // Description:
  // Get the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vtkGetMacro(Visibility,int);
  // Description:
  // Set the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vtkSetMacro(Visibility,int);
  // Description:
  // Set the visibility of the actor. Visibility is like a light switch
  // for actors. Use it to turn them on or off.
  vtkBooleanMacro(Visibility,int);

  // Description:
  // Get the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vtkGetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vtkSetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the actor can 
  // be picked (typically using the mouse). Also see dragable.
  vtkBooleanMacro(Pickable,int);

  // Description:
  // Get the value of the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkGetMacro(Dragable,int);
  // Description:
  // Set the value of the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkSetMacro(Dragable,int);
  // Description:
  // Turn on/off the dragable instance variable. This determines if 
  // an actor once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkBooleanMacro(Dragable,int);

  vtkMatrix4x4& GetMatrix();
  virtual void GetMatrix(vtkMatrix4x4& m);

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
  vtkMatrix4x4 *UserMatrix;
  vtkProperty *Property; 
  vtkTexture *Texture; 
  vtkMapper *Mapper;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
  vtkTransform Transform;
  float Bounds[6];
  int SelfCreatedProperty;
};

#endif

