/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderWindowInteractor - platform-independent render window
// interaction including picking and frame rate control.

// .SECTION Description
// vtkRenderWindowInteractor provides a platform-independent interaction
// mechanism for mouse/key/time events. It serves as a base class for
// platform-dependent implementations that handle routing of mouse/key/timer
// messages to vtkInteractorObserver and its subclasses. vtkRenderWindowInteractor 
// also provides controls for picking, rendering frame rate, and headlights.
//
// vtkRenderWindowInteractor has changed from previous implementations and
// now serves only as a shell to hold user preferences and route messages to
// vtkInteractorStyle. Callbacks are available for many events.  Platform
// specific subclasses should provide methods for manipulating timers,
// TerminateApp, and an event loop if required via
// Initialize/Start/Enable/Disable.

// .SECTION Caveats
// vtkRenderWindowInteractor routes events through VTK's command/observer
// design pattern. That is, when vtkRenderWindowInteractor (actually, one of
// its subclasses) sees a platform-dependent event, it translates this into
// a VTK event using the InvokeEvent() method. Then any vtkInteractorObservers
// registered for that event are expected to respond as appropriate.

// .SECTION See Also
// vtkInteractorObserver

#ifndef __vtkRenderWindowInteractor_h
#define __vtkRenderWindowInteractor_h

#include "vtkObject.h"

class vtkTimerIdMap;

// Timer flags for win32/X compatibility
#define VTKI_TIMER_FIRST  0
#define VTKI_TIMER_UPDATE 1

class vtkAbstractPicker;
class vtkAbstractPropPicker;
class vtkInteractorObserver;
class vtkRenderWindow;
class vtkRenderer;
class vtkObserverMediator;
class vtkInteractorEventRecorder;

class VTK_RENDERING_EXPORT vtkRenderWindowInteractor : public vtkObject
{
//BTX
  friend class vtkInteractorEventRecorder;
//ETX
public:
  static vtkRenderWindowInteractor *New();
  vtkTypeMacro(vtkRenderWindowInteractor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Prepare for handling events. This must be called before the
  // interactor will work.
  virtual void Initialize();
  void ReInitialize() {  this->Initialized = 0; this->Enabled = 0;
                        this->Initialize(); } 

  // Description:
  // This Method detects loops of RenderWindow-Interactor,
  // so objects are freed properly.
  virtual void UnRegister(vtkObjectBase *o);

  // Description:
  // Start the event loop. This is provided so that you do not have to
  // implement your own event loop. You still can use your own
  // event loop if you want. Initialize should be called before Start.
  virtual void Start() {}

  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable() { this->Enabled = 1; this->Modified();}
  virtual void Disable() { this->Enabled = 0; this->Modified();}
  vtkGetMacro(Enabled, int);

  // Description:
  // Enable/Disable whether vtkRenderWindowInteractor::Render() calls
  // this->RenderWindow->Render().
  vtkBooleanMacro(EnableRender, bool);
  vtkSetMacro(EnableRender, bool);
  vtkGetMacro(EnableRender, bool);

  // Description:
  // Set/Get the rendering window being controlled by this object.
  void SetRenderWindow(vtkRenderWindow *aren);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Event loop notification member for window size change.
  // Window size is measured in pixels.
  virtual void UpdateSize(int x,int y);

  // Description:
  // This class provides two groups of methods for manipulating timers.  The
  // first group (CreateTimer(timerType) and DestroyTimer()) implicitly use
  // an internal timer id (and are present for backward compatibility). The
  // second group (CreateRepeatingTimer(long),CreateOneShotTimer(long),
  // ResetTimer(int),DestroyTimer(int)) use timer ids so multiple timers can
  // be independently managed. In the first group, the CreateTimer() method
  // takes an argument indicating whether the timer is created the first time
  // (timerType==VTKI_TIMER_FIRST) or whether it is being reset
  // (timerType==VTKI_TIMER_UPDATE). (In initial implementations of VTK this
  // was how one shot and repeating timers were managed.) In the second
  // group, the create methods take a timer duration argument (in
  // milliseconds) and return a timer id. Thus the ResetTimer(timerId) and
  // DestroyTimer(timerId) methods take this timer id and operate on the
  // timer as appropriate. Methods are also available for determining
  virtual int CreateTimer(int timerType); //first group, for backward compatibility
  virtual int DestroyTimer(); //first group, for backward compatibility

  // Description:
  // Create a repeating timer, with the specified duration (in milliseconds).
  // \return the timer id.
  int CreateRepeatingTimer(unsigned long duration);

  // Description:
  // Create a one shot timer, with the specified duretion (in milliseconds).
  // \return the timer id.
  int CreateOneShotTimer(unsigned long duration);

  // Description:
  // Query whether the specified timerId is a one shot timer.
  // \return 1 if the timer is a one shot timer.
  int IsOneShotTimer(int timerId);

  // Description:
  // Get the duration (in milliseconds) for the specified timerId.
  unsigned long GetTimerDuration(int timerId);

  // Description:
  // Reset the specified timer.
  int ResetTimer(int timerId);

  // Description:
  // Destroy the timer specified by timerId.
  // \return 1 if the timer was destroyed.
  int DestroyTimer(int timerId);

  // Description:
  // Get the VTK timer ID that corresponds to the supplied platform ID.
  virtual int GetVTKTimerId(int platformTimerId);

  //BTX
  // Moved into the public section of the class so that classless timer procs
  // can access these enum members without being "friends"...
  enum {OneShotTimer=1,RepeatingTimer};
  //ETX

  // Description:
  // Specify the default timer interval (in milliseconds). (This is used in
  // conjunction with the timer methods described previously, e.g.,
  // CreateTimer() uses this value; and CreateRepeatingTimer(duration) and
  // CreateOneShotTimer(duration) use the default value if the parameter
  // "duration" is less than or equal to zero.) Care must be taken when
  // adjusting the timer interval from the default value of 10
  // milliseconds--it may adversely affect the interactors.
  vtkSetClampMacro(TimerDuration,unsigned long,1,100000);
  vtkGetMacro(TimerDuration,unsigned long);

  // Description:
  // These methods are used to communicate information about the currently
  // firing CreateTimerEvent or DestroyTimerEvent. The caller of
  // CreateTimerEvent sets up TimerEventId, TimerEventType and
  // TimerEventDuration. The observer of CreateTimerEvent should set up an
  // appropriate platform specific timer based on those values and set the
  // TimerEventPlatformId before returning. The caller of DestroyTimerEvent
  // sets up TimerEventPlatformId. The observer of DestroyTimerEvent should
  // simply destroy the platform specific timer created by CreateTimerEvent.
  // See vtkGenericRenderWindowInteractor's InternalCreateTimer and
  // InternalDestroyTimer for an example.
  vtkSetMacro(TimerEventId, int);
  vtkGetMacro(TimerEventId, int);
  vtkSetMacro(TimerEventType, int);
  vtkGetMacro(TimerEventType, int);
  vtkSetMacro(TimerEventDuration, int);
  vtkGetMacro(TimerEventDuration, int);
  vtkSetMacro(TimerEventPlatformId, int);
  vtkGetMacro(TimerEventPlatformId, int);

  // Description:
  // This function is called on 'q','e' keypress if exitmethod is not
  // specified and should be overridden by platform dependent subclasses
  // to provide a termination procedure if one is required.
  virtual void TerminateApp(void) {}

  // Description:
  // External switching between joystick/trackball/new? modes. Initial value
  // is a vtkInteractorStyleSwitch object.
  virtual void SetInteractorStyle(vtkInteractorObserver *);
  vtkGetObjectMacro(InteractorStyle,vtkInteractorObserver);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  // Default is On.
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // Set/Get the desired update rate. This is used by vtkLODActor's to tell
  // them how quickly they need to render.  This update is in effect only
  // when the camera is being rotated, or zoomed.  When the interactor is
  // still, the StillUpdateRate is used instead. 
  // The default is 15.
  vtkSetClampMacro(DesiredUpdateRate,double,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(DesiredUpdateRate,double);

  // Description:
  // Set/Get the desired update rate when movement has stopped.
  // For the non-still update rate, see the SetDesiredUpdateRate method.
  // The default is 0.0001
  vtkSetClampMacro(StillUpdateRate,double,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(StillUpdateRate,double);

  // Description:
  // See whether interactor has been initialized yet.
  // Default is 0.
  vtkGetMacro(Initialized,int);

  // Description:
  // Set/Get the object used to perform pick operations. In order to
  // pick instances of vtkProp, the picker must be a subclass of 
  // vtkAbstractPropPicker, meaning that it can identify a particular 
  // instance of vtkProp.
  virtual void SetPicker(vtkAbstractPicker*);
  vtkGetObjectMacro(Picker,vtkAbstractPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  // Default is an instance of vtkPropPicker.
  virtual vtkAbstractPropPicker *CreateDefaultPicker();

  // Description:
  // These methods correspond to the the Exit, User and Pick
  // callbacks. They allow for the Style to invoke them.
  virtual void ExitCallback();
  virtual void UserCallback();
  virtual void StartPickCallback();
  virtual void EndPickCallback();
  
  // Description:
  // Get the current position of the mouse.
  virtual void GetMousePosition(int *x, int *y) { *x = 0 ; *y = 0; }

  // Description:
  // Hide or show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();

  // Description:
  // Render the scene. Just pass the render call on to the 
  // associated vtkRenderWindow.
  virtual void Render();

  // Description:
  // Given a position x, move the current camera's focal point to x.
  // The movement is animated over the number of frames specified in
  // NumberOfFlyFrames. The LOD desired frame rate is used.
  void FlyTo(vtkRenderer *ren, double x, double y, double z);
  void FlyTo(vtkRenderer *ren, double *x)
    {this->FlyTo(ren, x[0], x[1], x[2]);}
  void FlyToImage(vtkRenderer *ren, double x, double y);
  void FlyToImage(vtkRenderer *ren, double *x)
    {this->FlyToImage(ren, x[0], x[1]);}

  // Description:
  // Set the number of frames to fly to when FlyTo is invoked.
  vtkSetClampMacro(NumberOfFlyFrames,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfFlyFrames,int);

  // Description:
  // Set the total Dolly value to use when flying to (FlyTo()) a
  // specified point. Negative values fly away from the point.
  vtkSetMacro(Dolly,double);
  vtkGetMacro(Dolly,double);

  // Description:
  // Set/Get information about the current event. 
  // The current x,y position is in the EventPosition, and the previous
  // event position is in LastEventPosition, updated automatically each
  // time EventPosition is set using its Set() method. Mouse positions
  // are measured in pixels.
  // The other information is about key board input.
  vtkGetVector2Macro(EventPosition,int);
  vtkGetVector2Macro(LastEventPosition,int);
  vtkSetVector2Macro(LastEventPosition,int);
  virtual void SetEventPosition(int x, int y)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this 
                  << "): setting EventPosition to (" << x << "," << y << ")");
    if (this->EventPosition[0] != x || this->EventPosition[1] != y ||
        this->LastEventPosition[0] != x || this->LastEventPosition[1] != y)
      {
      this->LastEventPosition[0] = this->EventPosition[0];
      this->LastEventPosition[1] = this->EventPosition[1];
      this->EventPosition[0] = x;
      this->EventPosition[1] = y;
      this->Modified();
      }
  }
  virtual void SetEventPosition(int pos[2])
  {
    this->SetEventPosition(pos[0], pos[1]);
  } 
  virtual void SetEventPositionFlipY(int x, int y)
  {
    this->SetEventPosition(x, this->Size[1] - y - 1);
  }
  virtual void SetEventPositionFlipY(int pos[2])
  {
    this->SetEventPositionFlipY(pos[0], pos[1]);
  }

  // Description:
  // Set/get whether alt modifier key was pressed.
  vtkSetMacro(AltKey, int);
  vtkGetMacro(AltKey, int);

  // Description:
  // Set/get whether control modifier key was pressed.
  vtkSetMacro(ControlKey, int);
  vtkGetMacro(ControlKey, int);

  // Description:
  // Set/get whether shift modifier key was pressed.
  vtkSetMacro(ShiftKey, int);
  vtkGetMacro(ShiftKey, int);

  // Description:
  // Set/get the key code for the key that was pressed.
  vtkSetMacro(KeyCode, char);
  vtkGetMacro(KeyCode, char);

  // Description:
  // Set/get the repear count for the key or mouse event. This specifies how
  // many times a key has been pressed.
  vtkSetMacro(RepeatCount, int);
  vtkGetMacro(RepeatCount, int);

  // Description:
  // Set/get the key symbol for the key that was pressed. This is the key
  // symbol as defined by the relevant X headers. On X based platforms this
  // corresponds to the installed X sevrer, whereas on other platforms the
  // native key codes are translated into a string representation.
  vtkSetStringMacro(KeySym);
  vtkGetStringMacro(KeySym);

  // Description:
  // Set all the event information in one call.
  void SetEventInformation(int x, 
                           int y, 
                           int ctrl=0, 
                           int shift=0, 
                           char keycode=0, 
                           int repeatcount=0,
                           const char* keysym=0)
    {
      this->LastEventPosition[0] = this->EventPosition[0];
      this->LastEventPosition[1] = this->EventPosition[1];
      this->EventPosition[0] = x;
      this->EventPosition[1] = y;
      this->ControlKey = ctrl;
      this->ShiftKey = shift;
      this->KeyCode = keycode;
      this->RepeatCount = repeatcount;
      if(keysym)
        {
        this->SetKeySym(keysym);
        }
      this->Modified();
    }

  // Description:
  // Calls SetEventInformation, but flips the Y based on the current Size[1] 
  // value (i.e. y = this->Size[1] - y - 1).
  void SetEventInformationFlipY(int x, 
                                int y, 
                                int ctrl=0, 
                                int shift=0, 
                                char keycode=0, 
                                int repeatcount=0,
                                const char* keysym=0)
    {
      this->SetEventInformation(x, 
                                this->Size[1] - y - 1, 
                                ctrl, 
                                shift, 
                                keycode, 
                                repeatcount, 
                                keysym);
    }

  // Description:
  // Set all the keyboard-related event information in one call.
  void SetKeyEventInformation(int ctrl=0, 
                              int shift=0, 
                              char keycode=0, 
                              int repeatcount=0,
                              const char* keysym=0)
    {
      this->ControlKey = ctrl;
      this->ShiftKey = shift;
      this->KeyCode = keycode;
      this->RepeatCount = repeatcount;
      if(keysym)
        {
        this->SetKeySym(keysym);
        }
      this->Modified();
    }

  // Description:
  // This methods sets the Size ivar of the interactor without
  // actually changing the size of the window. Normally
  // application programmers would use UpdateSize if anything.
  // This is useful for letting someone else change the size of
  // the rendering window and just letting the interactor
  // know about the change.
  // The current event width/height (if any) is in EventSize 
  // (Expose event, for example).
  // Window size is measured in pixels.
  vtkSetVector2Macro(Size,int);
  vtkGetVector2Macro(Size,int);
  vtkSetVector2Macro(EventSize,int);
  vtkGetVector2Macro(EventSize,int);

  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple
  // renderers.
  virtual vtkRenderer *FindPokedRenderer(int,int);

  // Description:
  // Return the object used to mediate between vtkInteractorObservers
  // contending for resources. Multiple interactor observers will often
  // request different resources (e.g., cursor shape); the mediator uses a
  // strategy to provide the resource based on priority of the observer plus
  // the particular request (default versus non-default cursor shape).
  vtkObserverMediator *GetObserverMediator();

  // Description:
  // Use a 3DConnexion device. Initial value is false.
  // If VTK is not build with the TDx option, this is no-op.
  // If VTK is build with the TDx option, and a device is not connected,
  // a warning is emitted.
  // It is must be called before the first Render to be effective, otherwise
  // it is ignored.
  vtkSetMacro(UseTDx,bool);
  vtkGetMacro(UseTDx,bool);

  // Description:
  // Fire various events. SetEventInformation should be called just prior
  // to calling any of these methods. These methods will Invoke the
  // corresponding vtk event.
  virtual void MouseMoveEvent();
  virtual void RightButtonPressEvent();
  virtual void RightButtonReleaseEvent();
  virtual void LeftButtonPressEvent();
  virtual void LeftButtonReleaseEvent();
  virtual void MiddleButtonPressEvent();
  virtual void MiddleButtonReleaseEvent();
  virtual void MouseWheelForwardEvent();
  virtual void MouseWheelBackwardEvent();
  virtual void ExposeEvent();
  virtual void ConfigureEvent();
  virtual void EnterEvent();
  virtual void LeaveEvent();
  virtual void KeyPressEvent();
  virtual void KeyReleaseEvent();
  virtual void CharEvent();
  virtual void ExitEvent();
  
protected:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor();

  vtkRenderWindow       *RenderWindow;
  vtkInteractorObserver *InteractorStyle;

  // Used as a helper object to pick instances of vtkProp
  vtkAbstractPicker     *Picker;

  int    Initialized;
  int    Enabled;
  bool   EnableRender;
  int    Style;
  int    LightFollowCamera;
  int    ActorMode;
  double DesiredUpdateRate;
  double StillUpdateRate;  

  // Event information
  int   AltKey;
  int   ControlKey;
  int   ShiftKey;
  char  KeyCode;
  int   RepeatCount;
  char* KeySym; 
  int   EventPosition[2];
  int   LastEventPosition[2];
  int   EventSize[2];
  int   Size[2];
  int   TimerEventId;
  int   TimerEventType;
  int   TimerEventDuration;
  int   TimerEventPlatformId;

  // control the fly to
  int NumberOfFlyFrames;
  double Dolly;

//BTX
  // Description:
  // These methods allow the interactor to control which events are
  // processed.  When the GrabFocus() method is called, then only events that
  // the supplied vtkCommands have registered are invoked. (This method is
  // typically used by widgets, i.e., subclasses of vtkInteractorObserver, to
  // grab events once an event sequence begins.) Note that the friend
  // declaration is done here to avoid doing so in the superclass vtkObject.
  friend class vtkInteractorObserver;
  void GrabFocus(vtkCommand *mouseEvents, vtkCommand *keypressEvents=NULL)
    {this->Superclass::InternalGrabFocus(mouseEvents,keypressEvents);}
  void ReleaseFocus()
    {this->Superclass::InternalReleaseFocus();}
//ETX

  // Description:
  // Widget mediators are used to resolve contention for cursors and other resources.
  vtkObserverMediator *ObserverMediator;

//BTX
  // Timer related members
  friend struct vtkTimerStruct;
  vtkTimerIdMap *TimerMap; // An internal, PIMPLd map of timers and associated attributes
  unsigned long  TimerDuration; //in milliseconds
  // Description
  // Internal methods for creating and destroying timers that must be
  // implemented by subclasses. InternalCreateTimer() returns a
  // platform-specific timerId and InternalDestroyTimer() returns
  // non-zero value on success.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  int GetCurrentTimerId();
//ETX

  // Force the interactor to handle the Start() event loop, ignoring any 
  // overrides. (Overrides are registered by observing StartEvent on the 
  // interactor.)
  int HandleEventLoop;
  
  bool UseTDx; // 3DConnexion device.
  
private:
  vtkRenderWindowInteractor(const vtkRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkRenderWindowInteractor&);  // Not implemented.
};

#endif
