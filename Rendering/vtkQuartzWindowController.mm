#import "vtkQuartzWindowController.h"
#import "vtkQuartzGLView.h"
#import "vtkQuartzWindow.h"

@implementation vtkQuartzWindowController

-(id)init {
    NSDictionary *nameTable = [NSDictionary dictionaryWithObject: self forKey: @"NSOwner"];
    if([NSBundle loadNibFile:nibFileName externalNameTable:nameTable withZone:[self zone]]) {
        self = [super init];
        } else {
        fprintf(stderr,"No nib file found!\n"); }
    [myvtkQuartzGLView setvtkQuartzWindowController:self];
    [myvtkQuartzWindow setvtkQuartzWindowController:self];
    [myvtkQuartzWindow makeKeyAndOrderFront: nil];
    return self;
}

-(void)setNibFileName:(NSString *)theName {
    nibFileName = theName;
}

- (NSMenu *)getMyMenu {
    return myNSMenu;
}

- (vtkQuartzGLView *)getvtkQuartzGLView {
    return myvtkQuartzGLView;
}

- (vtkQuartzWindow *)getvtkQuartzWindow {
    return myvtkQuartzWindow;
}

- (void *)getVTKRenderWindow {
    return myVTKRenderWindow;
}

- (void)setVTKRenderWindow:(void *)theVTKRenderWindow {
    myVTKRenderWindow = theVTKRenderWindow;
}

- (void *)getVTKRenderWindowInteractor {
    return myVTKRenderWindowInteractor;
}

- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor {
    myVTKRenderWindowInteractor = theVTKRenderWindowInteractor;
}

- (void)makeCurrentContext {
    [[myvtkQuartzGLView openGLContext] makeCurrentContext];
}

@end
