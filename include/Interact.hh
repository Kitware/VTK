/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Interact.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkRenderWindowInteractor - provide event driven interface to rendering window
// .SECTION Description
// vtkRenderWindowInteractor is a convenience object that provides event 
// bindings to common graphics functions. For example, camera 
// zoom-in/zoom-out, pan, rotate, and reset view; picking of actors, points, 
// or cells; in/out of stereo mode; property changes such as wireframe 
// and surface; and a toggle to force the light to be placed at camera 
// viewpoint (pointing in view direction).

// .SECTION Event Bindings
// Specific devices have different camera bindings. The bindings are on both
// mouse events as well as keyboard presses. See vtkXInteractor and 
// vtkWindowsInteractor for specific information.

#ifndef __vtkRenderWindowInteractor_h
#define __vtkRenderWindowInteractor_h

#include "RenderW.hh"
#include "Camera.hh"
#include "Light.hh"
#include "Picker.hh"
#include "PolyMap.hh"
#include "Outline.hh"

class vtkRenderWindowInteractor : public vtkObject
{
public:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor();
  char *GetClassName() {return "vtkRenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize() = 0;
  virtual void Start() = 0;

  // Description:
  // Get the rendering window being controlled by this object.
  vtkSetObjectMacro(RenderWindow,vtkRenderWindow);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // See whether interactor has been initialized yet.
  vtkGetMacro(Initialized,int);

  void FindPokedCamera(int,int);
  void FindPokedRenderer(int,int);

  virtual void HighlightActor(vtkActor *actor);

  void SetStartPickMethod(void (*f)(void *), void *arg);
  void SetStartPickMethodArgDelete(void (*f)(void *));
  void SetEndPickMethod(void (*f)(void *), void *arg);
  void SetEndPickMethodArgDelete(void (*f)(void *));

  void SetPicker(vtkPicker *picker);
  void SetPicker(vtkPicker& picker) {this->SetPicker(&picker);};

  // Description:
  // Get the object used to perform pick operations.
  vtkGetObjectMacro(Picker,vtkPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  virtual vtkPicker *CreateDefaultPicker();

  void SetUserMethod(void (*f)(void *), void *arg);
  void SetUserMethodArgDelete(void (*f)(void *));

protected:
  vtkRenderWindow *RenderWindow;
  vtkCamera   *CurrentCamera;
  vtkLight    *CurrentLight;
  vtkRenderer *CurrentRenderer;
  int LightFollowCamera;
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int Size[2];
  int   State;
  float FocalDepth;
  int Initialized;

  // for picking actors
  vtkPicker *Picker;
  int SelfCreatedPicker;
  vtkOutlineSource Outline;
  vtkPolyMapper OutlineMapper;
  vtkActor *OutlineActor;
  vtkRenderer *PickedRenderer;
  vtkActor *CurrentActor;

  // methods called prior to and after picking
  void (*StartPickMethod)(void *);
  void (*StartPickMethodArgDelete)(void *);
  void *StartPickMethodArg;
  void (*EndPickMethod)(void *);
  void (*EndPickMethodArgDelete)(void *);
  void *EndPickMethodArg;
  void (*UserMethod)(void *);
  void (*UserMethodArgDelete)(void *);
  void *UserMethodArg;

};

#endif
