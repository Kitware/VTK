#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>

@class vtkQuartzWindowController;

@interface vtkQuartzGLView : NSOpenGLView
{
    @private
    IBOutlet vtkQuartzWindowController *controller;
}

// vtkQuartzWindowController accessors and convenience methods
- (void)setvtkQuartzWindowController:(vtkQuartzWindowController *)theController;
- (vtkQuartzWindowController *)getvtkQuartzWindowController;


@end
