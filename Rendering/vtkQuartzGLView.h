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

- (vtkQuartzRenderWindow *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkQuartzRenderWindow *)theVTKRenderWindow;

- (vtkQuartzRenderWindowInteractor *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(vtkQuartzRenderWindowInteractor *)theVTKRenderWindowInteractor;


@end
