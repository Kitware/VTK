#import <AppKit/AppKit.h>

@class vtkQuartzGLView;
@class vtkQuartzWindow;

@interface vtkQuartzWindowController : NSWindowController
{
    @private 
    IBOutlet vtkQuartzGLView *myvtkQuartzGLView;
    IBOutlet vtkQuartzWindow *myvtkQuartzWindow;
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

- (vtkQuartzGLView *)getvtkQuartzGLView;

- (vtkQuartzWindow *)getvtkQuartzWindow;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;

- (void)makeCurrentContext;

@end