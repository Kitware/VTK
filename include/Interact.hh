/*=========================================================================

  Program:   Visualization Library
  Module:    Interact.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlRenderWindowInteractor - provide event driven interface to rendering window
// .SECTION Description
// vlRenderWindowInteractor is a convenience object that provides event 
// bindings to common graphics functions. For example, camera 
// zoom-in/zoom-out, pan, rotate, and reset view; picking of actors, points, 
// or cells; in/out of stereo mode; property changes such as wireframe 
// and surface; and a toggle to force the light to be placed at camera 
// viewpoint (pointing in view direction).

// .SECTION Event Bindings
// Specific devices have different camera bindings. The bindings are on both
// mouse events as well as keyboard presses. See vlXInteractor and 
// vlWindowsInteractor for specific information.

#ifndef __vlRenderWindowInteractor_h
#define __vlRenderWindowInteractor_h

#include "RenderW.hh"
#include "Camera.hh"
#include "Light.hh"
#include "Picker.hh"
#include "PolyMap.hh"
#include "Outline.hh"

class vlRenderWindowInteractor : public vlObject
{
public:
  vlRenderWindowInteractor();
  ~vlRenderWindowInteractor();
  char *GetClassName() {return "vlRenderWindowInteractor";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void Initialize() = 0;
  virtual void Start() = 0;

  // Description:
  // Get the rendering window being controlled by this object.
  vlSetObjectMacro(RenderWindow,vlRenderWindow);
  vlGetObjectMacro(RenderWindow,vlRenderWindow);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vlSetMacro(LightFollowCamera,int);
  vlGetMacro(LightFollowCamera,int);
  vlBooleanMacro(LightFollowCamera,int);

  // Description:
  // See whether interactor has been initialized yet.
  vlGetMacro(Initialized,int);

  void FindPokedCamera(int,int);
  void FindPokedRenderer(int,int);

  virtual void HighlightActor(vlActor *actor);

  void SetStartPickMethod(void (*f)(void *), void *arg);
  void SetEndPickMethod(void (*f)(void *), void *arg);

  void SetPicker(vlPicker *picker);
  void SetPicker(vlPicker& picker) {this->SetPicker(&picker);};

  // Description:
  // Get the object used to perform pick operations.
  vlGetObjectMacro(Picker,vlPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  virtual vlPicker *CreateDefaultPicker();

protected:
  vlRenderWindow *RenderWindow;
  vlCamera   *CurrentCamera;
  vlLight    *CurrentLight;
  vlRenderer *CurrentRenderer;
  int LightFollowCamera;
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int Size[2];
  int   State;
  float FocalDepth;
  int Initialized;

  // for picking actors
  vlPicker *Picker;
  int SelfCreatedPicker;
  vlOutlineSource Outline;
  vlPolyMapper OutlineMapper;
  vlActor *OutlineActor;
  vlRenderer *PickedRenderer;
  vlActor *CurrentActor;

  // methods called prior to and after picking
  void (*StartPickMethod)(void *);
  void *StartPickMethodArg;
  void (*EndPickMethod)(void *);
  void *EndPickMethodArg;
};

#endif
