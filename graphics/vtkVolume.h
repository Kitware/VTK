/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.h
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
// .NAME vtkVolume - a volumetric entity in a rendered image
// .SECTION Description
// vtkVolume is used to represent a volume entity in a rendering scene.  
// It handles functions related to the volume's position, orientation, and 
// scaling. It combines these instance variables into one matrix as 
// follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale) Rot(y) 
// Rot(x) Rot (z) Trans(origin) Trans(position).

// .SECTION see also
// vtkActor vtkVolumeCollection vtkVolumeRenderer

#ifndef __vtkVolume_h
#define __vtkVolume_h

#include "vtkObject.h"
#include "vtkTransform.h"
#include "vtkStructuredPoints.h"
#include "vtkLookupTable.h"

class vtkRenderer;

class vtkVolume : public vtkObject
{
 public:
  vtkVolume();
  ~vtkVolume();
  char *GetClassName() {return "vtkVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the method that is used to connect a volume to the end of a
  // visualization pipeline.
  vtkSetObjectMacro(Input,vtkStructuredPoints);

  // Description:
  // Returns the input that this volume is getting its data from.
  vtkGetObjectMacro(Input,vtkStructuredPoints);

  // Description:
  // Set/Get the Look Up Table for this volume.
  void SetLookupTable(vtkLookupTable *lut);
  void SetLookupTable(vtkLookupTable& lut) {this->SetLookupTable(&lut);};
  vtkLookupTable *GetLookupTable();

  // Description:
  // Create a default lookup table. Generally used to create one when 
  // one wasn't specified by the user.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Specify range in terms of (smin,smax) through which to map scalars
  // into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Set/Get/Add the position of the volume.
  vtkGetVectorMacro(Position,float,3);
  vtkSetVector3Macro(Position,float);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Set/Get the origin of the volume. This is the point about which all 
  // rotations take place.
  vtkGetVectorMacro(Origin,float,3);
  vtkSetVector3Macro(Origin,float);

  // Description:
  // Set/Get the scale of the volume. Scaling in performed independently on the
  // X,Y and Z axis. Any scale values that are zero will be automatically
  // converted to one.
  vtkGetVectorMacro(Scale,float,3);
  vtkSetVector3Macro(Scale,float);

  // Description:
  // Set/Get the visibility of the volume. Visibility is like a light switch:
  // use it to turn them on or off.
  vtkGetMacro(Visibility,int);
  vtkSetMacro(Visibility,int);
  vtkBooleanMacro(Visibility,int);

  // Description:
  // Set/Get the pickable instance variable.  This determines if the
  // Volume can be picked (typically using the mouse). Also see
  // dragable.
  vtkGetMacro(Pickable,int);
  vtkSetMacro(Pickable,int);
  vtkBooleanMacro(Pickable,int);

  // Description:
  // Set/Get the value of the dragable instance variable. This determines if 
  // a volume, once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vtkGetMacro(Dragable,int);
  vtkSetMacro(Dragable,int);
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

