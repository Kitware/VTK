/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaRenderWindowInteractor.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCommand.h"
#import "vtkObjectFactory.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>

#ifndef MAC_OS_X_VERSION_10_4
#define MAC_OS_X_VERSION_10_4 1040
#endif

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkCocoaRenderWindowInteractor, "1.9");
vtkStandardNewMacro(vtkCocoaRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*vtkCocoaRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkCocoaRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

// This is a private class and an implementation detail, do not use it.
//----------------------------------------------------------------------------
@interface vtkCocoaTimer : NSObject
{
  NSTimer *timer;
  vtkCocoaRenderWindowInteractor *interactor;
}

- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor;
- (void)startTimer;
- (void)stopTimer;
- (void)timerFired:(NSTimer *)myTimer;

@end

//----------------------------------------------------------------------------
@implementation vtkCocoaTimer

- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor
{
  self = [super init]; 
  if (self)
    {
    interactor = myInteractor;
    }
  return self;
}

- (void)timerFired:(NSTimer *)myTimer
{
  (void)myTimer;
  interactor->InvokeEvent(vtkCommand::TimerEvent,NULL);
}

- (void)startTimer
{
  timer = [NSTimer timerWithTimeInterval:0.01
  target:self
  selector:@selector(timerFired:)
  userInfo:nil
  repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode];
}

- (void)stopTimer
{
  [timer invalidate];
}

@end



//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkCocoaRenderWindowInteractor::vtkCocoaRenderWindowInteractor() 
{
  this->InstallMessageProc = 1;
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::~vtkCocoaRenderWindowInteractor() 
{
  this->Enabled = 0;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Start() 
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent))
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc) 
    {
    return;
    }

  (void)[NSApplication sharedApplication]; //make sure the app is initialized
  [NSApp run];
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkCocoaRenderWindowInteractor::Initialize()
{
  vtkCocoaRenderWindow *renWin;
  int *size;

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
  renWin = (vtkCocoaRenderWindow *)(this->RenderWindow);
  renWin->Start();
  size = renWin->GetSize();

  renWin->GetPosition(); // update values of this->Position[2]

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Enable() 
{
  if (this->Enabled) 
    {
    return;
    }

  // Set the RenderWindow's interactor so that when the vtkCocoaGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(this);

  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Disable() 
{
  if (!this->Enabled) 
    {
    return;
    }

  // Set the RenderWindow's interactor so that when the vtkCocoaGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(NULL);

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::TerminateApp() 
{
  [NSApp terminate:NSApp];
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindowInteractor::CreateTimer(int timertype) 
{
  if (timertype==VTKI_TIMER_FIRST)
    {
    this->Timer = (void*)[[vtkCocoaTimer alloc] initWithInteractor:this];
    [(vtkCocoaTimer*)this->Timer startTimer];
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindowInteractor::DestroyTimer()
{
  [(vtkCocoaTimer*)this->Timer stopTimer];
  [(vtkCocoaTimer*)this->Timer release];
  this->Timer = 0;
  return 1;
}

//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkCocoaRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkCocoaRenderWindowInteractor::ClassExitMethod
  || arg != vtkCocoaRenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkCocoaRenderWindowInteractor::ClassExitMethodArg)
     && (vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkCocoaRenderWindowInteractor::ClassExitMethodArg);
      }
    vtkCocoaRenderWindowInteractor::ClassExitMethod = f;
    vtkCocoaRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}


//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkCocoaRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::ExitCallback()
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

