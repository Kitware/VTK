#import "vtkCocoaWindow.h"
#define id Id // needed since id is a reserved word in ObjC!
#import "vtkCocoaRenderWindow.h"
#undef id
#import "vtkCocoaGLView.h"

@implementation vtkCocoaWindow


- (void)close {
    [super close];
    [NSApp stop:self];
}


- (vtkCocoaGLView *)getvtkCocoaGLView {
    return myvtkCocoaGLView;
}

- (void)setvtkCocoaGLView:(vtkCocoaGLView *)thevtkCocoaGLView {
    myvtkCocoaGLView = thevtkCocoaGLView;
    [self setContentView:myvtkCocoaGLView];
    [self makeFirstResponder:thevtkCocoaGLView];//so keyboard events go there
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
    [[myvtkCocoaGLView openGLContext] makeCurrentContext];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
    (void)sender;
    ((vtkCocoaRenderWindow *)myVTKRenderWindow)->
        UpdateSizeAndPosition( (int)proposedFrameSize.width, (int)proposedFrameSize.height,
                               (int)[self frame].origin.x, (int)[self frame].origin.y);
    return proposedFrameSize;
}

- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame {
    (void)sender;
    ((vtkCocoaRenderWindow *)myVTKRenderWindow)->
        UpdateSizeAndPosition((int)newFrame.size.width, (int)newFrame.size.height,
                              (int)newFrame.origin.x, (int)newFrame.origin.y);
    [myvtkCocoaGLView setNeedsDisplay:YES];
    return YES;
}
@end
