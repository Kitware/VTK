/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUser.cxx
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
#include "vtkInteractorStyleUser.h"
#include "vtkMath.h"
#include "vtkCellPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkOldStyleCallbackCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleUser, "1.23");
vtkStandardNewMacro(vtkInteractorStyleUser);

//----------------------------------------------------------------------------
vtkInteractorStyleUser::vtkInteractorStyleUser()
{
  this->MouseMoveTag = 0;
  this->KeyPressTag = 0;
  this->KeyReleaseTag = 0;
  this->CharTag = 0;
  this->EnterTag = 0;
  this->LeaveTag = 0;
  this->ConfigureTag = 0;
  this->TimerTag = 0;
  this->UserTag = 0;
  // Tell the parent class not to handle observers
  // that has to be done here
  this->HandleObserversOff();
  this->LastPos[0] = this->LastPos[1] = 0;
  this->OldPos[0] = this->OldPos[1] = 0;
  this->Char = '\0';
  this->KeySym = (char *) "";
  this->Button = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleUser::~vtkInteractorStyleUser() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LastPos: (" << this->LastPos[0] << ", " 
                               << this->LastPos[1] << ")\n";  
  os << indent << "OldPos: (" << this->OldPos[0] << ", " 
                              << this->OldPos[1] << ")\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "CtrlKey: " << this->CtrlKey << "\n";
  os << indent << "Char: " << this->Char << "\n";
  os << indent << "KeySym: " << this->KeySym << "\n";
  os << indent << "Button: " << this->Button << "\n";
}

void vtkInteractorStyleUser::vtkSetOldCallback(unsigned long &tag, 
                                               unsigned long event, 
                                               void (*f)(void *), void *arg)
{
  if ( tag )
    {
    this->RemoveObserver(tag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    tag = this->AddObserver(event,cbc);
    cbc->Delete();
    }
}

void vtkInteractorStyleUser::vtkSetOldDelete(unsigned long tag, void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(tag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetMouseMoveMethod(void (*f)(void *), 
                                                      void *arg)
{
  this->vtkSetOldCallback(this->MouseMoveTag,
                    vtkCommand::MouseMoveEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetMouseMoveMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->MouseMoveTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetButtonPressMethod(void (*f)(void *), 
                                                  void *arg)
{
  this->SetLeftButtonPressMethod(f,arg);
  this->SetMiddleButtonPressMethod(f,arg);
  this->SetRightButtonPressMethod(f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetButtonPressMethodArgDelete(void (*f)(void *))
{
  this->SetLeftButtonPressMethodArgDelete(f);
  this->SetMiddleButtonPressMethodArgDelete(f);
  this->SetRightButtonPressMethodArgDelete(f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetButtonReleaseMethod(void (*f)(void *), 
                                                    void *arg)
{
  this->SetLeftButtonReleaseMethod(f,arg);
  this->SetMiddleButtonReleaseMethod(f,arg);
  this->SetRightButtonReleaseMethod(f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetButtonReleaseMethodArgDelete(void (*f)(void *))
{
  this->SetLeftButtonReleaseMethodArgDelete(f);
  this->SetMiddleButtonReleaseMethodArgDelete(f);
  this->SetRightButtonReleaseMethodArgDelete(f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetKeyPressMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->KeyPressTag,
                    vtkCommand::KeyPressEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetKeyPressMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->KeyPressTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetKeyReleaseMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->KeyReleaseTag,
                    vtkCommand::KeyReleaseEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetKeyReleaseMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->KeyReleaseTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetEnterMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->EnterTag,
                    vtkCommand::EnterEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetEnterMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->EnterTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetLeaveMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->LeaveTag,
                    vtkCommand::LeaveEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetLeaveMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->LeaveTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetConfigureMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->ConfigureTag,
                    vtkCommand::ConfigureEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetConfigureMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->ConfigureTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetCharMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->CharTag,
                    vtkCommand::CharEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetCharMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->CharTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetTimerMethod(void (*f)(void *), void *arg)
{
  this->vtkSetOldCallback(this->TimerTag,
                    vtkCommand::TimerEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetTimerMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->TimerTag, f);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetUserInteractionMethod(void (*f)(void *), 
                                                      void *arg)
{
  this->vtkSetOldCallback(this->UserTag,
                    vtkCommand::UserEvent,f,arg);
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetUserInteractionMethodArgDelete(void (*f)(void *))
{
  this->vtkSetOldDelete(this->UserTag, f);
}

//----------------------------------------------------------------------------
void  vtkInteractorStyleUser::StartUserInteraction() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_USERINTERACTION);
}

//----------------------------------------------------------------------------
void  vtkInteractorStyleUser::EndUserInteraction() 
{
  if (this->State != VTKIS_USERINTERACTION) 
    {
    return;
    }
  this->StopState();
}

//----------------------------------------------------------------------------
// checks for USERINTERACTION state, then defers to the superclass modes
void vtkInteractorStyleUser::OnTimer(void) 
{
  if (this->HasObserver(vtkCommand::TimerEvent)) 
    {
    this->InvokeEvent(vtkCommand::TimerEvent,NULL);
    }

  if (this->State == VTKIS_USERINTERACTION)
    {
    if (this->HasObserver(vtkCommand::UserEvent)) 
      {
      this->InvokeEvent(vtkCommand::UserEvent,NULL);
      this->OldPos[0] = this->LastPos[0];
      this->OldPos[1] = this->LastPos[1];
      this->Interactor->CreateTimer(VTKI_TIMER_UPDATE);
      }
    }
  else if (!(this->HasObserver(vtkCommand::MouseMoveEvent) && 
             (this->Button == 0 ||
              (this->HasObserver(vtkCommand::LeftButtonPressEvent) && this->Button == 1) ||
              (this->HasObserver(vtkCommand::MiddleButtonPressEvent) && this->Button == 2) ||
              (this->HasObserver(vtkCommand::RightButtonPressEvent) && this->Button == 3))))
    {
    this->vtkInteractorStyleSwitch::OnTimer();
    }
  else if (this->HasObserver(vtkCommand::TimerEvent))
    {
    this->Interactor->CreateTimer(VTKI_TIMER_UPDATE);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnKeyPress(int ctrl, int shift, 
                                        char keycode, char *keysym, 
                                        int vtkNotUsed(repeatcount))
{
  if (this->HasObserver(vtkCommand::KeyPressEvent)) 
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->KeySym = keysym;
    this->Char = keycode;  
    this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnKeyRelease(int ctrl, int shift, 
                                          char keycode, char *keysym, 
                                          int vtkNotUsed(repeatcount))
{
  if (this->HasObserver(vtkCommand::KeyReleaseEvent)) 
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->KeySym = keysym;
    this->Char = keycode;  

    this->InvokeEvent(vtkCommand::KeyReleaseEvent,NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnChar(int ctrl, int shift, char keycode,
                                    int repeatcount) 
{
  // do nothing if a KeyPressMethod has been set,
  // otherwise pass the OnChar to the vtkInteractorStyleSwitch.
  if (this->HasObserver(vtkCommand::CharEvent)) 
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->Char = keycode;  
    
    this->InvokeEvent(vtkCommand::CharEvent,NULL);
    }
  else
    {
    this->vtkInteractorStyleSwitch::OnChar(ctrl,shift,keycode,repeatcount);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnRightButtonDown(int ctrl, int shift, int x, int y) 
{
  this->Button = 3;

  if (this->HasObserver(vtkCommand::RightButtonPressEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnRightButtonDown(ctrl, shift, x, y);
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnRightButtonUp(int ctrl, int shift, int x, int y) 
{
  if (this->HasObserver(vtkCommand::RightButtonReleaseEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnRightButtonUp(ctrl, shift, x, y);
    }

  if (this->Button == 3)
    {
    this->Button = 0;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnMiddleButtonDown(int ctrl,int shift,int x,int y) 
{
  this->Button = 2;

  if (this->HasObserver(vtkCommand::MiddleButtonPressEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnMiddleButtonDown(ctrl, shift, x, y);
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnMiddleButtonUp(int ctrl,int shift,int x,int y) 
{
  if (this->HasObserver(vtkCommand::MiddleButtonReleaseEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnMiddleButtonUp(ctrl, shift, x, y);
    }

  if (this->Button == 2)
    {
    this->Button = 0;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnLeftButtonDown(int ctrl,int shift,int x,int y) 
{
  this->Button = 1;

  if (this->HasObserver(vtkCommand::LeftButtonPressEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnLeftButtonDown(ctrl, shift, x, y);
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnLeftButtonUp(int ctrl, int shift, int x, int y) 
{
  if (this->HasObserver(vtkCommand::LeftButtonReleaseEvent)) 
    {
    this->CtrlKey  = ctrl;
    this->ShiftKey = shift;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    // this last one is for backwards compatibility
    this->Interactor->SetEventPosition(x, y);

    this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
  else 
    {
    this->vtkInteractorStyleSwitch::OnLeftButtonUp(ctrl, shift, x, y);
    }

  if (this->Button == 1)
    {
    this->Button = 0;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnMouseMove(int ctrl, int shift, int x, int y) 
{
  this->vtkInteractorStyleSwitch::OnMouseMove(ctrl,shift,x,y);

  
  this->LastPos[0] = x;
  this->LastPos[1] = y;
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;

  if (this->HasObserver(vtkCommand::MouseMoveEvent)) 
    {
    this->InvokeEvent(vtkCommand::MouseMoveEvent,NULL);

    this->OldPos[0] = x;
    this->OldPos[1] = y;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnConfigure(int vtkNotUsed(width), 
                                         int vtkNotUsed(height)) 
{
  if (this->HasObserver(vtkCommand::ConfigureEvent)) 
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent,NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnEnter(int ctrl, int shift, int x, int y)
{
  if (this->HasObserver(vtkCommand::EnterEvent)) 
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->LastPos[0] = x;
    this->LastPos[1] = y;

    this->InvokeEvent(vtkCommand::EnterEvent,NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnLeave(int ctrl, int shift, int x, int y)
{
  if (this->HasObserver(vtkCommand::LeaveEvent)) 
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->LastPos[0] = x;
    this->LastPos[1] = y;

    this->InvokeEvent(vtkCommand::LeaveEvent,NULL);
    }
}



