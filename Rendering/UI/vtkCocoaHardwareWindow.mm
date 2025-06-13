/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaHardwareWindow.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// vtk includes
#import "vtkCocoaHardwareWindow.h"
#import "vtkObjectFactory.h"

// cocoa
#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs
#import <Cocoa/Cocoa.h>

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCocoaHardwareWindow);
VTK_ABI_NAMESPACE_END

//-------------------------------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
// For fullscreen, an NSWindow that captures key events even when borderless
@interface vtkCocoaFullScreenWindow : NSWindow
{
}
@end

@implementation vtkCocoaFullScreenWindow
- (BOOL)canBecomeKeyWindow
{
  return YES;
}
@end

//-------------------------------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
// It manages the NSView/NSWindow. It observes for the NSView's frame changing
// position or size. It observes for the NSWindow closing.
//-------------------------------------------------------------------------------------------------
@interface vtkCocoaServer : NSObject
{
@private
  vtkCocoaHardwareWindow* _hardwareWin;
}

// Designated initializer
- (id)initWithHardwareWindow:(vtkCocoaHardwareWindow*)inHardwareWindow;

- (void)startObservations;
- (void)stopObservations;

@end

//-------------------------------------------------------------------------------------------------
@implementation vtkCocoaServer

//-------------------------------------------------------------------------------------------------
- (id)initWithHardwareWindow:(vtkCocoaHardwareWindow*)inHardwareWindow
{
  self = [super init];
  if (self)
  {
    _hardwareWin = inHardwareWindow;
  }
  return self;
}

//-------------------------------------------------------------------------------------------------
- (void)startObservations
{
  assert(_hardwareWin);

  vtkTypeBool windowCreated = _hardwareWin->GetWindowCreated();
  NSWindow* win = reinterpret_cast<NSWindow*>(_hardwareWin->GetRootWindow());
  if (windowCreated && win)
  {
    // Receive notifications of this, and only this, window's closing.
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self
           selector:@selector(windowWillClose:)
               name:NSWindowWillCloseNotification
             object:win];
  }

  NSView* view = reinterpret_cast<NSView*>(_hardwareWin->GetWindowId());
  if (view)
  {
    // Receive notifications of this, and only this, view's frame changing.
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self
           selector:@selector(viewFrameDidChange:)
               name:NSViewFrameDidChangeNotification
             object:view];
  }
}

//-------------------------------------------------------------------------------------------------
- (void)stopObservations
{
  assert(_hardwareWin);

  vtkTypeBool windowCreated = _hardwareWin->GetWindowCreated();
  NSWindow* win = reinterpret_cast<NSWindow*>(_hardwareWin->GetRootWindow());
  if (windowCreated && win)
  {
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:self name:NSWindowWillCloseNotification object:win];
  }

  NSView* view = reinterpret_cast<NSView*>(_hardwareWin->GetWindowId());
  if (view)
  {
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:self name:NSViewFrameDidChangeNotification object:view];
  }
}

//-------------------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification*)aNotification
{
  // We should only get here if it was us that created the NSWindow.
  assert(_hardwareWin);
  assert(_hardwareWin->GetWindowCreated());

  // We should only have observed our own NSWindow.
  assert([aNotification object] == _hardwareWin->GetRootWindow());
  (void)aNotification;

  // Stop observing because the window is closing.
  [self stopObservations];

  // The NSWindow is closing, so prevent anyone from accidentally using it.
  _hardwareWin->SetRootWindow(nullptr);

  // Tell interactor to stop the NSApplication's run loop
  vtkRenderWindowInteractor* interactor = _hardwareWin->GetInteractor();
  vtkTypeBool windowCreated = _hardwareWin->GetWindowCreated();
  if (interactor && windowCreated)
  {
    interactor->TerminateApp();
  }
}

//-------------------------------------------------------------------------------------------------
- (void)viewFrameDidChange:(NSNotification*)aNotification
{
  // We should only have observed our own NSView.
  assert(_hardwareWin);
  assert([aNotification object] == _hardwareWin->GetWindowId());
  (void)aNotification;

  // Retrieve the Interactor.
  vtkRenderWindowInteractor* interactor = _hardwareWin->GetInteractor();
  if (!interactor || !interactor->GetEnabled())
  {
    return;
  }

  // Get the NSView's new frame size (in points).
  NSView* view = reinterpret_cast<NSView*>(_hardwareWin->GetWindowId());
  assert(view);
  NSRect viewRect = [view frame];

  // Convert from points to pixels.
  NSRect backingViewRect = [view convertRectToBacking:viewRect];

  int newWidth = static_cast<int>(NSWidth(backingViewRect));
  int newHeight = static_cast<int>(NSHeight(backingViewRect));

  // Get the interactor's current cache of the size.
  int size[2];
  interactor->GetSize(size);

  if (newWidth != size[0] || newHeight != size[1])
  {
    // Process the size change, this sends vtkCommand::WindowResizeEvent.
    interactor->UpdateSize(newWidth, newHeight);

    // Send vtkCommand::ConfigureEvent from the Interactor.
    interactor->InvokeEvent(vtkCommand::ConfigureEvent, nullptr);
  }
}

@end

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------------------------------------------------
vtkCocoaHardwareWindow::vtkCocoaHardwareWindow()
{
  // First, create the cocoa objects manager. The dictionary is empty so
  // essentially all objects are initialized to NULL.
  NSMutableDictionary* cocoaManager = [NSMutableDictionary dictionary];

  // SetCocoaManager works like an Obj-C setter, so do like Obj-C and
  // init the ivar to null first.
  this->CocoaManager = nullptr;
  this->SetCocoaManager(reinterpret_cast<void*>(cocoaManager));
  [cocoaManager self]; // prevent premature collection under GC.

  this->WindowCreated = 0;
  this->ViewCreated = 0;
  this->SetWindowName("Visualization Toolkit - Cocoa");
  this->CursorHidden = 0;
  this->OnScreenInitialized = 0;
}

//----------------------------------------------------------------------------------------------------------------------
vtkCocoaHardwareWindow::~vtkCocoaHardwareWindow()
{
  // If we created a vtkCocoaHardwareView, clear its reference back to us.
  if (this->GetViewCreated())
  {
    NSView* hardwareView = (NSView*)this->GetWindowId();
    if ([hardwareView isKindOfClass:[vtkCocoaHardwareView class]])
    {
      [(vtkCocoaHardwareView*)hardwareView setHardwareWindow:nullptr];
    }
  }

  if (this->CursorHidden)
  {
    this->ShowCursor();
  }

  this->Destroy();

  // Release the cocoa object manager.
  this->SetCocoaManager(nullptr);
}

//----------------------------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::Create()
{
  // Initialize the window for rendering.
  static unsigned count = 1;

  // As vtk is both crossplatform and a library, we don't know if it is being
  // used in a 'regular Cocoa application' or as a 'pure vtk application'.
  // By the former I mean a regular Cocoa application that happens to have
  // a vtkCocoaHardwareView, by the latter I mean an application that only uses
  // vtk APIs (which happen to use Cocoa as an implementation detail).
  // Specifically, we can't know if NSApplicationMain() was ever called
  // (which is usually done in main()), nor whether the NSApplication exists.
  //
  // So here we call +sharedApplication which will create the NSApplication
  // if it does not exist.  If it does exist, this does nothing.
  // This call is intentionally delayed until this CreateAWindow call
  // to prevent Cocoa-window related stuff from happening in scenarios
  // where vtkRenderWindows are created but never shown.
  NSApplication* app = [NSApplication sharedApplication];

  // create an NSWindow only if neither an NSView nor an NSWindow have
  // been specified already.  This is the case for a 'pure vtk application'.
  // If you are using vtk in a 'regular Mac application' you should call
  // SetRootWindow() and SetWindowId() so that a window is not created here.
  if (!this->GetRootWindow() && !this->GetWindowId() && !this->GetParentId())
  {
    // Ordinarily, only .app bundles get proper mouse and keyboard interaction,
    // but here we change the 'activation policy' to behave as if we were a
    // .app bundle (which we may or may not be).
    (void)[app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSWindow* theWindow = nil;

    // Revert to main screen if specified screen isn't available.
    NSScreen* screen;
    NSArray* allScreens = [NSScreen screens];
    if (this->DisplayIndex >= 0 && (NSUInteger)this->DisplayIndex < [allScreens count])
    {
      screen = [allScreens objectAtIndex:this->DisplayIndex];
    }
    else
    {
      screen = [NSScreen mainScreen];
    }

    // Get the screen's size (in points).  (If there's no screen, the
    // rectangle will become all zeros and not used anyway.)
    NSRect screenRect = [screen frame];

    // Convert from points to pixels.
    NSRect backingScreenRect = [screen convertRectToBacking:screenRect];

    if (this->FullScreen && screen)
    {
      this->Size[0] = static_cast<int>(NSWidth(backingScreenRect));
      this->Size[1] = static_cast<int>(NSHeight(backingScreenRect));

      // Create an NSWindow with the screen's full size (in points, not pixels).
      theWindow = [[vtkCocoaFullScreenWindow alloc] initWithContentRect:screenRect
                                                              styleMask:NSWindowStyleMaskBorderless
                                                                backing:NSBackingStoreBuffered
                                                                  defer:NO];

      // This will hide the menu and the dock
      [theWindow setLevel:NSMainMenuWindowLevel + 1];
      // This will show the menu and the dock
      //[theWindow setLevel:NSFloatingWindowLevel];
    }
    else
    {
      if ((this->Size[0] == 0) && (this->Size[1] == 0))
      {
        this->Size[0] = 300;
        this->Size[1] = 300;
      }
      if ((this->Position[0] == 0) && (this->Position[1] == 0))
      {
        this->Position[0] = 50;
        this->Position[1] = 50;
      }

      NSRect backingContentRect = NSMakeRect((CGFloat)this->Position[0], (CGFloat)this->Position[1],
        (CGFloat)this->Size[0], (CGFloat)this->Size[1]);

      // Convert from pixels to points.
      NSRect contentRect;
      if (screen)
      {
        contentRect = [screen convertRectFromBacking:backingContentRect];
      }
      else
      {
        contentRect = backingContentRect;
      }

      NSWindowStyleMask styleMask = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
      theWindow = [[NSWindow alloc] initWithContentRect:contentRect
                                              styleMask:styleMask
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    }

    if (!theWindow)
    {
      vtkErrorMacro("Could not create window, serious error!");
      return;
    }

    this->SetRootWindow(theWindow);
    this->WindowCreated = 1;

    // Since we created the NSWindow, give it a title.
    NSString* winName = [NSString stringWithFormat:@"Visualization Toolkit - Cocoa #%u", count++];
    this->SetWindowName([winName UTF8String]);

    // makeKeyAndOrderFront: will show the window
    if (this->ShowWindow)
    {
      [theWindow makeKeyAndOrderFront:nil];
      [theWindow setAcceptsMouseMovedEvents:YES];
    }
  }

  // create an NSView if one has not been specified
  if (!this->GetWindowId())
  {
    // For NSViews that display OpenGL, the OS defaults to drawing magnified,
    // not in high resolution. There is a tradeoff here between better visual
    // quality vs memory use and processing time. VTK decides on the opposite
    // default and enables best resolution by default. It does so partly because
    // the system python sets NSHighResolutionCapable in this file:
    // /System/Library/Frameworks/Python.framework/Versions/2.7/Resources/Python.app/Contents/Info.plist
    // If you want magnified drawing instead, call SetWantsBestResolution(false)
    bool wantsBest = this->GetWantsBestResolution();

    if (this->GetParentId())
    {
      // Get the NSView's current frame (in points).
      NSView* parent = (NSView*)this->GetParentId();
      NSRect parentRect = [parent frame];
      CGFloat parentHeight = NSHeight(parentRect);
      CGFloat parentWidth = NSWidth(parentRect);

      // Convert from pixels to points.
      NSWindow* window = [parent window];
      assert(window);
      NSRect backingViewRect = NSMakeRect((CGFloat)this->Position[0], (CGFloat)this->Position[1],
        (CGFloat)this->Size[0], (CGFloat)this->Size[1]);
      NSRect viewRect = [window convertRectFromBacking:backingViewRect];

      CGFloat width = NSWidth(viewRect);
      CGFloat height = NSHeight(viewRect);
      CGFloat x = NSMinX(viewRect);
      CGFloat y = parentHeight - height - NSMinY(viewRect);

      // A whole bunch of sanity checks: frame must be inside parent
      if (x > parentWidth - 1)
      {
        x = parentWidth - 1;
      }
      if (y > parentHeight - 1)
      {
        y = parentHeight - 1;
      }
      if (x < 0.0)
      {
        x = 0.0;
      }
      if (y < 0.0)
      {
        y = 0.0;
      }
      if (x + width > parentWidth)
      {
        width = parentWidth - x;
      }
      if (y + height > parentWidth)
      {
        height = parentHeight - y;
      }

      // Don't use vtkCocoaHardwareView, because if we are in Tk (which is what
      // SetParentId() was added for) then the Tk superview handles the events.
      NSRect viewRect = NSMakeRect(x, y, width, height);
      NSView* hardwareView = [[NSView alloc] initWithFrame:viewRect];
      [parent addSubview:hardwareView];
      this->SetWindowId(hardwareView);
      this->ViewCreated = 1;

#if !VTK_OBJC_IS_ARC
      [hardwareView release];
#endif
    }
    else
    {
      NSRect backingViewRect = NSMakeRect(0.0, 0.0, (CGFloat)this->Size[0], (CGFloat)this->Size[1]);

      // Convert from points to pixels.
      NSWindow* window = (NSWindow*)this->GetRootWindow();
      assert(window);
      NSRect viewRect = [window convertRectFromBacking:backingViewRect];

      // Create a vtkCocoaHardwareView.
      vtkCocoaHardwareView* hardwareView = [[vtkCocoaHardwareView alloc] initWithFrame:viewRect];
      [window setContentView:hardwareView];
      // We have to set the frame's view rect again to work around rounding
      // that occurs when setting the window's content view.
      [hardwareView setFrame:viewRect];
      this->SetWindowId(hardwareView);
      this->ViewCreated = 1;
      [hardwareView setHardwareWindow:this];

#if !VTK_OBJC_IS_ARC
      [hardwareView release];
#endif
    }
  }

  // Now that the NSView and NSWindow exist, the vtkCocoaServer can start its observations.
  vtkCocoaServer* server = [[vtkCocoaServer alloc] initWithHardwareWindow:this];
  this->SetCocoaServer(reinterpret_cast<void*>(server));
  [server startObservations];
#if !VTK_OBJC_IS_ARC
  [server release];
#endif
}

//----------------------------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::Destroy()
{
  vtkCocoaServer* server = (vtkCocoaServer*)this->GetCocoaServer();
  [server stopObservations];
  this->SetCocoaServer(nullptr);

  // If we created it, close the NSWindow.
  if (this->WindowCreated)
  {
    NSWindow* window = (NSWindow*)this->GetRootWindow();
    [window close];
  }

  this->SetWindowId(nullptr);
  this->SetParentId(nullptr);
  this->SetRootWindow(nullptr);
  this->WindowCreated = 0;
  this->ViewCreated = 0;
}

//----------------------------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CocoaManager: " << this->GetCocoaManager() << endl;
  os << indent << "RootWindow (NSWindow): " << this->GetRootWindow() << endl;
  os << indent << "WindowId (NSView): " << this->GetWindowId() << endl;
  os << indent << "ParentId (NSView): " << this->GetParentId() << endl;
  os << indent << "PixelFormat: " << this->GetPixelFormat() << endl;
  os << indent << "WindowCreated: " << (this->GetWindowCreated() ? "Yes" : "No") << endl;
  os << indent << "ViewCreated: " << (this->GetViewCreated() ? "Yes" : "No") << endl;
  << endl;
}

//-------------------------------------------------------------------------------------------------
int* vtkCocoaHardwareWindow::GetSize()
{
  // if we aren't mapped then just call super
  if (!this->WindowId)
  {
    // We want to return the size of 'the window'.  But the term 'window'
    // is overloaded. It's really the NSView that vtk draws into, so we
    // return its size. If there's no NSView, it will result in zeros.
    NSView* view = (NSView*)this->GetWindowId();

    // Get the NSView's current frame (in points).
    NSRect viewRect = [view frame];

    // Convert from points to pixels.
    NSRect backingViewRect = [view convertRectToBacking:viewRect];

    // Update the ivar.
    this->Size[0] = static_cast<int>(NSWidth(backingViewRect));
    this->Size[1] = static_cast<int>(NSHeight(backingViewRect));
  }

  return this->Superclass::GetSize();
}

//-------------------------------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int* vtkCocoaHardwareWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->WindowId)
  {
    return this->Position;
  }

  NSView* parent = (NSView*)this->GetParentId();
  NSView* view = (NSView*)this->GetWindowId();
  if (parent && view)
  {
    // Get display position of the NSView within its parent (in points).
    NSRect parentRect = [parent frame];
    NSRect viewRect = [view frame];

    // Convert from points to pixels.
    NSRect backingParentRect = [parent convertRectToBacking:parentRect];
    NSRect backingViewRect = [view convertRectToBacking:viewRect];

    this->Position[0] = static_cast<int>(NSMinX(backingViewRect));
    this->Position[1] = static_cast<int>(
      NSHeight(backingParentRect) - NSHeight(backingViewRect) - NSMinY(backingViewRect));
  }
  else
  {
    // We want to return the position of 'the window'.  But the term 'window'
    // is overloaded. In this case, it's the position of the NSWindow itself
    // on the screen that we return. We don't much care where the NSView is
    // within the NSWindow.
    NSWindow* window = (NSWindow*)this->GetRootWindow();
    if (window)
    {
      // Get the NSWindow's current frame (in points).
      NSRect windowRect = [window frame];

      // Convert from points to pixels.
      NSRect backingWindowRect = [window convertRectToBacking:windowRect];

      this->Position[0] = static_cast<int>(NSMinX(backingWindowRect));
      this->Position[1] = static_cast<int>(NSMinY(backingWindowRect));
    }
  }

  return this->Position;
}

//-------------------------------------------------------------------------------------------------
// Returns the NSWindow* associated with this vtkWindow.
void* vtkCocoaHardwareWindow::GetRootWindow()
{
  NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
  return reinterpret_cast<void*>([manager objectForKey:@"RootWindow"]);
}

//-------------------------------------------------------------------------------------------------
// Sets the NSWindow* associated with this vtkWindow.
void vtkCocoaHardwareWindow::SetRootWindow(void* arg)
{
  if (arg != nullptr)
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg) forKey:@"RootWindow"];
  }
  else
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager removeObjectForKey:@"RootWindow"];
  }
}

//-------------------------------------------------------------------------------------------------
// Returns the NSView* associated with this vtkWindow.
void* vtkCocoaHardwareWindow::GetWindowId()
{
  NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
  return reinterpret_cast<void*>([manager objectForKey:@"WindowId"]);
}

//-------------------------------------------------------------------------------------------------
// Sets the NSView* associated with this vtkWindow.
void vtkCocoaHardwareWindow::SetWindowId(void* arg)
{
  if (arg != nullptr)
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg) forKey:@"WindowId"];
  }
  else
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager removeObjectForKey:@"WindowId"];
  }
}

//-------------------------------------------------------------------------------------------------
// Returns the NSView* that is the parent of this vtkWindow.
void* vtkCocoaHardwareWindow::GetParentId()
{
  NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
  return reinterpret_cast<void*>([manager objectForKey:@"ParentId"]);
}

//-------------------------------------------------------------------------------------------------
// Sets the NSView* that this vtkWindow should use as a parent.
void vtkCocoaHardwareWindow::SetParentId(void* arg)
{
  if (arg != nullptr)
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg) forKey:@"ParentId"];
  }
  else
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager removeObjectForKey:@"ParentId"];
  }
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetCocoaServer(void* server)
{
  if (server != nullptr)
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<vtkCocoaServer*>(server) forKey:@"CocoaServer"];
  }
  else
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager removeObjectForKey:@"CocoaServer"];
  }
}

//-------------------------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetCocoaServer()
{
  NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
  return reinterpret_cast<void*>([manager objectForKey:@"CocoaServer"]);
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetCocoaManager(void* manager)
{
  NSMutableDictionary* currentCocoaManager =
    reinterpret_cast<NSMutableDictionary*>(this->CocoaManager);
  NSMutableDictionary* newCocoaManager = reinterpret_cast<NSMutableDictionary*>(manager);

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
      this->CocoaManager = const_cast<void*>(CFRetain(newCocoaManager));
    }
    else
    {
      this->CocoaManager = nullptr;
    }
  }
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetWindowName(const char* arg)
{
  vtkWindow::SetWindowName(arg);
  NSWindow* win = (NSWindow*)this->GetRootWindow();
  if (win)
  {
    NSString* winTitleStr = arg ? [NSString stringWithUTF8String:arg] : @"";
    [win setTitle:winTitleStr];
  }
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetWindowInfo(const char* info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetWindowId(reinterpret_cast<void*>(tmp));
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetParentInfo(const char* info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetParentId(reinterpret_cast<void*>(tmp));
}

//-------------------------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetCocoaManager()
{
  return this->CocoaManager;
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::HideCursor()
{
  if (this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 1;

  [NSCursor hide];
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::ShowCursor()
{
  if (!this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 0;

  [NSCursor unhide];
}

// ---------------------------------------------------------------------------
vtkTypeBool vtkCocoaHardwareWindow::GetViewCreated()
{
  return this->ViewCreated;
}

// ---------------------------------------------------------------------------
vtkTypeBool vtkCocoaHardwareWindow::GetWindowCreated()
{
  return this->WindowCreated;
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetCursorPosition(int x, int y)
{
  // The given coordinates are from the bottom left of the view.
  NSPoint newViewPoint = NSMakePoint(x, y);

  // Convert to screen coordinates.
  NSView* view = (NSView*)this->GetWindowId();
  if (view)
  {
    NSPoint screenPoint = [view convertPoint:newViewPoint toView:nil];
    CGPoint newCursorPosition = NSPointToCGPoint(screenPoint);

    // Move the cursor there.
    (void)CGWarpMouseCursorPosition(newCursorPosition);
  }
}

//-------------------------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetCurrentCursor(int shape)
{
  if (this->InvokeEvent(vtkCommand::CursorChangedEvent, &shape))
  {
    return;
  }
  this->Superclass::SetCurrentCursor(shape);

  NSCursor* cursor = nil;
  switch (shape)
  {
    case VTK_CURSOR_DEFAULT:
    case VTK_CURSOR_CUSTOM:
    case VTK_CURSOR_ARROW:
      cursor = [NSCursor arrowCursor];
      break;
    case VTK_CURSOR_SIZENS:
      cursor = [NSCursor resizeUpDownCursor];
      break;
    case VTK_CURSOR_SIZEWE:
      cursor = [NSCursor resizeLeftRightCursor];
      break;
    case VTK_CURSOR_HAND:
      cursor = [NSCursor pointingHandCursor];
      break;
    case VTK_CURSOR_CROSSHAIR:
      cursor = [NSCursor crosshairCursor];
      break;

    // NSCursor does not have cursors for these.
    case VTK_CURSOR_SIZENE:
    case VTK_CURSOR_SIZESW:
    case VTK_CURSOR_SIZENW:
    case VTK_CURSOR_SIZESE:
    case VTK_CURSOR_SIZEALL:
      cursor = [NSCursor arrowCursor];
      break;
  }

  [cursor set];
}

//-------------------------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetViewLayer()
{
  NSView* view = (NSView*)this->GetWindowId();
  if (view)
  {
    return [view layer]
  }
  return nullptr;
}

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
