/*=========================================================================

  Program:   Visualization Library
  Module:    Renderer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlRenderer_hh
#define __vlRenderer_hh

#include "Object.hh"
#include "Mat4x4.hh"
#include "LightC.hh"
#include "Camera.hh"
#include "ActorC.hh"
#include "GeomPrim.hh"

class vlRenderWindow;

class vlRenderer : public vlObject
{
protected:
  vlCamera *ActiveCamera;
  vlLightCollection Lights;
  vlActorCollection Actors;
  float Ambient[3];  
  float Background[3];  
  int BackLight;
  vlRenderWindow *RenderWindow;
  float DisplayPoint[3];
  float ViewPoint[3];
  float WorldPoint[4];
  float Viewport[4];
  int   Erase;
  float Aspect[2];
  int StereoRender;

public:
  vlRenderer();
  char *GetClassName() {return "vlRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddLights(vlLight *);
  void AddActors(vlActor *);
  void SetActiveCamera(vlCamera *);
  vlCamera *GetActiveCamera();

  vlSetVector3Macro(Background,float);
  vlGetVectorMacro(Background,float);

  vlSetVector2Macro(Aspect,float);
  vlGetVectorMacro(Aspect,float);

  vlSetVector3Macro(Ambient,float);
  vlGetVectorMacro(Ambient,float);

  vlSetMacro(BackLight,int);
  vlGetMacro(BackLight,int);
  vlBooleanMacro(BackLight,int);

  vlSetMacro(Erase,int);
  vlGetMacro(Erase,int);
  vlBooleanMacro(Erase,int);

  vlGetMacro(StereoRender,int);

  virtual void Render() = 0;
  virtual vlGeometryPrimitive *GetPrimitive(char *) = 0;
  
  virtual int UpdateActors(void) = 0;
  virtual int UpdateCameras(void) = 0;
  virtual int UpdateLights(void) = 0;

  void DoCameras();
  void DoLights();
  void DoActors();
  void ResetCamera();

  void SetRenderWindow(vlRenderWindow *);
  
  vlSetVector3Macro(DisplayPoint,float);
  vlGetVectorMacro(DisplayPoint,float);

  vlSetVector3Macro(ViewPoint,float);
  vlGetVectorMacro(ViewPoint,float);

  vlSetVector4Macro(WorldPoint,float);
  vlGetVectorMacro(WorldPoint,float);

  vlSetVector4Macro(Viewport,float);
  vlGetVectorMacro(Viewport,float);

  void DisplayToView();
  void ViewToDisplay();
  void WorldToView();
  void ViewToWorld();
  void DisplayToWorld() {DisplayToView(); ViewToWorld();};
  void WorldToDisplay() {WorldToView(); ViewToDisplay();};
};

#endif
