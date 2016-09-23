/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOculusRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include "vtkOculusRenderWindowInteractor.h"
#include "vtkOculusRenderWindow.h"
#include "vtkRendererCollection.h"


#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkOculusCamera.h"
#include "vtkNew.h"
#include "vtkInteractorStyle3D.h"
#include "vtkPropPicker3D.h"

vtkStandardNewMacro(vtkOculusRenderWindowInteractor);

void (*vtkOculusRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkOculusRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkOculusRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOculusRenderWindowInteractor::vtkOculusRenderWindowInteractor()
{
}

//----------------------------------------------------------------------------
vtkOculusRenderWindowInteractor::~vtkOculusRenderWindowInteractor()
{
}

void vtkOculusRenderWindowInteractor::SetPhysicalTranslation(vtkCamera *camin, double t1, double t2, double t3)
{
  vtkOculusCamera *cam =
    static_cast<vtkOculusCamera *>(camin);
  cam->SetTranslation(t1,t2,t3);
}

double *vtkOculusRenderWindowInteractor::GetPhysicalTranslation(vtkCamera *camin)
{
  vtkOculusCamera *cam =
    static_cast<vtkOculusCamera *>(camin);
  return cam->GetTranslation();
}

//----------------------------------------------------------------------------
void  vtkOculusRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkOculusRenderWindow *renWin =
    vtkOculusRenderWindow::SafeDownCast(this->RenderWindow);

  SDL_Event event;
  ovrSessionStatus ss;
  ovrSession session = renWin->GetSession();

  while (!this->Done)
  {
    if (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN &&
          event.key.keysym.sym == SDLK_SPACE)
      {
        this->Done = true;
      }
      if (event.type == SDL_KEYDOWN &&
          event.key.keysym.sym == SDLK_r)
      {
        ovr_RecenterTrackingOrigin(session); // or ovr_ClearShouldRecenterFlag(session) to ignore the request.
      }
    }
    ovr_GetSessionStatus(session, &ss);
    if (ss.ShouldQuit)
    {
      this->Done = true;
    }
    if (ss.ShouldRecenter)
    {
      ovr_RecenterTrackingOrigin(session); // or ovr_ClearShouldRecenterFlag(session) to ignore the request.
    }

    renWin->Render();
  }
}


//----------------------------------------------------------------------------
void vtkOculusRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
  {
    vtkErrorMacro(<<"No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }

  vtkOculusRenderWindow *ren =
    vtkOculusRenderWindow::SafeDownCast(this->RenderWindow);
  int *size;

  this->Initialized = 1;
  // get the info we need from the RenderingWindow

  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkOculusRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;
}

//----------------------------------------------------------------------------
int vtkOculusRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId),
  int vtkNotUsed(timerType),
  unsigned long vtkNotUsed(duration))
{
  // todo
  return 0;
}

//----------------------------------------------------------------------------
int vtkOculusRenderWindowInteractor::InternalDestroyTimer(
  int vtkNotUsed(platformTimerId))
{
  // todo
  return 0;
}


//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkOculusRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkOculusRenderWindowInteractor::ClassExitMethod
       || arg != vtkOculusRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkOculusRenderWindowInteractor::ClassExitMethodArg)
        && (vtkOculusRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkOculusRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkOculusRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkOculusRenderWindowInteractor::ClassExitMethod = f;
    vtkOculusRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void
vtkOculusRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkOculusRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkOculusRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void vtkOculusRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
void vtkOculusRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }

  this->TerminateApp();
}
