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



//------------------------------------------------------------------------------
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




vtkInteractorStyleUser::vtkInteractorStyleUser()
{
  this->UserInteractionMethod = NULL;
  this->UserInteractionMethodArgDelete = NULL;
  this->UserInteractionMethodArg = NULL;
  this->LastPos[0] = this->LastPos[1] = 0;
  this->OldPos[0] = this->OldPos[1] = 0;
}

vtkInteractorStyleUser::~vtkInteractorStyleUser() 
{
  if ((this->UserInteractionMethodArg)&&
      (this->UserInteractionMethodArgDelete))
    {
    (*this->UserInteractionMethodArgDelete)(this->UserInteractionMethodArg);
    }
}

void vtkInteractorStyleUser::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->vtkInteractorStyleTrackball::PrintSelf(os,indent);

  os << indent << "LastPos: (" << this->LastPos[0] << ", " 
                               << this->LastPos[1] << ")\n";  
  os << indent << "OldPos: (" << this->OldPos[0] << ", " 
                              << this->OldPos[1] << ")\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "CtrlKey: " << this->CtrlKey << "\n";
  os << indent << "ActorMode: " << this->ActorMode << "\n";
  os << indent << "TrackballMode: " << this->TrackballMode << "\n";
  os << indent << "ControlMode: " << this->ControlMode << "\n";
}

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

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyleUser::SetUserInteractionMethodArgDelete(void (*f)(void *))
{
  if ( f != this->UserInteractionMethodArgDelete)
    {
    this->UserInteractionMethodArgDelete = f;
    this->Modified();
    }
}

void  vtkInteractorStyleUser::StartUserInteraction() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_USERINTERACTION);
}

void  vtkInteractorStyleUser::EndUserInteraction() 
{
  if (this->State != VTKIS_USERINTERACTION) 
    {
    return;
    }
  this->StopState();
}

// checks for USERINTERACTION state, then defers to the trackball modes
void vtkInteractorStyleUser::OnTimer(void) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->State == VTKIS_USERINTERACTION)
    {
    if (this->UserInteractionMethod)
      {
      this->OldPos[0] = int(this->OldX);
      this->OldPos[1] = int(this->OldY);
      (*this->UserInteractionMethod)(this->UserInteractionMethodArg);
      this->OldX = this->LastPos[0];
      this->OldY = this->LastPos[1];
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      }
    }
  else
    {
    this->vtkInteractorStyleTrackball::OnTimer();
    }
}



