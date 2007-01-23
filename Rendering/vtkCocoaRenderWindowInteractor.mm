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
vtkCxxRevisionMacro(vtkCocoaRenderWindowInteractor, "1.14");
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
  int timerId;
}

- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor timerId:(int)myTimerId;
- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating;
- (void)stopTimer;
- (void)timerFired:(NSTimer *)myTimer;

@end

// This is a private class and an implementation detail, do not use it.
//
// As vtk is both crossplatform and a library, we don't know if it is being
// used in a 'regular Cocoa application' or as a 'pure vtk application'.
// By the former I mean a regular Cocoa application that happens to have
// a vtkCocoaGLView, by the latter I mean an application that only uses
// vtk APIs (which happen to use Cocoa as an implementation detail).
// Specifically, we can't know if NSApplicationMain() was ever called
// (which is usually done in main()), nor whether the NSApplication exists.
//
// Any code that uses Cocoa needs to be sure that NSAutoreleasePools are
// correctly setup.  Specifically, an NSAutoreleasePool needs to exist before
// any Cocoa objects are autoreleased, which may happen as a side effect of
// pretty much any Cocoa use.  Once we start the event loop by calling 
// either NSApplicationMain() or [NSApp run] then a new pool will be created
// for every event.  In a 'regular Cocoa application' NSApplicationMain() is
// called at the start of main() and so we know a pool will be in place before
// any vtk code is used.  But in a 'pure vtk application' the event loop does
// not start until [NSApp run] is called by this class's Start() method. The
// problem thus is that other vtk code may be called before Start() and that
// code may call Cocoa and thus autorelease objects with no autorelease pool
// in place.  The (ugly) solution is to create a 'pool of last resort' so
// that we know a pool is always in place.
// See: <http://lists.apple.com/archives/cocoa-dev/2006/Sep/msg00222.html>

class vtkEarlyCocoaSetup
{
public:
  vtkEarlyCocoaSetup::vtkEarlyCocoaSetup()
  {
    this->CreatePoolOfLastResort();
  }

  vtkEarlyCocoaSetup::~vtkEarlyCocoaSetup()
  {
    this->DestroyPoolOfLastResort();
  }

  void vtkEarlyCocoaSetup::DestroyPoolOfLastResort()
  {
    [Pool release];
    Pool = nil;
  }

protected:
  void vtkEarlyCocoaSetup::CreatePoolOfLastResort()
  {
    Pool = [[NSAutoreleasePool alloc] init];
  }

private:
  NSAutoreleasePool     *Pool;
};

// We create a global/static instance of this class to ensure that we have an
// autorelease pool before main() starts.
vtkEarlyCocoaSetup * gEarlyCocoaSetup = new vtkEarlyCocoaSetup();

//----------------------------------------------------------------------------
@implementation vtkCocoaTimer

- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor timerId:(int)myTimerId
{
  self = [super init]; 
  if (self)
    {
    interactor = myInteractor;
    timerId = myTimerId;
    }
  return self;
}

- (void)timerFired:(NSTimer *)myTimer
{
  (void)myTimer;
  interactor->InvokeEvent(vtkCommand::TimerEvent, &timerId);
}

- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating
{
  timer = [[NSTimer timerWithTimeInterval:interval
    target:self
    selector:@selector(timerFired:)
    userInfo:nil
    repeats:repeating] retain];
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode];
}

- (void)stopTimer
{
  [timer invalidate];
  [timer release];
  timer = nil;
}

@end

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::vtkCocoaRenderWindowInteractor() 
{
  this->InstallMessageProc = 1;
  this->TimerDictionary = (void*)[[NSMutableDictionary dictionary] retain];
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::~vtkCocoaRenderWindowInteractor() 
{
  this->Enabled = 0;
  NSMutableDictionary* timerDict = (NSMutableDictionary*)(this->TimerDictionary);
  [timerDict release];
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

  // Now that we are about to begin the standard Cocoa event loop, we can get
  // rid of the 'pool of last resort' because [NSApp run] will create a new
  // pool for event event
  gEarlyCocoaSetup->DestroyPoolOfLastResort();
  
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
int vtkCocoaRenderWindowInteractor::InternalCreateTimer(int timerId,
  int timerType, unsigned long duration)
{
  BOOL repeating = NO;

  if (RepeatingTimer == timerType)
    {
    repeating = YES;
    }

  // Create a vtkCocoaTimer and add it to a dictionary using the timerId
  // as key, this will let us find the vtkCocoaTimer later by timerId
  vtkCocoaTimer* cocoaTimer = [[vtkCocoaTimer alloc] initWithInteractor:this
    timerId:timerId];  
  NSString* timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary* timerDict = (NSMutableDictionary*)(this->TimerDictionary);
  [timerDict setObject:cocoaTimer forKey:timerIdAsStr];
  [cocoaTimer startTimerWithInterval:((NSTimeInterval)duration/1000.0)
    repeating:repeating];

  // In this implementation, timerId and platformTimerId are the same
  int platformTimerId = timerId;
  
  return platformTimerId;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  // In this implementation, timerId and platformTimerId are the same;
  // but calling this anyway is more correct
  int timerId = this->GetVTKTimerId(platformTimerId);

  NSString* timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary* timerDict = (NSMutableDictionary*)(this->TimerDictionary);
  vtkCocoaTimer* cocoaTimer = [timerDict objectForKey:timerIdAsStr];
  [timerDict removeObjectForKey:timerIdAsStr];

  if (nil != cocoaTimer)
    {
    [cocoaTimer stopTimer];
    [cocoaTimer release];
    return 1; // success
    }
  else
    {
    return 0; // fail
    }
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
