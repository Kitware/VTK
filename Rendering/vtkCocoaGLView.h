#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#define id Id
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#undef id

@interface vtkCocoaGLView : NSOpenGLView
{
    enum NSOpenGLPixelFormatAttribute bitsPerPixel, depthSize;

    @private
    vtkCocoaRenderWindow *myVTKRenderWindow;
    vtkCocoaRenderWindowInteractor *myVTKRenderWindowInteractor;
}

// Overrides
- (void) drawRect:(NSRect)theRect;
- (id)initWithFrame:(NSRect)theFrame;

- (vtkCocoaRenderWindow *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow *)theVTKRenderWindow;

- (vtkCocoaRenderWindowInteractor *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(vtkCocoaRenderWindowInteractor *)theVTKRenderWindowInteractor;

- (void*)getOpenGLContext;

@end
