#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>

@class vtkQuartzWindowController;

@interface vtkQuartzGLView : NSOpenGLView
{
    enum NSOpenGLPixelFormatAttribute bitsPerPixel, depthSize;

    @private
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

// Overrides
- (void) drawRect:(NSRect)theRect;
- (id)initWithFrame:(NSRect)theFrame;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;


@end
