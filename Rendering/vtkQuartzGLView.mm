#import "vtkQuartzGLView.h"
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

@implementation vtkQuartzGLView

// perform post-nib load setup here - not called unless using nib file
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
        NSOpenGLPFANoRecovery,
        NSOpenGLPFADoubleBuffer,
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

- (vtkQuartzRenderWindow *)getVTKRenderWindow {
    return myVTKRenderWindow;
}

- (void)setVTKRenderWindow:(vtkQuartzRenderWindow *)theVTKRenderWindow {
    myVTKRenderWindow = theVTKRenderWindow;
}

- (vtkQuartzRenderWindowInteractor *)getVTKRenderWindowInteractor {
    return myVTKRenderWindowInteractor;
}

- (void)setVTKRenderWindowInteractor:(vtkQuartzRenderWindowInteractor *)theVTKRenderWindowInteractor {
    myVTKRenderWindowInteractor = theVTKRenderWindowInteractor;
}

- (void)drawRect:(NSRect)theRect
{
  NSRect visibleRect;

  // Get visible bounds...
  visibleRect = [self bounds];
  
  // Set proper viewport
  //glViewport((long int)visibleRect.origin.x, (long int)visibleRect.origin.y, 
	//     (long int)visibleRect.size.width, (long int)visibleRect.size.height);
  myVTKRenderWindow->Render();
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

    myVTKRenderWindowInteractor->GetInteractorStyle()->
        OnMouseMove(controlDown, shiftDown, mouseLoc.x, mouseLoc.y);
}


- (void)mouseDown:(NSEvent *)theEvent {
    BOOL keepOn = YES;
    BOOL isInside = YES;
    NSPoint mouseLoc;
    int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
    int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);
    int altDown = ([theEvent modifierFlags] & NSAlternateKeyMask);
    int commandDown = ([theEvent modifierFlags] & NSCommandKeyMask);
    int button=1;
    if (altDown) {button=2;}
    if (commandDown) {button=3;}

    myVTKRenderWindowInteractor->SetButtonDown(button);

    mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    switch (button)
    {
    case 1:
        myVTKRenderWindowInteractor->GetInteractorStyle()->
            OnLeftButtonDown(controlDown, shiftDown, mouseLoc.x, mouseLoc.y);
        break;
    case 2:
        myVTKRenderWindowInteractor->GetInteractorStyle()->
            OnMiddleButtonDown(controlDown, shiftDown,mouseLoc.x, mouseLoc.y);
        break;
    case 3:
        myVTKRenderWindowInteractor->GetInteractorStyle()->
            OnRightButtonDown(controlDown, shiftDown,mouseLoc.x, mouseLoc.y);
        break;
    default:
        break;
    }
    
    do {
        isInside = [self mouse:mouseLoc inRect:[self bounds]];
        switch ([theEvent type]) {
            case NSLeftMouseDragged:
                myVTKRenderWindowInteractor->GetInteractorStyle()->
                    OnMouseMove(controlDown, shiftDown, mouseLoc.x, mouseLoc.y);                               			break;
            case NSLeftMouseUp:
                if (isInside)
                    switch (myVTKRenderWindowInteractor->GetButtonDown())
                        {
                        case 1:
                            myVTKRenderWindowInteractor->GetInteractorStyle()->
                                OnLeftButtonUp(controlDown, shiftDown,mouseLoc.x, mouseLoc.y);
                            break;
                        case 2:
                            myVTKRenderWindowInteractor->GetInteractorStyle()->
                                OnMiddleButtonUp(controlDown, shiftDown,mouseLoc.x, mouseLoc.y);
                            break;
                        case 3:
                            myVTKRenderWindowInteractor->GetInteractorStyle()->
                                OnRightButtonUp(controlDown, shiftDown,mouseLoc.x, mouseLoc.y);
                            break;
                        default:
                            break;
                        }
                myVTKRenderWindowInteractor->SetButtonDown(0);
                        
                keepOn = NO;
                [NSEvent stopPeriodicEvents];
                return;
            case NSPeriodic:
                [NSEvent stopPeriodicEvents];
                myVTKRenderWindowInteractor->GetInteractorStyle()->OnTimer();
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
