/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Volume.hh
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
// .NAME vtkVolume - an entity in a rendered image
// .SECTION Description
// vtkVolume is used to represent a volume entity in a rendering scene.  
// It handles functions related to the Volumes position, orientation and 
// scaling. It combines these instance variables into one matrix as 
// follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale) Rot(y) 
// Rot(x) Rot (z) Trans(origin) Trans(position).
//

#ifndef __vtkVolume_hh
#define __vtkVolume_hh

#include "Object.hh"
#include "Trans.hh"
#include "StrPts.hh"
#include "Lut.hh"

class vtkRenderer;

class vtkVolume : public vtkObject
{
 public:
  vtkVolume();
  ~vtkVolume();
  char *GetClassName() {return "vtkVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the method that is used to connect an Volume to the end of a
  // visualization pipeline.
  vtkSetObjectMacro(Input,vtkStructuredPoints);
  // Description:
  // Returns the Input that this Volume is getting it's data from.
  vtkGetObjectMacro(Input,vtkStructuredPoints);

  // Description:
  // Sets the Look up Table for this volume.
  void SetLookupTable(vtkLookupTable *lut);
  void SetLookupTable(vtkLookupTable& lut) {this->SetLookupTable(&lut);};
  // Description:
  // Sets the Look up Table for this volume.
  vtkLookupTable *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Specify range in terms of (smin,smax) through which to map scalars
  // into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Get the position of the Volume.
  vtkGetVectorMacro(Position,float,3);
  // Description:
  // Sets the posiiton of the Volume.
  vtkSetVector3Macro(Position,float);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Get the origin of the Volume. This is the point about which all 
  // rotations take place.
  vtkGetVectorMacro(Origin,float,3);
  // Description:
  // Set the origin of the Volume. This is the point about which all 
  // rotations take place.
  vtkSetVector3Macro(Origin,float);

  // Description:
  // Get the scale of the Volume. Scaling in performed independently on the
  // X,Y and Z axis.
  vtkGetVectorMacro(Scale,float,3);
  // Description:
  // Set the scale of the Volume. Scaling in performed independently on the
  // X,Y and Z axis.
  vtkSetVector3Macro(Scale,float);

  // Description:
  // Get the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vtkGetMacro(Visibility,int);
  // Description:
  // Set the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vtkSetMacro(Visibility,int);
  // Description:
  // Set the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vtkBooleanMacro(Visibility,int);

  // Description:
  // Get the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vtkGetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vtkSetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vtkBooleanMacro(Pickable,int);

  // Description:
  // Get the value of the dragable instance variable. This determines if 
  // an Volume once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkGetMacro(Dragable,int);
  // Description:
  // Set the value of the dragable instance variable. This determines if 
  // an Volume once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkSetMacro(Dragable,int);
  // Description:
  // Turn on/off the dragable instance variable. This determines if 
  // an Volume  once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkBooleanMacro(Dragable,int);

  vtkMatrix4x4& GetMatrix();
  void GetMatrix(vtkMatrix4x4& m);

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

  // Description:
  // Builds the lookuptable and Input.
  void Render();

protected:
  vtkStructuredPoints *Input;
  vtkLookupTable *LookupTable;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
  vtkTransform Transform;
  float Bounds[6];
  float ScalarRange[2];
  int SelfCreatedLookupTable;
};

#endif

