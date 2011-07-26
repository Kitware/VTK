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
  this->SceneCallbackCommand = vtkCallbackCommand::New();
  this->SceneCallbackCommand->SetClientData(this);
  this->SceneCallbackCommand->SetCallback(
    vtkContextInteractorStyle::ProcessSceneEvents);
  this->LastSceneRepaintMTime = 0;
}

//--------------------------------------------------------------------------
vtkContextInteractorStyle::~vtkContextInteractorStyle()
{
  if (this->SceneCallbackCommand)
    {
    this->SceneCallbackCommand->Delete();
    this->SceneCallbackCommand = 0;
    }
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
    this->Scene->RemoveObserver(this->SceneCallbackCommand);
    }

  this->Scene = scene;

  if (this->Scene)
    {
    this->Scene->AddObserver(vtkCommand::ModifiedEvent,
                             this->SceneCallbackCommand,
                             this->Priority);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::ProcessSceneEvents(vtkObject* vtkNotUsed(object),
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
  this->LastSceneRepaintMTime = this->Scene->GetMTime();
  this->Interactor->GetRenderWindow()->Render();
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent = this->Scene->MouseMoveEvent(x, y);
    }
  if (!eatEvent)
    {
    this->Superclass::OnMouseMove();
    }

  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void vtkContextInteractorStyle::OnLeftButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
    {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    if (this->Interactor->GetRepeatCount())
      {
      eatEvent =
        this->Scene->DoubleClickEvent(vtkContextMouseEvent::LEFT_BUTTON, x, y);
      }
    else
      {
      eatEvent =
        this->Scene->ButtonPressEvent(vtkContextMouseEvent::LEFT_BUTTON, x, y);
      }
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent =
      this->Scene->ButtonReleaseEvent(vtkContextMouseEvent::LEFT_BUTTON, x, y);
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    if (this->Interactor->GetRepeatCount())
      {
      eatEvent =
        this->Scene->DoubleClickEvent(vtkContextMouseEvent::MIDDLE_BUTTON, x, y);
      }
    else
      {
      eatEvent =
        this->Scene->ButtonPressEvent(vtkContextMouseEvent::MIDDLE_BUTTON, x, y);
      }
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent =
      this->Scene->ButtonReleaseEvent(vtkContextMouseEvent::MIDDLE_BUTTON, x, y);
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    if (this->Interactor->GetRepeatCount())
      {
      eatEvent =
        this->Scene->DoubleClickEvent(vtkContextMouseEvent::RIGHT_BUTTON, x, y);
      }
    else
      {
      eatEvent =
        this->Scene->ButtonPressEvent(vtkContextMouseEvent::RIGHT_BUTTON, x, y);
      }
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent =
      this->Scene->ButtonReleaseEvent(vtkContextMouseEvent::RIGHT_BUTTON, x, y);
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent =
      this->Scene->MouseWheelEvent(static_cast<int>(this->MouseWheelMotionFactor), x, y);
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
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    eatEvent =
      this->Scene->MouseWheelEvent(-static_cast<int>(this->MouseWheelMotionFactor), x, y);
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
