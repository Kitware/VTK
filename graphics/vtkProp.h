/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// vtkProp is an abstract class used to represent an entity in a rendering scene.
// It handles functions related to the position, orientation and
// scaling. It combines these instance variables into one 4x4
// transformation matrix as follows: [x y z 1] = [x y z 1]
// Translate(-origin) Scale(scale) Rot(y) Rot(x) Rot (z) Trans(origin)
// Trans(position). Both vtkActor and vtkVolume are specializations of class
// vtkProp. 


// .SECTION See Also
// vtkActor vtkVolume


#ifndef __vtkProp_h
#define __vtkProp_h

#include "vtkObject.h"
#include "vtkTransform.h"

class vtkRenderer;

class VTK_EXPORT vtkProp : public vtkObject
{
 public:
  vtkProp();
  char *GetClassName() {return "vtkProp";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Render(vtkRenderer *ren) = 0;

  vtkProp &operator=(const vtkProp& Prop);

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
  // Set/Get the value of the dragable instance variable. This determines if 
  // an Prop, once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition, which will continue
  // to work.  It is just intended to prevent some Props from being
  // dragged from within a user interface.
  vtkSetMacro(Dragable,int);
  vtkGetMacro(Dragable,int);
  vtkBooleanMacro(Dragable,int);

  vtkMatrix4x4& GetMatrix();
  virtual void GetMatrix(vtkMatrix4x4& m) = 0;

  virtual float *GetBounds() = 0;
  void GetBounds(float bounds[6]);
  float *GetCenter();
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
  float *GetOrientationWXYZ();
  void AddOrientation(float,float,float);
  void AddOrientation(float a[3]);

protected:
  vtkMatrix4x4 *UserMatrix;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Center[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
  vtkTransform Transform;
  float Bounds[6];
};

#endif

