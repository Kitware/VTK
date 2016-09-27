/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSRenderWindowInteractor.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderWindow.h"

#import "vtkIOSRenderWindowInteractor.h"
#import "vtkIOSRenderWindow.h"
#import "vtkCommand.h"
#import "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkIOSRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*vtkIOSRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkIOSRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
vtkIOSRenderWindowInteractor::vtkIOSRenderWindowInteractor()
{
}

//----------------------------------------------------------------------------
vtkIOSRenderWindowInteractor::~vtkIOSRenderWindowInteractor()
{
  this->Enabled = 0;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::StartEventLoop()
{
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkIOSRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if ( !this->RenderWindow )
  {
    vtkErrorMacro(<<"No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  vtkIOSRenderWindow *renWin = (vtkIOSRenderWindow *)(this->RenderWindow);
  renWin->Start();
  int *size = renWin->GetSize();

  renWin->GetPosition(); // update values of this->Position[2]

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::Enable()
{
  if (this->Enabled)
  {
    return;
  }

  // Set the RenderWindow's interactor so that when the vtkIOSGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(this);

  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized())
  {
    this->Device->Close();
  }
#endif

  // Set the RenderWindow's interactor so that when the vtkIOSGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(NULL);

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::TerminateApp()
{
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindowInteractor::InternalCreateTimer(int timerId,
  int timerType, unsigned long duration)
{
  // In this implementation, timerId and platformTimerId are the same
  int platformTimerId = timerId;

  return platformTimerId;
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  // In this implementation, timerId and platformTimerId are the same;
  // but calling this anyway is more correct
  int timerId = this->GetVTKTimerId(platformTimerId);

  return 0; // fail
}

//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkIOSRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkIOSRenderWindowInteractor::ClassExitMethod
  || arg != vtkIOSRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkIOSRenderWindowInteractor::ClassExitMethodArg)
     && (vtkIOSRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkIOSRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkIOSRenderWindowInteractor::ClassExitMethod = f;
    vtkIOSRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkIOSRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkIOSRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindowInteractor::ExitCallback()
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
