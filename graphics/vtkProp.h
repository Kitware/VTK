/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkProp - represents an object for placement in a rendered scene 
// .SECTION Description
// vtkProp is an abstract class used to represent an entity in a rendering
// scene.  It handles functions related to the position, orientation and
// scaling. It combines these instance variables into one 4x4 transformation
// matrix as follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale)
// Rot(y) Rot(x) Rot (z) Trans(origin) Trans(position). Both vtkActor and
// vtkVolume are specializations of class vtkProp.  The constructor defaults
// to: origin(0,0,0) position=(0,0,0) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.

// .SECTION See Also
// vtkActor vtkAssembly vtkVolume

#ifndef __vtkProp_h
#define __vtkProp_h

#include "vtkObject.h"
#include "vtkTransform.h"

class vtkRenderer;

class VTK_EXPORT vtkProp : public vtkObject
{
 public:
  vtkProp();
  ~vtkProp();
  const char *GetClassName() {return "vtkProp";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // All concrete subclasses must implement a Render method.
  virtual void Render(vtkRenderer *ren) = 0;

  // Description:
  // Set/Get/Add the position of the Prop in world coordinates.
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Set/Get the origin of the Prop. This is the point about which all 
  // rotations take place.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set/Get the visibility of the Prop. Visibility is like a light switch
  // for Props. Use it to turn them on or off.
  vtkSetMacro(Visibility,int);
  vtkGetMacro(Visibility,int);
  vtkBooleanMacro(Visibility,int);

  // Description:
  // Set/Get the pickable instance variable.  This determines if the Prop can 
  // be picked (typically using the mouse). Also see dragable.
  vtkSetMacro(Pickable,int);
  vtkGetMacro(Pickable,int);
  vtkBooleanMacro(Pickable,int);

  // Description:
  // This method is invoked when an instance of vtkProp (or subclass, 
  // e.g., vtkActor) is picked by vtkPicker.
  void SetPickMethod(void (*f)(void *), void *arg);
  void SetPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Set/Get the value of the dragable instance variable. This determines if 
  // an Prop, once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition, which will continue
  // to work.  It is just intended to prevent some Props from being
  // dragged from within a user interface.
  vtkSetMacro(Dragable,int);
  vtkGetMacro(Dragable,int);
  vtkBooleanMacro(Dragable,int);

  // Description:
  // In addition to the instance variables such as position and orientation,
  // you can specify your own 4x4 transformation matrix that will
  // get concatenated with the actor's 4x4 matrix as determined
  // by the other instance variables. If the other instance variables such
  // as position and orientation are left with  their default values then 
  // they will result in the identity matrix. And the resulting matrix
  // will be the user defined matrix.
  vtkSetReferenceCountedObjectMacro(UserMatrix,vtkMatrix4x4);
  vtkGetObjectMacro(UserMatrix,vtkMatrix4x4);

  // Description:
  // Return a reference to the prop's 4x4 composite matrix.
  vtkMatrix4x4 *GetMatrixPointer();
  virtual void GetMatrix(vtkMatrix4x4 *m) = 0;

  // Description:
  // Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  void GetBounds(float bounds[6]);
  virtual float *GetBounds() = 0;
  
  // Description:
  // Get the center of the bounding box in world coordinates.
  float *GetCenter();

  // Description:
  // Get the prop's x range in world coordinates.
  float *GetXRange();

  // Description:
  // Get the prop's y range in world coordinates.
  float *GetYRange();

  // Description:
  // Get the prop's z range in world coordinates.
  float *GetZRange();

  // Description:
  // Get the length of the diagonal of the bounding box.
  float GetLength();

  // Description:
  // Rotate the prop in degrees about the X axis using the right hand
  // rule. The axis is the prop's X axis, which can change as other
  // rotations are performed.  To rotate about the world X axis use
  // RotateWXYZ (angle, 1, 0, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateX(float);

  // Description:
  // Rotate the prop in degrees about the Y axis using the right hand
  // rule. The axis is the prop's Y axis, which can change as other
  // rotations are performed.  To rotate about the world Y axis use
  // RotateWXYZ (angle, 0, 1, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateY(float);

  // Description:
  // Rotate the prop in degrees about the Z axis using the right hand
  // rule. The axis is the prop's Z axis, which can change as other
  // rotations are performed.  To rotate about the world Z axis use
  // RotateWXYZ (angle, 0, 0, 1). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateZ(float);

  // Description:
  // Rotate the prop in degrees about an arbitrary axis specified by
  // the last three arguments. The axis is specified in world
  // coordinates. To rotate an about its model axes, use RotateX,
  // RotateY, RotateZ.
  void RotateWXYZ(float,float,float,float);

  // Description:
  // Sets the orientation of the prop.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(float,float,float);

  // Description:
  // Sets the orientation of the prop.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(float a[3]);

  // Description:
  // Returns the orientation of the prop as s vector of X,Y and Z rotation.
  // The ordering in which these rotations must be done to generate the 
  // same matrix is RotateZ, RotateX, and finally RotateY. See also 
  // SetOrientation.
  float *GetOrientation();

  // Description:
  // Returns the WXYZ orientation of the prop. 
  float *GetOrientationWXYZ();

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(float,float,float);

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(float a[3]);

  // Description:
  // Method invokes PickMethod() if one defined
  virtual void Pick() {
    if (this->PickMethod) (*this->PickMethod)(this->PickMethodArg);};

  // Description:
  // For legacy compatability. Do not use.
  vtkProp &operator=(const vtkProp& Prop);
  vtkMatrix4x4& GetMatrix() {return *(this->GetMatrixPointer());}
  void GetMatrix(vtkMatrix4x4 &m) {this->GetMatrix(&m);}

protected:
  vtkMatrix4x4  *UserMatrix;
  vtkMatrix4x4  *Matrix;
  vtkTimeStamp  MatrixMTime;
  float         Origin[3];
  float         Position[3];
  float         Orientation[3];
  float         Center[3];
  int           Visibility;
  int           Pickable;
  void          (*PickMethod)(void *);
  void          (*PickMethodArgDelete)(void *);
  void          *PickMethodArg;
  int           Dragable;
  vtkTransform  *Transform;
  float         Bounds[6];
};

#endif

