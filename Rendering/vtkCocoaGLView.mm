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
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#include "vtkCommand.h"

@implementation vtkCocoaGLView

//----------------------------------------------------------------------------
// perform post-nib load setup here - not called unless using nib file
- (void)awakeFromNib
{
    // Initialization
    bitsPerPixel = depthSize = (NSOpenGLPixelFormatAttribute)32;
}

//----------------------------------------------------------------------------
- (id)initWithFrame:(NSRect)theFrame
{

  NSOpenGLPixelFormatAttribute attribs[] = 
    {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFADepthSize, 
    (NSOpenGLPixelFormatAttribute)32,
    (NSOpenGLPixelFormatAttribute)nil
    };

  NSOpenGLPixelFormat *fmt;

  /* Choose a pixel format */
  fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
  if(!fmt)
    {
    NSLog(@"Pixel format is nil");
    }
  
  /* Create a GLX context */
  self = [super initWithFrame:theFrame pixelFormat:fmt];
  if (!self)
    {
    NSLog(@"initWithFrame failed");
    }

  [[self openGLContext] makeCurrentContext];
  [[self window] setAcceptsMouseMovedEvents:YES];
  return self;
}

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
- (vtkCocoaRenderWindowInteractor *)getVTKRenderWindowInteractor
{
  return myVTKRenderWindowInteractor;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindowInteractor:(vtkCocoaRenderWindowInteractor *)theVTKRenderWindowInteractor
{
  myVTKRenderWindowInteractor = theVTKRenderWindowInteractor;
}

//----------------------------------------------------------------------------
- (void)drawRect:(NSRect)theRect
{
  NSRect visibleRect;
  (void)theRect;

  // Get visible bounds...
  visibleRect = [self bounds];

  // Set proper viewport
  //glViewport((long int)visibleRect.origin.x, (long int)visibleRect.origin.y, 
  //     (long int)visibleRect.size.width, (long int)visibleRect.size.height);
  if ( myVTKRenderWindow->GetMapped() )
    {
    myVTKRenderWindow->Render();
    }
  [[self openGLContext] flushBuffer];
}

//----------------------------------------------------------------------------
- (BOOL)acceptsFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
- (BOOL)becomeFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
- (BOOL)resignFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
- (void*)getOpenGLContext
{
  return [ self openGLContext ];
}

//----------------------------------------------------------------------------
- (void)keyDown:(NSEvent *)theEvent
{
  NSPoint mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown,
    (unsigned short int)[[theEvent characters] cString][0], 1, 
    [[theEvent characters] cString] );

  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//----------------------------------------------------------------------------
- (void)keyUp:(NSEvent *)theEvent
{
  NSPoint mouseLoc = 
    [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y,controlDown, shiftDown,
    (unsigned short int)[[theEvent characters] cString][0], 1, 
    [[theEvent characters] cString]);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}


//----------------------------------------------------------------------------
- (void)mouseMoved:(NSEvent *)theEvent
{
  NSPoint mouseLoc = 
    [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
- (void)scrollWheel:(NSEvent *)theEvent
{
  NSPoint mouseLoc = 
    [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  if( [theEvent deltaY] > 0)
    {
    myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
    }
  else
    {
    myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
    }
}


//----------------------------------------------------------------------------
- (void)mouseDown:(NSEvent *)theEvent
{
  BOOL keepOn = YES;
  NSPoint mouseLoc;
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
  int repeat = [theEvent clickCount];

  mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown, 0, repeat-1);

  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    
  do
    {
    theEvent = [[self window] nextEventMatchingMask: NSLeftMouseUpMask | NSLeftMouseDraggedMask | NSPeriodicMask];
    mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
    myVTKRenderWindowInteractor->SetEventInformation(
      (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
    switch ([theEvent type])
      {
      case NSLeftMouseDragged:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSLeftMouseUp:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
        keepOn = NO;
        return;
      case NSPeriodic:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::TimerEvent, NULL);
        break;
      default:
        break;
      }
    }
  while (keepOn);

}

//----------------------------------------------------------------------------
- (void)rightMouseDown:(NSEvent *)theEvent
{
  BOOL keepOn = YES;
  NSPoint mouseLoc;
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);

  do
    {
    theEvent = [[self window] nextEventMatchingMask: NSRightMouseUpMask | NSRightMouseDraggedMask | NSPeriodicMask];
    mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
    myVTKRenderWindowInteractor->SetEventInformation((int)mouseLoc.x, (int)mouseLoc.y, 
    controlDown, shiftDown);
    switch ([theEvent type])
      {
      case NSRightMouseDragged:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSRightMouseUp:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
        keepOn = NO;
        return;
      case NSPeriodic:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::TimerEvent, NULL);
        break;
      default:
        break;
      }
    }
  while (keepOn);

}

//----------------------------------------------------------------------------
- (void)otherMouseDown:(NSEvent *)theEvent
{
  BOOL keepOn = YES;
  NSPoint mouseLoc;
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);

  do
    {
    theEvent = [[self window] nextEventMatchingMask: NSOtherMouseUpMask | NSOtherMouseDraggedMask | NSPeriodicMask];
    mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
    myVTKRenderWindowInteractor->SetEventInformation((int)mouseLoc.x, (int)mouseLoc.y, 
    controlDown, shiftDown);
    switch ([theEvent type])
      {
      case NSOtherMouseDragged:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSOtherMouseUp:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
        keepOn = NO;
        return;
      case NSPeriodic:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::TimerEvent, NULL);
        break;
      default:
        break;
      }
    }
  while (keepOn);

}

@end
