// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#import "vtkCocoaHardwareWindow.h"
#import "vtkCocoaHardwareView.h"
#import "vtkCommand.h"
#import "vtkObjectFactory.h"
#include <AppKit/AppKit.h>

#import <Cocoa/Cocoa.h>

//------------------------------------------------------------------------------
// A custom delegate to handle NSWindow notifications.
@interface vtkCocoaWindowDelegate : NSObject<NSWindowDelegate>
@property (nonatomic, assign) vtkCocoaHardwareWindow* vtkWindow;
@end

@implementation vtkCocoaWindowDelegate
// Called when the window is about to close.
- (BOOL)windowShouldClose:(NSNotification*)notification
{
  if (self.vtkWindow)
  {
    // Tell interactor to stop the NSApplication's run loop
    vtkRenderWindowInteractor* interactor = self.vtkWindow->GetInteractor();
    if (interactor)
    {
      interactor->TerminateApp();
    }
    // Forward the event to the VTK event mechanism.
    self.vtkWindow->InvokeEvent(vtkCommand::DeleteEvent, nullptr);
  }
  return YES;
}

// Called after the window has been resized.
- (void)windowDidResize:(NSNotification*)notification
{
  if (self.vtkWindow)
  {
    NSWindow* window = (NSWindow*)notification.object;
    NSRect frame = [window.contentView frame];
    // Update the size in the vtkHardwareWindow. The SetSize method has an
    // internal check to prevent unintended recursion.
    // self.vtkWindow->SetSize(frame.size.width, frame.size.height);
    int newWidth = static_cast<int>(frame.size.width);
    int newHeight = static_cast<int>(frame.size.height);
    // Get the interactor's current cache of the size.
    int size[2];
    vtkRenderWindowInteractor* interactor = self.vtkWindow->GetInteractor();
    interactor->GetSize(size);

    if (newWidth != size[0] || newHeight != size[1])
    {
      // Process the size change, this sends vtkCommand::WindowResizeEvent.
      interactor->UpdateSize(newWidth, newHeight);

      // Send vtkCommand::ConfigureEvent from the Interactor.
      interactor->InvokeEvent(vtkCommand::ConfigureEvent, nullptr);
    }
  }
}
@end

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCocoaHardwareWindow);

//------------------------------------------------------------------------------
vtkCocoaHardwareWindow::vtkCocoaHardwareWindow()
{
  this->WindowId = nullptr;
  this->ViewId = nullptr;
  this->Delegate = nullptr;
  this->OwnsWindow = false;
  this->CursorHidden = false;
  this->Mapped = false;
}

//------------------------------------------------------------------------------
vtkCocoaHardwareWindow::~vtkCocoaHardwareWindow()
{
  this->Destroy();
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::Create()
{
  if (this->WindowId)
  {
    return;
  }

  @autoreleasepool
  {
    // This setup should only happen once per application lifetime.
    static bool appInitialized = false;
    if (!appInitialized)
    {
      [NSApplication sharedApplication];
      // Make this a regular GUI app that can be the key application,
      // appear in the dock, and have a menu bar.
      if ([NSApp activationPolicy] != NSApplicationActivationPolicyRegular)
      {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
      }
      appInitialized = true;
    }

    // Define the window's initial size.
    CGFloat initialWidth = (this->Size[0] > 0) ? this->Size[0] : 300;
    CGFloat initialHeight = (this->Size[1] > 0) ? this->Size[1] : 300;

    // Convert VTK's top-left coordinates to Cocoa's bottom-left coordinates for the window frame.
    NSScreen* primaryScreen = [NSScreen mainScreen] ?: [NSScreen screens].firstObject;
    NSRect screenFrame = [primaryScreen frame];
    CGFloat initialY = screenFrame.size.height - this->Position[1] - initialHeight;

    NSRect frame = NSMakeRect(this->Position[0], initialY, initialWidth, initialHeight);
    // NSRect frame = NSMakeRect(0, 0, 200, 200); // TODO

    // Create the NSWindow instance.
    this->WindowId =
      [[NSWindow alloc] initWithContentRect:frame
                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                    backing:NSBackingStoreBuffered
                                      defer:NO];

    if (!this->WindowId)
    {
      vtkErrorMacro("Could not create NSWindow.");
      return;
    }
    this->OwnsWindow = true;

    // Set properties
    [this->WindowId setAcceptsMouseMovedEvents:YES];

    // Create and set the custom delegate
    this->Delegate = [[vtkCocoaWindowDelegate alloc] init];
    this->Delegate.vtkWindow = this;
    [this->WindowId setDelegate:this->Delegate];

    // Create our custom Metal-ready view
    vtkCocoaHardwareView* view = [[vtkCocoaHardwareView alloc] initWithFrame:frame];
    this->ViewId = view;
    [this->ViewId setWantsLayer:YES]; // Explicitly enable layer-backing
    [view setHardwareWindow:this];
    [this->WindowId setContentView:this->ViewId];
    [this->WindowId makeFirstResponder:this->ViewId];
    // When not using ARC, this would require a release.
    // [this->ViewId release];

    // Update the title if it has been set
    if (this->WindowName)
    {
      this->SetWindowName(this->WindowName);
    }

    if (this->ShowWindow)
    {
      // makeKeyAndOrderFront: will show the window
      [this->WindowId makeKeyAndOrderFront:nil];
    }

    this->Mapped = true;
  } // end @autoreleasepool
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::Destroy()
{
  if (this->OwnsWindow && this->WindowId)
  {
    this->WindowId = nullptr;
    this->ViewId = nullptr; // Was the content view of the window
  }

  if (this->Delegate)
  {
    this->Delegate.vtkWindow = nullptr;
#if !__has_feature(objc_arc)
    [this->Delegate release];
#endif
    this->Delegate = nullptr;
  }

  this->Mapped = false;
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetSize(int width, int height)
{
  if (this->Size[0] != width || this->Size[1] != height)
  {
    this->Superclass::SetSize(width, height);
    if (this->WindowId)
    {
      [this->WindowId setContentSize:NSMakeSize(width, height)];
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetPosition(int x, int y)
{
  if (this->Position[0] != x || this->Position[1] != y)
  {
    this->Superclass::SetPosition(x, y);
    if (this->WindowId)
    {
      // Convert top-left (VTK) to bottom-left (Cocoa frame origin).
      NSRect windowFrame = [this->WindowId frame];
      NSRect screenFrame = [[NSScreen mainScreen] frame];
      CGFloat newY = screenFrame.size.height - y - windowFrame.size.height;
      [this->WindowId setFrameOrigin:NSMakePoint(x, newY)];
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetWindowName(const char* name)
{
  this->Superclass::SetWindowName(name);
  if (this->WindowId)
  {
    [this->WindowId setTitle:[NSString stringWithUTF8String:name]];
  }
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::HideCursor()
{
  if (!this->CursorHidden)
  {
    [NSCursor hide];
    this->CursorHidden = true;
  }
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::ShowCursor()
{
  if (this->CursorHidden)
  {
    [NSCursor unhide];
    this->CursorHidden = false;
  }
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::SetCurrentCursor(int shape)
{
  if (this->InvokeEvent(vtkCommand::CursorChangedEvent, &shape))
  {
    return;
  }
  this->Superclass::SetCurrentCursor(shape);

  if (this->CursorHidden)
  {
    this->ShowCursor();
  }

  NSCursor* cursor = nil;
  switch (shape)
  {
    case VTK_CURSOR_ARROW:
      cursor = [NSCursor arrowCursor];
      break;
    case VTK_CURSOR_SIZENE:
    case VTK_CURSOR_SIZENW:
    case VTK_CURSOR_SIZESE:
    case VTK_CURSOR_SIZESW:
      cursor = [NSCursor arrowCursor]; // No direct equivalent, use arrow
      break;
    case VTK_CURSOR_SIZENS:
      cursor = [NSCursor resizeUpDownCursor];
      break;
    case VTK_CURSOR_SIZEWE:
      cursor = [NSCursor resizeLeftRightCursor];
      break;
    case VTK_CURSOR_SIZEALL:
      cursor = [NSCursor openHandCursor]; // Or closedHandCursor
      break;
    case VTK_CURSOR_HAND:
      cursor = [NSCursor pointingHandCursor];
      break;
    case VTK_CURSOR_CROSSHAIR:
      cursor = [NSCursor crosshairCursor];
      break;
    case VTK_CURSOR_DEFAULT:
    default:
      cursor = [NSCursor arrowCursor];
      break;
  }
  [[this->WindowId contentView] addCursorRect:[[this->WindowId contentView] bounds] cursor:cursor];
  [cursor set];
}

//------------------------------------------------------------------------------
NSWindow* vtkCocoaHardwareWindow::GetWindowId()
{
  return this->WindowId;
}

//------------------------------------------------------------------------------
NSView* vtkCocoaHardwareWindow::GetViewId()
{
  return this->ViewId;
}

//------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetMetalLayer()
{
  return (void*)[this->ViewId layer];
}

//------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetGenericWindowId()
{
  return reinterpret_cast<void*>(this->WindowId);
}

//------------------------------------------------------------------------------
void* vtkCocoaHardwareWindow::GetGenericParentId()
{
  // NSWindow can be a child of another NSWindow, but we don't expose that here yet.
  return reinterpret_cast<void*>([this->WindowId parentWindow]);
}

//------------------------------------------------------------------------------
void vtkCocoaHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WindowId: " << this->WindowId << "\n";
  os << indent << "ViewId: " << this->ViewId << "\n";
  os << indent << "OwnsWindow: " << (this->OwnsWindow ? "Yes" : "No") << "\n";
}

VTK_ABI_NAMESPACE_END
