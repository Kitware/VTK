#import "vtkQuartzWindow.h"
#define id Id // needed since id is a reserved word in ObjC!
#import "vtkQuartzRenderWindow.h"
#undef id
#import "vtkQuartzWindowController.h"
//#import "vtkCaller.h"
#import "vtkQuartzGLView.h"

@implementation vtkQuartzWindow

- (void)close {
    [super close];
    [NSApp stop:self];
//    VBDestroyWindow(myVTKRenderWindow);
}

//vtkQuartzWindowController accessors and convenience
- (void)setvtkQuartzWindowController:(vtkQuartzWindowController *)theController
{
    controller = theController;
}

- (vtkQuartzWindowController *)getvtkQuartzWindowController {
    return controller;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
    VBResizeWindow([[self getvtkQuartzWindowController] getVTKRenderWindow], proposedFrameSize.width, proposedFrameSize.height,
                    [self frame].origin.x, [self frame].origin.y);
    return proposedFrameSize;
}

- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame {
    VBResizeWindow([[self getvtkQuartzWindowController] getVTKRenderWindow], newFrame.size.width, newFrame.size.height,
                    newFrame.origin.x, newFrame.origin.y);
    [[[self getvtkQuartzWindowController] getvtkQuartzGLView] setNeedsDisplay:YES];
    return YES;
}


- (void)startTimer {
    if(MyNSTimer != nil) return;
    else {
        MyNSTimer = [[NSTimer scheduledTimerWithTimeInterval:0.510 target:self selector:@selector(timerEvent) userInfo:nil repeats:YES] retain];
    }
}

- (void)stopTimer {
    if(MyNSTimer != nil) {
        [MyNSTimer invalidate];
        [MyNSTimer release];
        MyNSTimer = nil;
    }
}

@end
