/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkGenericRenderWindowInteractor, "1.1");
vtkStandardNewMacro(vtkGenericRenderWindowInteractor);
// Construct object so that light follows camera motion.
vtkGenericRenderWindowInteractor::vtkGenericRenderWindowInteractor()
{
}

void vtkGenericRenderWindowInteractor::MouseMoveEvent()
{
  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

void vtkGenericRenderWindowInteractor::RightButtonPressEvent()
{
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

void vtkGenericRenderWindowInteractor::RightButtonReleaseEvent()
{
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

void vtkGenericRenderWindowInteractor::LeftButtonPressEvent()
{
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

void vtkGenericRenderWindowInteractor::LeftButtonReleaseEvent()
{
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

void vtkGenericRenderWindowInteractor::MiddleButtonPressEvent()
{
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

void vtkGenericRenderWindowInteractor::MiddleButtonReleaseEvent()
{
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

void vtkGenericRenderWindowInteractor::ConfigureEvent()
{
  this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
}

void vtkGenericRenderWindowInteractor::EnterEvent()
{
  this->InvokeEvent(vtkCommand::EnterEvent, NULL);
}

void vtkGenericRenderWindowInteractor::LeaveEvent()
{
  this->InvokeEvent(vtkCommand::LeaveEvent, NULL);
}

void vtkGenericRenderWindowInteractor::TimerEvent()
{
  this->InvokeEvent(vtkCommand::TimerEvent, NULL);
}

void vtkGenericRenderWindowInteractor::KeyPressEvent()
{
  this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
}

void vtkGenericRenderWindowInteractor::KeyReleaseEvent()
{
  this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

void vtkGenericRenderWindowInteractor::CharEvent()
{
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

void vtkGenericRenderWindowInteractor::ExitEvent()
{
  this->InvokeEvent(vtkCommand::ExitEvent, NULL);
}

  
vtkGenericRenderWindowInteractor::~vtkGenericRenderWindowInteractor()
{
}


void vtkGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkGenericRenderWindowInteractor::CreateTimer(int )
{
  if(this->HasObserver(vtkCommand::CreateTimerEvent))
    {
    this->InvokeEvent(vtkCommand::CreateTimerEvent, NULL);
    return 1;
    }
  return 0;
}

int vtkGenericRenderWindowInteractor::DestroyTimer()
{
  if(this->HasObserver(vtkCommand::CreateTimerEvent))
    {
    this->InvokeEvent(vtkCommand::CreateTimerEvent, NULL);
    return 1;
    }
  return 0;
}








