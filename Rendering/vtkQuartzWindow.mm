#import "vtkQuartzWindow.h"
#define id Id // needed since id is a reserved word in ObjC!
#import "vtkQuartzRenderWindow.h"
#undef id
#import "vtkQuartzGLView.h"

@implementation vtkQuartzWindow


- (void)close {
    [super close];
    [NSApp stop:self];
}


- (vtkQuartzGLView *)getvtkQuartzGLView {
    return myvtkQuartzGLView;
}

- (void)setvtkQuartzGLView:(vtkQuartzGLView *)thevtkQuartzGLView {
    myvtkQuartzGLView = thevtkQuartzGLView;
    [self setContentView:myvtkQuartzGLView];
    [self makeFirstResponder:thevtkQuartzGLView];//so keyboard events go there
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

- (void)makeCurrentContext {
    [[myvtkQuartzGLView openGLContext] makeCurrentContext];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
    ((vtkQuartzRenderWindow *)myVTKRenderWindow)->
        UpdateSizeAndPosition( (int)proposedFrameSize.width, (int)proposedFrameSize.height,
                               (int)[self frame].origin.x, (int)[self frame].origin.y);
    return proposedFrameSize;
}

- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame {
    ((vtkQuartzRenderWindow *)myVTKRenderWindow)->
        UpdateSizeAndPosition((int)newFrame.size.width, (int)newFrame.size.height,
                              (int)newFrame.origin.x, (int)newFrame.origin.y);
    [myvtkQuartzGLView setNeedsDisplay:YES];
    return YES;
}
@end
