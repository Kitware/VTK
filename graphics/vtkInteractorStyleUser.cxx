/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUser.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. 
Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkInteractorStyleUser.h"
#include "vtkMath.h"
#include "vtkCellPicker.h"
#include "vtkRenderWindowInteractor.h"

vtkInteractorStyleUser::vtkInteractorStyleUser()
{
  this->UserInteractionMethod = NULL;
  this->UserInteractionMethodArgDelete = NULL;
  this->UserInteractionMethodArg = NULL;
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



