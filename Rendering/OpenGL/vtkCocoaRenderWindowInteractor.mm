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

#import <Cocoa/Cocoa.h>
#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs

#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCommand.h"
#import "vtkObjectFactory.h"

#ifdef VTK_USE_TDX
#import "vtkTDxMacDevice.h"
#endif


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCocoaRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*vtkCocoaRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkCocoaRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
//----------------------------------------------------------------------------
// As vtk is both crossplatform and a library, we don't know if it is being
// used in a 'regular Cocoa application' or as a 'pure vtk application'.
// By the former we mean a regular Cocoa application that happens to have
// a vtkCocoaGLView, by the latter we mean an application that only uses
// VTK APIs (which happen to use Cocoa as an implementation detail).
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
// problem thus is that other VTK code may be called before Start() and that
// code may call Cocoa and thus autorelease objects with no autorelease pool
// in place.  The (ugly) solution is to create a 'pool of last resort' so
// that we know a pool is always in place.
// See: <http://lists.apple.com/archives/cocoa-dev/2006/Sep/msg00222.html>
//
// With garbage collection (GC), autorelease pools are a thing of the past,
// and this hack is not needed.  However, Obj-C code can be compiled in
// 1 of 4 memory management modes: Manual Retain Release (MRR), GC supported,
// GC required, or Automatic Reference Counting (ARC).  Library code like
// VTK should work with all 4; currently all are supported except ARC.
class vtkEarlyCocoaSetup
  {
    public:
    vtkEarlyCocoaSetup()
    {
      this->CreatePoolOfLastResort();
    }

    virtual ~vtkEarlyCocoaSetup()
    {
      this->DestroyPoolOfLastResort();
    }

    protected:
    void DestroyPoolOfLastResort()
    {
      [Pool drain];
      Pool = nil;
    }

    void CreatePoolOfLastResort()
    {
      Pool = [[NSAutoreleasePool alloc] init];
    }

    private:
    NSAutoreleasePool     *Pool;
  };

// We create a global/static instance of this class to ensure that we have an
// autorelease pool before main() starts (the C++ constructor for a global
// object runs before main).  Note: I am unable to find a place to delete this
// object safely, but having it around for the lifetime of the process is ok.
static vtkEarlyCocoaSetup * gEarlyCocoaSetup = new vtkEarlyCocoaSetup();

//----------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
//----------------------------------------------------------------------------
@interface vtkCocoaTimer : NSObject
{
  @private
  NSTimer *_timer;
  vtkCocoaRenderWindowInteractor *_interactor;
  int _timerId;
}

- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor timerId:(int)myTimerId;
- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating;
- (void)stopTimer;
- (void)timerFired:(NSTimer *)myTimer;

@end

//----------------------------------------------------------------------------
@implementation vtkCocoaTimer

//----------------------------------------------------------------------------
- (id)initWithInteractor:(vtkCocoaRenderWindowInteractor *)myInteractor timerId:(int)myTimerId
{
  self = [super init];
  if (self)
    {
    _interactor = myInteractor;
    _timerId = myTimerId;
    }
  return self;
}

//----------------------------------------------------------------------------
- (void)timerFired:(NSTimer *)myTimer
{
  (void)myTimer;
  _interactor->InvokeEvent(vtkCommand::TimerEvent, &_timerId);
}

//----------------------------------------------------------------------------
- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating
{
  _timer = [[NSTimer timerWithTimeInterval:interval
                                    target:self
                                  selector:@selector(timerFired:)
                                  userInfo:nil
                                   repeats:repeating] retain];
  [[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSDefaultRunLoopMode];
  [[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSEventTrackingRunLoopMode];
}

//----------------------------------------------------------------------------
- (void)stopTimer
{
  [_timer invalidate];
  [_timer release];
  _timer = nil;
}

@end

// This is a private class and an implementation detail, do not use it.
// It manages the application run loop of a "pure vtk application",
// as opposed to a regular Mac app that happens to use VTK.
//----------------------------------------------------------------------------
@interface vtkCocoaServer : NSObject
{
  @private
  vtkCocoaRenderWindow* _renWin;
}

+ (id)cocoaServerWithRenderWindow:(vtkCocoaRenderWindow *)inRenderWindow;

- (void)start;
- (void)stop;

@end

//----------------------------------------------------------------------------
@implementation vtkCocoaServer

//----------------------------------------------------------------------------
- (id)initWithRenderWindow:(vtkCocoaRenderWindow *)inRenderWindow
{
  self = [super init];
  if (self)
    {
    _renWin = inRenderWindow;
    }
  return self;
}

//----------------------------------------------------------------------------
+ (id)cocoaServerWithRenderWindow:(vtkCocoaRenderWindow *)inRenderWindow
{
  vtkCocoaServer *server = [[[vtkCocoaServer alloc]
                            initWithRenderWindow:inRenderWindow]
                            autorelease];
  return server;
}

//----------------------------------------------------------------------------
- (void)start
{
  // Retrieve the NSWindow.
  NSWindow *win = nil;
  if (_renWin != NULL)
    {
    win = reinterpret_cast<NSWindow *>(_renWin->GetRootWindow());

    // We don't want to be informed of every window closing, so check for nil.
    if (win != nil)
      {
      // Register for the windowWillClose notification in order to stop
      // the run loop if the window closes.
      NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
      [nc addObserver:self selector:@selector(windowWillClose:)
                               name:NSWindowWillCloseNotification
                             object:win];
      }
    }

  // Start the NSApplication's run loop
  NSApplication* application = [NSApplication sharedApplication];
  [application run];
}

//----------------------------------------------------------------------------
- (void)stop
{
  // Retrieve the NSWindow.
  NSWindow *win = nil;
  if (_renWin != NULL)
    {
    win = reinterpret_cast<NSWindow *>(_renWin->GetRootWindow());
    }

  // Close the window, removing it from the screen and releasing it
  [win close];
}

//----------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification*)aNotification
{
  (void)aNotification;

  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:self name:NSWindowWillCloseNotification object:nil];

  if (_renWin)
    {
    int windowCreated = _renWin->GetWindowCreated();
    if (windowCreated)
      {
      // Stop the current run loop. Do not terminate as it will not return to
      // main. However, the stop message puts the run loop asleep. Let's send
      // some event to wake it up so it can get out of the loop.
      NSApplication* application = [NSApplication sharedApplication];
      [application stop:application];

      NSEvent *event = [NSEvent otherEventWithType:NSApplicationDefined
                                          location:NSZeroPoint
                                     modifierFlags:0
                                         timestamp:0
                                      windowNumber:-1
                                           context:nil
                                           subtype:0
                                             data1:0
                                             data2:0];
      [application postEvent:event atStart:YES];

      // The NSWindow is closing, so prevent anyone from accidentally using it
      _renWin->SetRootWindow(NULL);
      }
    }
}

@end

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::vtkCocoaRenderWindowInteractor()
{
  this->InstallMessageProc = 1;

  // First, create the cocoa objects manager. The dictionary is empty so
  // essentially all objects are initialized to NULL.
  NSMutableDictionary * cocoaManager = [NSMutableDictionary dictionary];

  // SetCocoaManager works like an Obj-C setter, so do like Obj-C and
  // init the ivar to null first.
  this->CocoaManager = NULL;
  this->SetCocoaManager(reinterpret_cast<void *>(cocoaManager));
  [cocoaManager self]; // prevent premature collection.

  NSMutableDictionary* timerDictionary = [NSMutableDictionary dictionary];
  this->SetTimerDictionary(reinterpret_cast<void *>(timerDictionary));
  [timerDictionary self]; // prevent premature collection.

#ifdef VTK_USE_TDX
  this->Device=vtkTDxMacDevice::New();
#endif
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::~vtkCocoaRenderWindowInteractor()
{
  this->Enabled = 0;
  this->SetTimerDictionary(NULL);
  this->SetCocoaServer(NULL);
  this->SetCocoaManager(NULL);
#ifdef VTK_USE_TDX
  this->Device->Delete();
#endif
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent) && !this->HandleEventLoop)
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc)
    {
    return;
    }

  // Start server.
  vtkCocoaRenderWindow *renWin = vtkCocoaRenderWindow::SafeDownCast(this->GetRenderWindow());
  if (renWin != NULL)
    {
    vtkCocoaServer *server = [vtkCocoaServer cocoaServerWithRenderWindow:renWin];
    this->SetCocoaServer(reinterpret_cast<void *>(server));
    [server start];
    }
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkCocoaRenderWindowInteractor::Initialize()
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
  vtkCocoaRenderWindow *renWin = (vtkCocoaRenderWindow *)(this->RenderWindow);
  renWin->Start();
  int *size = renWin->GetSize();

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

#ifdef VTK_USE_TDX
  if(this->UseTDx)
    {
    this->Device->SetInteractor(this);
    this->Device->Initialize();
    }
#endif

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

#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized())
    {
    this->Device->Close();
    }
#endif

  // Set the RenderWindow's interactor so that when the vtkCocoaGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(NULL);

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::TerminateApp()
{
  // Retrieve the cocoa render window.
  vtkCocoaRenderWindow *renWin = vtkCocoaRenderWindow::SafeDownCast(this->RenderWindow);
  if (renWin)
    {
    // Check if the NSWindow was created by VTK. If so, let's use the
    // vtkCocoaServer to stop the application since it started it.
    // This means that the interactor was started using the Start method.
    // The vtkCocoaServer is used to get out of that method without quiting
    // the application, allowing to access code after the Start() call, like
    // object deletion for example.
    int windowCreated = renWin->GetWindowCreated();
    if (windowCreated)
      {
      vtkCocoaServer *server = reinterpret_cast<vtkCocoaServer *>(this->GetCocoaServer());
      [server stop];
      }
    else
      {
      // The NSWindow was not created by vtk itself, so let's terminate the
      // application as requested.
      NSApplication* application = [NSApplication sharedApplication];
      [application terminate:application];
      }
    }

   // Release vtkCocoaServer if created by the start method.
   this->SetCocoaServer(NULL);
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
  vtkCocoaTimer *cocoaTimer = [[vtkCocoaTimer alloc] initWithInteractor:this
                                                                timerId:timerId];
  NSString *timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary *timerDict = (NSMutableDictionary*)(this->GetTimerDictionary());
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

  NSString *timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary *timerDict = (NSMutableDictionary*)(this->GetTimerDictionary());
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

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::SetTimerDictionary(void *dictionary)
{
  if (dictionary != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(dictionary)
                forKey:@"TimerDictionary"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"TimerDictionary"];
    }
}

//----------------------------------------------------------------------------
void *vtkCocoaRenderWindowInteractor::GetTimerDictionary()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"TimerDictionary"]);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::SetCocoaServer(void *server)
{
  if (server != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<vtkCocoaServer *>(server)
                forKey:@"CocoaServer"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"CocoaServer"];
    }
}

//----------------------------------------------------------------------------
void *vtkCocoaRenderWindowInteractor::GetCocoaServer()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"CocoaServer"]);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::SetCocoaManager(void *manager)
{
  NSMutableDictionary* currentCocoaManager =
    reinterpret_cast<NSMutableDictionary *>(this->CocoaManager);
  NSMutableDictionary* newCocoaManager =
    reinterpret_cast<NSMutableDictionary *>(manager);

  if (currentCocoaManager != newCocoaManager)
    {
    // Why not use Cocoa's retain and release?  Without garbage collection
    // (GC), the two are equivalent anyway because of 'toll free bridging',
    // so no problem there.  With GC, retain and release do nothing, but
    // CFRetain and CFRelease still manipulate the internal reference count.
    // We need that, since we are not using strong references (we don't want
    // it collected out from under us!).
    if (currentCocoaManager)
      {
      CFRelease(currentCocoaManager);
      }
    if (newCocoaManager)
      {
      this->CocoaManager = const_cast<void*>(CFRetain (newCocoaManager));
      }
    else
      {
      this->CocoaManager = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void *vtkCocoaRenderWindowInteractor::GetCocoaManager()
{
  return this->CocoaManager;
}
