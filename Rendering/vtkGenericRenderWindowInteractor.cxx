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

vtkCxxRevisionMacro(vtkGenericRenderWindowInteractor, "1.7");
vtkStandardNewMacro(vtkGenericRenderWindowInteractor);

//------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkGenericRenderWindowInteractor::vtkGenericRenderWindowInteractor()
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
  int timerId = 1;
  this->InvokeEvent(vtkCommand::TimerEvent, (void*)&timerId);
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
vtkGenericRenderWindowInteractor::~vtkGenericRenderWindowInteractor()
{
}

//------------------------------------------------------------------
void vtkGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------
int vtkGenericRenderWindowInteractor::InternalCreateTimer(int, int,
                                                          unsigned long)
{
  if(this->HasObserver(vtkCommand::CreateTimerEvent))
    {
    int timerId = 1;
    this->InvokeEvent(vtkCommand::CreateTimerEvent, (void*)&timerId);
    return 1;
    }
  return 0;
}

//------------------------------------------------------------------
int vtkGenericRenderWindowInteractor::DestroyTimer(int)
{
  if(this->HasObserver(vtkCommand::DestroyTimerEvent))
    {
    int timerId = 1;
    this->InvokeEvent(vtkCommand::DestroyTimerEvent, (void*)&timerId);
    return 1;
    }
  return 0;
}
