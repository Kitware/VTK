/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkRenderWindowInteractor - platform-independent render window interaction including picking and frame rate control.

// .SECTION Description
// vtkRenderWindowInteractor provides a platform-independent interaction
// mechanism for mouse/key/time events. It serves as a base class for
// platform-dependent implementations that handle routing of mouse/key/timer
// messages to vtkInterActorStyle and its subclasses. 
// vtkRenderWindowInteractor also provides controls for picking,
// rendering frame rate, and headlights.
//
// vtkRenderWindowInteractor has changed from previous implementations and
// now serves only as a shell to hold user preferences and route messages
// to vtkInteractorStyle. Callbacks are available for many Events.
// Platform specific subclasses should provide methods for
// CreateTimer/DestroyTimer, TerminateApp, and an event loop if required
// via Initialize/Start/Enable/Disable.

#ifndef __vtkRenderWindowInteractor_h
#define __vtkRenderWindowInteractor_h

#include "vtkObject.h"
#include "vtkRenderWindow.h"
#include "vtkAbstractPropPicker.h"

// Timer flags for win32/X compatibility
#define VTKI_TIMER_FIRST  0
#define VTKI_TIMER_UPDATE 1

class vtkInteractorStyle;

class VTK_RENDERING_EXPORT vtkRenderWindowInteractor : public vtkObject
{
public:
  static vtkRenderWindowInteractor *New();
  vtkTypeMacro(vtkRenderWindowInteractor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Prepare for handling events. This must be called before the
  // interactor will work.
  virtual void Initialize() {this->Initialized=1; this->Enable();
                             this->RenderWindow->Render();}
  void ReInitialize() {  this->Initialized = 0; this->Enabled = 0;
                        this->Initialize(); } 

  // Description:
  // This Method detects loops of RenderWindow-Interactor,
  // so objects are freed properly.
  void UnRegister(vtkObject *o);

  // Description:
  // Start the event loop. This is provided so that you do not have to
  // implement your own event loop. You still can use your own
  // event loop if you want. Initialize should be called before Start.
  virtual void Start() {};

  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable() { this->Enabled = 1; this->Modified();};
  virtual void Disable() { this->Enabled = 0; this->Modified();};
  vtkGetMacro(Enabled, int);

  // Description:
  // Set/Get the rendering window being controlled by this object.
  void SetRenderWindow(vtkRenderWindow *aren);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Event loop notification member for Window size change
  virtual void UpdateSize(int x,int y);

  // Description:
  // This methods sets the Size ivar of the interactor without
  // actually changing the size of the window. Normally
  // application programmers would use UpdateSize if anything.
  // This is useful for letting someone else change the size of
  // the rendering window and just letting the interactor
  // know about the change.
  vtkSetVector2Macro(Size,int);
  vtkGetVector2Macro(Size,int);

  // Description:
  // Timer methods must be overridden by platform dependent subclasses.
  // flag is passed to indicate if this is first timer set or an update
  // as Win32 uses repeating timers, whereas X uses One shot more timer
  // if flag==VTKXI_TIMER_FIRST Win32 and X should createtimer
  // otherwise Win32 should exit and X should perform AddTimeOut()
  virtual int CreateTimer(int )  { return 1; };
  virtual int DestroyTimer()    { return 1; };

  // Description:
  // This function is called on 'q','e' keypress if exitmethod is not
  // specified and should be overridden by platform dependent subclasses
  // to provide a termination procedure if one is required.
  virtual void TerminateApp(void) {};

  // Description:
  // External switching between joystick/trackball/new? modes.
  virtual void SetInteractorStyle(vtkInteractorStyle *);
  vtkGetObjectMacro(InteractorStyle,vtkInteractorStyle);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // This method can be used by user callbacks to get the
  // x, y, coordinates of the current event.
  vtkSetVector2Macro(EventPosition,int);
  vtkGetVectorMacro(EventPosition,int,2);

  // Description:
  // Set/Get the desired update rate. This is used by vtkLODActor's to tell
  // them how quickly they need to render.  This update is in effect only
  // when the camera is being rotated, or zoomed.  When the interactor is
  // still, the StillUpdateRate is used instead.
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
  // Set/Get the object used to perform pick operations. In order to
  // pick instances of vtkProp, the picker must be a subclass of 
  // vtkAbstractPropPicker, meaning that it can identify a particular 
  // instance of vtkProp.
  vtkSetObjectMacro(Picker,vtkAbstractPicker);
  vtkGetObjectMacro(Picker,vtkAbstractPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  // Default is an instance of vtkPropPicker.
  virtual vtkAbstractPropPicker *CreateDefaultPicker();

  // Description:
  // Specify a method to be executed prior to the pick operation.
  void SetStartPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetStartPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Specify a method to be executed after the pick operation.
  void SetEndPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetEndPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the user method. This method is invoked on a "u" keypress.
  void SetUserMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetUserMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a "e" or "q" keypress.
  void SetExitMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetExitMethodArgDelete(void (*f)(void *));

  // Description:
  // These methods correspond to the the Exit, User and Pick
  // callbacks. They allow for the Style to invoke them.
  virtual void ExitCallback();
  virtual void UserCallback();
  virtual void StartPickCallback();
  virtual void EndPickCallback();
  
  // Description:
  // Get the current position of the mouse.
  virtual void GetMousePosition(int *x, int *y) { *x = 0 ; *y = 0; };

  // Description:
  // Hide or show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor() { this->RenderWindow->HideCursor(); };
  void ShowCursor() { this->RenderWindow->ShowCursor(); };

  // Description:
  // Render the scene. Just pass the render call on to the 
  // associated vtkRenderWindow.
  void Render();

  // Description:
  // Given a position x, move the current camera's focal point to x.
  // The movement is animated over the number of frames specified in
  // NumberOfFlyFrames. The LOD desired frame rate is used.
  void FlyTo(vtkRenderer *ren, float x, float y, float z);
  void FlyTo(vtkRenderer *ren, float *x)
    {this->FlyTo(ren, x[0], x[1], x[2]);}

  // Description:
  // Set the number of frames to fly to when FlyTo is invoked.
  vtkSetClampMacro(NumberOfFlyFrames,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfFlyFrames,int);

  // Description:
  // Set the total Dolly value to use when flying to (FlyTo()) a
  // specified point. Negative values fly away from the point.
  vtkSetMacro(Dolly,float);
  vtkGetMacro(Dolly,float);

protected:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor();
  vtkRenderWindowInteractor(const vtkRenderWindowInteractor&);
  void operator=(const vtkRenderWindowInteractor&);

  vtkRenderWindow    *RenderWindow;
  vtkInteractorStyle *InteractorStyle;

  // Used as a helper object to pick instances of vtkProp
  vtkAbstractPicker          *Picker;

  //
  int   Initialized;
  int   Enabled;
  int   Style;
  int   LightFollowCamera;
  int   ActorMode;
  float DesiredUpdateRate;
  float StillUpdateRate;
  int   EventPosition[2];
  int   Size[2];
  
  // user methods that can be used to override default behavior
  unsigned long StartPickTag;
  unsigned long EndPickTag;
  unsigned long UserTag;
  unsigned long ExitTag;
  
  // control the fly to
  int NumberOfFlyFrames;
  float Dolly;
  
};

#endif
