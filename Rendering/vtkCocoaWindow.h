#import <Cocoa/Cocoa.h>

@class vtkCocoaGLView;

@interface vtkCocoaWindow : NSWindow
{
    @private
    IBOutlet vtkCocoaGLView *myvtkCocoaGLView;
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

// accessor and convenience

- (vtkCocoaGLView *)getvtkCocoaGLView;
- (void)setvtkCocoaGLView:vtkCocoaGLView;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;

- (void)makeCurrentContext;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize;
- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame;
- (void)close; //close your face!

@end
