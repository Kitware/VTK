#import "vtkQuartzGLView.h"
#import "vtkQuartzWindowController.h"
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

//#include "vtkCaller.h"

@implementation vtkQuartzGLView

- (id)initWithFrame:(NSRect)theFrame
{

  NSOpenGLPixelFormatAttribute attribs[] = {NSOpenGLPFAAccelerated, 1,\
                                            NSOpenGLPFADepthSize, 8,\
                                             0};
  NSOpenGLPixelFormat *fmt;

  /* Choose a pixel format */
  fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
  
  /* Create a GLX context */
  self = [super initWithFrame:theFrame pixelFormat:fmt];
  
  /* Destroy the pixel format */
  [fmt release];

  return self;
}


// vtkQuartzWindowController accessors and convenience methods
- (void)setvtkQuartzWindowController:(vtkQuartzWindowController *)theController
{
  controller = theController;
}

- (vtkQuartzWindowController *)getvtkQuartzWindowController {
    return controller;
}

- (void)drawRect:(NSRect)theRect
{
  NSRect visibleRect;

  // Get visible bounds...
  visibleRect = [self bounds];
  
  // Set proper viewport
  glViewport(visibleRect.origin.x, visibleRect.origin.y, visibleRect.size.width, visibleRect.size.height);
  VBRedrawWindow([[self getvtkQuartzWindowController] getVTKRenderWindow]);
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)becomeFirstResponder
{
  return YES;
}

- (BOOL)resignFirstResponder
{
  return YES;
}

- (void)mouseMoved:(NSEvent *)theEvent {
    NSPoint mouseLoc;
    int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
    int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
    int altDown = ([theEvent modifierFlags] & NSAlternateKeyMask);
    int commandDown = ([theEvent modifierFlags] & NSCommandKeyMask);

    mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];

    DoMouseMoved([[self getvtkQuartzWindowController] getVTKRenderWindowInteractor], shiftDown, controlDown, altDown, commandDown, mouseLoc.x, mouseLoc.y);

}


- (void)mouseDown:(NSEvent *)theEvent {
    BOOL keepOn = YES;
    BOOL isInside = YES;
    NSPoint mouseLoc;
    int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
    int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
    int altDown = ([theEvent modifierFlags] & NSAlternateKeyMask);
    int commandDown = ([theEvent modifierFlags] & NSCommandKeyMask);

    mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    DoMouseDown([[self getvtkQuartzWindowController]  getVTKRenderWindowInteractor], shiftDown, controlDown, altDown, commandDown, mouseLoc.x, mouseLoc.y);
    
    do {
        isInside = [self mouse:mouseLoc inRect:[self bounds]];
        switch ([theEvent type]) {
            case NSLeftMouseDragged:
                DoMouseDragged([[self getvtkQuartzWindowController] getVTKRenderWindowInteractor], shiftDown, controlDown, altDown, commandDown,
                                mouseLoc.x, mouseLoc.y);
                break;
            case NSLeftMouseUp:
                if (isInside) DoMouseUp([[self getvtkQuartzWindowController]  getVTKRenderWindowInteractor], shiftDown, 					controlDown, altDown, commandDown, 
                                mouseLoc.x, mouseLoc.y);
                keepOn = NO;
                [NSEvent stopPeriodicEvents];
                return;
            case NSPeriodic:
                [NSEvent stopPeriodicEvents];
                VBTimerEvent([[self getvtkQuartzWindowController] getVTKRenderWindowInteractor]);
                break;
            default:
                break;
        }
        theEvent = [[self window] nextEventMatchingMask: NSLeftMouseUpMask | NSLeftMouseDraggedMask | NSPeriodicMask];
        mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    }while (keepOn);
    return;
}


@end
