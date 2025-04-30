// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderWindowInteractor
 * @brief   platform-independent render window
 * interaction including picking and frame rate control.
 *
 *
 * vtkRenderWindowInteractor provides a platform-independent interaction
 * mechanism for mouse/key/time events. It serves as a base class for
 * platform-dependent implementations that handle routing of mouse/key/timer
 * messages to vtkInteractorObserver and its subclasses. vtkRenderWindowInteractor
 * also provides controls for picking, rendering frame rate, and headlights.
 *
 * vtkRenderWindowInteractor has changed from previous implementations and
 * now serves only as a shell to hold user preferences and route messages to
 * vtkInteractorStyle. Callbacks are available for many events.  Platform
 * specific subclasses should provide methods for manipulating timers,
 * TerminateApp, and an event loop if required via
 * Initialize/Start/Enable/Disable.
 *
 * @warning
 * vtkRenderWindowInteractor routes events through VTK's command/observer
 * design pattern. That is, when vtkRenderWindowInteractor (actually, one of
 * its subclasses) sees a platform-dependent event, it translates this into
 * a VTK event using the InvokeEvent() method. Then any vtkInteractorObservers
 * registered for that event are expected to respond as appropriate.
 *
 * @sa
 * vtkInteractorObserver
 */

#ifndef vtkRenderWindowInteractor_h
#define vtkRenderWindowInteractor_h

#include "vtkCommand.h" // for method sig
#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"        // For InteractorStyle
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkTimerIdMap;

// Timer flags for win32/X compatibility
#define VTKI_TIMER_FIRST 0
#define VTKI_TIMER_UPDATE 1

// maximum pointers active at once
// for example in multitouch
#define VTKI_MAX_POINTERS 5

class vtkAbstractPicker;
class vtkAbstractPropPicker;
class vtkAssemblyPath;
class vtkHardwareWindow;
class vtkInteractorObserver;
class vtkRenderWindow;
class vtkRenderer;
class vtkObserverMediator;
class vtkInteractorEventRecorder;
class vtkPickingManager;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRenderWindowInteractor : public vtkObject
{

  friend class vtkInteractorEventRecorder;

public:
  static vtkRenderWindowInteractor* New();
  vtkTypeMacro(vtkRenderWindowInteractor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Prepare for handling events and set the Enabled flag to true.
   * This will be called automatically by Start() if the interactor
   * is not initialized, but it can be called manually if you need
   * to perform any operations between initialization and the start
   * of the event loop.
   */
  virtual void Initialize();
  void ReInitialize()
  {
    this->Initialized = 0;
    this->Enabled = 0;
    this->Initialize();
  }
  ///@}

  /**
   * This Method detects loops of RenderWindow-Interactor,
   * so objects are freed properly.
   */
  void UnRegister(vtkObjectBase* o) override;

  /**
   * Start the event loop. This is provided so that you do not have to
   * implement your own event loop. You still can use your own
   * event loop if you want.
   */
  virtual void Start();

  /**
   * Process all user-interaction, timer events and return.
   * If there are no events, this method returns immediately.
   * This method is implemented only on desktop (macOS, linux, windows) and WebAssembly (SDL2).
   * It is not implemented on iOS and Android platforms.
   */
  virtual void ProcessEvents() {}

  /**
   * Is the interactor loop done
   */
  vtkGetMacro(Done, bool);
  vtkSetMacro(Done, bool);

  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  virtual void Enable()
  {
    this->Enabled = 1;
    this->Modified();
  }
  virtual void Disable()
  {
    this->Enabled = 0;
    this->Modified();
  }
  vtkGetMacro(Enabled, int);

  ///@{
  /**
   * Enable/Disable whether vtkRenderWindowInteractor::Render() calls
   * this->RenderWindow->Render().
   */
  vtkBooleanMacro(EnableRender, bool);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkSetMacro(EnableRender, bool);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkGetMacro(EnableRender, bool);
  ///@}

  ///@{
  /**
   * Set/Get the rendering window being controlled by this object.
   */
  void SetRenderWindow(vtkRenderWindow* aren);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  ///@}

  ///@{
  /**
   * Set/Get the hardware window being controlled by this object.
   * For opengl the hardware window is not used as the opengl
   * subclasses of RenderWindow provide the functionality.
   */
  void SetHardwareWindow(vtkHardwareWindow* aren);
  vtkGetObjectMacro(HardwareWindow, vtkHardwareWindow);
  ///@}

  /**
   * When the event loop notifies the interactor that the window size has
   * changed, this method is called to update the Size of the interactor
   * and its vtkRenderWindow.
   *
   * The interactor will fire vtkCommand::ConfigureEvent after the size has
   * updated.
   */
  virtual void UpdateSize(int x, int y);

  /**
   * This class provides two groups of methods for manipulating timers.  The
   * first group (CreateTimer(timerType) and DestroyTimer()) implicitly use
   * an internal timer id (and are present for backward compatibility). The
   * second group (CreateRepeatingTimer(long),CreateOneShotTimer(long),
   * ResetTimer(int),DestroyTimer(int)) use timer ids so multiple timers can
   * be independently managed. In the first group, the CreateTimer() method
   * takes an argument indicating whether the timer is created the first time
   * (timerType==VTKI_TIMER_FIRST) or whether it is being reset
   * (timerType==VTKI_TIMER_UPDATE). (In initial implementations of VTK this
   * was how one shot and repeating timers were managed.) In the second
   * group, the create methods take a timer duration argument (in
   * milliseconds) and return a timer id. Thus the ResetTimer(timerId) and
   * DestroyTimer(timerId) methods take this timer id and operate on the
   * timer as appropriate. Make sure you run Initialize() before creating
   * the timer in order for it to work.
   */
  virtual int CreateTimer(int timerType); // first group, for backward compatibility
  virtual int DestroyTimer();             // first group, for backward compatibility

  /**
   * Create a repeating timer, with the specified duration (in milliseconds).
   * \return the timer id.
   */
  int CreateRepeatingTimer(unsigned long duration);

  /**
   * Create a one shot timer, with the specified duration (in milliseconds).
   * \return the timer id.
   */
  int CreateOneShotTimer(unsigned long duration);

  /**
   * Query whether the specified timerId is a one shot timer.
   * \return 1 if the timer is a one shot timer.
   */
  int IsOneShotTimer(int timerId);

  /**
   * Get the duration (in milliseconds) for the specified timerId.
   */
  unsigned long GetTimerDuration(int timerId);

  /**
   * Reset the specified timer.
   */
  int ResetTimer(int timerId);

  /**
   * Destroy the timer specified by timerId.
   * \return 1 if the timer was destroyed.
   */
  int DestroyTimer(int timerId);

  /**
   * Get the VTK timer ID that corresponds to the supplied platform ID.
   */
  virtual int GetVTKTimerId(int platformTimerId);

  // Moved into the public section of the class so that classless timer procs
  // can access these enum members without being "friends"...
  enum
  {
    OneShotTimer = 1,
    RepeatingTimer
  };

  ///@{
  /**
   * Specify the default timer interval (in milliseconds). (This is used in
   * conjunction with the timer methods described previously, e.g.,
   * CreateTimer() uses this value; and CreateRepeatingTimer(duration) and
   * CreateOneShotTimer(duration) use the default value if the parameter
   * "duration" is less than or equal to zero.) Care must be taken when
   * adjusting the timer interval from the default value of 10
   * milliseconds--it may adversely affect the interactors.
   */
  vtkSetClampMacro(TimerDuration, unsigned long, 1, 100000);
  vtkGetMacro(TimerDuration, unsigned long);
  ///@}

  ///@{
  /**
   * These methods are used to communicate information about the currently
   * firing CreateTimerEvent or DestroyTimerEvent. The caller of
   * CreateTimerEvent sets up TimerEventId, TimerEventType and
   * TimerEventDuration. The observer of CreateTimerEvent should set up an
   * appropriate platform specific timer based on those values and set the
   * TimerEventPlatformId before returning. The caller of DestroyTimerEvent
   * sets up TimerEventPlatformId. The observer of DestroyTimerEvent should
   * simply destroy the platform specific timer created by CreateTimerEvent.
   * See vtkGenericRenderWindowInteractor's InternalCreateTimer and
   * InternalDestroyTimer for an example.
   */
  vtkSetMacro(TimerEventId, int);
  vtkGetMacro(TimerEventId, int);
  vtkSetMacro(TimerEventType, int);
  vtkGetMacro(TimerEventType, int);
  vtkSetMacro(TimerEventDuration, int);
  vtkGetMacro(TimerEventDuration, int);
  vtkSetMacro(TimerEventPlatformId, int);
  vtkGetMacro(TimerEventPlatformId, int);
  ///@}

  /**
   * This function is called on 'q','e' keypress if exitmethod is not
   * specified and should be overridden by platform dependent subclasses
   * to provide a termination procedure if one is required.
   */
  virtual void TerminateApp() { this->Done = true; }

  ///@{
  /**
   * External switching between joystick/trackball/new? modes. Initial value
   * is a vtkInteractorStyleSwitch object.
   */
  virtual void SetInteractorStyle(vtkInteractorObserver*);
  virtual vtkInteractorObserver* GetInteractorStyle();
  ///@}

  ///@{
  /**
   * Turn on/off the automatic repositioning of lights as the camera moves.
   * Default is On.
   */
  vtkSetMacro(LightFollowCamera, vtkTypeBool);
  vtkGetMacro(LightFollowCamera, vtkTypeBool);
  vtkBooleanMacro(LightFollowCamera, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the desired update rate. This is used by vtkLODActor's to tell
   * them how quickly they need to render.  This update is in effect only
   * when the camera is being rotated, or zoomed.  When the interactor is
   * still, the StillUpdateRate is used instead.
   * The default is 15.
   */
  vtkSetClampMacro(DesiredUpdateRate, double, 0.0001, VTK_FLOAT_MAX);
  vtkGetMacro(DesiredUpdateRate, double);
  ///@}

  ///@{
  /**
   * Set/Get the desired update rate when movement has stopped.
   * For the non-still update rate, see the SetDesiredUpdateRate method.
   * The default is 0.0001
   */
  vtkSetClampMacro(StillUpdateRate, double, 0.0001, VTK_FLOAT_MAX);
  vtkGetMacro(StillUpdateRate, double);
  ///@}

  ///@{
  /**
   * See whether interactor has been initialized yet.
   * Default is 0.
   */
  vtkGetMacro(Initialized, int);
  ///@}

  ///@{
  /**
   * Set/Get the object used to perform pick operations. In order to
   * pick instances of vtkProp, the picker must be a subclass of
   * vtkAbstractPropPicker, meaning that it can identify a particular
   * instance of vtkProp.
   */
  virtual void SetPicker(vtkAbstractPicker*);
  vtkGetObjectMacro(Picker, vtkAbstractPicker);
  ///@}

  /**
   * Create default picker. Used to create one when none is specified.
   * Default is an instance of vtkPropPicker.
   */
  virtual vtkAbstractPropPicker* CreateDefaultPicker();

  ///@{
  /**
   * Set the picking manager.
   * Set/Get the object used to perform operations through the interactor
   * By default, a valid but disabled picking manager is instantiated.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  virtual void SetPickingManager(vtkPickingManager*);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  vtkGetObjectMacro(PickingManager, vtkPickingManager);
  ///@}

  ///@{
  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();
  virtual void UserCallback();
  virtual void StartPickCallback();
  virtual void EndPickCallback();
  ///@}

  /**
   * Get the current position of the mouse.
   */
  virtual void GetMousePosition(int* x, int* y)
  {
    *x = 0;
    *y = 0;
  }

  ///@{
  /**
   * Hide or show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor();
  void ShowCursor();
  ///@}

  /**
   * Render the scene. Just pass the render call on to the
   * associated vtkRenderWindow.
   */
  virtual void Render();

  ///@{
  /**
   * Given a position x, move the current camera's focal point to x.
   * The movement is animated over the number of frames specified in
   * NumberOfFlyFrames. The LOD desired frame rate is used.
   */
  void FlyTo(vtkRenderer* ren, double x, double y, double z);
  void FlyTo(vtkRenderer* ren, double* x) { this->FlyTo(ren, x[0], x[1], x[2]); }
  void FlyToImage(vtkRenderer* ren, double x, double y);
  void FlyToImage(vtkRenderer* ren, double* x) { this->FlyToImage(ren, x[0], x[1]); }
  ///@}

  ///@{
  /**
   * Set the number of frames to fly to when FlyTo is invoked.
   */
  vtkSetClampMacro(NumberOfFlyFrames, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfFlyFrames, int);
  ///@}

  ///@{
  /**
   * Set the total Dolly value to use when flying to (FlyTo()) a
   * specified point. Negative values fly away from the point.
   */
  vtkSetMacro(Dolly, double);
  vtkGetMacro(Dolly, double);
  ///@}

  ///@{
  /**
   * Set/Get information about the current event.
   * The current x,y position is in the EventPosition, and the previous
   * event position is in LastEventPosition, updated automatically each
   * time EventPosition is set using its Set() method. Mouse positions
   * are measured in pixels.
   * The other information is about key board input.
   */
  vtkGetVector2Macro(EventPosition, int);
  vtkGetVector2Macro(LastEventPosition, int);
  vtkSetVector2Macro(LastEventPosition, int);
  virtual void SetEventPosition(int x, int y)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting EventPosition to (" << x
                  << "," << y << ")");
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
  virtual void SetEventPosition(int pos[2]) { this->SetEventPosition(pos[0], pos[1]); }
  virtual void SetEventPositionFlipY(int x, int y)
  {
    this->SetEventPosition(x, this->Size[1] - y - 1);
  }
  virtual void SetEventPositionFlipY(int pos[2]) { this->SetEventPositionFlipY(pos[0], pos[1]); }
  ///@}

  virtual int* GetEventPositions(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->EventPositions[pointerIndex];
  }
  virtual int* GetLastEventPositions(int pointerIndex)
  {
    if (pointerIndex >= VTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->LastEventPositions[pointerIndex];
  }
  virtual void SetEventPosition(int x, int y, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= VTKI_MAX_POINTERS)
    {
      return;
    }
    if (pointerIndex == 0)
    {
      this->LastEventPosition[0] = this->EventPosition[0];
      this->LastEventPosition[1] = this->EventPosition[1];
      this->EventPosition[0] = x;
      this->EventPosition[1] = y;
    }
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting EventPosition to (" << x
                  << "," << y << ") for pointerIndex number " << pointerIndex);
    if (this->EventPositions[pointerIndex][0] != x || this->EventPositions[pointerIndex][1] != y ||
      this->LastEventPositions[pointerIndex][0] != x ||
      this->LastEventPositions[pointerIndex][1] != y)
    {
      this->LastEventPositions[pointerIndex][0] = this->EventPositions[pointerIndex][0];
      this->LastEventPositions[pointerIndex][1] = this->EventPositions[pointerIndex][1];
      this->EventPositions[pointerIndex][0] = x;
      this->EventPositions[pointerIndex][1] = y;
      this->Modified();
    }
  }
  virtual void SetEventPosition(int pos[2], int pointerIndex)
  {
    this->SetEventPosition(pos[0], pos[1], pointerIndex);
  }
  virtual void SetEventPositionFlipY(int x, int y, int pointerIndex)
  {
    this->SetEventPosition(x, this->Size[1] - y - 1, pointerIndex);
  }
  virtual void SetEventPositionFlipY(int pos[2], int pointerIndex)
  {
    this->SetEventPositionFlipY(pos[0], pos[1], pointerIndex);
  }

  ///@{
  /**
   * Set/get whether alt modifier key was pressed.
   * On macOS, this corresponds to the Option key
   * which may have unexpected effect on the KeyCode and KeySym.
   *
   * AltGr does NOT trigger this modifier.
   */
  vtkSetMacro(AltKey, int);
  vtkGetMacro(AltKey, int);
  ///@}

  ///@{
  /**
   * Set/get whether control modifier key was pressed.
   * On macOS, pressing either Cmd or Control turn this
   * modifier on.
   */
  vtkSetMacro(ControlKey, int);
  vtkGetMacro(ControlKey, int);
  ///@}

  ///@{
  /**
   * Set/get whether shift modifier key was pressed.
   */
  vtkSetMacro(ShiftKey, int);
  vtkGetMacro(ShiftKey, int);
  ///@}

  ///@{
  /**
   * Set/get the unicode value for the key that was pressed, as an 8-bit char value.
   * This restricts the value to the Basic Latin and Latin1 blocks of unicode.
   *
   * Since the 'char' type may be signed, one should cast to 'unsigned char' before retrieving the
   * code value.
   *
   * unsigned char keyCode = static_cast<unsigned char>(rwi->GetKeyCode())
   *
   * Please note KeyCode is impacted by modifiers:
   *
   * "A" -> 'a'
   * "Shift" + "A" -> 'A'
   * "Ctrl" + "A" -> 1
   * "Alt" + "A" -> 'a'
   *
   * The behavior with Control modifier is related to C0 and C1 control codes.
   *
   * Please note KeyCode IS NOT reliable across platforms, especially for special characters with
   * modifiers. Using KeySym should be more reliable.
   *
   * Default is 0.
   */
  vtkSetMacro(KeyCode, char);
  vtkGetMacro(KeyCode, char);
  ///@}

  ///@{
  /**
   * Set/get the repeat count for the key or mouse event. This specifies how
   * many times a key has been pressed.
   */
  vtkSetMacro(RepeatCount, int);
  vtkGetMacro(RepeatCount, int);
  ///@}

  ///@{
  /**
   * Set/get the key symbol for the key that was pressed. This is the key
   * symbol as defined by the relevant X headers (xlib/X11/keysymdef.h).
   * On X based platforms this corresponds to the installed X server, whereas on other platforms the
   * native key codes are translated into a string representation using VTK defined tables.
   *
   * Please note the KeySym is impacted by modifiers:
   *
   * "A" -> "a"
   * "Shift" + "A" -> "A"
   * "Alt" + "A" -> "a"
   * "Ctrl" + "A" -> "a"
   *
   * Please note KeySym may NOT be fully reliable across platforms, especially for special
   * characters with modifiers. Please check the actual KeySym on supported platform before relying
   * on it. However, KeySym is intended to always correspond to the key the user intended to press,
   * even across layouts and platforms.
   *
   * Default is nullptr.
   */
  vtkSetStringMacro(KeySym);
  vtkGetStringMacro(KeySym);
  ///@}

  ///@{
  /**
   * Set/get the index of the most recent pointer to have an event
   */
  vtkSetMacro(PointerIndex, int);
  vtkGetMacro(PointerIndex, int);
  ///@}

  ///@{
  /**
   * Set/get the rotation for the gesture in degrees, update LastRotation
   */
  void SetRotation(double rotation);
  vtkGetMacro(Rotation, double);
  vtkGetMacro(LastRotation, double);
  ///@}

  ///@{
  /**
   * Set/get the scale for the gesture, updates LastScale
   */
  void SetScale(double scale);
  vtkGetMacro(Scale, double);
  vtkGetMacro(LastScale, double);
  ///@}

  ///@{
  /**
   * Set/get the translation for pan/swipe gestures, update LastTranslation
   */
  void SetTranslation(double val[2]);
  vtkGetVector2Macro(Translation, double);
  vtkGetVector2Macro(LastTranslation, double);
  ///@}

  ///@{
  /**
   * Set all the event information in one call.
   */
  void SetEventInformation(int x, int y, int ctrl, int shift, char keycode, int repeatcount,
    const char* keysym, int pointerIndex)
  {
    this->SetEventPosition(x, y, pointerIndex);
    this->ControlKey = ctrl;
    this->ShiftKey = shift;
    this->KeyCode = keycode;
    this->RepeatCount = repeatcount;
    this->PointerIndex = pointerIndex;
    if (keysym)
    {
      this->SetKeySym(keysym);
    }
    this->Modified();
  }
  void SetEventInformation(int x, int y, int ctrl = 0, int shift = 0, char keycode = 0,
    int repeatcount = 0, const char* keysym = nullptr)
  {
    this->SetEventInformation(x, y, ctrl, shift, keycode, repeatcount, keysym, 0);
  }
  ///@}

  ///@{
  /**
   * Calls SetEventInformation, but flips the Y based on the current Size[1]
   * value (i.e. y = this->Size[1] - y - 1).
   */
  void SetEventInformationFlipY(int x, int y, int ctrl, int shift, char keycode, int repeatcount,
    const char* keysym, int pointerIndex)
  {
    this->SetEventInformation(
      x, this->Size[1] - y - 1, ctrl, shift, keycode, repeatcount, keysym, pointerIndex);
  }
  void SetEventInformationFlipY(int x, int y, int ctrl = 0, int shift = 0, char keycode = 0,
    int repeatcount = 0, const char* keysym = nullptr)
  {
    this->SetEventInformationFlipY(x, y, ctrl, shift, keycode, repeatcount, keysym, 0);
  }
  ///@}

  ///@{
  /**
   * Set all the keyboard-related event information in one call.
   */
  void SetKeyEventInformation(int ctrl = 0, int shift = 0, char keycode = 0, int repeatcount = 0,
    const char* keysym = nullptr)
  {
    this->ControlKey = ctrl;
    this->ShiftKey = shift;
    this->KeyCode = keycode;
    this->RepeatCount = repeatcount;
    if (keysym)
    {
      this->SetKeySym(keysym);
    }
    this->Modified();
  }
  ///@}

  ///@{
  /**
   * This methods sets the Size ivar of the interactor without
   * actually changing the size of the window. Normally
   * application programmers would use UpdateSize if anything.
   * This is useful for letting someone else change the size of
   * the rendering window and just letting the interactor
   * know about the change.
   * The current event width/height (if any) is in EventSize
   * (Expose event, for example).
   * Window size is measured in pixels.
   */
  vtkSetVector2Macro(Size, int);
  vtkGetVector2Macro(Size, int);
  vtkSetVector2Macro(EventSize, int);
  vtkGetVector2Macro(EventSize, int);
  ///@}

  /**
   * When an event occurs, we must determine which Renderer the event
   * occurred within, since one RenderWindow may contain multiple
   * renderers.
   */
  virtual vtkRenderer* FindPokedRenderer(int, int);

  /**
   * Return the object used to mediate between vtkInteractorObservers
   * contending for resources. Multiple interactor observers will often
   * request different resources (e.g., cursor shape); the mediator uses a
   * strategy to provide the resource based on priority of the observer plus
   * the particular request (default versus non-default cursor shape).
   */
  vtkObserverMediator* GetObserverMediator();

  ///@{
  /**
   * Use a 3DConnexion device. Initial value is false.
   * If VTK is not build with the TDx option, this is no-op.
   * If VTK is build with the TDx option, and a device is not connected,
   * a warning is emitted.
   * It is must be called before the first Render to be effective, otherwise
   * it is ignored.
   */
  vtkSetMacro(UseTDx, bool);
  vtkGetMacro(UseTDx, bool);
  ///@}

  ///@{
  /**
   * Fire various events. SetEventInformation should be called just prior
   * to calling any of these methods. These methods will Invoke the
   * corresponding vtk event.
   */
  virtual void MouseMoveEvent();
  virtual void RightButtonPressEvent();
  virtual void RightButtonReleaseEvent();
  virtual void LeftButtonPressEvent();
  virtual void LeftButtonReleaseEvent();
  virtual void MiddleButtonPressEvent();
  virtual void MiddleButtonReleaseEvent();
  virtual void MouseWheelForwardEvent();
  virtual void MouseWheelBackwardEvent();
  virtual void MouseWheelLeftEvent();
  virtual void MouseWheelRightEvent();
  virtual void ExposeEvent();
  virtual void ConfigureEvent();
  virtual void EnterEvent();
  virtual void LeaveEvent();
  virtual void KeyPressEvent();
  virtual void KeyReleaseEvent();
  virtual void CharEvent();
  virtual void ExitEvent();
  virtual void FourthButtonPressEvent();
  virtual void FourthButtonReleaseEvent();
  virtual void FifthButtonPressEvent();
  virtual void FifthButtonReleaseEvent();
  ///@}

  ///@{
  /**
   * Fire various gesture based events.  These methods will Invoke the
   * corresponding vtk event.
   */
  virtual void StartPinchEvent();
  virtual void PinchEvent();
  virtual void EndPinchEvent();
  virtual void StartRotateEvent();
  virtual void RotateEvent();
  virtual void EndRotateEvent();
  virtual void StartPanEvent();
  virtual void PanEvent();
  virtual void EndPanEvent();
  virtual void TapEvent();
  virtual void LongTapEvent();
  virtual void SwipeEvent();
  ///@}

  ///@{
  /**
   * Convert multitouch events into gestures. When this is on
   * (its default) multitouch events received by this interactor
   * will be converted into gestures by VTK. If turned off the
   * raw multitouch events will be passed down.
   */
  vtkSetMacro(RecognizeGestures, bool);
  vtkGetMacro(RecognizeGestures, bool);
  ///@}

  ///@{
  /**
   * When handling gestures you can query this value to
   * determine how many pointers are down for the gesture
   * this is useful for pan gestures for example
   */
  vtkGetMacro(PointersDownCount, int);
  ///@}

  ///@{
  /**
   * Most multitouch systems use persistent contact/pointer ids to
   * track events/motion during multitouch events. We keep an array
   * that maps these system dependent contact ids to our pointer index
   * These functions return -1 if the ID is not found or if there
   * is no more room for contacts
   */
  void ClearContact(size_t contactID);
  int GetPointerIndexForContact(size_t contactID);
  int GetPointerIndexForExistingContact(size_t contactID);
  bool IsPointerIndexSet(int i);
  void ClearPointerIndex(int i);
  ///@}

  /**
   * This flag is useful when you are integrating VTK in a larger system.
   * In such cases, an application can lock up if the `Start()` method
   * in vtkRenderWindowInteractor processes events indefinitely without
   * giving the system a chance to execute anything.
   * The default value for this flag is true. It currently only affects
   * VTK webassembly applications.
   *
   * As an example with webassembly in the browser through emscripten SDK:
   * 1. If your app has an `int main` entry point, leave this value enabled.
   *    Emscripten will simulate an infinite event loop and avoid running code
   *    after `interactor->Start()` which is usually the end of `main`.
   *    Otherwise, all VTK objects will go out of scope immediately without
   *    giving a chance for user interaction with the render window.
   * 2. If your app does not have an `int main` entry point, disable this
   *    behavior.
   *    Otherwise, the webassembly application will not start up successfully.
   */
  static bool InteractorManagesTheEventLoop;

  ///@{
  /**
   * Get the current gesture that was recognized when handling multitouch and VR events.
   *
   * \sa RecognizeGestures()
   * \sa vtkVRRenderWindowInteractor::RecognizeComplexGesture()
   */
  virtual vtkCommand::EventIds GetCurrentGesture() const;
  virtual void SetCurrentGesture(vtkCommand::EventIds eid);
  ///@}

protected:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor() override;

  vtkRenderWindow* RenderWindow;
  vtkHardwareWindow* HardwareWindow;
  vtkSmartPointer<vtkInteractorObserver> InteractorStyle;

  // Used as a helper object to pick instances of vtkProp
  vtkAbstractPicker* Picker;
  vtkPickingManager* PickingManager;

  bool Done; // is the event loop done running

  /**
   * Create default pickingManager. Used to create one when none is specified.
   * Default is an instance of vtkPickingManager.
   */
  virtual vtkPickingManager* CreateDefaultPickingManager();

  int Initialized;
  int Enabled;
  bool EnableRender;
  int Style;
  vtkTypeBool LightFollowCamera;
  int ActorMode;
  double DesiredUpdateRate;
  double StillUpdateRate;

  // Event information
  int AltKey;
  int ControlKey;
  int ShiftKey;
  char KeyCode;
  double Rotation;
  double LastRotation;
  double Scale;
  double LastScale;
  double Translation[2];
  double LastTranslation[2];
  int RepeatCount;
  char* KeySym;
  int EventPosition[2];
  int LastEventPosition[2];
  int EventSize[2];
  int Size[2];
  int TimerEventId;
  int TimerEventType;
  int TimerEventDuration;
  int TimerEventPlatformId;

  int EventPositions[VTKI_MAX_POINTERS][2];
  int LastEventPositions[VTKI_MAX_POINTERS][2];
  int PointerIndex;

  size_t PointerIndexLookup[VTKI_MAX_POINTERS];

  // control the fly to
  int NumberOfFlyFrames;
  double Dolly;

  /**
   * These methods allow the interactor to control which events are
   * processed.  When the GrabFocus() method is called, then only events that
   * the supplied vtkCommands have registered are invoked. (This method is
   * typically used by widgets, i.e., subclasses of vtkInteractorObserver, to
   * grab events once an event sequence begins.) Note that the friend
   * declaration is done here to avoid doing so in the superclass vtkObject.
   */
  friend class vtkInteractorObserver;
  void GrabFocus(vtkCommand* mouseEvents, vtkCommand* keypressEvents = nullptr)
  {
    this->Superclass::InternalGrabFocus(mouseEvents, keypressEvents);
  }
  void ReleaseFocus() { this->Superclass::InternalReleaseFocus(); }

  /**
   * Widget mediators are used to resolve contention for cursors and other resources.
   */
  vtkObserverMediator* ObserverMediator;

  // Timer related members
  friend struct vtkTimerStruct;
  vtkTimerIdMap* TimerMap;     // An internal, PIMPLd map of timers and associated attributes
  unsigned long TimerDuration; // in milliseconds
  ///@{
  /**
   * Internal methods for creating and destroying timers that must be
   * implemented by subclasses. InternalCreateTimer() returns a
   * platform-specific timerId and InternalDestroyTimer() returns
   * non-zero value on success.
   */
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  int GetCurrentTimerId();
  ///@}

  // Force the interactor to handle the Start() event loop, ignoring any
  // overrides. (Overrides are registered by observing StartEvent on the
  // interactor.)
  int HandleEventLoop;

  /**
   * Run the event loop (does not return until TerminateApp is called).
   */
  virtual void StartEventLoop() {}

  bool UseTDx; // 3DConnexion device.

  // when recognizing gestures VTK will take multitouch events
  // if it receives them and convert them to gestures
  bool RecognizeGestures;
  int PointersDownCount;
  int PointersDown[VTKI_MAX_POINTERS];
  virtual void RecognizeGesture(vtkCommand::EventIds);
  int StartingEventPositions[VTKI_MAX_POINTERS][2];
  vtkCommand::EventIds CurrentGesture;

private:
  vtkRenderWindowInteractor(const vtkRenderWindowInteractor&) = delete;
  void operator=(const vtkRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
