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

#import "vtkCocoaGLView.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCommand.h"


@implementation vtkCocoaGLView

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindow *)getVTKRenderWindow
{
  return myVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow *)theVTKRenderWindow
{
  myVTKRenderWindow = theVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindowInteractor *)getInteractor
{
  if (myVTKRenderWindow)
    {
    return (vtkCocoaRenderWindowInteractor*)myVTKRenderWindow->GetInteractor();
    }
  else
    {
    return NULL;
    }
}


//----------------------------------------------------------------------------
- (void)drawRect:(NSRect)theRect
{
  (void)theRect;

  if ( myVTKRenderWindow && myVTKRenderWindow->GetMapped() )
    {
    myVTKRenderWindow->Render();
    }
}

//----------------------------------------------------------------------------
- (BOOL)acceptsFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
- (void)keyDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a NOT mouseevent, we can not use locationInWindow
  // Instead we get the mouse location at this instant, which may not be the exact
  // location of the mouse at the time of the keypress, but should be quite close.
  // There seems to be no better way.  And, yes, vtk does sometimes need the mouse
  // location even for key events, example: pressing 'p' to pick the actor under
  // the mouse. Also note that 'mouseLoc' may have nonsense values if a key is
  // pressed while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint mouseLoc = [[self window] mouseLocationOutsideOfEventStream];
  mouseLoc = [self convertPoint:mouseLoc fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  // Get the characters associated with the key event as a utf8 string.
  // This pointer is only valid for the duration of the current autorelease context!
  const char* keyedChars = [[theEvent characters] UTF8String];

  // Since vtk only supports ascii, we just blindly pass the first element
  // of the above string, hoping it's ascii
  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown,
    (unsigned short)keyedChars[0], 1, keyedChars);

  interactor->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
  interactor->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//----------------------------------------------------------------------------
- (void)keyUp:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a NOT mouseevent, we can not use locationInWindow
  // Instead we get the mouse location at this instant, which may not be the exact
  // location of the mouse at the time of the keypress, but should be quite close.
  // There seems to be no better way.  And, yes, vtk does sometimes need the mouse
  // location even for key events, example: pressing 'p' to pick the actor under
  // the mouse. Also note that 'mouseLoc' may have nonsense values if a key is
  // pressed while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint mouseLoc = [[self window] mouseLocationOutsideOfEventStream];
  mouseLoc = [self convertPoint:mouseLoc fromView:nil];

  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  // Get the characters associated with the key event as a utf8 string.
  // This pointer is only valid for the duration of the current autorelease context!
  const char* keyedChars = [[theEvent characters] UTF8String];

  // Since vtk only supports ascii, we just blindly pass the first element
  // of the above string, hoping it's ascii
  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown,
    (unsigned short)keyedChars[0], 1, keyedChars);

  interactor->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}


//----------------------------------------------------------------------------
- (void)mouseMoved:(NSEvent *)theEvent
{
  // Note: this method will only be called if this view's NSWindow
  // is set to receive mouse moved events.  See setAcceptsMouseMovedEvents:
  // An NSWindow created by vtk automatically does accept such events.
  
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a mouseevent, we can use locationInWindow
  NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
- (void)scrollWheel:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a mouseevent, we can use locationInWindow
  NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  if( [theEvent deltaY] > 0)
    {
    interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
    }
  else
    {
    interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
    }
}


//----------------------------------------------------------------------------
- (void)mouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a mouseevent, we can use locationInWindow
  NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);

  interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    
  NSDate*  infinity = [NSDate distantFuture];
  do
    {
    theEvent = 
      [NSApp nextEventMatchingMask: NSLeftMouseUpMask | NSLeftMouseDraggedMask
      untilDate: infinity
      inMode: NSEventTrackingRunLoopMode
      dequeue: YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
      interactor->SetEventInformation(
        (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
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
- (void)rightMouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a mouseevent, we can use locationInWindow
  NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);

  interactor->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);

  NSDate*  infinity = [NSDate distantFuture];
  do
    {
    theEvent = 
      [NSApp nextEventMatchingMask: NSRightMouseUpMask | NSRightMouseDraggedMask
      untilDate: infinity
      inMode: NSEventTrackingRunLoopMode
      dequeue: YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
      interactor->SetEventInformation(
        (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
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
- (void)otherMouseDown:(NSEvent *)theEvent
{
  vtkCocoaRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    return;
  
  BOOL keepOn = YES;

  // Get the location of the mouse event relative to this NSView's bottom left corner
  // Since this is a mouseevent, we can use locationInWindow
  NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  interactor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);

  interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
  
  NSDate*  infinity = [NSDate distantFuture];
  do
    {
    theEvent = 
      [NSApp nextEventMatchingMask: NSOtherMouseUpMask | NSOtherMouseDraggedMask
      untilDate: infinity
      inMode: NSEventTrackingRunLoopMode
      dequeue: YES];
    if (theEvent)
      {
      mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
      interactor->SetEventInformation(
        (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
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
