/*=========================================================================

  Program:   Visualization Library
  Module:    Volume.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlVolume - an entity in a rendered image
// .SECTION Description
// vlVolume is used to represent a volume entity in a rendering scene.  
// It handles functions related to the Volumes position, orientation and 
// scaling. It combines these instance variables into one matrix as 
// follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale) Rot(y) 
// Rot(x) Rot (z) Trans(origin) Trans(position).
//

#ifndef __vlVolume_hh
#define __vlVolume_hh

#include "Object.hh"
#include "Trans.hh"
#include "StrPts.hh"
#include "Lut.hh"

class vlRenderer;

class vlVolume : public vlObject
{
 public:
  vlVolume();
  ~vlVolume();
  char *GetClassName() {return "vlVolume";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // This is the method that is used to connect an Volume to the end of a
  // visualization pipeline.
  vlSetObjectMacro(Input,vlStructuredPoints);
  // Description:
  // Returns the Input that this Volume is getting it's data from.
  vlGetObjectMacro(Input,vlStructuredPoints);

  // Description:
  // Sets the Look up Table for this volume.
  void SetLookupTable(vlLookupTable *lut);
  void SetLookupTable(vlLookupTable& lut) {this->SetLookupTable(&lut);};
  // Description:
  // Sets the Look up Table for this volume.
  vlLookupTable *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Specify range in terms of (smin,smax) through which to map scalars
  // into lookup table.
  vlSetVector2Macro(ScalarRange,float);
  vlGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Get the position of the Volume.
  vlGetVectorMacro(Position,float,3);
  // Description:
  // Sets the posiiton of the Volume.
  vlSetVector3Macro(Position,float);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Get the origin of the Volume. This is the point about which all 
  // rotations take place.
  vlGetVectorMacro(Origin,float,3);
  // Description:
  // Set the origin of the Volume. This is the point about which all 
  // rotations take place.
  vlSetVector3Macro(Origin,float);

  // Description:
  // Get the scale of the Volume. Scaling in performed independently on the
  // X,Y and Z axis.
  vlGetVectorMacro(Scale,float,3);
  // Description:
  // Set the scale of the Volume. Scaling in performed independently on the
  // X,Y and Z axis.
  vlSetVector3Macro(Scale,float);

  // Description:
  // Get the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vlGetMacro(Visibility,int);
  // Description:
  // Set the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vlSetMacro(Visibility,int);
  // Description:
  // Set the visibility of the Volume. Visibility is like a light switch
  // for Volumes. Use it to turn them on or off.
  vlBooleanMacro(Visibility,int);

  // Description:
  // Get the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vlGetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vlSetMacro(Pickable,int);
  // Description:
  // Set the pickable instance variable.  This determines if the Volume can 
  // be picked (typically using the mouse). Also see dragable.
  vlBooleanMacro(Pickable,int);

  // Description:
  // Get the value of the dragable instance variable. This determines if 
  // an Volume once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlGetMacro(Dragable,int);
  // Description:
  // Set the value of the dragable instance variable. This determines if 
  // an Volume once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlSetMacro(Dragable,int);
  // Description:
  // Turn on/off the dragable instance variable. This determines if 
  // an Volume  once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition.
  vlBooleanMacro(Dragable,int);

  vlMatrix4x4& GetMatrix();
  void GetMatrix(vlMatrix4x4& m);

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
  vlStructuredPoints *Input;
  vlLookupTable *LookupTable;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
  vlTransform Transform;
  float Bounds[6];
  float ScalarRange[2];
  int SelfCreatedLookupTable;
};

#endif

