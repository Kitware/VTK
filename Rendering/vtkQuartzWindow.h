#import <Cocoa/Cocoa.h>

@class vtkQuartzGLView;

@interface vtkQuartzWindow : NSWindow
{
    @private
    NSTimer *MyNSTimer;
    IBOutlet vtkQuartzGLView *myvtkQuartzGLView;
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

// accessor and convenience

- (vtkQuartzGLView *)getvtkQuartzGLView;
- (void)setvtkQuartzGLView:vtkQuartzGLView;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;

- (void)makeCurrentContext;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize;
- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame;
- (void)close; //close your face!


@end
