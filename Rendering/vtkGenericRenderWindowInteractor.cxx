/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkGenericRenderWindowInteractor);

//------------------------------------------------------------------
vtkGenericRenderWindowInteractor::vtkGenericRenderWindowInteractor()
{
  this->TimerEventResetsTimer = 1;
}

//------------------------------------------------------------------
vtkGenericRenderWindowInteractor::~vtkGenericRenderWindowInteractor()
{
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::MouseMoveEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::RightButtonPressEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::RightButtonReleaseEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::LeftButtonPressEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::LeftButtonReleaseEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::MiddleButtonPressEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::MiddleButtonReleaseEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::MouseWheelForwardEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::MouseWheelBackwardEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::ExposeEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::ExposeEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::ConfigureEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::EnterEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::EnterEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::LeaveEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::LeaveEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::TimerEvent()
{
  if (!this->Enabled) 
    {
    return;
    }

  int timerId = this->GetCurrentTimerId();
  this->InvokeEvent(vtkCommand::TimerEvent, &timerId);

  if (!this->IsOneShotTimer(timerId) &&
    this->GetTimerEventResetsTimer())
    {
    this->ResetTimer(timerId);
    }
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::KeyPressEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::KeyReleaseEvent()
{
  if (!this->Enabled)
    {
    return;
    }
  this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::CharEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::ExitEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::ExitEvent, NULL);
}

//------------------------------------------------------------------
int vtkGenericRenderWindowInteractor::InternalCreateTimer(int timerId, int timerType,
                                                          unsigned long duration)
{
  if(this->HasObserver(vtkCommand::CreateTimerEvent))
    {
    this->SetTimerEventId(timerId);
    this->SetTimerEventType(timerType);
    this->SetTimerEventDuration(duration);
    this->SetTimerEventPlatformId(timerId);
    this->InvokeEvent(vtkCommand::CreateTimerEvent, &timerId);
    return this->GetTimerEventPlatformId();
    }
  return 0;
}

//------------------------------------------------------------------
int vtkGenericRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  if(this->HasObserver(vtkCommand::DestroyTimerEvent))
    {
    this->SetTimerEventPlatformId(platformTimerId);
    this->InvokeEvent(vtkCommand::DestroyTimerEvent, &platformTimerId);
    return 1;
    }
  return 0;
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TimerEventResetsTimer: " << this->TimerEventResetsTimer << "\n";
}
