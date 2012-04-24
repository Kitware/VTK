/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContextInteractorStyle.h"

#include "vtkContextMouseEvent.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextScene.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <cassert>

vtkStandardNewMacro(vtkContextInteractorStyle);

//--------------------------------------------------------------------------
vtkContextInteractorStyle::vtkContextInteractorStyle()
{
  this->Scene = NULL;
  this->ProcessingEvents = 0;
  this->SceneCallbackCommand->SetClientData(this);
  this->SceneCallbackCommand->SetCallback(
    vtkContextInteractorStyle::ProcessSceneEvents);
  this->InteractorCallbackCommand->SetClientData(this);
  this->InteractorCallbackCommand->SetCallback(
    vtkContextInteractorStyle::ProcessInteractorEvents);
  this->LastSceneRepaintMTime = 0;
  this->TimerId = 0;
  this->TimerCallbackInitialized = false;
}

//--------------------------------------------------------------------------
vtkContextInteractorStyle::~vtkContextInteractorStyle()
{
  // to remove observers.
  this->SetScene(0);
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scene: " << this->Scene << endl;
  if (this->Scene)
    {
    this->Scene->PrintSelf(os, indent.GetNextIndent());
    }
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::SetScene(vtkContextScene* scene)
{
  if (this->Scene == scene)
    {
    return;
    }
  if (this->Scene)
    {
    this->Scene->RemoveObserver(this->SceneCallbackCommand.GetPointer());
    }

  this->Scene = scene;

  if (this->Scene)
    {
    this->Scene->AddObserver(vtkCommand::ModifiedEvent,
                             this->SceneCallbackCommand.GetPointer(),
                             this->Priority);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkContextScene* vtkContextInteractorStyle::GetScene()
{
  return this->Scene;
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::ProcessSceneEvents(vtkObject*,
                                                   unsigned long event,
                                                   void* clientdata,
                                                   void* vtkNotUsed(calldata))
{
  vtkContextInteractorStyle* self =
    reinterpret_cast<vtkContextInteractorStyle *>( clientdata );
  switch (event)
    {
    case vtkCommand::ModifiedEvent:
      self->OnSceneModified();
      break;
    default:
      break;
    }
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::ProcessInteractorEvents(vtkObject*,
                                                        unsigned long,
                                                        void* clientdata,
                                                        void* vtkNotUsed(calldata))
{
  vtkContextInteractorStyle* self =
    reinterpret_cast<vtkContextInteractorStyle *>(clientdata);
  self->RenderNow();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::RenderNow()
{
  this->TimerId = 0;
  if (this->Scene && !this->ProcessingEvents &&
      this->Interactor->GetInitialized())
    {
    this->Interactor->GetRenderWindow()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnSceneModified()
{
  if (!this->Scene
      || !this->Scene->GetDirty()
      || this->ProcessingEvents
      || this->Scene->GetMTime() == this->LastSceneRepaintMTime
      || !this->Interactor->GetInitialized())
    {
    return;
    }
  this->BeginProcessingEvent();
  if (!this->TimerCallbackInitialized && this->Interactor)
    {
    this->Interactor->AddObserver(vtkCommand::TimerEvent,
                                  this->InteractorCallbackCommand.GetPointer(),
                                  0.0);
    this->TimerCallbackInitialized = true;
    }
  this->LastSceneRepaintMTime = this->Scene->GetMTime();
  // If there is no timer, create a one shot timer to render an updated scene
  if (this->TimerId == 0)
    {
    this->Interactor->CreateOneShotTimer(40);
    }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::BeginProcessingEvent()
{
  ++this->ProcessingEvents;
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::EndProcessingEvent()
{
  --this->ProcessingEvents;
  assert(this->ProcessingEvents >= 0);
  if (this->ProcessingEvents == 0)
    {
    this->OnSceneModified();
    }
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnMouseMove()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::NO_BUTTON);
    eatEvent = this->Scene->MouseMoveEvent(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMouseMove();
    }

  this->EndProcessingEvent();
}

inline bool vtkContextInteractorStyle::ProcessMousePress(
    const vtkContextMouseEvent &event)
{
  bool eatEvent(false);
  if (this->Interactor->GetRepeatCount())
    {
    eatEvent = this->Scene->DoubleClickEvent(event);
    //
    // The second ButtonRelease event seems not to be processed automatically,
    // need manually processing here so that the following MouseMove event will
    // not think the mouse button is still pressed down, and we don't really
    // care about the return result from the second ButtonRelease.
    //
    if (eatEvent)
      {
      this->Scene->ButtonReleaseEvent(event);
      }
    }
  else
    {
    eatEvent = this->Scene->ButtonPressEvent(event);
    }
  return eatEvent;
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnLeftButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::LEFT_BUTTON);
    eatEvent = this->ProcessMousePress(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnLeftButtonDown();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnLeftButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::LEFT_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnLeftButtonUp();
    }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnMiddleButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->ProcessMousePress(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMiddleButtonDown();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnMiddleButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMiddleButtonUp();
    }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnRightButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::RIGHT_BUTTON);
    eatEvent = this->ProcessMousePress(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnRightButtonDown();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnRightButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::RIGHT_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnRightButtonUp();
    }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnMouseWheelForward()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->MouseWheelEvent(
          static_cast<int>(this->MouseWheelMotionFactor), event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMouseWheelForward();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnMouseWheelBackward()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    vtkContextMouseEvent event;
    this->ConstructMouseEvent(event, vtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->MouseWheelEvent(
          -static_cast<int>(this->MouseWheelMotionFactor), event);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMouseWheelBackward();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnSelection(unsigned int rect[5])
{
  this->BeginProcessingEvent();
  if (this->Scene)
    {
    this->Scene->ProcessSelectionEvent(rect);
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnChar()
{
  this->Superclass::OnChar();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnKeyPress()
{
  this->BeginProcessingEvent();
  vtkContextKeyEvent event;
  vtkVector2i position(this->Interactor->GetEventPosition()[0],
                       this->Interactor->GetEventPosition()[0]);
  event.SetInteractor(this->Interactor);
  event.SetPosition(position);
  bool keepEvent = false;
  if (this->Scene)
    {
    keepEvent = this->Scene->KeyPressEvent(event);
    }
  if (!keepEvent)
    {
    this->Superclass::OnKeyPress();
    }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void vtkContextInteractorStyle::OnKeyRelease()
{
  this->BeginProcessingEvent();
  vtkContextKeyEvent event;
  vtkVector2i position(this->Interactor->GetEventPosition()[0],
                       this->Interactor->GetEventPosition()[0]);
  event.SetInteractor(this->Interactor);
  event.SetPosition(position);
  bool keepEvent = false;
  if (this->Scene)
    {
    keepEvent = this->Scene->KeyReleaseEvent(event);
    }
  if (!keepEvent)
    {
    this->Superclass::OnKeyRelease();
    }
  this->EndProcessingEvent();
}

//-------------------------------------------------------------------------
inline void vtkContextInteractorStyle::ConstructMouseEvent(
    vtkContextMouseEvent &event, int button)
{
  event.SetInteractor(this->Interactor);
  event.SetPos(vtkVector2f(this->Interactor->GetEventPosition()[0],
                           this->Interactor->GetEventPosition()[1]));
  event.SetButton(button);
}
