/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUser.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInteractorStyleUser.h"
#include "vtkMath.h"
#include "vtkCellPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkInteractorStyleUser* vtkInteractorStyleUser::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleUser");
  if(ret)
    {
    return (vtkInteractorStyleUser*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleUser;
}

//----------------------------------------------------------------------------
vtkInteractorStyleUser::vtkInteractorStyleUser()
{
  this->UserInteractionMethod = NULL;
  this->UserInteractionMethodArgDelete = NULL;
  this->UserInteractionMethodArg = NULL;
  this->KeyPressMethod = NULL;
  this->KeyPressMethodArgDelete = NULL;
  this->KeyPressMethodArg = NULL;
  this->KeyReleaseMethod = NULL;
  this->KeyReleaseMethodArgDelete = NULL;
  this->KeyReleaseMethodArg = NULL;
  this->EnterMethod = NULL;
  this->EnterMethodArgDelete = NULL;
  this->EnterMethodArg = NULL;
  this->LeaveMethod = NULL;
  this->LeaveMethodArgDelete = NULL;
  this->LeaveMethodArg = NULL;
  this->ConfigureMethod = NULL;
  this->ConfigureMethodArgDelete = NULL;
  this->ConfigureMethodArg = NULL;
  this->CharMethod = NULL;
  this->CharMethodArgDelete = NULL;
  this->CharMethodArg = NULL;

  this->LastPos[0] = this->LastPos[1] = 0;
  this->OldPos[0] = this->OldPos[1] = 0;
  this->Char = '\0';
  this->KeySym = "";
}

//----------------------------------------------------------------------------
vtkInteractorStyleUser::~vtkInteractorStyleUser() 
{
  if ((this->UserInteractionMethodArg)&&
      (this->UserInteractionMethodArgDelete))
    {
    (*this->UserInteractionMethodArgDelete)(this->UserInteractionMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->vtkInteractorStyleTrackball::PrintSelf(os,indent);

  os << indent << "LastPos: (" << this->LastPos[0] << ", " 
                               << this->LastPos[1] << ")\n";  
  os << indent << "OldPos: (" << this->OldPos[0] << ", " 
                              << this->OldPos[1] << ")\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "CtrlKey: " << this->CtrlKey << "\n";
  os << indent << "Char: " << this->Char << "\n";
  os << indent << "KeySym: " << this->KeySym << "\n";
  os << indent << "ActorMode: " << this->ActorMode << "\n";
  os << indent << "TrackballMode: " << this->TrackballMode << "\n";
  os << indent << "ControlMode: " << this->ControlMode << "\n";
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetUserInteractionMethod(void (*f)(void *), 
						      void *arg)
{
  if ( f != this->UserInteractionMethod || 
       arg != this->UserInteractionMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->UserInteractionMethodArg)&&
        (this->UserInteractionMethodArgDelete))
      {
      (*this->UserInteractionMethodArgDelete)(this->UserInteractionMethodArg);
      }
    this->UserInteractionMethod = f;
    this->UserInteractionMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetUserInteractionMethodArgDelete(void (*f)(void *))
{
  if ( f != this->UserInteractionMethodArgDelete)
    {
    this->UserInteractionMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetKeyPressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->KeyPressMethod || 
       arg != this->KeyPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->KeyPressMethodArg)&&
        (this->KeyPressMethodArgDelete))
      {
      (*this->KeyPressMethodArgDelete)(this->KeyPressMethodArg);
      }
    this->KeyPressMethod = f;
    this->KeyPressMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetKeyPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->KeyPressMethodArgDelete)
    {
    this->KeyPressMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetKeyReleaseMethod(void (*f)(void *), void *arg)
{
  if ( f != this->KeyReleaseMethod || 
       arg != this->KeyReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->KeyReleaseMethodArg)&&
        (this->KeyReleaseMethodArgDelete))
      {
      (*this->KeyReleaseMethodArgDelete)(this->KeyReleaseMethodArg);
      }
    this->KeyReleaseMethod = f;
    this->KeyReleaseMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetKeyReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->KeyReleaseMethodArgDelete)
    {
    this->KeyReleaseMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetEnterMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EnterMethod || 
       arg != this->EnterMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EnterMethodArg)&&
        (this->EnterMethodArgDelete))
      {
      (*this->EnterMethodArgDelete)(this->EnterMethodArg);
      }
    this->EnterMethod = f;
    this->EnterMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetEnterMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EnterMethodArgDelete)
    {
    this->EnterMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetLeaveMethod(void (*f)(void *), void *arg)
{
  if ( f != this->LeaveMethod || 
       arg != this->LeaveMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->LeaveMethodArg)&&
        (this->LeaveMethodArgDelete))
      {
      (*this->LeaveMethodArgDelete)(this->LeaveMethodArg);
      }
    this->LeaveMethod = f;
    this->LeaveMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetLeaveMethodArgDelete(void (*f)(void *))
{
  if ( f != this->LeaveMethodArgDelete)
    {
    this->LeaveMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetConfigureMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ConfigureMethod || 
       arg != this->ConfigureMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ConfigureMethodArg)&&
        (this->ConfigureMethodArgDelete))
      {
      (*this->ConfigureMethodArgDelete)(this->ConfigureMethodArg);
      }
    this->ConfigureMethod = f;
    this->ConfigureMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetConfigureMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ConfigureMethodArgDelete)
    {
    this->ConfigureMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::SetCharMethod(void (*f)(void *), void *arg)
{
  if ( f != this->CharMethod || 
       arg != this->CharMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->CharMethodArg)&&
        (this->CharMethodArgDelete))
      {
      (*this->CharMethodArgDelete)(this->CharMethodArg);
      }
    this->CharMethod = f;
    this->CharMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetCharMethodArgDelete(void (*f)(void *))
{
  if ( f != this->CharMethodArgDelete)
    {
    this->CharMethodArgDelete = f;
    this->Modified();
    }
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
// checks for USERINTERACTION state, then defers to the trackball modes
void vtkInteractorStyleUser::OnTimer(void) 
{
  if (this->State != VTKIS_USERINTERACTION)
    {
    this->vtkInteractorStyleTrackball::OnTimer();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnKeyPress(int ctrl, int shift, 
					char keycode, char *keysym, 
					int vtkNotUsed(repeatcount))
{
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;
  this->KeySym = keysym;
  this->Char = keycode;  

  if (this->KeyPressMethod)
    {
    (*this->KeyPressMethod)(this->KeyPressMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnKeyRelease(int ctrl, int shift, 
					  char keycode, char *keysym, 
					  int vtkNotUsed(repeatcount))
{
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;
  this->KeySym = keysym;
  this->Char = keycode;  

  if (this->KeyReleaseMethod)
    {
    (*this->KeyReleaseMethod)(this->KeyReleaseMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnChar(int ctrl, int shift, char keycode,
				    int repeatcount) 
{
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;
  this->Char = keycode;  

  // do nothing if a KeyPressMethod has been set,
  // otherwise pass the OnChar to the vtkInteractorStyleTrackball.
  if (this->CharMethod)
    {
    (*this->CharMethod)(this->CharMethodArg);    
    }
  else
    {
    this->vtkInteractorStyleTrackball::OnChar(ctrl,shift,keycode,repeatcount);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnMouseMove(int ctrl, int shift, int x, int y) 
{
  if (this->State == VTKIS_USERINTERACTION)
    {
    this->ShiftKey = shift;
    this->CtrlKey = ctrl;
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->OldPos[0] = int(this->OldX);
    this->OldPos[1] = int(this->OldY);
    if (this->UserInteractionMethod && (x != this->OldX || y != this->OldY))
      {
      (*this->UserInteractionMethod)(this->UserInteractionMethodArg);
      }
    this->OldX = x;
    this->OldY = y;
    }
  else
    {
    this->vtkInteractorStyleTrackball::OnMouseMove(ctrl,shift,x,y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnConfigure(int vtkNotUsed(width), 
					 int vtkNotUsed(height)) 
{
  if (this->ConfigureMethod)
    {
    (*this->ConfigureMethod)(this->ConfigureMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnEnter(int ctrl, int shift, int x, int y)
{
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;
  this->LastPos[0] = x;
  this->LastPos[1] = y;
  if (this->EnterMethod)
    {
    (*this->EnterMethod)(this->EnterMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUser::OnLeave(int ctrl, int shift, int x, int y)
{
  this->ShiftKey = shift;
  this->CtrlKey = ctrl;
  this->LastPos[0] = x;
  this->LastPos[1] = y;
  if (this->LeaveMethod)
    {
    (*this->LeaveMethod)(this->LeaveMethodArg);
    }
}



