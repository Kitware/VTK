#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#define id Id
#import "vtkQuartzRenderWindow.h"
#import "vtkQuartzRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#undef id

@interface vtkQuartzGLView : NSOpenGLView
{
    enum NSOpenGLPixelFormatAttribute bitsPerPixel, depthSize;

    @private
    vtkQuartzRenderWindow *myVTKRenderWindow;
    vtkQuartzRenderWindowInteractor *myVTKRenderWindowInteractor;
}

// Overrides
- (void) drawRect:(NSRect)theRect;
- (id)initWithFrame:(NSRect)theFrame;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;


@end
