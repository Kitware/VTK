/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.h
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
// .NAME vtkRenderWindowInteractor - provide event driven interface to rendering window

// .SECTION Description
// vtkRenderWindowInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera
// zoom-in/zoom-out, pan, rotate, resetting; picking of actors, points,
// or cells; switching in/out of stereo mode; property changes such as
// wireframe and surface; and a toggle to force the light to be placed at
// camera viewpoint (pointing in view direction).

// .SECTION Event Bindings
// Specific devices have different camera bindings. The bindings are on both
// mouse events as well as keyboard presses. See vtkXRenderWindowInteractor  
// and vtkWin32RenderWindowInteractor for specific information.

// .SECTION See Also
// vtkXRenderWindowInteractor vtkWin32RenderWindowInteractor vtkPicker

#ifndef __vtkRenderWindowInteractor_h
#define __vtkRenderWindowInteractor_h

#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkPicker.h"
#include "vtkPolyMapper.h"
#include "vtkOutlineSource.h"

class VTK_EXPORT vtkRenderWindowInteractor : public vtkObject
{
public:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor();
  static vtkRenderWindowInteractor *New();
  char *GetClassName() {return "vtkRenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize() {this->Initialized=1;this->RenderWindow->Render();};
  virtual void Start() {};

  // Description:
  // Set/Get the rendering window being controlled by this object.
  vtkSetObjectMacro(RenderWindow,vtkRenderWindow);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // Set/Get the desired update rate. This is used by vtkLODActor's to tell 
  // them how quickly they need to render.  This update is in effect only
  // when the camera is being rotated, or zoomed.  When the interactor is
  // still, the StillUpdateRate is used instead. A value of zero indicates
  // that the update rate is unimportant.
  vtkSetClampMacro(DesiredUpdateRate,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(DesiredUpdateRate,float);

  // Description:
  // Set/Get the desired update rate when movement has stopped.
  // See the SetDesiredUpdateRate method. 
  vtkSetClampMacro(StillUpdateRate,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(StillUpdateRate,float);

  // Description:
  // See whether interactor has been initialized yet.
  vtkGetMacro(Initialized,int);

  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple 
  // renderers. We also need to know what camera to operate on.
  // This is just the ActiveCamera of the poked renderer. 
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
  int   LightFollowCamera;
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int   Size[2];
  int   State;
  float FocalDepth;
  int   Initialized;
  float DesiredUpdateRate;
  float StillUpdateRate;

  // for picking actors
  vtkPicker *Picker;
  int SelfCreatedPicker;
  vtkOutlineSource Outline;
  vtkPolyMapper *OutlineMapper;
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
