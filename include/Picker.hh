/*=========================================================================

  Program:   Visualization Library
  Module:    Picker.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPicker - select an actor by shooting a ray into graphics window
// .SECTION Description
// vlPicker is used to select actors by shooting a ray into graphics window
// and intersecting with actor's bounding box. The ray is defined from a 
// point defined in window (or pixel) coordinates, and a point located from 
// the camera's position.
//    vlPicker may return more than one actor, since more than one bounding 
// box may be intersected. VlPicker returns the list of actors that were hit, 
// the pick coordinates in world and untransformed mapper space, and the 
// actor and mapper that are "closest" to the camera. The closest actor is 
// the one whose center point (i.e., center of bounding box) projected on the
// the ray is closest to the camera.
// .SECTION Caveats
// vlPicker and its subclasses will not pick actors that are "unpickable" 
// (see vlActor) or are fully transparent.
// .SECTION See Also
// vlPicker is used for quick picking. If you desire to pick points
// or cells, use the subclass vlPointPicker or vlCellPicker, respectively.

#ifndef __vlPicker_h
#define __vlPicker_h

#include "Object.hh"
#include "Renderer.hh"
#include "Actor.hh"
#include "ActorC.hh"
#include "Mapper.hh"
#include "Trans.hh"

class vlPicker : public vlObject
{
public:
  vlPicker();
  ~vlPicker() {};
  char *GetClassName() {return "vlPicker";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Get the renderer in which pick event occurred.
  vlGetObjectMacro(Renderer,vlRenderer);

  // Description:
  // Get the selection point in screen (pixel) coordinates. The third
  // value is related to z-buffer depth. (Normally should be =0).
  vlGetVectorMacro(SelectionPoint,float,3);

  // Description:
  // Specify tolerance for performing pick operation.
  vlSetMacro(Tolerance,float);
  vlGetMacro(Tolerance,float);

  // Description:
  // Return position in global coordinates of pick point.
  vlGetVectorMacro(PickPosition,float,3);

  // Description:
  // Return position in mapper (i.e., non-transformed) coordinates of 
  // pick point.
  vlGetVectorMacro(MapperPosition,float,3);

  // Description:
  // Return actor that was picked.
  vlGetObjectMacro(Actor,vlActor);

  // Description:
  // Return mapper that was picked.
  vlGetObjectMacro(Mapper,vlMapper);

  // Description:
  // Get a pointer to the dataset that was picked. If nothing was picked then
  // NULL is returned.
  vlGetObjectMacro(DataSet,vlDataSet);

  vlActorCollection *GetActors();

  int Pick(float selectionX, float selectionY, float selectionZ, 
           vlRenderer *renderer);  
  int Pick(float selectionPt[3], vlRenderer *renderer);  

protected:
  void MarkPicked(vlActor *a, vlMapper *m, float tMin, float mapperPos[3]);
  virtual void IntersectWithLine(float p1[3], float p2[3], float tol, vlActor *a, vlMapper *m);
  virtual void Initialize();

  vlRenderer *Renderer; //pick occurred in this renderer's viewport
  float SelectionPoint[3]; //selection point in window (pixel) coordinates
  float Tolerance; //tolerance for computation (% of window)
  float PickPosition[3]; //selection point in world coordinates
  float MapperPosition[3]; //selection point in untransformed coordinates
  vlActor *Actor; //selected actor
  vlMapper *Mapper; //selected mapper
  vlDataSet *DataSet; //selected dataset
  float GlobalTMin; //parametric coordinate along pick ray where hit occured
  vlTransform Transform; //use to perform ray transformation
  vlActorCollection Actors; //candidate actors (based on bounding box)
};

inline vlActorCollection* vlPicker::GetActors() {return &(this->Actors);}

inline int vlPicker::Pick(float selectionPt[3], vlRenderer *renderer)
{
  return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], renderer);
}

#endif


