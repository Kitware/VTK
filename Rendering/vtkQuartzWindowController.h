#import <AppKit/AppKit.h>

@class vtkQuartzGLView;
@class vtkQuartzWindow;

@interface vtkQuartzWindowController : NSWindowController
{
    @private 
    IBOutlet vtkQuartzGLView *myvtkQuartzGLView;
    IBOutlet vtkQuartzWindow *myvtkQuartzWindow;
    IBOutlet NSMenu *myNSMenu;
    NSString *nibFileName;
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

- (void)setNibFileName:(NSString *)theName;

- (NSMenu *)getMyMenu;

- (vtkQuartzGLView *)getvtkQuartzGLView;

- (vtkQuartzWindow *)getvtkQuartzWindow;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;

- (void)makeCurrentContext;

@end