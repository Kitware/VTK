#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>

@class vtkQuartzWindowController;

@interface vtkQuartzGLView : NSOpenGLView
{
    enum NSOpenGLPixelFormatAttribute bitsPerPixel, depthSize;

    @private
    IBOutlet vtkQuartzWindowController *controller;
}

// Overrides
- (void) drawRect:(NSRect)theRect;
- (id)initWithFrame:(NSRect)theFrame;

// vtkQuartzWindowController accessors and convenience methods
- (void)setvtkQuartzWindowController:(vtkQuartzWindowController *)theController;
- (vtkQuartzWindowController *)getvtkQuartzWindowController;


@end
