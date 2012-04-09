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
