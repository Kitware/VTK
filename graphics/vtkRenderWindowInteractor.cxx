/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.cxx
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
#ifdef _WIN32
#include "vtkWin32RenderWindowInteractor.h"
#else
#include "vtkXRenderWindowInteractor.h"
#endif
#include "vtkActor.h"


// Construct object so that light follows camera motion.
vtkRenderWindowInteractor::vtkRenderWindowInteractor()
{
  this->RenderWindow    = NULL;
  this->CurrentCamera   = NULL;
  this->CurrentLight    = NULL;
  this->CurrentRenderer = NULL;
  this->Outline = vtkOutlineSource::New();

  this->LightFollowCamera = 1;
  this->Initialized = 0;
  this->Enabled = 0;
  this->DesiredUpdateRate = 15;
  // default limit is 3 hours per frame
  this->StillUpdateRate = 0.0001;
  
  this->Picker = this->CreateDefaultPicker();
  this->Picker->Register(this);
  this->Picker->Delete();
  
  this->OutlineActor = NULL;
  this->OutlineMapper = vtkPolyDataMapper::New();  
  this->OutlineMapper->SetInput(this->Outline->GetOutput());
  this->PickedRenderer = NULL;
  this->CurrentActor = NULL;


  
  // for actor interactions
  this->TrackballFactor = 10.0;
  this->InteractionPicker = vtkCellPicker::New();
  // set a tight tolerance so picking will be precise.
  this->InteractionPicker->SetTolerance(0.001);
  this->ActorPicked = 0;
  this->InteractionActor = NULL;

  // set to default modes
  this->TrackballMode = VTKXI_JOY;
  this->ActorMode = VTKXI_CAMERA;
  this->ControlMode = VTKXI_CONTROL_OFF;

  this->Preprocess = 0;
  this->RadianToDegree = 180.0 / vtkMath::Pi();
  
  this->NewPickPoint[0] = 0.0;
  this->NewPickPoint[1] = 0.0;
  this->NewPickPoint[2] = 0.0;
  this->NewPickPoint[3] = 1.0;
  this->OldPickPoint[0] = 0.0;
  this->OldPickPoint[1] = 0.0;
  this->OldPickPoint[2] = 0.0;
  this->OldPickPoint[3] = 1.0;
  this->MotionVector[0] = 0.0;
  this->MotionVector[1] = 0.0;
  this->MotionVector[2] = 0.0;
  this->OldX = 0.0;
  this->OldY = 0.0;
  this->ViewLook[0] = 0.0;
  this->ViewLook[1] = 0.0;
  this->ViewLook[2] = 0.0;
  this->ViewPoint[0] = 0.0;
  this->ViewPoint[1] = 0.0;
  this->ViewPoint[2] = 0.0;
  this->ViewFocus[0] = 0.0;
  this->ViewFocus[1] = 0.0;
  this->ViewFocus[2] = 0.0;
  this->ViewUp[0] = 0.0;
  this->ViewUp[1] = 0.0;
  this->ViewUp[2] = 0.0;
  this->ViewRight[0] = 0.0;
  this->ViewRight[1] = 0.0;
  this->ViewRight[2] = 0.0;  

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;
  this->ObjCenter[0] = 0.0;
  this->ObjCenter[1] = 0.0;
  this->ObjCenter[2] = 0.0;  
  this->DispObjCenter[0] = 0.0;
  this->DispObjCenter[1] = 0.0;
  this->DispObjCenter[2] = 0.0;
  this->Radius = 0.0;
  
  this->StartPickMethod = NULL;
  this->StartPickMethodArgDelete = NULL;
  this->StartPickMethodArg = NULL;
  this->EndPickMethod = NULL;
  this->EndPickMethodArgDelete = NULL;
  this->EndPickMethodArg = NULL;
  this->UserMethod = NULL;
  this->UserMethodArgDelete = NULL;
  this->UserMethodArg = NULL;
  this->ExitMethod = NULL;
  this->ExitMethodArgDelete = NULL;
  this->ExitMethodArg = NULL;

  this->StartInteractionPickMethod = NULL;
  this->StartInteractionPickMethodArgDelete = NULL;
  this->StartInteractionPickMethodArg = NULL;
  this->EndInteractionPickMethod = NULL;
  this->EndInteractionPickMethodArgDelete = NULL;
  this->EndInteractionPickMethodArg = NULL;


  this->CameraModeMethod = NULL;
  this->CameraModeMethodArgDelete = NULL;
  this->CameraModeMethodArg = NULL;
  this->ActorModeMethod = NULL;
  this->ActorModeMethodArgDelete = NULL;
  this->ActorModeMethodArg = NULL;
  this->JoystickModeMethod = NULL;
  this->JoystickModeMethodArgDelete = NULL;
  this->JoystickModeMethodArg = NULL;
  this->TrackballModeMethod = NULL;
  this->TrackballModeMethodArgDelete = NULL;
  this->TrackballModeMethodArg = NULL;
  
  this->TimerMethod = NULL;
  this->TimerMethodArgDelete = NULL;
  this->TimerMethodArg = NULL;

  this->LeftButtonPressMethod = NULL;
  this->LeftButtonPressMethodArgDelete = NULL;
  this->LeftButtonPressMethodArg = NULL;
  this->LeftButtonReleaseMethod = NULL;
  this->LeftButtonReleaseMethodArgDelete = NULL;
  this->LeftButtonReleaseMethodArg = NULL;

  this->MiddleButtonPressMethod = NULL;
  this->MiddleButtonPressMethodArgDelete = NULL;
  this->MiddleButtonPressMethodArg = NULL;
  this->MiddleButtonReleaseMethod = NULL;
  this->MiddleButtonReleaseMethodArgDelete = NULL;
  this->MiddleButtonReleaseMethodArg = NULL;

  this->RightButtonPressMethod = NULL;
  this->RightButtonPressMethodArgDelete = NULL;
  this->RightButtonPressMethodArg = NULL;
  this->RightButtonReleaseMethod = NULL;
  this->RightButtonReleaseMethodArgDelete = NULL;
  this->RightButtonReleaseMethodArg = NULL;

  this->EventPosition[0] = 0;
  this->EventPosition[1] = 0;
}

vtkRenderWindowInteractor::~vtkRenderWindowInteractor()
{
  this->InteractionPicker->Delete();

  if ( this->OutlineActor )
    {
    this->OutlineActor->Delete();
    }
  if ( this->OutlineMapper )
    {
    this->OutlineMapper->Delete();
    }
  if ( this->Picker)
    {
    this->Picker->UnRegister(this);
    }

  this->Outline->Delete();
  this->Outline = NULL;
  
  // delete the current arg if there is one and a delete meth
  if ((this->UserMethodArg)&&(this->UserMethodArgDelete))
    {
    (*this->UserMethodArgDelete)(this->UserMethodArg);
    }
  if ((this->ExitMethodArg)&&(this->ExitMethodArgDelete))
    {
    (*this->ExitMethodArgDelete)(this->ExitMethodArg);
    }
  if ((this->StartPickMethodArg)&&(this->StartPickMethodArgDelete))
    {
    (*this->StartPickMethodArgDelete)(this->StartPickMethodArg);
    }
  if ((this->EndPickMethodArg)&&(this->EndPickMethodArgDelete))
    {
    (*this->EndPickMethodArgDelete)(this->EndPickMethodArg);
    }
  if ((this->StartInteractionPickMethodArg)&&
      (this->StartInteractionPickMethodArgDelete))
    {
    (*this->StartInteractionPickMethodArgDelete)
      (this->StartInteractionPickMethodArg);
    }
  if ((this->EndInteractionPickMethodArg)&&
      (this->EndInteractionPickMethodArgDelete))
    {
    (*this->EndInteractionPickMethodArgDelete)
      (this->EndInteractionPickMethodArg);
    }
  if ((this->TimerMethodArg)&&(this->TimerMethodArgDelete))
    {
    (*this->TimerMethodArgDelete)(this->TimerMethodArg);
    }
  if ((this->LeftButtonPressMethodArg)&&(this->LeftButtonPressMethodArgDelete))
    {
    (*this->LeftButtonPressMethodArgDelete)(this->LeftButtonPressMethodArg);
    }
  if ((this->LeftButtonReleaseMethodArg)&&
      (this->LeftButtonReleaseMethodArgDelete))
    {
    (*this->LeftButtonReleaseMethodArgDelete)
      (this->LeftButtonReleaseMethodArg);
    }
  if ((this->MiddleButtonPressMethodArg)&&
      (this->MiddleButtonPressMethodArgDelete))
    {
    (*this->MiddleButtonPressMethodArgDelete)
      (this->MiddleButtonPressMethodArg);
    }
  if ((this->MiddleButtonReleaseMethodArg)&&
      (this->MiddleButtonReleaseMethodArgDelete))
    {
    (*this->MiddleButtonReleaseMethodArgDelete)
      (this->MiddleButtonReleaseMethodArg);
    }
  if ((this->RightButtonPressMethodArg)&&
      (this->RightButtonPressMethodArgDelete))
    {
    (*this->RightButtonPressMethodArgDelete)(this->RightButtonPressMethodArg);
    }
  if ((this->RightButtonReleaseMethodArg)&&
      (this->RightButtonReleaseMethodArgDelete))
    {
    (*this->RightButtonReleaseMethodArgDelete)
      (this->RightButtonReleaseMethodArg);
    }

  if ((this->CameraModeMethodArg)&&(this->CameraModeMethodArgDelete))
    {
    (*this->CameraModeMethodArgDelete)(this->CameraModeMethodArg);
    }
  if ((this->ActorModeMethodArg)&&(this->ActorModeMethodArgDelete))
    {
    (*this->ActorModeMethodArgDelete)(this->ActorModeMethodArg);
    }
  if ((this->JoystickModeMethodArg)&&(this->JoystickModeMethodArgDelete))
    {
    (*this->JoystickModeMethodArgDelete)(this->JoystickModeMethodArg);
    }
  if ((this->TrackballModeMethodArg)&&(this->TrackballModeMethodArgDelete))
    {
    (*this->TrackballModeMethodArgDelete)(this->TrackballModeMethodArg);
    }
}

vtkRenderWindowInteractor *vtkRenderWindowInteractor::New()
{
#ifdef _WIN32
  return vtkWin32RenderWindowInteractor::New();
#else
  return vtkXRenderWindowInteractor::New();
#endif  
}

void vtkRenderWindowInteractor::SetRenderWindow(vtkRenderWindow *aren)
{
  if (this->RenderWindow != aren)
    {
    // to avoid destructor recursion
    vtkRenderWindow *temp = this->RenderWindow;
    this->RenderWindow = aren;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->RenderWindow != NULL) 
      {
      this->RenderWindow->Register(this);
      if (this->RenderWindow->GetInteractor() != this)
	{
	this->RenderWindow->SetInteractor(this);
	}
      }
    }
}

void vtkRenderWindowInteractor::FindPokedRenderer(int x,int y)
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;
  int numRens, i;
  
  this->CurrentRenderer = NULL;

  rc = this->RenderWindow->GetRenderers();

  numRens = rc->GetNumberOfItems();

  for (i = numRens -1; (i >= 0) && !this->CurrentRenderer; i--)
    {
    aren = (vtkRenderer *)rc->GetItemAsObject(i);
    if (aren->IsInViewport(x,y))
      {
      this->CurrentRenderer = aren;
      }
    }
  
  // we must have a value 
  if (this->CurrentRenderer == NULL)
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    this->CurrentRenderer = aren;
    }
}

void  vtkRenderWindowInteractor::FindPokedCamera(int x,int y)
{
  float *vp;
  vtkLightCollection *lc;

  this->FindPokedRenderer(x,y);
  vp = this->CurrentRenderer->GetViewport();

  this->CurrentCamera = this->CurrentRenderer->GetActiveCamera();  
  memcpy(this->Center,this->CurrentRenderer->GetCenter(),sizeof(int)*2);
  this->DeltaElevation = -20.0/((vp[3] - vp[1])*this->Size[1]);
  this->DeltaAzimuth = -20.0/((vp[2] - vp[0])*this->Size[0]);

  // as a side effect also set the light 
  // in case they are using light follow camera 
  lc = this->CurrentRenderer->GetLights();
  lc->InitTraversal();
  this->CurrentLight = lc->GetNextItem();
}

// When pick action successfully selects actor, this method highlights the 
// actor appropriately. Currently this is done by placing a bounding box
// around the actor.
void vtkRenderWindowInteractor::HighlightActor(vtkActor *actor)
{
  if ( ! this->OutlineActor )
    {
    // have to defer creation to get right type
    this->OutlineActor = vtkActor::New();
    this->OutlineActor->PickableOff();
    this->OutlineActor->DragableOff();
    this->OutlineActor->SetMapper(this->OutlineMapper);
    this->OutlineActor->GetProperty()->SetColor(1.0,1.0,1.0);
    this->OutlineActor->GetProperty()->SetAmbient(1.0);
    this->OutlineActor->GetProperty()->SetDiffuse(0.0);
    }

  if ( this->PickedRenderer ) 
    {
    this->PickedRenderer->RemoveActor(this->OutlineActor);
    }

  if ( ! actor )
    {
    this->PickedRenderer = NULL;
    }
  else 
    {
    this->PickedRenderer = this->CurrentRenderer;
    this->CurrentRenderer->AddActor(this->OutlineActor);
    this->Outline->SetBounds(actor->GetBounds());
    this->CurrentActor = actor;
    }
  this->RenderWindow->Render();
}

// Specify a method to be executed prior to the pick operation.
void vtkRenderWindowInteractor::SetStartPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartPickMethod || arg != this->StartPickMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartPickMethodArg)&&(this->StartPickMethodArgDelete))
      {
      (*this->StartPickMethodArgDelete)(this->StartPickMethodArg);
      }
    this->StartPickMethod = f;
    this->StartPickMethodArg = arg;
    this->Modified();
    }
}

// Specify a method to be executed after the pick operation.
void vtkRenderWindowInteractor::SetEndPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndPickMethod || arg != this->EndPickMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndPickMethodArg)&&(this->EndPickMethodArgDelete))
      {
      (*this->EndPickMethodArgDelete)(this->EndPickMethodArg);
      }
    this->EndPickMethod = f;
    this->EndPickMethodArg = arg;
    this->Modified();
    }
}

// Specify a method to be executed prior to the pick operation.
void vtkRenderWindowInteractor::SetStartInteractionPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartInteractionPickMethod ||
       arg != this->StartInteractionPickMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartInteractionPickMethodArg)&&
        (this->StartInteractionPickMethodArgDelete))
      {
      (*this->StartInteractionPickMethodArgDelete)
        (this->StartInteractionPickMethodArg);
      }
    this->StartInteractionPickMethod = f;
    this->StartInteractionPickMethodArg = arg;
    this->Modified();
    }
}

// Specify a method to be executed after the pick operation.
void vtkRenderWindowInteractor::SetEndInteractionPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndInteractionPickMethod ||
       arg != this->EndInteractionPickMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndInteractionPickMethodArg)&&
        (this->EndInteractionPickMethodArgDelete))
      {
      (*this->EndInteractionPickMethodArgDelete)
        (this->EndInteractionPickMethodArg);
      }
    this->EndInteractionPickMethod = f;
    this->EndInteractionPickMethodArg = arg;
    this->Modified();
    }
}

// Set the object used to perform pick operations. You can use this to 
// control what type of data is picked.
void vtkRenderWindowInteractor::SetPicker(vtkPicker *picker)
{
  if ( this->Picker != picker ) 
    {
    if ( this->Picker != NULL )
      {
      this->Picker->UnRegister(this);
      }
    this->Picker = picker;
    if ( this->Picker != NULL )
      {
      this->Picker->Register(this);
      }
    this->Modified();
    }
}

vtkPicker *vtkRenderWindowInteractor::CreateDefaultPicker()
{
  return vtkCellPicker::New();
}

// Set the user method. This method is invoked on a <u> keypress.
void vtkRenderWindowInteractor::SetUserMethod(void (*f)(void *), void *arg)
{
  if ( f != this->UserMethod || arg != this->UserMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->UserMethodArg)&&(this->UserMethodArgDelete))
      {
      (*this->UserMethodArgDelete)(this->UserMethodArg);
      }
    this->UserMethod = f;
    this->UserMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetUserMethodArgDelete(void (*f)(void *))
{
  if ( f != this->UserMethodArgDelete)
    {
    this->UserMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkRenderWindowInteractor::SetExitMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ExitMethod || arg != this->ExitMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ExitMethodArg)&&(this->ExitMethodArgDelete))
      {
      (*this->ExitMethodArgDelete)(this->ExitMethodArg);
      }
    this->ExitMethod = f;
    this->ExitMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetExitMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExitMethodArgDelete)
    {
    this->ExitMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked during rotate/zoom/pan
void vtkRenderWindowInteractor::SetTimerMethod(void (*f)(void *), void *arg)
{
  if ( f != this->TimerMethod || arg != this->TimerMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->TimerMethodArg)&&(this->TimerMethodArgDelete))
      {
      (*this->TimerMethodArgDelete)(this->TimerMethodArg);
      }
    this->TimerMethod = f;
    this->TimerMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetTimerMethodArgDelete(void (*f)(void *))
{
  if ( f != this->TimerMethodArgDelete)
    {
    this->TimerMethodArgDelete = f;
    this->Modified();
    }
}

// Set the left button pressed method. This method is invoked on a left mouse button press.
void vtkRenderWindowInteractor::SetLeftButtonPressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->LeftButtonPressMethod || arg != this->LeftButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->LeftButtonPressMethodArg)&&(this->LeftButtonPressMethodArgDelete))
      {
      (*this->LeftButtonPressMethodArgDelete)(this->LeftButtonPressMethodArg);
      }
    this->LeftButtonPressMethod = f;
    this->LeftButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetLeftButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->LeftButtonPressMethodArgDelete)
    {
    this->LeftButtonPressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkRenderWindowInteractor::SetLeftButtonReleaseMethod(void (*f)(void *), void *arg)
{
  if ( f != this->LeftButtonReleaseMethod || arg != this->LeftButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->LeftButtonReleaseMethodArg)&&(this->LeftButtonReleaseMethodArgDelete))
      {
      (*this->LeftButtonReleaseMethodArgDelete)(this->LeftButtonReleaseMethodArg);
      }
    this->LeftButtonReleaseMethod = f;
    this->LeftButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetLeftButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->LeftButtonReleaseMethodArgDelete)
    {
    this->LeftButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkRenderWindowInteractor::SetMiddleButtonPressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->MiddleButtonPressMethod || arg != this->MiddleButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->MiddleButtonPressMethodArg)&&(this->MiddleButtonPressMethodArgDelete))
      {
      (*this->MiddleButtonPressMethodArgDelete)(this->MiddleButtonPressMethodArg);
      }
    this->MiddleButtonPressMethod = f;
    this->MiddleButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetMiddleButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->MiddleButtonPressMethodArgDelete)
    {
    this->MiddleButtonPressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkRenderWindowInteractor::SetMiddleButtonReleaseMethod(void (*f)(void *), void *arg)
{
  if ( f != this->MiddleButtonReleaseMethod || arg != this->MiddleButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->MiddleButtonReleaseMethodArg)&&(this->MiddleButtonReleaseMethodArgDelete))
      {
      (*this->MiddleButtonReleaseMethodArgDelete)(this->MiddleButtonReleaseMethodArg);
      }
    this->MiddleButtonReleaseMethod = f;
    this->MiddleButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->MiddleButtonReleaseMethodArgDelete)
    {
    this->MiddleButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkRenderWindowInteractor::SetRightButtonPressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->RightButtonPressMethod || arg != this->RightButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->RightButtonPressMethodArg)&&(this->RightButtonPressMethodArgDelete))
      {
      (*this->RightButtonPressMethodArgDelete)(this->RightButtonPressMethodArg);
      }
    this->RightButtonPressMethod = f;
    this->RightButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetRightButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->RightButtonPressMethodArgDelete)
    {
    this->RightButtonPressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkRenderWindowInteractor::SetRightButtonReleaseMethod(void (*f)(void *), void *arg)
{
  if ( f != this->RightButtonReleaseMethod || arg != this->RightButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->RightButtonReleaseMethodArg)&&(this->RightButtonReleaseMethodArgDelete))
      {
      (*this->RightButtonReleaseMethodArgDelete)(this->RightButtonReleaseMethodArg);
      }
    this->RightButtonReleaseMethod = f;
    this->RightButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetRightButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->RightButtonReleaseMethodArgDelete)
    {
    this->RightButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetStartPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartPickMethodArgDelete)
    {
    this->StartPickMethodArgDelete = f;
    this->Modified();
    }
}
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetEndPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndPickMethodArgDelete)
    {
    this->EndPickMethodArgDelete = f;
    this->Modified();
    }
}


// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetStartInteractionPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartInteractionPickMethodArgDelete)
    {
    this->StartInteractionPickMethodArgDelete = f;
    this->Modified();
    }
}
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetEndInteractionPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndInteractionPickMethodArgDelete)
    {
    this->EndInteractionPickMethodArgDelete = f;
    this->Modified();
    }
}

// Set the CameraMode method. This method is invoked on a <c> keypress.
void 
vtkRenderWindowInteractor::SetCameraModeMethod(void (*f)(void *), void *arg)
{
  if ( f != this->CameraModeMethod || arg != this->CameraModeMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->CameraModeMethodArg)&&(this->CameraModeMethodArgDelete))
      {
      (*this->CameraModeMethodArgDelete)(this->CameraModeMethodArg);
      }
    this->CameraModeMethod = f;
    this->CameraModeMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetCameraModeMethodArgDelete(void (*f)(void *))
{
  if ( f != this->CameraModeMethodArgDelete)
    {
    this->CameraModeMethodArgDelete = f;
    this->Modified();
    }
}

// Set the ActorMode method. This method is invoked on a <a> keypress.
void 
vtkRenderWindowInteractor::SetActorModeMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ActorModeMethod || arg != this->ActorModeMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ActorModeMethodArg)&&(this->ActorModeMethodArgDelete))
      {
      (*this->ActorModeMethodArgDelete)(this->ActorModeMethodArg);
      }
    this->ActorModeMethod = f;
    this->ActorModeMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetActorModeMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ActorModeMethodArgDelete)
    {
    this->ActorModeMethodArgDelete = f;
    this->Modified();
    }
}

// Set the JoystickMode method. This method is invoked on a <j> keypress.
void 
vtkRenderWindowInteractor::SetJoystickModeMethod(void (*f)(void *), void *arg)
{
  if ( f != this->JoystickModeMethod || arg != this->JoystickModeMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->JoystickModeMethodArg)&&(this->JoystickModeMethodArgDelete))
      {
      (*this->JoystickModeMethodArgDelete)(this->JoystickModeMethodArg);
      }
    this->JoystickModeMethod = f;
    this->JoystickModeMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void 
vtkRenderWindowInteractor::SetJoystickModeMethodArgDelete(void (*f)(void *))
{
  if ( f != this->JoystickModeMethodArgDelete)
    {
    this->JoystickModeMethodArgDelete = f;
    this->Modified();
    }
}

// Set the TrackballMode method. This method is invoked on a <t> keypress.
void 
vtkRenderWindowInteractor::SetTrackballModeMethod(void (*f)(void *), void *arg)
{
  if ( f != this->TrackballModeMethod || arg != this->TrackballModeMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->TrackballModeMethodArg)&&(this->TrackballModeMethodArgDelete))
      {
      (*this->TrackballModeMethodArgDelete)(this->TrackballModeMethodArg);
      }
    this->TrackballModeMethod = f;
    this->TrackballModeMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void 
vtkRenderWindowInteractor::SetTrackballModeMethodArgDelete(void (*f)(void *))
{
  if ( f != this->TrackballModeMethodArgDelete)
    {
    this->TrackballModeMethodArgDelete = f;
    this->Modified();
    }
}






// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void vtkRenderWindowInteractor::UnRegister(vtkObject *o)
{
  if (this->RenderWindow && this->RenderWindow->GetInteractor() == this &&
      this->RenderWindow != o)
    {
    if (this->GetReferenceCount()+this->RenderWindow->GetReferenceCount() == 3)
      {
      this->RenderWindow->SetInteractor(NULL);
      this->SetRenderWindow(NULL);
      }
    }
  
  this->vtkObject::UnRegister(o);
}
      


void vtkRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  os << indent << "CurrentCamera:   " << this->CurrentCamera << "\n";
  os << indent << "CurrentLight:    " << this->CurrentLight << "\n";
  os << indent << "CurrentRenderer: " << this->CurrentRenderer << "\n";
  if ( this->Picker )
    {
    os << indent << "Picker: " << this->Picker << "\n";
    }
  else
    {
    os << indent << "Picker: (none)\n";
    }
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
  os << indent << "DesiredUpdateRate: " << this->DesiredUpdateRate << "\n";
  os << indent << "StillUpdateRate: " << this->StillUpdateRate << "\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "EventPosition: " << "( " << this->EventPosition[0] <<
    ", " << this->EventPosition[1] << " )\n";
  os << indent << "Viewport Center: " << "( " << this->Center[0] <<
    ", " << this->Center[1] << " )\n";
  os << indent << "Viewport Size: " << "( " << this->Size[0] <<
    ", " << this->Size[1] << " )\n";

  if ( this->PickedRenderer )
    {
    os << indent << "Picked Renderer: " << this->PickedRenderer << "\n";
    }
  else
    {
    os << indent << "Picked Renderer: (none)\n";
    }
  if ( this->CurrentActor )
    {
    os << indent << "Current Actor: " << this->CurrentActor << "\n";
    }
  else
    {
    os << indent << "Current Actor: (none)\n";
    }

  os << indent << "Interaction Picker: " << this->InteractionPicker;
  os << indent << "Actor Picked: " <<
    (this->ActorPicked ? "Yes\n" : "No\n");
  if ( this->InteractionActor )
    {
    os << indent << "Interacting Actor: " << this->InteractionActor << "\n";
    }
  else
    {
    os << indent << "Interacting Actor: (none)\n";
    }
  os << indent << "Mode: " <<
    (this->ActorMode ? "Actor\n" : "Camera\n");
  os << indent << "Mode: " <<
    (this->TrackballMode ? "Trackball\n" : "Joystick\n");
  os << indent << "Control Key: " <<
    (this->ControlMode ? "On\n" : "Off\n");
  os << indent << "Preprocessing: " <<
    (this->Preprocess ? "Yes\n" : "No\n");
}




// Description:
// transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkRenderWindowInteractor::ComputeDisplayToWorld(float x, float y,
                                                      float z,
                                                      float *worldPt)
{
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}


// Description:
// transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkRenderWindowInteractor::ComputeWorldToDisplay(float x, float y,
                                                      float z,
                                                      float *displayPt)
{
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}


// Description:
// rotate the camera in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickRotateCamera(int x, int y)
{
  if (this->Preprocess)
    {
    this->Preprocess = 0;
    }
  
  float rxf = (float)(x - this->Center[0]) * this->DeltaAzimuth;
  float ryf = (float)((this->Size[1] - y) -
                      this->Center[1]) * this->DeltaElevation;
  
  this->CurrentCamera->Azimuth(rxf);
  this->CurrentCamera->Elevation(ryf);
  this->CurrentCamera->OrthogonalizeViewUp();
  if (this->LightFollowCamera)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  this->RenderWindow->Render();
}

// Description:
// spin the camera in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickSpinCamera(int x, int y)
{

  if (this->Preprocess)
    {
    this->Preprocess = 0;
    }

  // spin is based on y value
  float yf = (float)(this->Size[1] - y -
                     this->Center[1]) / (float)(this->Center[1]);
  if (yf > 1)
    {
    yf = 1;
    }
  else if (yf < -1)
    {
    yf = -1;
    }

  float newAngle = asin(yf) * this->RadianToDegree / this->TrackballFactor;

  this->CurrentCamera->Roll(newAngle);
  this->CurrentCamera->OrthogonalizeViewUp();
  this->RenderWindow->Render();
}

// Description:
// Pan the camera in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickPanCamera(int x, int y)
{
  
  if (this->Preprocess)
    {
    // calculate the focal depth since we'll be using it a lot
    this->CurrentCamera->GetFocalPoint(this->ViewFocus);      
    this->ComputeWorldToDisplay(this->ViewFocus[0], this->ViewFocus[1],
                                this->ViewFocus[2], this->ViewFocus);
    this->FocalDepth = this->ViewFocus[2];

    this->Preprocess = 0;
    }

  this->ComputeDisplayToWorld(float(x), float(this->Size[1] - y),
                              this->FocalDepth,
                              this->NewPickPoint);

  // get the current focal point and position
  this->CurrentCamera->GetFocalPoint(this->ViewFocus);
  this->CurrentCamera->GetPosition(this->ViewPoint);  

  /*
   * Compute a translation vector, moving everything 1/10 
   * the distance to the cursor. (Arbitrary scale factor)
   */
  this->MotionVector[0] = (this->ViewFocus[0] -
                           this->NewPickPoint[0]) / this->TrackballFactor;
  this->MotionVector[1] = (this->ViewFocus[1] -
                           this->NewPickPoint[1]) / this->TrackballFactor;
  this->MotionVector[2] = (this->ViewFocus[2] -
                           this->NewPickPoint[2]) / this->TrackballFactor;

  this->CurrentCamera->SetFocalPoint(this->MotionVector[0]
                                     + this->ViewFocus[0],
                                     this->MotionVector[1]
                                     + this->ViewFocus[1],
                                     this->MotionVector[2]
                                     + this->ViewFocus[2]);
  this->CurrentCamera->SetPosition(this->MotionVector[0]
                                   + this->ViewPoint[0],
                                   this->MotionVector[1]
                                   + this->ViewPoint[1],
                                   this->MotionVector[2]
                                   + this->ViewPoint[2]);
  
  if (this->LightFollowCamera)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  this->RenderWindow->Render();
}

// Description:
// dolly the camera in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickDollyCamera(int x, int y)
{
  if (this->Preprocess)
    {
    this->Preprocess = 0;
    }
  
  float dyf = 0.5 * (float)((this->Size[1] - y) - this->Center[1]) /
    (float)(this->Center[1]);
  float zoomFactor = pow((float)1.1, dyf);
  if (this->CurrentCamera->GetParallelProjection())
    {
    this->CurrentCamera->
      SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
    }
  else
    {
    float *clippingRange = this->CurrentCamera->GetClippingRange();
    float dist = clippingRange[1] - clippingRange[0];
    this->CurrentCamera->SetClippingRange(clippingRange[0]/zoomFactor,
                                          clippingRange[0]/zoomFactor +
                                          dist);
    this->CurrentCamera->Dolly(zoomFactor);
    }

  if (this->LightFollowCamera)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  this->RenderWindow->Render();
}



// Description:
// rotate the camera in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballRotateCamera(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      this->Preprocess = 0;
      }

    float rxf = (float)(x - this->OldX) * this->DeltaAzimuth *
      this->TrackballFactor;
    float ryf = (float)(this->OldY - y) * this->DeltaElevation *
      this->TrackballFactor;
    
    this->CurrentCamera->Azimuth(rxf);
    this->CurrentCamera->Elevation(ryf);
    this->CurrentCamera->OrthogonalizeViewUp();
    if (this->LightFollowCamera)
      {
      // get the first light
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }	
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}

// Description:
// spin the camera in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballSpinCamera(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      this->Preprocess = 0;
      }

    float newAngle = atan2((float)(this->Size[1] - y - this->Center[1]),
                           (float)(x - this->Center[0]));
    float oldAngle = atan2((float)(this->Size[1] - this->OldY -
                                   this->Center[1]),
                           (float)(this->OldX - this->Center[0]));
  
    newAngle *= this->RadianToDegree;
    oldAngle *= this->RadianToDegree;

    this->CurrentCamera->Roll(newAngle - oldAngle);
    this->CurrentCamera->OrthogonalizeViewUp();
      
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}


// Description:
// pan the camera in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballPanCamera(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // calculate the focal depth since we'll be using it a lot
      this->CurrentCamera->GetFocalPoint(this->ViewFocus);      
      this->ComputeWorldToDisplay(this->ViewFocus[0], this->ViewFocus[1],
                                  this->ViewFocus[2], this->ViewFocus);
      this->FocalDepth = this->ViewFocus[2];

      this->Preprocess = 0;
      }

    this->ComputeDisplayToWorld(float(x), float(this->Size[1] - y),
                                this->FocalDepth,
                                this->NewPickPoint);
    
    // has to recalc old mouse point since the viewport has moved,
    // so can't move it outside the loop
    this->ComputeDisplayToWorld(float(this->OldX),
                                float(this->Size[1] - this->OldY),
                                this->FocalDepth, this->OldPickPoint);

    // camera motion is reversed
    this->MotionVector[0] = this->OldPickPoint[0] - this->NewPickPoint[0];
    this->MotionVector[1] = this->OldPickPoint[1] - this->NewPickPoint[1];
    this->MotionVector[2] = this->OldPickPoint[2] - this->NewPickPoint[2];
    
    this->CurrentCamera->GetFocalPoint(this->ViewFocus);
    this->CurrentCamera->GetPosition(this->ViewPoint);
    this->CurrentCamera->SetFocalPoint(this->MotionVector[0] +
                                       this->ViewFocus[0],
                                       this->MotionVector[1] +
                                       this->ViewFocus[1],
                                       this->MotionVector[2] +
                                       this->ViewFocus[2]);
    this->CurrentCamera->SetPosition(this->MotionVector[0] +
                                     this->ViewPoint[0],
                                     this->MotionVector[1] +
                                     this->ViewPoint[1],
                                     this->MotionVector[2] +
                                     this->ViewPoint[2]);
      
    if (this->LightFollowCamera)
      {
      /* get the first light */
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }
    
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}

// Description:
// dolly the camera in trackball (motion sensitive) style
// dolly is based on distance from center of screen,
// and the upper half is positive, lower half is negative
void vtkRenderWindowInteractor::TrackballDollyCamera(int x, int y)
{
  if (this->OldY != y)
    {
    if (this->Preprocess)
      {
      this->Preprocess = 0;
      }
    
    float dyf = this->TrackballFactor * (float)(this->OldY - y) /
      (float)(this->Center[1]);
    float zoomFactor = pow((float)1.1, dyf);
          
    if (this->CurrentCamera->GetParallelProjection())
      {
      this->CurrentCamera->
        SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
      }
    else
      {
      float *clippingRange = this->CurrentCamera->GetClippingRange();
      this->CurrentCamera->SetClippingRange(clippingRange[0]/zoomFactor,
                                            clippingRange[1]/zoomFactor);
      this->CurrentCamera->Dolly(zoomFactor);
      }
    
    if (this->LightFollowCamera)
      {
      /* get the first light */
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }

    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}


// Description:
// rotate the actor in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickRotateActor(int x, int y)
{
  if (this->Preprocess)
    {
    // first get the origin of the assembly
    memmove(this->ObjCenter,
            this->InteractionActor->GetCenter(), 3 * sizeof(float));

    // GetLength gets the length of the diagonal of the bounding box
    float boundRadius = this->InteractionActor->GetLength() * 0.5;

    // get the view up and view right vectors
    this->CurrentCamera->OrthogonalizeViewUp();
    this->CurrentCamera->ComputeViewPlaneNormal();
    this->CurrentCamera->GetViewUp(this->ViewUp);
    vtkMath::Normalize(this->ViewUp);
    this->CurrentCamera->GetViewPlaneNormal(this->ViewLook);
    vtkMath::Cross(this->ViewUp, this->ViewLook, this->ViewRight);
    vtkMath::Normalize(this->ViewRight);

    // get the furtherest point from object bounding box center
    float outsidept[3];
    outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
    outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
    outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;

    // convert to display coord
    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                                outsidept[2], outsidept);
    
    this->Radius = sqrt(vtkMath::Distance2BetweenPoints(this->DispObjCenter,
                                                        outsidept));

    this->HighlightActor(NULL);
    this->Preprocess = 0;
    }

    
  float nxf = (float)(x - this->DispObjCenter[0]) / this->Radius;
  float nyf = (float)(this->Size[1] - y - this->DispObjCenter[1]) /
    this->Radius;

  if (nxf > 1.0)
    {
    nxf = 1.0;
    }
  else if (nxf < -1.0)
    {
    nxf = -1.0;
    }
  
  if (nyf > 1.0)
    {
    nyf = 1.0;
    }
  else if (nyf < -1.0)
    {
    nyf = -1.0;
    }
  
  float newXAngle = asin(nxf) * this->RadianToDegree / this->TrackballFactor;
  float newYAngle = asin(nyf) * this->RadianToDegree / this->TrackballFactor;

  float scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  float **rotate = new float*[2];
  rotate[0] = new float[4];
  rotate[1] = new float[4];

  rotate[0][0] = newXAngle;
  rotate[0][1] = this->ViewUp[0];
  rotate[0][2] = this->ViewUp[1];
  rotate[0][3] = this->ViewUp[2];

  rotate[1][0] = -newYAngle;
  rotate[1][1] = this->ViewRight[0];
  rotate[1][2] = this->ViewRight[1];
  rotate[1][3] = this->ViewRight[2];
  
    
  this->ActorTransform(this->InteractionActor,
                       this->ObjCenter,
                       2, rotate, scale);

  delete [] rotate[0];
  delete [] rotate[1];
  delete [] rotate;
  
  this->RenderWindow->Render();
}


// Description:
// spin the actor in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickSpinActor(int x, int y)
{

  // get the axis to rotate around = vector from eye to origin
  if (this->Preprocess)
    {

    memmove(this->ObjCenter, this->InteractionActor->GetCenter(),
            3 * sizeof(float));

    if (this->CurrentCamera->GetParallelProjection())
      {
      // if parallel projection, want to get the view plane normal...
      this->CurrentCamera->ComputeViewPlaneNormal();
      this->CurrentCamera->GetViewPlaneNormal(this->MotionVector);
      }
    else
      {
      // perspective projection, get vector from eye to center of actor
      this->CurrentCamera->GetPosition(this->ViewPoint);
      this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
      this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
      this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
      vtkMath::Normalize(this->MotionVector);
      }
    
    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    
    this->HighlightActor(NULL);
    this->Preprocess = 0;
    }
  
  float yf = (float)(this->Size[1] - y -
                     this->DispObjCenter[1]) / (float)(this->Center[1]);
  if (yf > 1.0)
    {
    yf = 1.0;
    }
  else if (yf < -1.0)
    {
    yf = -1.0;
    }

  float newAngle = asin(yf) * this->RadianToDegree / this->TrackballFactor;

  float scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  float **rotate = new float*[1];
  rotate[0] = new float[4];

  rotate[0][0] = newAngle;
  rotate[0][1] = this->MotionVector[0];
  rotate[0][2] = this->MotionVector[1];
  rotate[0][3] = this->MotionVector[2];
  
  this->ActorTransform(this->InteractionActor,
                       this->ObjCenter,
                       1, rotate, scale);

  delete [] rotate[0];
  delete [] rotate;
  
  this->RenderWindow->Render();
}


// Description:
// pan the actor in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickPanActor(int x, int y)
{
  if (this->Preprocess)
    {
    // use initial center as the origin from which to pan
    memmove(this->ObjCenter,
            this->InteractionActor->GetCenter(), 3 * sizeof(float));

    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    this->FocalDepth = this->DispObjCenter[2];
    
    this->HighlightActor(NULL);
    this->Preprocess = 0;
    }
  
  this->ComputeDisplayToWorld(float(x), float(this->Size[1] - y),
                              this->FocalDepth,
                              this->NewPickPoint);
  
  /*
   * Compute a translation vector, moving everything 1/10 
   * the distance to the cursor. (Arbitrary scale factor)
   */
  this->MotionVector[0] = (this->NewPickPoint[0] -
                           this->ObjCenter[0]) / this->TrackballFactor;
  this->MotionVector[1] = (this->NewPickPoint[1] -
                           this->ObjCenter[1]) / this->TrackballFactor;
  this->MotionVector[2] = (this->NewPickPoint[2] -
                           this->ObjCenter[2]) / this->TrackballFactor;

  if (this->InteractionActor->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(*(this->InteractionActor->GetUserMatrix()));
    t->Translate(this->MotionVector[0], this->MotionVector[1], 
		 this->MotionVector[2]);
    this->InteractionActor->GetUserMatrix()->DeepCopy(t->GetMatrixPointer());
    t->Delete();
    }
  else
    {
    this->InteractionActor->AddPosition(this->MotionVector);
    }

  this->RenderWindow->Render();
}

// Description:
// Dolly the actor in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickDollyActor(int x, int y)
{
  // dolly is based on distance from center of screen,
  // and the upper half is positive, lower half is negative

  if (this->Preprocess)
    {
    this->CurrentCamera->GetPosition(this->ViewPoint);
    this->CurrentCamera->GetFocalPoint(this->ViewFocus);

    // use initial center as the origin from which to pan
    memmove(this->ObjCenter,
            this->InteractionActor->GetCenter(), 3 * sizeof(float));
    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);

    this->HighlightActor(NULL);
    this->Preprocess = 0;
    }
  
  float yf = (float)((this->Size[1] - y) - this->DispObjCenter[1]) /
    (float)(this->Center[1]);
  float dollyFactor = pow((float)1.1, yf);
  
  dollyFactor -= 1.0;
  this->MotionVector[0] = (this->ViewPoint[0] -
                           this->ViewFocus[0]) * dollyFactor;
  this->MotionVector[1] = (this->ViewPoint[1] -
                           this->ViewFocus[1]) * dollyFactor;
  this->MotionVector[2] = (this->ViewPoint[2] -
                           this->ViewFocus[2]) * dollyFactor;

  if (this->InteractionActor->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(*(this->InteractionActor->GetUserMatrix()));
    t->Translate(this->MotionVector[0], this->MotionVector[1], 
		 this->MotionVector[2]);
    this->InteractionActor->GetUserMatrix()->DeepCopy(t->GetMatrixPointer());
    t->Delete();
    }
  else
    {
    this->InteractionActor->AddPosition(this->MotionVector);
    }

  this->RenderWindow->Render();
}



// Description:
// scale the actor in joystick (position sensitive) style
void vtkRenderWindowInteractor::JoystickScaleActor(int x, int y)
{
  // Uniform scale is based on distance from center of screen,
  // and the upper half is positive, lower half is negative

  if (this->Preprocess)
    {
    // use bounding box center as the origin from which to pan
    memmove(this->ObjCenter,
            this->InteractionActor->GetCenter(), 3 * sizeof(float));

    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    
    this->HighlightActor(NULL);
    this->Preprocess = 0;
    }
  
  float yf = (float)(this->Size[1] - y - this->DispObjCenter[1]) /
    (float)(this->Center[1]);
  float scaleFactor = pow((float)1.1, yf);          

  float **rotate = NULL;
  
  float scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;

  this->ActorTransform(this->InteractionActor,
                       this->ObjCenter,
                       0, rotate, scale);

  this->RenderWindow->Render();
}



// Description:
// rotate the actor in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballRotateActor(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {

    if (this->Preprocess)
      {
      memmove(this->ObjCenter, this->InteractionActor->GetCenter(),
              3 * sizeof(float));

      // GetLength gets the length of the diagonal of the bounding box
      float boundRadius = this->InteractionActor->GetLength() * 0.5;
      
      // get the view up and view right vectors
      this->CurrentCamera->OrthogonalizeViewUp();
      this->CurrentCamera->ComputeViewPlaneNormal();
      this->CurrentCamera->GetViewUp(this->ViewUp);
      vtkMath::Normalize(this->ViewUp);
      this->CurrentCamera->GetViewPlaneNormal(this->ViewLook);
      vtkMath::Cross(this->ViewUp, this->ViewLook, this->ViewRight);
      vtkMath::Normalize(this->ViewRight);

      // get the furtherest point from object position+origin
      float outsidept[3];
      outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
      outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
      outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;

      // convert them to display coord
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);
      this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                                  outsidept[2], outsidept);

      // get the radius in display coord
      this->Radius = sqrt(vtkMath::Distance2BetweenPoints(this->DispObjCenter,
                                                          outsidept));

      this->HighlightActor(NULL);
      this->Preprocess = 0;
      }

    float nxf = (float)(x - this->DispObjCenter[0]) / this->Radius;
    float nyf = (float)(this->Size[1] - y -
                        this->DispObjCenter[1]) / this->Radius;
    float oxf = (float)(this->OldX - this->DispObjCenter[0]) / this->Radius;
    float oyf = (float)(this->Size[1] - this->OldY -
                        this->DispObjCenter[1]) / this->Radius;

    if (((nxf * nxf + nyf * nyf) <= 1.0) &&
        ((oxf * oxf + oyf * oyf) <= 1.0))
      {
	    
      float newXAngle = asin(nxf) * this->RadianToDegree;
      float newYAngle = asin(nyf) * this->RadianToDegree;
      float oldXAngle = asin(oxf) * this->RadianToDegree;
      float oldYAngle = asin(oyf) * this->RadianToDegree;

      float scale[3];
      scale[0] = scale[1] = scale[2] = 1.0;
      float **rotate = new float*[2];
      rotate[0] = new float[4];
      rotate[1] = new float[4];

      rotate[0][0] = newXAngle - oldXAngle;
      rotate[0][1] = this->ViewUp[0];
      rotate[0][2] = this->ViewUp[1];
      rotate[0][3] = this->ViewUp[2];
      
      rotate[1][0] = oldYAngle - newYAngle;
      rotate[1][1] = this->ViewRight[0];
      rotate[1][2] = this->ViewRight[1];
      rotate[1][3] = this->ViewRight[2];
      
      
      this->ActorTransform(this->InteractionActor,
                           this->ObjCenter,
                           2, rotate, scale);

      delete [] rotate[0];
      delete [] rotate[1];
      delete [] rotate;
      
      this->OldX = x;
      this->OldY = y;
      this->RenderWindow->Render();
      }
    }
}

// Description:
// spin the actor in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballSpinActor(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // get the position plus origin of the object
      memmove(this->ObjCenter, this->InteractionActor->GetCenter(),
              3 * sizeof(float));
  
      // get the axis to rotate around = vector from eye to origin
      if (this->CurrentCamera->GetParallelProjection())
        {
        this->CurrentCamera->ComputeViewPlaneNormal();
        this->CurrentCamera->GetViewPlaneNormal(this->MotionVector);
        }
      else
        {   
        this->CurrentCamera->GetPosition(this->ViewPoint);
        this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
        this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
        this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
        vtkMath::Normalize(this->MotionVector);
        }
      
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);

      this->HighlightActor(NULL);
      this->Preprocess = 0;
      }
    
    // this has to be in the loop
    float newAngle = atan2((float)(this->Size[1] - y - this->DispObjCenter[1]),
                           (float)(x - this->DispObjCenter[0]));
    float oldAngle = atan2((float)(this->Size[1] - this->OldY -
                                   this->DispObjCenter[1]),
                           (float)(this->OldX - this->DispObjCenter[0]));
    
    newAngle *= this->RadianToDegree;
    oldAngle *= this->RadianToDegree;

    float scale[3];
    scale[0] = scale[1] = scale[2] = 1.0;
    float **rotate = new float*[1];
    rotate[0] = new float[4];

    rotate[0][0] = newAngle - oldAngle;
    rotate[0][1] = this->MotionVector[0];
    rotate[0][2] = this->MotionVector[1];
    rotate[0][3] = this->MotionVector[2];
  
    this->ActorTransform(this->InteractionActor,
                         this->ObjCenter,
                         1, rotate, scale);

    delete [] rotate[0];
    delete [] rotate;
    
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}

// Description:
// pan the actor in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballPanActor(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // use initial center as the origin from which to pan
      memmove(this->ObjCenter,
              this->InteractionActor->GetCenter(), 3 * sizeof(float));
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);
      this->FocalDepth = this->DispObjCenter[2];
      
      this->HighlightActor(NULL);
      this->Preprocess = 0;
      }
  
    this->ComputeDisplayToWorld(float(x), float(this->Size[1] - y),
                                this->FocalDepth,
                                this->NewPickPoint);

    this->ComputeDisplayToWorld(float(this->OldX),
                                float(this->Size[1] - this->OldY),
                                this->FocalDepth, this->OldPickPoint);
    
    this->MotionVector[0] = this->NewPickPoint[0] - this->OldPickPoint[0];
    this->MotionVector[1] = this->NewPickPoint[1] - this->OldPickPoint[1];
    this->MotionVector[2] = this->NewPickPoint[2] - this->OldPickPoint[2];

    if (this->InteractionActor->GetUserMatrix() != NULL)
      {
      vtkTransform *t = vtkTransform::New();
      t->PostMultiply();
      t->SetMatrix(*(this->InteractionActor->GetUserMatrix()));
      t->Translate(this->MotionVector[0], this->MotionVector[1], 
		   this->MotionVector[2]);
      this->InteractionActor->GetUserMatrix()->DeepCopy(t->GetMatrixPointer());
      t->Delete();
      }
    else
      {
      this->InteractionActor->AddPosition(this->MotionVector);
      }
      
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}


// Description:
// Dolly the actor in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballDollyActor(int x, int y)
{
  if (this->OldY != y)
    {
    if (this->Preprocess)
      {
      this->CurrentCamera->GetPosition(this->ViewPoint);
      this->CurrentCamera->GetFocalPoint(this->ViewFocus);

      this->HighlightActor(NULL);
      this->Preprocess = 0;
      }
    
    float yf = (float)(this->OldY - y) / (float)(this->Center[1]) *
      this->TrackballFactor;
    float dollyFactor = pow((float)1.1, yf);

    dollyFactor -= 1.0;
    this->MotionVector[0] = (this->ViewPoint[0] -
                             this->ViewFocus[0]) * dollyFactor;
    this->MotionVector[1] = (this->ViewPoint[1] -
                             this->ViewFocus[1]) * dollyFactor;
    this->MotionVector[2] = (this->ViewPoint[2] -
                             this->ViewFocus[2]) * dollyFactor;
    
    if (this->InteractionActor->GetUserMatrix() != NULL)
      {
      vtkTransform *t = vtkTransform::New();
      t->PostMultiply();
      t->SetMatrix(*(this->InteractionActor->GetUserMatrix()));
      t->Translate(this->MotionVector[0], this->MotionVector[1], 
		   this->MotionVector[2]);
      this->InteractionActor->GetUserMatrix()->DeepCopy(t->GetMatrixPointer());
      t->Delete();
      }
    else
      {
      this->InteractionActor->AddPosition(this->MotionVector);
      }
  
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}

// Description:
// Scale the actor in trackball (motion sensitive) style
void vtkRenderWindowInteractor::TrackballScaleActor(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      memmove(this->ObjCenter, this->InteractionActor->GetCenter(),
              3 * sizeof(float));

      this->HighlightActor(NULL);
      this->Preprocess = 0;
      }
    
    float yf = (float)(this->OldY - y) / (float)(this->Center[1]) *
      this->TrackballFactor;
    float scaleFactor = pow((float)1.1, yf);          

    float **rotate = NULL;
    
    float scale[3];
    scale[0] = scale[1] = scale[2] = scaleFactor;
    
    this->ActorTransform(this->InteractionActor,
                         this->ObjCenter,
                         0, rotate, scale);
    
    this->OldX = x;
    this->OldY = y;
    this->RenderWindow->Render();
    }
}


void vtkRenderWindowInteractor::ActorTransform(vtkActor *actor,
                                               float *boxCenter,
                                               int numRotation,
                                               float **rotate,
                                               float *scale)
{
  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  actor->GetMatrix(oldMatrix);

  float orig[3];
  actor->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  if (actor->GetUserMatrix() != NULL)
    {
    newTransform->SetMatrix(*(actor->GetUserMatrix()));
    }
  else
    {
    newTransform->SetMatrix(*oldMatrix);
    }

  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
  for (int i = 0; i < numRotation; i++)
    {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1],
                             rotate[i][2], rotate[i][3]);
    }

  if ((scale[0] * scale[1] * scale[2]) != 0.0)
    {
    newTransform->Scale(scale[0], scale[1], scale[2]);
    }

  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);

  // now try to get the composit of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);
  
  if (actor->GetUserMatrix() != NULL)
    {
    newTransform->GetMatrix(actor->GetUserMatrix());
    }
  else
    {
    actor->SetPosition(newTransform->GetPosition());
    actor->SetScale(newTransform->GetScale());
    actor->SetOrientation(newTransform->GetOrientation());
    }
  
  oldMatrix->Delete();
  newTransform->Delete();
}


//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetActorModeToCamera()
{
  if (this->ActorMode == VTKXI_CAMERA)
    {
    return;
    }
  this->ActorMode = VTKXI_CAMERA;
  this->Modified();
  if (this->CameraModeMethod) 
    {
    (*this->CameraModeMethod)(this->CameraModeMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetActorModeToActor()
{
  if (this->ActorMode == VTKXI_ACTOR)
    {
    return;
    }
  this->ActorMode = VTKXI_ACTOR;
  this->Modified();
  if (this->ActorModeMethod) 
    {
    (*this->ActorModeMethod)(this->ActorModeMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetTrackballModeToTrackball()
{
  if (this->TrackballMode == VTKXI_TRACK)
    {
    return;
    }
  this->TrackballMode = VTKXI_TRACK;
  this->Modified();
  if (this->TrackballModeMethod) 
    {
    (*this->TrackballModeMethod)(this->TrackballModeMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetTrackballModeToJoystick()
{
  if (this->TrackballMode == VTKXI_JOY)
    {
    return;
    }
  this->TrackballMode = VTKXI_JOY;
  this->Modified();
  if (this->JoystickModeMethod) 
    {
    (*this->JoystickModeMethod)(this->JoystickModeMethodArg);
    }
}

  
  
