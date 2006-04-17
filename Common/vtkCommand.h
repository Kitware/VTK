/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCommand - superclass for callback/observer methods
// .SECTION Description
// vtkCommand is an implementation of the observer/command design
// pattern.  In this design pattern, any instance of vtkObject can be
// "observed" for any events it might invoke. For example,
// vtkRenderer invokes a StartEvent as it begins to render and a
// EndEvent when it finishes rendering. Filters (subclasses of
// vtkProcessObject) invoke StartEvent, ProgressEvent, and EndEvent as
// the filter processes data. Observers of events are added with the
// AddObserver() method found in vtkObject.  AddObserver(), besides
// requiring an event id or name, also takes an instance of vtkCommand
// (or a subclasses). Note that vtkCommand is meant to be subclassed,
// so that you can package the information necessary to support your
// callback.
//
// Event processing can be organized in priority lists, so it is
// possible to truncate the processing of a particular event by
// setting the AbortFlag variable. The priority is set using the
// AddObserver() method.  By default the priority is 0, events of the
// same priority are processed in last-in-first-processed order. The
// ordering/aborting of events is important for things like 3D
// widgets, which handle an event if the widget is selected (and then
// aborting further processing of that event).  Otherwise. the event
// is passed along for further processing.

// .SECTION See Also
// vtkObject vtkCallbackCommand vtkOldStyleCallbackCommand
// vtkInteractorObserver vtk3DWidget

#ifndef __vtkCommand_h
#define __vtkCommand_h

#include "vtkObjectBase.h"

class vtkObject;

// The superclass that all commands should be subclasses of
class VTK_COMMON_EXPORT vtkCommand : public vtkObjectBase
{
public:
  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  void UnRegister();
  virtual void UnRegister(vtkObjectBase *) 
    { this->UnRegister(); }
  
  // Description:
  // All derived classes of vtkCommand must implement this
  // method. This is the method that actually does the work of the
  // callback. The caller argument is the object invoking the event,
  // the eventId parameter is the id of the event, and callData
  // parameter is data that can be passed into the execute
  // method. (Note: vtkObject::InvokeEvent() takes two parameters: the
  // event id (or name) and call data. Typically call data is NULL,
  // but the user can package data and pass it this
  // way. Alternatively, a derived class of vtkCommand can be used to
  // pass data.)
  virtual void Execute(vtkObject *caller, unsigned long eventId, 
                       void *callData) = 0;

  // Description:
  // Convenience methods for translating between event names and event
  // ids.
  static const char *GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char *event);

  // Description:
  // Set/Get the abort flag. If this is set to true no further
  // commands are executed.
  void SetAbortFlag(int f)  
    { this->AbortFlag = f; }
  int GetAbortFlag() 
    { return this->AbortFlag; }
  void AbortFlagOn() 
    { this->SetAbortFlag(1); }
  void AbortFlagOff() 
    { this->SetAbortFlag(0); }
  
//BTX
  // Description:
  // All the currently defined events are listed here.  Developers can
  // use -- vtkCommand::UserEvent + int to specify their own event
  // ids. If this list is adjusted, be sure to adjust
  // vtkCommandEventStrings in vtkCommand.cxx to match.
  enum EventIds {
    NoEvent = 0,
    AnyEvent,
    DeleteEvent,
    StartEvent,
    EndEvent,
    RenderEvent,
    ProgressEvent,
    PickEvent,
    StartPickEvent,
    EndPickEvent,
    AbortCheckEvent,
    ExitEvent,
    LeftButtonPressEvent,
    LeftButtonReleaseEvent,
    MiddleButtonPressEvent,
    MiddleButtonReleaseEvent,
    RightButtonPressEvent,
    RightButtonReleaseEvent,
    EnterEvent,
    LeaveEvent,
    KeyPressEvent,
    KeyReleaseEvent,
    CharEvent,
    ExposeEvent,
    ConfigureEvent,
    TimerEvent,
    MouseMoveEvent,
    MouseWheelForwardEvent,
    MouseWheelBackwardEvent,
    ResetCameraEvent,
    ResetCameraClippingRangeEvent,
    ModifiedEvent,
    WindowLevelEvent,
    StartWindowLevelEvent,
    EndWindowLevelEvent,
    ResetWindowLevelEvent,
    SetOutputEvent,
    ErrorEvent,
    WarningEvent,
    StartInteractionEvent, //mainly used by vtkInteractorObservers
    InteractionEvent,
    EndInteractionEvent,
    EnableEvent,
    DisableEvent,
    CreateTimerEvent,
    DestroyTimerEvent,
    PlacePointEvent,
    PlaceWidgetEvent,
    CursorChangedEvent,
    ExecuteInformationEvent,
    RenderWindowMessageEvent,
    WrongTagEvent,
    StartAnimationCueEvent, // used by vtkAnimationCue
    AnimationCueTickEvent,
    EndAnimationCueEvent,
    VolumeMapperRenderEndEvent,
    VolumeMapperRenderProgressEvent,
    VolumeMapperRenderStartEvent,
    VolumeMapperComputeGradientsEndEvent,
    VolumeMapperComputeGradientsProgressEvent,
    VolumeMapperComputeGradientsStartEvent,
    WidgetModifiedEvent,
    WidgetValueChangedEvent,
    WidgetActivateEvent,
    ConnectionCreatedEvent, 
    ConnectionClosedEvent,
    DomainModifiedEvent,
    PropertyModifiedEvent,
    UpdateEvent,
    RegisterEvent,
    UnRegisterEvent,
    UserEvent = 1000
  };
//ETX

protected:
  int AbortFlag;
  vtkCommand();
  virtual ~vtkCommand() {}

  friend class vtkSubjectHelper;
//BTX
  vtkCommand(const vtkCommand& c) : vtkObjectBase(c) {}
  void operator=(const vtkCommand&) {}
//ETX
};

#endif /* __vtkCommand_h */
 
