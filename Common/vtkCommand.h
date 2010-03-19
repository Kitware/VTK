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
//
// When an instance of vtkObject invokes an event, it also passes an optional
// void pointer to a callData. This callData is NULL most of the time.
// The callData is not specific to a type of event but specific to a type
// of vtkObject invoking a specific event. For instance, vtkCommand::PickEvent
// is invoked by vtkProp with a NULL callData but is invoked by
// vtkInteractorStyleImage with a pointer to the vtkInteractorStyleImage object
// itself.
//
// Here is the list of events that may be invoked with a none NULL callData.
// - vtkCommand::ProgressEvent
//  - most of the objects return a pointer to a double value ranged between
// 0.0 and 1.0
//  - Infovis/vtkFixedWidthTextReader returns a pointer to a float value equal
// to the number of lines read so far.
// - vtkCommand::ErrorEvent
//  - an error message as a const char * string
// - vtkCommand::WarningEvent
//  - a warning message as a const char * string
// - vtkCommand::StartAnimationCueEvent
//  - a pointer to a vtkAnimationCue::AnimationCueInfo object
// - vtkCommand::EndAnimationCueEvent
//  - a pointer to a vtkAnimationCue::AnimationCueInfo object
// - vtkCommand::AnimationCueTickEvent
//  - a pointer to a vtkAnimationCue::AnimationCueInfo object
// - vtkCommand::PickEvent
//  - Common/vtkProp returns NULL
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::StartPickEvent
//  - Rendering/vtkPropPicker returns NULL
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::EndPickEvent
//  - Rendering/vtkPropPicker returns NULL
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::WrongTagEvent
//  - Parallel/vtkSocketCommunicator returns a received tag as a char *
// - vtkCommand::SelectionChangedEvent
//  - Views/vtkView returns NULL
//  - Views/vtkDataRepresentation returns a pointer to a vtkSelection
//  - Rendering/vtkInteractorStyleRubberBand2D returns an array of 5 unsigned
// integers (p1x, p1y, p2x, p2y, mode), where mode is
// vtkInteractorStyleRubberBand2D::SELECT_UNION or
// vtkInteractorStyleRubberBand2D::SELECT_NORMAL
// - vtkCommand::AnnotationChangedEvent
//  - GUISupport/Qt/vtkQtAnnotationView returns a pointer to a 
// vtkAnnotationLayers
// - vtkCommand::PlacePointEvent
//  - Widgets/vtkSeedWidget returns a pointer to an int, being the current
// handle number
// - vtkCommand::ResetWindowLevelEvent
//  - Widgets/vtkImagePlaneWidget returns an array of 2 double values (window
// and level)
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::StartWindowLevelEvent
//  - Widgets/vtkImagePlaneWidget returns an array of 2 double values (window
// and level)
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::EndWindowLevelEvent
//  - Widgets/vtkImagePlaneWidget returns an array of 2 double values (window
// and level)
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::WindowLevelEvent
//  - Widgets/vtkImagePlaneWidget returns an array of 2 double values (window
// and level)
//  - Rendering/vtkInteractorStyleImage returns a pointer to itself
// - vtkCommand::CharEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QKeyEvent *
// - vtkCommand::TimerEvent
//  - most of the objects return a to an int representing a timer id
//  - Rendering/vtkXRenderWindowTclInteractor returns NULL
//  - Widgets/vtkHoverWidget returns NULL
// - vtkCommand::CreateTimerEvent
//  - Rendering/vtkGenericRenderWindowInteractor returns a to an int
// representing a timer id
// - vtkCommand::DestroyTimerEvent
//  - Rendering/vtkGenericRenderWindowInteractor returns a to an int
// representing a timer id
// - vtkCommand::UserEvent
//  - most of the objects return NULL
//  - Infovis/vtkInteractorStyleTreeMapHover returns a pointer to a vtkIdType
// representing a pedigree id
// - vtkCommand::KeyPressEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QKeyEvent*
// - vtkCommand::KeyReleaseEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QKeyEvent*
// - vtkCommand::LeftButtonPressEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::LeftButtonReleaseEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::MouseMoveEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::MouseWheelForwardEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QWheelEvent*
// - vtkCommand::MouseWheelBackwardEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QWheelEvent*
// - vtkCommand::RightButtonPressEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::RightButtonReleaseEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::MiddleButtonPressEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::MiddleButtonReleaseEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QMouseEvent*
// - vtkCommand::CursorChangedEvent
//  - most of the objects return a pointer to an int representing a shape
//  - Rendering/vtkInteractorObserver returns NULL
// - vtkCommand::ResetCameraEvent
//  - Rendering/vtkRenderer returns a pointer to itself
// - vtkCommand::ResetCameraClippingRangeEvent
//  - Rendering/vtkRenderer returns a pointer to itself
// - vtkCommand::ActiveCameraEvent
//  - Rendering/vtkRenderer returns a pointer to the active camera
// - vtkCommand::CreateCameraEvent
//  - Rendering/vtkRenderer returns a pointer to the created camera
// - vtkCommand::EnterEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QEvent*
// - vtkCommand::LeaveEvent
//  - most of the objects return NULL
//  - GUISupport/Qt/QVTKWidget returns a QEvent*
// - vtkCommand::RenderWindowMessageEvent
//  - Rendering/vtkWin32OpenGLRenderWindow return a pointer to a UINT message
// - vtkCommand::ComputeVisiblePropBoundsEvent
//  - Rendering/vtkRenderer returns a pointer to itself
// - QVTKWidget::ContextMenuEvent
//  - GUISupport/Qt/QVTKWidget returns a QContextMenuEvent*
// - QVTKWidget::DragEnterEvent
//  - GUISupport/Qt/QVTKWidget returns a QDragEnterEvent*
// - QVTKWidget::DragMoveEvent
//  - GUISupport/Qt/QVTKWidget returns a QDragMoveEvent*
// - QVTKWidget::DragLeaveEvent
//  - GUISupport/Qt/QVTKWidget returns a QDragLeaveEvent*
// - QVTKWidget::DropEvent
//  - GUISupport/Qt/QVTKWidget returns a QDropEvent*
// - vtkCommand::ViewProgressEvent
//  - View/vtkView returns a ViewProgressEventCallData*
// - vtkCommand::VolumeMapperRenderProgressEvent
//  - A pointer to a double value between 0.0 and 1.0
// - vtkCommand::VolumeMapperComputeGradientsProgressEvent
//  - A pointer to a double value between 0.0 and 1.0
// - vtkCommand::TDxMotionEvent (TDx=3DConnexion)
//  - A vtkTDxMotionEventInfo*
// - vtkCommand::TDxButtonPressEvent
//  - A int* being the number of the button
// - vtkCommand::TDxButtonReleaseEvent
//  - A int* being the number of the button
//
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
  
  // Description:
  // Set/Get the passive observer flag. If this is set to true, this
  // indicates that this command does not change the state of the
  // system in any way. Passive observers are processed first, and
  // are not called even when another command has focus.
  void SetPassiveObserver(int f)  
    { this->PassiveObserver = f; }
  int GetPassiveObserver() 
    { return this->PassiveObserver; }
  void PassiveObserverOn() 
    { this->SetPassiveObserver(1); }
  void PassiveObserverOff() 
    { this->SetPassiveObserver(0); }
  
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
    ActiveCameraEvent,
    CreateCameraEvent,
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
    UpdateInformationEvent,
    AnnotationChangedEvent,
    SelectionChangedEvent,
    UpdatePropertyEvent,
    ViewProgressEvent,
    UpdateDataEvent,
    CurrentChangedEvent,
    ComputeVisiblePropBoundsEvent,
    TDxMotionEvent, // 3D Connexion device event
    TDxButtonPressEvent, // 3D Connexion device event
    TDxButtonReleaseEvent, // 3D Connexion device event
    HoverEvent,
    LoadStateEvent,
    SaveStateEvent,
    StateChangedEvent,
    UserEvent = 1000
  };
//ETX

protected:
  int AbortFlag;
  int PassiveObserver;

  vtkCommand();
  virtual ~vtkCommand() {}

  friend class vtkSubjectHelper;
//BTX
  vtkCommand(const vtkCommand& c) : vtkObjectBase(c) {}
  void operator=(const vtkCommand&) {}
//ETX
};

#endif /* __vtkCommand_h */
 
