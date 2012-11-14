/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaGLView.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <Cocoa/Cocoa.h>
#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs

#import "vtkCocoaGLView.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCommand.h"


@implementation vtkCocoaGLView

//----------------------------------------------------------------------------
- (void)emptyMethod:(id)sender
{
  (void)sender;
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
// designated initializer
- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
    {
    // The tracking rect is not set yet.
    _rolloverTrackingRectSet = NO;

    // Force Cocoa into "multi threaded mode" because VTK spawns pthreads.
    // Apple's docs say: "If you intend to use Cocoa calls, you must force
    // Cocoa into its multithreaded mode before detaching any POSIX threads.
    // To do this, simply detach an NSThread and have it promptly exit.
    // This is enough to ensure that the locks needed by the Cocoa
    // frameworks are put in place"
    if ([NSThread isMultiThreaded] == NO)
      {
      [NSThread detachNewThreadSelector:@selector(emptyMethod:)
                               toTarget:self
                             withObject:nil];
      }
    }
  return self;
}

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindow *)getVTKRenderWindow
{
  return _myVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow *)theVTKRenderWindow
{
  _myVTKRenderWindow = theVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindowInteractor *)getInteractor
{
  if (_myVTKRenderWindow)
    {
    return (vtkCocoaRenderWindowInteractor*)_myVTKRenderWindow->GetInteractor();
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)drawRect:(NSRect)theRect
{
  (void)theRect;

  if (_myVTKRenderWindow && _myVTKRenderWindow->GetMapped())
    {
    _myVTKRenderWindow->Render();
    }
}

//----------------------------------------------------------------------------
- (void)clearTrackingRect
{
  // remove any tracking rect we have
  if (_rolloverTrackingRectSet)
    {
    [self removeTrackingRect:_rolloverTrackingRectTag];
    _rolloverTrackingRectSet = NO;
    }
}

//----------------------------------------------------------------------------
- (void)resetTrackingRect
{
  //clear out the old tracking rect
  [self clearTrackingRect];

  //create a new tracking rect
  _rolloverTrackingRectTag = [self addTrackingRect:[self visibleRect]
                                             owner:self
                                          userData:NULL
                                      assumeInside:NO];
  _rolloverTrackingRectSet = YES;
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)resetCursorRects
{
  [super resetCursorRects];
  [self resetTrackingRect];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (BOOL)acceptsFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
// For generating keysyms that are compatible with other VTK interactors
static const char *vtkMacCharCodeToKeySymTable[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quoteright",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G",
  "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete",
};

// For generating keysyms that are compatible with other VTK interactors
static const char *vtkMacKeyCodeToKeySymTable[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, "Return", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "Tab", 0, 0, "Backspace", 0, "Escape", 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, "period", 0, "asterisk", 0, "plus", 0, "Clear",
  0, 0, 0, "slash", "KP_Enter", 0, "minus", 0,
  0, 0, "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5",
  "KP_6", "KP_7", 0, "KP_8", "KP_9", 0, 0, 0,
  "F5", "F6", "F7", "F3", "F8", 0, 0, 0,
  0, "Snapshot", 0, 0, 0, 0, 0, 0,
  0, 0, "Help", "Home", "Prior", "Delete", "F4", "End",
  "F2", "Next", "F1", "Left", "Right", "Down", "Up", 0,
};

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];

  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner.  Since this is a NOT mouseevent, we can not use
  // locationInWindow.  Instead we get the mouse location at this instant,
  // which may not be the exact location of the mouse at the time of the
  // keypress, but should be quite close.  There seems to be no better way.
  // And, yes, vtk does sometimes need the mouse location even for key
  // events, example: pressing 'p' to pick the actor under the mouse.
  // Also note that 'mouseLoc' may have nonsense values if a key is pressed
  // while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint mouseLoc = [[self window] mouseLocationOutsideOfEventStream];
  mouseLoc = [self convertPoint:mouseLoc fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;
  // Get the characters associated with the key event as a utf8 string.
  // This pointer is only valid for the duration of the current autorelease
  // context!
  const char* keyedChars = [[theEvent characters] UTF8String];
  // Since vtk only supports ASCII, we just blindly use the first element
  // of the above string, hoping it's ASCII.
  unsigned char charCode = (unsigned char)keyedChars[0];
  // Get the virtual key code and convert it to a keysym as best we can.
  unsigned short macKeyCode = [theEvent keyCode];
  const char *keySym = 0;
  if (macKeyCode < 128)
    {
    keySym = vtkMacKeyCodeToKeySymTable[macKeyCode];
    }
  if (keySym == 0 && charCode < 128)
    {
    keySym = vtkMacCharCodeToKeySymTable[charCode];
    }
  if (keySym == 0)
    {
    keySym = "None";
    }

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown,
                                  charCode, 1, keySym);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
  if (charCode != '\0')
    {
    interactor->InvokeEvent(vtkCommand::CharEvent, NULL);
    }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyUp:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];

  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner.  Since this is a NOT mouseevent, we can not use
  // locationInWindow.  Instead we get the mouse location at this instant,
  // which may not be the exact location of the mouse at the time of the
  // keypress, but should be quite close.  There seems to be no better way.
  // And, yes, vtk does sometimes need the mouse location even for key
  // events, example: pressing 'p' to pick the actor under the mouse.
  // Also note that 'mouseLoc' may have nonsense values if a key is pressed
  // while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint mouseLoc = [[self window] mouseLocationOutsideOfEventStream];
  mouseLoc = [self convertPoint:mouseLoc fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;
  // Get the characters associated with the key event as a utf8 string.
  // This pointer is only valid for the duration of the current autorelease
  // context!
  const char* keyedChars = [[theEvent characters] UTF8String];
  // Since vtk only supports ASCII, we just blindly use the first element
  // of the above string, hoping it's ASCII.
  unsigned char charCode = (unsigned char)keyedChars[0];
  // Get the virtual key code and convert it to a keysym as best we can.
  unsigned short macKeyCode = [theEvent keyCode];
  const char *keySym = 0;
  if (macKeyCode < 128)
    {
    keySym = vtkMacKeyCodeToKeySymTable[macKeyCode];
    }
  if (keySym == 0 && charCode < 128)
    {
    keySym = vtkMacCharCodeToKeySymTable[charCode];
    }
  if (keySym == 0)
    {
    keySym = "None";
    }

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown,
                                  charCode, 1, keySym);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)flagsChanged:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];

  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner.  Since this is a NOT mouseevent, we can not use
  // locationInWindow.  Instead we get the mouse location at this instant,
  // which may not be the exact location of the mouse at the time of the
  // keypress, but should be quite close.  There seems to be no better way.
  // And, yes, vtk does sometimes need the mouse location even for key
  // events, example: pressing 'p' to pick the actor under the mouse.
  // Also note that 'mouseLoc' may have nonsense values if a key is pressed
  // while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint mouseLoc = [[self window] mouseLocationOutsideOfEventStream];
  mouseLoc = [self convertPoint:mouseLoc fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;

  int oldControlDown = interactor->GetControlKey();
  int oldShiftDown = interactor->GetShiftKey();
  int oldAltDown = interactor->GetAltKey();

  int keyPress = 0;
  char charCode = '\0';
  const char *keySym = 0;
  if (controlDown != oldControlDown)
    {
    keySym = "Control_L";
    keyPress = oldControlDown = controlDown;
    }
  else if (shiftDown != oldShiftDown)
    {
    keySym = "Shift_L";
    keyPress = oldShiftDown = shiftDown;
    }
  else if (altDown != oldAltDown)
    {
    keySym = "Alt_L";
    keyPress = oldAltDown = altDown;
    }
  else
    {
    return;
    }

  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  oldControlDown, oldShiftDown,
                                  charCode, 1, keySym);
  interactor->SetAltKey(oldAltDown);

  if (keyPress)
    {
    interactor->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
    }
  else
    {
    interactor->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
    }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseMoved:(NSEvent *)theEvent
{
  // Note: this method will only be called if this view's NSWindow
  // is set to receive mouse moved events.  See setAcceptsMouseMovedEvents:
  // An NSWindow created by vtk automatically does accept such events.

  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  // Ignore motion outside the view in order to mimic other interactors
  if (!NSPointInRect(mouseLoc, [self visibleRect]))
    {
    return;
    }

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseEntered:(NSEvent *)theEvent
{
  // Note: the mouseEntered/mouseExited events depend on the maintenance of
  // the Tracking Rect, which is handled by the resetTrackingRect,
  // clearTrackingRect and resetCursorRects methods above.

  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                 (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(vtkCommand::EnterEvent, NULL);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseExited:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                 (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(vtkCommand::LeaveEvent, NULL);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)scrollWheel:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown);
  interactor->SetAltKey(altDown);

  if( [theEvent deltaY] > 0)
    {
    interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
    }
  else if( [theEvent deltaY] < 0)
    {
    interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
    }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;
  int clickCount = static_cast<int>([theEvent clickCount]);
  int repeatCount = clickCount > 1 ? clickCount - 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown,
                                  0, repeatCount);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);

  NSApplication* application = [NSApplication sharedApplication];
  NSDate* infinity = [NSDate distantFuture];
  do
    {
    theEvent = [application nextEventMatchingMask:NSLeftMouseUpMask |
                                                  NSLeftMouseDraggedMask
                                        untilDate:infinity
                                           inMode:NSEventTrackingRunLoopMode
                                          dequeue:YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];

      // The mouse location is in points, we must convert to pixels using the
      // scaling factor.
      interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                      (int)round(mouseLoc.y * factor),
                                      controlDown, shiftDown);
      interactor->SetAltKey(altDown);

      switch ([theEvent type])
        {
      case NSLeftMouseDragged:
        interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSLeftMouseUp:
        interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
        keepOn = NO;
      default:
        break;
        }
      }
    else
      {
      keepOn = NO;
      }
    }
  while (keepOn);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;
  int clickCount = [theEvent clickCount];
  int repeatCount = clickCount > 1 ? clickCount - 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown,
                                  0, repeatCount);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);

  NSApplication* application = [NSApplication sharedApplication];
  NSDate* infinity = [NSDate distantFuture];
  do
    {
    theEvent = [application nextEventMatchingMask:NSRightMouseUpMask |
                                                  NSRightMouseDraggedMask
                                        untilDate:infinity
                                           inMode:NSEventTrackingRunLoopMode
                                          dequeue:YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];

      // The mouse location is in points, we must convert to pixels using the
      // scaling factor.
      interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                      (int)round(mouseLoc.y * factor),
                                      controlDown, shiftDown);
      interactor->SetAltKey(altDown);

      switch ([theEvent type])
        {
      case NSRightMouseDragged:
        interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSRightMouseUp:
        interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
        keepOn = NO;
      default:
        break;
        }
      }
    else
      {
      keepOn = NO;
      }
    }
  while (keepOn);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkCocoaRenderWindow* renWin =
    vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!renWin)
    {
    return;
    }

  // Retrieve the scaling factor.
  double factor = renWin->GetScaleFactor();

  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouseevent, we can use locationInWindow.
  NSPoint mouseLoc =
    [self convertPoint:[theEvent locationInWindow] fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0;
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask) ? 1 : 0;
  int altDown = ([theEvent modifierFlags] &
                  (NSCommandKeyMask | NSAlternateKeyMask)) ? 1 : 0;
  int clickCount = [theEvent clickCount];
  int repeatCount = clickCount > 1 ? clickCount - 1 : 0;

  // The mouse location is in points, we must convert to pixels using the
  // scaling factor.
  interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                  (int)round(mouseLoc.y * factor),
                                  controlDown, shiftDown,
                                  0, repeatCount);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);

  NSApplication* application = [NSApplication sharedApplication];
  NSDate* infinity = [NSDate distantFuture];
  do
    {
    theEvent = [application nextEventMatchingMask:NSOtherMouseUpMask |
                                                  NSOtherMouseDraggedMask
                                        untilDate:infinity
                                           inMode:NSEventTrackingRunLoopMode
                                          dequeue:YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];

      // The mouse location is in points, we must convert to pixels using the
      // scaling factor.
      interactor->SetEventInformation((int)round(mouseLoc.x * factor),
                                      (int)round(mouseLoc.y * factor),
                                      controlDown, shiftDown);
      interactor->SetAltKey(altDown);

      switch ([theEvent type])
        {
      case NSOtherMouseDragged:
        interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSOtherMouseUp:
        interactor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
        keepOn = NO;
      default:
        break;
        }
      }
    else
      {
      keepOn = NO;
      }
    }
  while (keepOn);
}

@end
