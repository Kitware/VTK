#import "vtkQuartzGLView.h"
#define id Id // since id is a reserved word in ObjC which vtk uses a _lot_
#import "vtkQuartzRenderWindow.h"
#import "vtkQuartzRenderWindowInteractor.h"
#undef id
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

@implementation vtkQuartzGLView

// perform post-nib load setup here
- (void)awakeFromNib
{
    // Initialization
    bitsPerPixel = depthSize = (enum NSOpenGLPixelFormatAttribute)32;
}

- (id)initWithFrame:(NSRect)theFrame
{

  NSOpenGLPixelFormatAttribute attribs[] = 
    {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADepthSize, (enum NSOpenGLPixelFormatAttribute)32,
        (enum NSOpenGLPixelFormatAttribute)nil};
        
  NSOpenGLPixelFormat *fmt;

  /* Choose a pixel format */
  fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
  if(!fmt)
    NSLog(@"Pixel format is nil");
  
  /* Create a GLX context */
  self = [super initWithFrame:theFrame pixelFormat:fmt];
  if (!self)
    NSLog(@"initWithFrame failed");

  [[self openGLContext] makeCurrentContext];
  [[self window] setAcceptsMouseMovedEvents:YES];
  return self;
}

- (void *)getVTKRenderWindow {
    return myVTKRenderWindow;
}

- (void)setVTKRenderWindow:(void *)theVTKRenderWindow {
    myVTKRenderWindow = theVTKRenderWindow;
}

- (void *)getVTKRenderWindowInteractor {
    return myVTKRenderWindowInteractor;
}

- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor {
    myVTKRenderWindowInteractor = theVTKRenderWindowInteractor;
}

- (void)drawRect:(NSRect)theRect
{
  NSRect visibleRect;

  // Get visible bounds...
  visibleRect = [self bounds];
  
  // Set proper viewport
  glViewport((long int)visibleRect.origin.x, (long int)visibleRect.origin.y, 
	     (long int)visibleRect.size.width, (long int)visibleRect.size.height);
  VBRedrawWindow(myVTKRenderWindow);
  [[self openGLContext] flushBuffer];
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

    DoMouseMoved(myVTKRenderWindowInteractor, shiftDown, controlDown,
                 altDown, commandDown, mouseLoc.x, mouseLoc.y);

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
    DoMouseDown(myVTKRenderWindowInteractor, shiftDown, controlDown, altDown, commandDown, mouseLoc.x, mouseLoc.y);
    
    do {
        isInside = [self mouse:mouseLoc inRect:[self bounds]];
        switch ([theEvent type]) {
            case NSLeftMouseDragged:
                DoMouseDragged(myVTKRenderWindowInteractor, shiftDown, controlDown, altDown, commandDown,
                                mouseLoc.x, mouseLoc.y);
                break;
            case NSLeftMouseUp:
                if (isInside) DoMouseUp(myVTKRenderWindowInteractor, shiftDown, 					controlDown, altDown, commandDown, 
                                mouseLoc.x, mouseLoc.y);
                keepOn = NO;
                [NSEvent stopPeriodicEvents];
                return;
            case NSPeriodic:
                [NSEvent stopPeriodicEvents];
                VBTimerEvent(myVTKRenderWindowInteractor);
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
